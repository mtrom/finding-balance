package main

import (
	"crypto/aes"
    "fmt"
    "time"
    "net"

    . "github.com/ahenzinger/simplepir/pir"
)

/**
 * this seeds the randomness and allows the client to generate the lwe matrix
 * from the seed, but the simplepir library doesn't export it so we need to
 * force this link
 */
// go:linkname bufPrgReader pir.bufPrgReader
var bufPrgReader *BufPRGReader

/**
 * run protocol as the server
 *
 * @param <input> filename for the indices to query
 * @param <output> filename to write the answer to
 * @param <dbSize> size of the server's db in bytes
 * @param <bucketSize> size of a bucket in bytes
 */
func RunClient(input, output string, dbSize, bucketSize uint64) {

    // read in query indices
    queries := ReadQueries(input)

    // connect to server
    connection, err := net.Listen(SERVER_TYPE, SERVER_HOST)
    if err != nil { panic(err) }
    defer connection.Close()

    server, err := connection.Accept()
    if err != nil { panic(err) }
    server.SetDeadline(time.Time{})

    ///////////////////////// OFFLINE /////////////////////////

    timer := StartTimer("[ client ] pir offline", BLUE)

    // decide protocol parameters
    protocol, params, entryBits := SetupProtocol(dbSize, bucketSize)
    dbInfo := SetupDBInfo(dbSize, entryBits, params)

    // read in the 'offline' data (i.e., lwe matrix seed and hint)
    offline := ReadOverNetwork(server, aes.BlockSize + params.L * params.N * ELEMENT_SIZE)

    hint := BytesToMatrix(offline[aes.BlockSize:], params.L, params.N)

    // this seeds the randomness when generating the lwe matrix
    var seed PRGKey
    copy(seed[:], offline[:aes.BlockSize])
    bufPrgReader = NewBufPRG(NewPRG(&seed))

    // generate the lwe matrix from the seed
    lweMatrix := protocol.DecompressState(dbInfo, *params, MakeCompressedState(&seed))

    timer.End()

    ///////////////////////////////////////////////////////////

    fmt.Printf("[  both  ] offline comm (MB)\t: %.4f\n", float64(len(offline)) / 1000000)

    // let the server know we're ready for online
    ready := []byte{1}
    server.Write(ready)

    ////////////////////////// ONLINE /////////////////////////

    queryTimer := CreateTimer("[ client ] pir query", BLUE)
    recoverTimer := CreateTimer("[ client ] pir recover", BLUE)
    timer = StartTimer("[ client ] pir online", BLUE)

    comm := 0;
    var results []uint64
    queried := map[uint64]struct{}{}
    for _, col := range queries {

        // translate the hash value into the column by dividing by the number
        // of buckets per column
        translated := col / (params.L / bucketSize)

        // generate the query vector and send to server
        queryTimer.Start()
        secret, query := protocol.Query(translated, lweMatrix, *params, dbInfo)
        queryTimer.Stop()

        data := MatrixToBytes(query.Data[0])
        WriteOverNetwork(server, data)
        comm += len(data)

        // read the query's answer
        data = ReadOverNetwork(server, params.L * ELEMENT_SIZE)
        comm += len(data)

        answer := BytesToMatrix(data, params.L, 1)

        // if we've already queried this column, don't bother recovering
        if _, already_queried := queried[translated]; already_queried {
            continue;
        } else {
            queried[translated] = struct{}{}
        }

        // reconstruct the data based on the answer
        recoverTimer.Start()
        column := RecoverColumn(
            MakeMsg(hint),
            query,
            MakeMsg(answer),
            secret,
            *params,
            dbInfo,
        )
        recoverTimer.Stop()

        results = append(results, column...)
    }

    timer.End()
    queryTimer.Print()
    recoverTimer.Print()

    ///////////////////////////////////////////////////////////

    fmt.Printf("[  both  ] online comm (MB)\t: %.4f\n", float64(comm) / 1000000)

    WriteDatabase(output, results)
}
