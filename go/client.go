package main

import (
	"crypto/aes"
    "bytes"
    "fmt"
    "net"
    // "reflect"
    "time"
    // "unsafe"

    . "github.com/ahenzinger/simplepir/pir"
)

const (
    CLIENT_OPRF_RESULT = "out/client.edb"
    CLIENT_QUERIES     = "out/queries.db"
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
 * @param <psiParams> params of the greater psi protocol
 */
func RunClient(psiParams *PSIParams, expected int) {

    // read in query indices
    queries := ReadQueries(CLIENT_QUERIES)

    // read in result of oprf
    oprf := ReadDatabase[byte](CLIENT_OPRF_RESULT)

    // connect to server
    connection, err := net.Listen(SERVER_TYPE, SERVER_HOST)
    if err != nil { panic(err) }
    defer connection.Close()

    server, err := connection.Accept()
    if err != nil { panic(err) }
    server.SetDeadline(time.Time{})

    ///////////////////////// OFFLINE /////////////////////////

    timer := StartTimer("[ client ] pir offline", BLUE)

    subtimer := StartTimer("[ client ] pick params", YELLOW)
    // decide protocol parameters
    protocol, params, entryBits := SetupProtocol(psiParams)
    dbInfo := SetupDBInfo(psiParams, entryBits, params)


    subtimer.End()
    PrintParams(params)
    timer.Stop()

    // wait until the client is ready for offline
    ready := make([]byte, 1)
    server.Read(ready)

    timer.Start()
    subtimer = StartTimer("[ client ] read data", YELLOW)

    // read in the 'offline' data (i.e., lwe matrix seed and hint)
    offline := ReadOverNetwork(server, aes.BlockSize + params.L * params.N * ELEMENT_SIZE)

    subtimer.End()
    subtimer = StartTimer("[ client ] prepare data", YELLOW)

    hint := BytesToMatrix(offline[aes.BlockSize:], params.L, params.N)

    // this seeds the randomness when generating the lwe matrix
    var seed PRGKey
    copy(seed[:], offline[:aes.BlockSize])
    bufPrgReader = NewBufPRG(NewPRG(&seed))

    // generate the lwe matrix from the seed
    lweMatrix := protocol.DecompressState(dbInfo, *params, MakeCompressedState(&seed))

    subtimer.End()
    timer.End()

    ///////////////////////////////////////////////////////////

    fmt.Printf("[  both  ] offline comm (MB)\t: %.3f\n", float64(len(offline)) / 1000000)

    // let the server know we're ready for online
    ready = []byte{1}
    server.Write(ready)

    ////////////////////////// ONLINE /////////////////////////

    queryTimer := CreateTimer("[ client ] pir query", YELLOW)
    recoverTimer := CreateTimer("[ client ] pir recover", YELLOW)
    timer = StartTimer("[ client ] pir online", BLUE)

    comm := 0;
    var results []byte
    queried := map[uint64]struct{}{}
    for _, col := range queries {

        // translate the hash value into the column by dividing by the number
        // of buckets per column
        translated := col / psiParams.BucketsPerCol

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

        // if we've already queried this column, don't bother recovering
        if _, already_queried := queried[translated]; already_queried {
            continue;
        } else {
            queried[translated] = struct{}{}
        }

        answer := BytesToMatrix(data, params.L, 1)

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

    // find intersection between OPRF and PIR results
    subtimer = StartTimer("[ client ] find result", YELLOW)
    found := 0
    for i := 0; i < len(results); i += ENTRY_SIZE {
        for j := 0; j < len(oprf); j += ENTRY_SIZE {
            if bytes.Equal(results[i:i+ENTRY_SIZE], oprf[j:j+ENTRY_SIZE]) {
                found++;
            }
        }
    }
    subtimer.Stop()
    timer.End()
    queryTimer.Print()
    recoverTimer.Print()
    subtimer.Print()

    ///////////////////////////////////////////////////////////

    fmt.Printf("[  both  ] online comm (MB)\t: %.3f\n", float64(comm) / 1000000)

    if found == expected {
        fmt.Printf("%s>>>>>>>>>>>>>>> SUCCESS <<<<<<<<<<<<<<<%s\n", GREEN, RESET)
    } else {
        fmt.Printf("%s>>>>>>>>> FAILURE [%d vs %d] >>>>>>>>>%s\n", RED, found, expected, RESET)
    }

}
