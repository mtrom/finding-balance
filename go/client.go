package main

import (
	"crypto/aes"
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

    timer := StartTimer("[ client ] pir offline:")

    // decide protocol parameters and query indices
    queries := ReadQueries(input)
    protocol, params, entryBits := SetupProtocol(dbSize, bucketSize)
    dbInfo := SetupDBInfo(dbSize, entryBits, params)

    // connect to server
    connection, err := net.Listen(SERVER_TYPE, SERVER_HOST)
    if err != nil { panic(err) }
    defer connection.Close()

    server, err := connection.Accept()
    if err != nil { panic(err) }
    server.SetDeadline(time.Time{})

    // read in the 'offline' data in chunks (i.e., lwe matrix seed and hint)
    var bytes []byte
    for i := uint64(0);
        i < aes.BlockSize + params.L * params.N * ELEMENT_SIZE;
        i += CHUNK_SIZE {
        chunk := make([]byte, CHUNK_SIZE)
        _, err := server.Read(chunk)
        bytes = append(bytes, chunk...)
        if err != nil { panic(err) }
    }

    hint := BytesToMatrix(bytes[aes.BlockSize:], params.L, params.N)

    // this seeds the randomness when generating the lwe matrix
    var seed PRGKey
    copy(seed[:], bytes[:aes.BlockSize])
    bufPrgReader = NewBufPRG(NewPRG(&seed))

    // generate the lwe matrix from the seed
    lweMatrix := protocol.DecompressState(dbInfo, *params, MakeCompressedState(&seed))

    timer.End()
    timer = StartTimer("[ client ] pir online:")

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

        // don't query the same column twice
        if _, already_queried := queried[translated]; already_queried {
            continue;
        } else {
            queried[translated] = struct{}{}
        }

        // generate the query vector and send to server
        queryTimer := StartTimer("[ client ] pir query:")
        secret, query := protocol.Query(translated, lweMatrix, *params, dbInfo)
        queryTimer.End()

        bytes := MatrixToBytes(query.Data[0])
        server.Write(bytes)

        // read the query's answer
        bytes = make([]byte, params.L * ELEMENT_SIZE)
        server.Read(bytes)
        answer := BytesToMatrix(bytes, params.L, 1)

        // reconstruct the data based on the answer
        recoverTimer := StartTimer("[ client ] pir recover:")
        column := RecoverColumn(
            MakeMsg(hint),
            query,
            MakeMsg(answer),
            secret,
            *params,
            dbInfo,
        )
        recoverTimer.End()

        results = append(results, column...)
    }

    timer.End()

    WriteDatabase(output, results)
}
