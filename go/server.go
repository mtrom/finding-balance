package main

import (
	"crypto/aes"
    "encoding/binary"
    "fmt"
    "net"
    "sync"

    . "github.com/ahenzinger/simplepir/pir"
)

const (
    SERVER_HOST = "localhost:1122"
    SERVER_TYPE = "tcp"
    SERVER_DATABASE = "out/server.edb"
    SERVER_DATABASE_PREFIX = "out/"
    SERVER_DATABASE_SUFFIX = "/server.edb"

    CONNECTION_RETRIES = 5
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

    timer := StartTimer("[ server ] pir offline", RED)
    states := make([]*ServerState, psiParams.CuckooN)

    if psiParams.Threads == 1 {
        for i := range datasets {
            states[i] = CreateServerState(&params[i], datasets[i])
        }
    } else {
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

    // let the client know we're ready to send the hint
    ready := []byte{1}
    client.Write(ready)

    for i, state := range states {
        err := binary.Write(client, binary.LittleEndian, &params[i].BucketSize)
        if err != nil { panic(err) }
        WriteOverNetwork(client, state.Offline)
    }

    timer.End()

    ///////////////////////////////////////////////////////////

    // wait until the client is ready for online
    ready = make([]byte, 1)
    client.Read(ready)

    ////////////////////////// ONLINE /////////////////////////

    if psiParams.CuckooN == 1 {
        timer = StartTimer("[ server ] pir online", RED)
        for i := uint64(0); i < queries; i++ {
            request := ReadOverNetwork(client, states[0].QuerySize * ELEMENT_SIZE)
            response := states[0].AnswerQuery(request)
            WriteOverNetwork(client, response)
        }
    } else if psiParams.Threads == 1 {
        requests := make([][]byte, psiParams.CuckooN)
        for i, state := range states {
            requests[i] = ReadOverNetwork(client, state.QuerySize * ELEMENT_SIZE)
        }
        for i, state := range states {
            response := state.AnswerQuery(requests[i])
            WriteOverNetwork(client, response)
        }
    } else {
        channels := make([]chan []byte, psiParams.CuckooN)
        for i, state := range states {
            request := ReadOverNetwork(client, state.QuerySize * ELEMENT_SIZE)
            channels[i] = make(chan []byte)
            go func(i int, request []byte) {
                res := states[i].AnswerQuery(request)
                channels[i] <- res
            }(i, request) // TODO: can I remove request?
        }
        for i := range channels {
            response := <-channels[i]
            WriteOverNetwork(client, response)
        }
    }

    timer.End()
    ///////////////////////////////////////////////////////////
}

type ServerState struct {
    Params      *Params
    DB          *Database
    QuerySize   uint64
    LweMatrix   State
    Offline     []byte
}

func CreateServerState(psiParams *PSIParams, dataset []uint64) *ServerState {

    // decide protocol parameters and set up database
    protocol, params, ENTRY_BITS := SetupProtocol(psiParams)
    db := CreateDatabase(ENTRY_BITS, params, dataset)

    // the SimplePIR library has a global prng so the this cannot be parallelized
    mutex.Lock()

    // sample seed for lwe random matrix A
    lweMatrix, seed := protocol.InitCompressed(db.Info, *params)

    mutex.Unlock()

    // calculate hint seed
    _, hint := protocol.Setup(db, lweMatrix, *params)

    // gather lwe matrix seed and hint into 'offline' dataset
    var bytes = make([]byte, aes.BlockSize)
    ptr := *seed.Seed
    copy(bytes[:], ptr[:])
    offline := append(bytes, MatrixToBytes(hint.Data[0])...)

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
    return MatrixToBytes(answer.Data[0])
}

func ReadServerInputs(psiParams PSIParams) ([][]uint64, []PSIParams ) {
    datasets := make([][]uint64, psiParams.CuckooN)
    params   := make([]PSIParams, psiParams.CuckooN)

    for i := uint64(0); i < psiParams.CuckooN; i++ {
        filename := SERVER_DATABASE
        if (psiParams.CuckooN > 1) {
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
