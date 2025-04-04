package main

import (
	"crypto/aes"
    "encoding/binary"
    "fmt"
    "math"
    "net"
    "sync"
)

const (
    SERVER_HOST = "localhost:1122"
    SERVER_TYPE = "tcp"
    SERVER_DATABASE = "out/server.edb"
    SERVER_DATABASE_PREFIX = "out/"
    SERVER_DATABASE_SUFFIX = "/server.edb"

    CONNECTION_RETRIES = 5

    // size in bytes
    UINT64_SIZE = 8
)

var mutex sync.Mutex

/**
 * run protocol as the server
 *
 * @param <queries> number of queries to answer
 * @param <psiParams> params of the greater psi protocol
 */
func RunServer(psiParams *PSIParams, client net.Conn, queries uint64) {

    // read in encrypted database from file
    datasets, params := ReadServerInputs(*psiParams)

    ///////////////////////// OFFLINE /////////////////////////

    timer := StartTimer("[ server ] pir offline", BLUE)
    var states []*ServerState

    if psiParams.CuckooSize == 1 {
        states = make([]*ServerState, queries)
        state := CreateServerState(&params[0], datasets[0])
        // need a refernece to the state for each query
        for i := uint64(0); i < queries; i++ {
            states[i] = state
        }
    } else if psiParams.Threads == 1 {
        states = make([]*ServerState, psiParams.CuckooSize)
        for i := range datasets {
            states[i] = CreateServerState(&params[i], datasets[i])
        }
    } else {
        states = make([]*ServerState, psiParams.CuckooSize)
        var waitGroup sync.WaitGroup
        for i := range datasets {
            waitGroup.Add(1)
            go func(i int) {
                defer waitGroup.Done()
                states[i] = CreateServerState(&params[i], datasets[i])
            }(i)
        }
        waitGroup.Wait()
    }

    timer.End()

    ///////////////////////////////////////////////////////////

    for i := uint64(0); i < psiParams.CuckooSize; i++ {
        p := states[i].Params
        fmt.Printf("[  both  ] db dims (elems)\t: %dx%d\n", p.L, p.M)

        // taken from SimplePIR.database.SetupDB()
        fmt.Printf(
            "[  both  ] packed db size (MB)\t: %.3f\n",
            float64(p.L * p.M) * math.Log2(float64(p.P)) / (1024.0 * 1024.0 * 8.0),
        )
    }

    // let the client know we're ready to send the hint
    ready := []byte{1}
    client.Write(ready)

    comm := 0
    for i := uint64(0); i < psiParams.CuckooSize; i++ {
        err := binary.Write(client, binary.LittleEndian, &states[i].BucketSize)
        if err != nil { panic(err) }
        WriteOverNetwork(client, states[i].Offline)
        comm += UINT64_SIZE
        comm += len(states[i].Offline)
    }

    fmt.Printf("[  both  ] hint comm (MB)\t: %.3f\n", float64(comm) / 1000000)

    // wait until the client is ready for online
    ready = make([]byte, 1)
    client.Read(ready)

    ////////////////////////// ONLINE /////////////////////////

    upload := 0
    download := 0
    timer = StartTimer("[ server ] pir end2end", BLUE)
    if psiParams.Threads == 1 || len(states) == 1 {
        requests := make([][]byte, len(states))
        responses := make([][]byte, len(states))
        for i, state := range states {
            requests[i] = ReadOverNetwork(client, state.QuerySize * ELEMENT_SIZE)
            upload += len(requests[i])
        }
        comp := StartTimer("[ server ] online comp", BLUE)
        for i, state := range states {
            responses[i] = state.AnswerQuery(requests[i])
        }
        comp.End()
        for _, response := range responses {
            WriteOverNetwork(client, response)
            download += len(response)
        }
    } else {
        channels := make([]chan []byte, len(states))
        requests := make([][]byte, len(states))
        responses := make([][]byte, len(states))
        for i, state := range states {
            requests[i] = ReadOverNetwork(client, state.QuerySize * ELEMENT_SIZE)
            upload += len(requests[i])
        }
        comp := StartTimer("[ server ] online comp", BLUE)
        for i, request := range requests {
            channels[i] = make(chan []byte)
            go func(i int, request []byte) {
                res := states[i].AnswerQuery(request)
                channels[i] <- res
            }(i, request)
        }
        for i := range channels {
            responses[i] = <-channels[i]
        }
        comp.End()
        for _, response := range responses {
            WriteOverNetwork(client, response)
            download += len(response)
        }
    }
    timer.End()
    ///////////////////////////////////////////////////////////

    fmt.Printf("[  both  ] client->server (MB)\t: %.3f\n", float64(upload) / 1000000)
    fmt.Printf("[  both  ] client<-server (MB)\t: %.3f\n", float64(download) / 1000000)
}

