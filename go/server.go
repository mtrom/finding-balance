package main

import (
	"crypto/aes"
    "encoding/binary"
    "fmt"
    "net"
    "time"
    "sync"

    . "github.com/ahenzinger/simplepir/pir"
)

const (
    SERVER_DATABASE = "out/server.edb"
    SERVER_DATABASE_PREFIX = "out/"
    SERVER_DATABASE_SUFFIX = "/server.edb"
)

var mutex sync.Mutex

/**
 * run protocol as the server
 *
 * @param <queries> number of queries to answer
 * @param <psiParams> params of the greater psi protocol
 */
func RunServer(queries uint64, psiParams *PSIParams) {

    // read in encrypted database from file
    metadata, values := ReadDatabase[uint64](SERVER_DATABASE, "bucketSize")

    dynamicBucketSize := false
    if psiParams.BucketSize == 0 {
        dynamicBucketSize = true
        psiParams.BucketSize = metadata["bucketSize"]
    } else if psiParams.BucketSize != metadata["bucketSize"] {
        panic(fmt.Sprintf(
            "bucket size inconsistent between file and params: %d vs. %d",
            metadata["bucketSize"], psiParams.BucketSize,
        ))
    }

    if uint64(len(values)) != psiParams.DBBytes() {
        panic(fmt.Sprintf(
            "database size inconsistent between file and params: %d vs. %d",
            len(values), psiParams.DBBytes(),
        ))
    }

    // connect to client
    client, err := net.Dial(SERVER_TYPE, SERVER_HOST)
    if err != nil { panic(err) }
    defer client.Close()
    client.SetDeadline(time.Time{})

    ///////////////////////// OFFLINE /////////////////////////

    timer := StartTimer("[ server ] pir offline", RED)

    subtimer := StartTimer("[ server ] build database", GREEN)

    // decide protocol parameters and set up database
    protocol, params, ENTRY_BITS := SetupProtocol(psiParams)
    db := CreateDatabase(ENTRY_BITS, params, values)

    subtimer.End()
    subtimer = StartTimer("[ server ] lwe + hint", GREEN)

    // sample seed for lwe random matrix A
    lweMatrix, seed := protocol.InitCompressed(db.Info, *params)

    // calculate hint seed
    serverState, hint := protocol.Setup(db, lweMatrix, *params)

    subtimer.End()
    subtimer = StartTimer("[ server ] prepare data", GREEN)

    // gather lwe matrix seed and hint into 'offline' dataset
    var bytes = make([]byte, aes.BlockSize)
    ptr := *seed.Seed
    copy(bytes[:], ptr[:])
    offline := append(bytes, MatrixToBytes(hint.Data[0])...)

    subtimer.End()
    subtimer = StartTimer("[ server ] send data", GREEN)

    // let the client know we're ready for offline
    ready := []byte{1}
    client.Write(ready)

    if dynamicBucketSize {
        err := binary.Write(client, binary.LittleEndian, &psiParams.BucketSize)
        if err != nil { panic(err) }
    }

    // because the offline data is so large, it needs to be chunked
    WriteOverNetwork(client, offline)

    subtimer.End()
    timer.End()

    ///////////////////////////////////////////////////////////

    // wait until the client is ready for online
    ready = make([]byte, 1)
    client.Read(ready)

    ////////////////////////// ONLINE /////////////////////////

    timer = StartTimer("[ server ] pir online", RED)

    // expected size of the query vector
    queryRows := params.M

	// compensate for this squish artifact
	if params.M % db.Info.Squishing != 0 {
		queryRows += db.Info.Squishing - (params.M % db.Info.Squishing)
	}

    answerTimer := CreateTimer("[ server ] pir answer", GREEN)
    for i := uint64(0); i < queries; i++ {
        // read in query vector
        data := ReadOverNetwork(client, queryRows * ELEMENT_SIZE)
        query := BytesToMatrix(data, queryRows, 1)

        // generate answer and send to client
        answerTimer.Start()
        answer := protocol.Answer(db, MakeMsgSlice(MakeMsg(query)), serverState, lweMatrix, *params)
        answerTimer.Stop()

        WriteOverNetwork(client, MatrixToBytes(answer.Data[0]))
    }

    timer.End()
    answerTimer.Print()
}

type ServerState struct {
    params      *Params
    protocol    SimplePIR
    db          *Database
    lweMatrix   State
}

