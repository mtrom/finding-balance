package main

import (
	"crypto/aes"
    "bytes"
    "encoding/binary"
    "fmt"
    "net"
    "time"

    . "github.com/ahenzinger/simplepir/pir"
)

const (
    CLIENT_OPRF_RESULT  = "out/client.edb"
    CLIENT_QUERIES      = "out/queries.db"

    // running with cuckoo
    CLIENT_QUERY_PREFIX = "out/"
    CLIENT_QUERY_SUFFIX = "/queries.db"
    CLIENT_OPRF_PREFIX  = "out/"
    CLIENT_OPRF_SUFFIX  = "/client.edb"
)

/**
 * this seeds the randomness and allows the client to generate the lwe matrix
 * from the seed, but the simplepir library doesn't export it so we need to
 * force this link
 */
// go:linkname bufPrgReader pir.bufPrgReader
var bufPrgReader *BufPRGReader

/**
 * run protocol as the client
 *
 * @param <psiParams> params of the greater psi protocol
 */
func RunClient(psiParams *PSIParams, expected int64) {

    // read in query indices
    queries := ReadQueries(CLIENT_QUERIES)

    // read in result of oprf
    _, oprf := ReadDatabase[byte](CLIENT_OPRF_RESULT)

    // connect to server
    connection, err := net.Listen(SERVER_TYPE, SERVER_HOST)
    if err != nil { panic(err) }
    defer connection.Close()

    server, err := connection.Accept()
    if err != nil { panic(err) }
    server.SetDeadline(time.Time{})

    // wait until the client is ready for offline
    ready := make([]byte, 1)
    server.Read(ready)

    ///////////////////////// OFFLINE /////////////////////////

    timer := StartTimer("[ client ] pir offline", BLUE)

    subtimer := StartTimer("[ client ] pick params", YELLOW)
    err = binary.Read(server, binary.LittleEndian, &psiParams.BucketSize)
    if err != nil { panic(err) }

    // decide protocol parameters
    protocol, params, entryBits := SetupProtocol(psiParams)
    dbInfo := SetupDBInfo(psiParams, entryBits, params)

    subtimer.End()
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

    PrintParams(params)
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
    found := int64(0)
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

type ClientState struct {
    params    *Params
    dbInfo    DBinfo
    protocol  SimplePIR
    hint      *Matrix
    lweMatrix State
}

func RunClientOffline(
    psiParams *PSIParams,
    server net.Conn,
) ClientState {
    err := binary.Read(server, binary.LittleEndian, &psiParams.BucketSize)
    if err != nil { panic(err) }

    // decide protocol parameters
    protocol, params, entryBits := SetupProtocol(psiParams)
    dbInfo := SetupDBInfo(psiParams, entryBits, params)

    // read in the 'offline' data (i.e., lwe matrix seed and hint)
    offline := ReadOverNetwork(server, aes.BlockSize + params.L * params.N * ELEMENT_SIZE)

    hint := BytesToMatrix(offline[aes.BlockSize:], params.L, params.N)

    // this seeds the randomness when generating the lwe matrix
    var seed PRGKey
    copy(seed[:], offline[:aes.BlockSize])
    bufPrgReader = NewBufPRG(NewPRG(&seed))

    // generate the lwe matrix from the seed
    lweMatrix := protocol.DecompressState(dbInfo, *params, MakeCompressedState(&seed))

    return ClientState{
        params: params,
        dbInfo: dbInfo,
        protocol: protocol,
        hint: hint,
        lweMatrix: lweMatrix,
    }
}

func RunClientOnline(
    states []ClientState,
    indices []uint64,
    server net.Conn,
    already_queried bool,
) [][]byte {
    secrets := make([]State, len(indices))
    queries := make([]Msg, len(indices))
    requests := make([][]byte, len(indices))

    for i, index := range indices {
        secret, query := states[i].protocol.Query(
            index,
            states[i].lweMatrix,
            *states[i].params,
            states[i].dbInfo,
        )
        secrets[i] = secret
        queries[i] = query
        requests[i] = MatrixToBytes(query.Data[0])
    }

    for _, req := range requests {
        WriteOverNetwork(server, req)
    }

    responses := make([][]byte, len(queries))
    for i := range queries {
        // read the query's answer
        responses[i] = ReadOverNetwork(server, states[i].params.L * ELEMENT_SIZE)
    }

    // padding elements will all be sequences of 0
    var paddingElement [ENTRY_SIZE]byte
    cols := make([][]byte, len(responses))
    for i, response := range responses {
        // if we've already queried this column, don't bother recovering
        answer := BytesToMatrix(response, states[i].params.L, 1)

        // reconstruct the data based on the answer
        raw := RecoverColumn(
            MakeMsg(states[i].hint),
            queries[i],
            MakeMsg(answer),
            secrets[i],
            *states[i].params,
            states[i].dbInfo,
        )

        for j := 0; j < len(raw); j += ENTRY_SIZE {
            if !bytes.Equal(raw[j:j+ENTRY_SIZE], paddingElement[:]) {
                cols[i] = append(cols[i], raw[j:j+ENTRY_SIZE]...)
            }
        }
    }

    return cols
}

func RunClientOnlineMultiThread(
    states []ClientState,
    indices []uint64,
    server net.Conn,
    already_queried bool,
) [][]byte {
    secrets := make([]State, len(indices))
    queries := make([]Msg, len(indices))

    channels := make([]chan []byte, len(states))
    for i, index := range indices {
        channels[i] = make(chan []byte)
        go func(i int, index uint64, state ClientState) {
            secret, query := state.protocol.Query(
                index,
                state.lweMatrix,
                *state.params,
                state.dbInfo,
            )
            secrets[i] = secret
            queries[i] = query
            channels[i] <- MatrixToBytes(query.Data[0])
        }(i, index, states[i])
    }
    for _, channel := range channels {
        req := <-channel
        WriteOverNetwork(server, req)
    }

    // padding elements will all be sequences of 0
    var paddingElement [ENTRY_SIZE]byte

    for i, state := range states {
        go func(i int, state ClientState) {
            response := <-channels[i]
            // if we've already queried this column, don't bother recovering
            answer := BytesToMatrix(response, state.params.L, 1)

            // reconstruct the data based on the answer
            raw := RecoverColumn(
                MakeMsg(state.hint),
                queries[i],
                MakeMsg(answer),
                secrets[i],
                *state.params,
                state.dbInfo,
            )

            col := make([]byte, 0)
            for j := 0; j < len(raw); j += ENTRY_SIZE {
                if !bytes.Equal(raw[j:j+ENTRY_SIZE], paddingElement[:]) {
                    col = append(col, raw[j:j+ENTRY_SIZE]...)
                }
            }
            channels[i] <- col
        }(i, state)
    }

    for i := range queries {
        // read the query's answer
        channels[i] <- ReadOverNetwork(server, states[i].params.L * ELEMENT_SIZE)
    }

    cols := make([][]byte, len(queries))
    for i := range cols {
        cols[i] = <-channels[i]
    }

    return cols
}
