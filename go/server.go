package main

import (
	"crypto/aes"
    "net"
    "time"

    . "github.com/ahenzinger/simplepir/pir"
)

/**
 * run protocol as the server
 *
 * @param <input> filename for an encrypted database
 */
func RunServer(input string) {

    timer := StartTimer("[ server ] pir offline:")

    // decide protocol parameters
    protocol, params, db, _ := SetupProtocolAndDB(input)

    // sample seed for lwe random matrix A
    lweMatrix, seed := protocol.InitCompressed(db.Info, *params)

    // calculate hint seed
    serverState, hint := protocol.Setup(db, lweMatrix, *params)

    // connect to client
    client, err := net.Dial(SERVER_TYPE, SERVER_HOST)
    if err != nil { panic(err) }
    defer client.Close()
    client.SetDeadline(time.Time{})

    // gather lwe matrix seed and hint into 'offline' dataset
    var bytes = make([]byte, aes.BlockSize)
    ptr := *seed.Seed
    copy(bytes[:], ptr[:])
    offline := append(bytes, MatrixToBytes(hint.Data[0])...)

    // because the offline data is so large, it needs to be chunked
    for i := 0; i < len(offline); i += CHUNK_SIZE {
        _, err = client.Write(offline[i:i+CHUNK_SIZE])
        if err != nil { panic(err) }
    }

    timer.End()
    timer = StartTimer("[ server ] pir online:")

    // expected size of the query vector
    queryRows := params.M

	// compensate for this squish artifact
	if params.M % db.Info.Squishing != 0 {
		queryRows += db.Info.Squishing - (params.M % db.Info.Squishing)
	}

    // read in query vector
    bytes = make([]byte, queryRows * ELEMENT_SIZE)
    client.Read(bytes)
    query := BytesToMatrix(bytes, queryRows, 1)

    // generate answer and send to client
    answerTimer := StartTimer("[ server ] pir answer:")
    answer := protocol.Answer(db, MakeMsgSlice(MakeMsg(query)), serverState, lweMatrix, *params)
    answerTimer.End()

    client.Write(MatrixToBytes(answer.Data[0]))
    timer.End()
}
