package main

import (
	"crypto/aes"
    "bytes"
    "encoding/binary"
    "net"
)

const (
    CLIENT_OPRF_RESULT  = "out/client.edb"
    CLIENT_QUERIES      = "out/queries.db"

    BLANK_QUERY = ^uint64(0)
)

func RunClient(psiParams *PSIParams, server net.Conn) int64 {

    // read in query indices
    _, queries := ReadDatabase[uint64, uint64](CLIENT_QUERIES)

    // read in result of oprf
    _, oprf := ReadDatabase[byte, byte](CLIENT_OPRF_RESULT)

    // wait until the server is ready for offline
    ready := make([]byte, 1)
    server.Read(ready)

    /////////////////// OFFLINE ////////////////////
    timer := StartTimer("[ client ] pir offline", YELLOW)
    found := int64(0)
    states := make([]*ClientState, 0)
    if psiParams.CuckooSize == 1 {
        state := CreateClientState(psiParams, server)
        // need a copy of the state for each query
        for _ = range queries {
            cpy := *state
            states = append(states, &cpy)
        }
    } else {
        for i := uint64(0); i < psiParams.CuckooSize; i++ {
            states = append(states, CreateClientState(psiParams, server))
        }
    }
    timer.End()
    ////////////////////////////////////////////////

    //////////////////// ONLINE ////////////////////
    timer = StartTimer("[ client ] pir end2end", YELLOW)
    comp := StartTimer("[ client ] online comp", YELLOW)
    network := CreateTimer("[ client ] online net", YELLOW)
    requests := make([][]byte, len(states))
    for i, state := range states {
        requests[i] = state.ComputeQuery(queries[i])
    }
    comp.Stop()

    // let the server know we're ready for online
    ready = []byte{1}
    server.Write(ready)

    network.Start()
    for _, request := range requests {
        WriteOverNetwork(server, request)
    }
    network.Stop()

    for i, state := range states {
        network.Start()
        response := ReadOverNetwork(server, state.Params.L * ELEMENT_SIZE)
        network.Stop()
        comp.Start()
        found += state.ReadResponse(response, oprf[i*ENTRY_SIZE:(i+1)*ENTRY_SIZE])
        comp.Stop()
    }
    comp.End()
    timer.End()
    ////////////////////////////////////////////////

    return found
}

/**
 * holds info between offline and online
 */
type ClientState struct {
    Params        *Params
    BucketsPerCol uint64
    DBInfo        DBinfo
    Hint          *Matrix
    LweMatrix     State
    Secret        *State
    Query         *Msg
    SkipRecover   bool
}


/**
 * download hint / lwe matrix and compute parameters
 */
func CreateClientState(psiParams *PSIParams, server net.Conn) *ClientState {
    comp := CreateTimer("[ client ] offline comp", YELLOW)
    network := StartTimer("[ client ] offline net", YELLOW)
    // read bucket size from server
    err := binary.Read(server, binary.LittleEndian, &psiParams.BucketSize)
    if err != nil { panic(err) }
    network.Stop()

    comp.Start()

    // decide protocol parameters
    protocol, params, entryBits := SetupProtocol(psiParams)
    dbInfo := SetupDBInfo(psiParams, entryBits, params)

    comp.Stop()
    network.Start()
    // read in the 'offline' data (i.e., lwe matrix seed and hint)
    offline := ReadOverNetwork(server, aes.BlockSize + params.L * params.N * ELEMENT_SIZE)
    network.End()
    comp.Start()
    hint := BytesToMatrix(offline[aes.BlockSize:], params.L, params.N)

    // this seeds the randomness when generating the lwe matrix
    var seed PRGKey
    copy(seed[:], offline[:aes.BlockSize])
    bufPrgReader = NewBufPRG(NewPRG(&seed))

    // generate the lwe matrix from the seed
    lweMatrix := protocol.DecompressState(dbInfo, *params, MakeCompressedState(&seed))

    comp.End()
    return &ClientState{
        Params: params,
        DBInfo: dbInfo,
        BucketsPerCol: psiParams.BucketsPerCol,
        Hint: hint,
        LweMatrix: lweMatrix,
        Secret: nil,
        Query: nil,
        SkipRecover: false,
    }
}

/**
 * send out a query for a column
 */
func (state* ClientState) ComputeQuery(column uint64) []byte {
    protocol := SimplePIR{}

    // translate the hash table index into the rectangular database column
    //  (or if this is a blank query just use 0)
    translated := column / state.BucketsPerCol
    if column == BLANK_QUERY {
        translated = 0
        state.SkipRecover = true
    }

    // generate the query vector and send to server
    secret, query := protocol.Query(
        translated,
        state.LweMatrix,
        *state.Params,
        state.DBInfo,
    )

    state.Secret, state.Query = &secret, &query

    return MatrixToBytes(state.Query.Data[0])
}

/**
 *  process query response and determine intersection
 */
func (state* ClientState) ReadResponse(response, oprf []byte) int64 {

    // if this is a blank query, don't bother recovering
    if state.SkipRecover { return 0; }

    answer := BytesToMatrix(response, state.Params.L, 1)

    // reconstruct the data based on the answer
    var results []byte
    column := RecoverColumn(
        MakeMsg(state.Hint),
        *state.Query,
        MakeMsg(answer),
        *state.Secret,
        *state.Params,
        state.DBInfo,
    )

    // only add non-zero results (i.e., not padding)
    var PADDING [ENTRY_SIZE]byte
    for j := 0; j < len(column); j += ENTRY_SIZE {
        if !bytes.Equal(column[j:j+ENTRY_SIZE], PADDING[:]) {
            results = append(results, column[j:j+ENTRY_SIZE]...)
        }
    }

    // find intersection between oprf and pir results
    intersection := int64(0)
    for i := 0; i < len(results); i += ENTRY_SIZE {
        if bytes.Equal(results[i:i+ENTRY_SIZE], oprf) {
            intersection++;
        }
    }

    return intersection
}
