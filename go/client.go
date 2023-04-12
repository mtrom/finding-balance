package main

import (
	"crypto/aes"
    "log"
    "time"
    "net"
    "io"

    . "github.com/ahenzinger/simplepir/pir"
)

// TODO: document this war crime
// go:linkname bufPrgReader pir.bufPrgReader
var bufPrgReader *BufPRGReader


func RunClient(input, output string, dbSize, bucketSize uint64) {
    queries := ReadQueries(input)
    protocol, params, entryBits := SetupProtocol(dbSize, bucketSize)

    // read lwe matrix seed from server
    server, err := net.Listen(SERVER_TYPE, SERVER_HOST)
    if err != nil { panic(err) }
    defer server.Close()

    connection, err := server.Accept()
    if err != nil { panic(err) }
    connection.SetDeadline(time.Time{})

    var bytes []byte
    log.Printf("[ client ] offline = %d", len(bytes))

    for i := uint64(0);
        i < aes.BlockSize + params.L * params.N * ELEMENT_SIZE;
        i += CHUNK_SIZE {
        // log.Printf("[ client ] reading chunk %d", i / CHUNK_SIZE)
        chunk := make([]byte, CHUNK_SIZE)
        _, err := connection.Read(chunk)
        bytes = append(bytes, chunk...)
        if err == io.EOF {
            break
        } else if err != nil {
            panic(err)
        }
    }

    log.Printf(
        "[ client ] hintBytes = %d - %d = %d",
        len(bytes), aes.BlockSize, len(bytes) - aes.BlockSize,
    )

    var seed PRGKey
    copy(seed[:], bytes[:aes.BlockSize])

    dbInfo := SetupDBInfo(dbSize, entryBits, params)

    hint := BytesToMatrix(bytes[aes.BlockSize:], params.L, params.N)

    // this seeds the randomness when generating the lwe matrix
    bufPrgReader = NewBufPRG(NewPRG(&seed))
    lweMatrix := protocol.DecompressState(dbInfo, *params, MakeCompressedState(&seed))

    params.PrintParams()
    log.Printf(
        "[ client ] Packing = %d, Ne = %d, Basis = %d, Squishing = %d, Cols = %d\n",
        dbInfo.Packing, dbInfo.Ne, dbInfo.Basis, dbInfo.Squishing, dbInfo.Cols,
    )


    var results []uint64
    queried := map[uint64]struct{}{}
    for _, col := range queries {
        // translate the hash value into the column by dividing by the number
        // of buckets per column
        translated := col / (params.L / bucketSize)

        // this should not happen
        if translated % params.M != translated {
            panic("our translated query larger then the number of columns")
        }

        log.Printf("[ go/pir ] translated query %d to %d\n", col, translated)

        // don't query the same column twice
        if _, already_queried := queried[translated]; already_queried {
            continue;
        } else {
            queried[translated] = struct{}{}
        }

        secret, query := protocol.Query(translated, lweMatrix, *params, dbInfo)

        bytes := MatrixToBytes(query.Data[0])

        connection.Write(bytes)

        bytes = make([]byte, params.L * ELEMENT_SIZE)
        connection.Read(bytes)
        answer := BytesToMatrix(bytes, params.L, 1)

        column := RecoverColumn(
            MakeMsg(hint),
            query,
            MakeMsg(answer),
            secret,
            *params,
            dbInfo,
        )

        results = append(results, column...)
    }

    WriteDatabase(output, results)
}