type ServerState struct {
    Params     *Params
    DB         *Database
    QuerySize  uint64
    BucketSize uint64
    LweMatrix  State
    Offline    []byte
}

func CreateServerState(psiParams *PSIParams, dataset []uint64) *ServerState {

    // decide protocol parameters and set up database
    protocol, params, ENTRY_BITS := SetupProtocol(psiParams)
    db := CreateDatabase(ENTRY_BITS, params, dataset)

    // the SimplePIR library has a global prng so the this cannot be parallelized
    mutex.Lock()

    timer := StartTimer("[ server ] lwe matrix", BLUE)
    // sample seed for lwe random matrix A
    lweMatrix, seed := protocol.InitCompressed(db.Info, *params)
    timer.End()

    mutex.Unlock()

    timer = StartTimer("[ server ] calc hint", BLUE)
    // calculate hint seed
    _, hint := protocol.Setup(db, lweMatrix, *params, *psiParams)
    timer.End()

    // gather lwe matrix seed and hint into 'offline' dataset
    var bytes = make([]byte, aes.BlockSize)
    ptr := *seed.Seed
    copy(bytes[:], ptr[:])
    offline := append(bytes, ModuloSwitch(hint.Data[0])...)

    // expected size of incoming query vectors
    querySize := params.M

	// compensate for this squish artifact
	if params.M % db.Info.Squishing != 0 {
		querySize += db.Info.Squishing - (params.M % db.Info.Squishing)
	}

    return &ServerState{
        Params: params,
        DB: db,
        QuerySize: querySize,
        BucketSize: psiParams.BucketSize,
        LweMatrix: lweMatrix,
        Offline: offline,
    }
}

func (state* ServerState) AnswerQuery(request []byte) []byte {
    protocol := SimplePIR{}
    query := BytesToMatrix(request, state.QuerySize, 1)
    answer := protocol.Answer(
        state.DB,
        MakeMsgSlice(MakeMsg(query)),
        MakeState(),
        state.LweMatrix,
        *state.Params,
    )
    return ModuloSwitch(answer.Data[0])
}

func ReadServerInputs(psiParams PSIParams) ([][]uint64, []PSIParams ) {
    datasets := make([][]uint64, psiParams.CuckooSize)
    params   := make([]PSIParams, psiParams.CuckooSize)

    for i := uint64(0); i < psiParams.CuckooSize; i++ {
        filename := SERVER_DATABASE
        if (psiParams.CuckooSize > 1) {
            filename = fmt.Sprintf(
                "%s%d%s", SERVER_DATABASE_PREFIX, i, SERVER_DATABASE_SUFFIX,
            )
        }
        metadata, dataset := ReadDatabase[byte, uint64](filename, "bucketSize")

        params[i] = psiParams
        params[i].BucketSize = metadata["bucketSize"]

        if uint64(len(dataset)) != params[i].DBBytes() {
            panic("database size inconsistent between file and params")
        }

        datasets[i] = dataset
    }
    return datasets, params
}