type ServerOfflineResult struct {
    state  ServerState
    payload []byte
}

func RunServerOffline(
    psiParams *PSIParams,
    values []uint64,
) (ServerOfflineResult) {

    // decide protocol parameters and set up database
    protocol, params, ENTRY_BITS := SetupProtocol(psiParams)
    db := CreateDatabase(ENTRY_BITS, params, values)

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

    state := ServerState{
        params: params,
        protocol: protocol,
        db: db,
        lweMatrix: lweMatrix,
    }

    return ServerOfflineResult{state: state, payload: offline}
}

func RunServerOnline(
    psiParams *PSIParams,
    states []ServerState,
    client net.Conn,
) {
    queries := make([]*Matrix, len(states))
    for i, state := range states {
        // expected size of the query vector
        queryRows := state.params.M

        // compensate for this squish artifact
        if state.params.M % state.db.Info.Squishing != 0 {
            queryRows += state.db.Info.Squishing - (state.params.M % state.db.Info.Squishing)
        }

        // read in query vector
        req := ReadOverNetwork(client, queryRows * ELEMENT_SIZE)
        queries[i] = BytesToMatrix(req, queryRows, 1)
    }

    if psiParams.Threads == 1 {
        for i, state := range states {
            // generate answer and send to client
            answer := state.protocol.Answer(
                state.db,
                MakeMsgSlice(MakeMsg(queries[i])),
                MakeState(),
                state.lweMatrix,
                *state.params,
            )
            res := MatrixToBytes(answer.Data[0])
            WriteOverNetwork(client, res)
        }
    } else {
        channels := make([]chan []byte, len(states))
        for i, state := range states {
            channels[i] = make(chan []byte)
            go func(i int, state ServerState) {
                answer := state.protocol.Answer(
                    state.db,
                    MakeMsgSlice(MakeMsg(queries[i])),
                    MakeState(),
                    state.lweMatrix,
                    *state.params,
                )
                channels[i] <- MatrixToBytes(answer.Data[0])
            }(i, state)
        }
        for _, channel := range channels {
            res := <-channel
            WriteOverNetwork(client, res)
        }
    }
}
/**
 * run protocol
 */
func RunServerAsync(
    queries uint64,
    psiParams *PSIParams,
    dynamicBucketSize bool,
    values []uint64,
    client net.Conn,
    offline_comm chan<- int64,
    online_comm chan<- int64,
) {
    ///////////////////////// OFFLINE /////////////////////////

    // decide protocol parameters and set up database
    protocol, params, ENTRY_BITS := SetupProtocol(psiParams)
    db := CreateDatabase(ENTRY_BITS, params, values)

    // sample seed for lwe random matrix A
    lweMatrix, seed := protocol.InitCompressed(db.Info, *params)

    // calculate hint seed
    serverState, hint := protocol.Setup(db, lweMatrix, *params)

    // gather lwe matrix seed and hint into 'offline' dataset
    var bytes = make([]byte, aes.BlockSize)
    ptr := *seed.Seed
    copy(bytes[:], ptr[:])
    offline := append(bytes, MatrixToBytes(hint.Data[0])...)

    if dynamicBucketSize {
        err := binary.Write(client, binary.LittleEndian, &psiParams.BucketSize)
        if err != nil { panic(err) }
    }

    WriteOverNetwork(client, offline)

    ///////////////////////////////////////////////////////////

    ////////////////////////// ONLINE /////////////////////////

    // expected size of the query vector
    queryRows := params.M

	// compensate for this squish artifact
	if params.M % db.Info.Squishing != 0 {
		queryRows += db.Info.Squishing - (params.M % db.Info.Squishing)
	}

    comm := 0
    for i := uint64(0); i < queries; i++ {
        // read in query vector
        req := ReadOverNetwork(client, queryRows * ELEMENT_SIZE)
        query := BytesToMatrix(req, queryRows, 1)

        comm += len(req)

        // generate answer and send to client
        answer := protocol.Answer(db, MakeMsgSlice(MakeMsg(query)), serverState, lweMatrix, *params)
        res := MatrixToBytes(answer.Data[0])

        WriteOverNetwork(client, res)

        comm += len(res)
    }

    // size of communication
    offline_comm <- int64(len(offline))
    online_comm <- int64(comm)
}
