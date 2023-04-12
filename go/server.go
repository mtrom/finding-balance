package main

import (
	"crypto/aes"
    "log"
    "net"
    "time"

    . "github.com/ahenzinger/simplepir/pir"
)

func RunServer(input string) {
    protocol, params, db, _ := SetupProtocolAndDB(input)

    // sample seed for lwe random matrix A
    lweMatrix, seed := protocol.InitCompressed(db.Info, *params)

    // calculate hint seed
    serverState, hint := protocol.Setup(db, lweMatrix, *params)

    // send lwe matrix seed to client
    client, err := net.Dial(SERVER_TYPE, SERVER_HOST)
    if err != nil { panic(err) }
    defer client.Close()
    client.SetDeadline(time.Time{})

    var bytes = make([]byte, aes.BlockSize)
    ptr := *seed.Seed
    copy(bytes[:], ptr[:])

    hintBytes := MatrixToBytes(hint.Data[0])

    offline := append(bytes, hintBytes...)

    log.Printf("[ server ] offline = %d", len(offline))

    for i := 0; i < len(offline); i += CHUNK_SIZE {
        // log.Printf("[ server ] writing chunk %d", i / CHUNK_SIZE)
        _, err = client.Write(offline[i:i+CHUNK_SIZE])
        if err != nil { panic(err) }
    }

    params.PrintParams()
    log.Printf(
        "[ server ] Packing = %d, Ne = %d, Basis = %d, Squishing = %d, Cols = %d\n",
        db.Info.Packing, db.Info.Ne, db.Info.Basis, db.Info.Squishing, db.Info.Cols,
    )

    queryRows := params.M

	// compensate for this squish artifact
	if params.M % db.Info.Squishing != 0 {
		queryRows += db.Info.Squishing - (params.M % db.Info.Squishing)
	}
    log.Printf("[ server ] queryRows=%d\n", queryRows)

    bytes = make([]byte, queryRows * ELEMENT_SIZE)
    client.Read(bytes)
    query := BytesToMatrix(bytes, queryRows, 1)

    querySlice := MakeMsgSlice(MakeMsg(query))
    answer := protocol.Answer(db, querySlice, serverState, lweMatrix, *params)

    answerBytes := MatrixToBytes(answer.Data[0])
    client.Write(answerBytes)

    log.Printf("[ go/pir ] M, L, N is\t%d, %d, %d", params.M, params.L, params.N)
    log.Printf("[ go/pir ] hint is\t\t%d x %d", hint.Data[0].Rows, hint.Data[0].Cols)
}
