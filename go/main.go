package main

import (
    "io/ioutil"
    "log"
    "math"
    "os"
    "strconv"
)

const (
    SERVER_HOST = "localhost:1122"
    SERVER_TYPE = "tcp"
    CHUNK_SIZE  = 2048
)

func RunBoth(dbFn, queriesFn, outputFn string) {
    protocol, params, db, bucketSize := SetupProtocolAndDB(dbFn)

    log.Printf(
        "Packing = %d, Ne = %d, Basis = %d, Squishing = %d, Cols = %d\n",
        db.Info.Packing, db.Info.Ne, db.Info.Basis, db.Info.Squishing, db.Info.Cols,
    )
    queries := ReadQueries(queriesFn)
    results := RunProtocol(&protocol, db, *params, queries, bucketSize)
    WriteDatabase(outputFn, results)
}

func main() {
    log.SetOutput(ioutil.Discard)

    if os.Args[1] == "client" {
        serverN, _ := strconv.ParseFloat(os.Args[2], 64)
        // bucketSize, _ := strconv.ParseUint(os.Args[3], 10, 64)

        bucketSize := POINT_SIZE * uint64(math.Log2(serverN))
        dbSize := uint64(serverN) * bucketSize

        if len(os.Args) < 5 {
            RunClient("out/queries.db", "out/answer.edb", dbSize, bucketSize)
        } else {
            RunClient(os.Args[3], os.Args[4], dbSize, bucketSize)
        }
    } else if os.Args[1] == "server" {
        if len(os.Args) < 3 {
            RunServer("out/server.edb")
        } else {
            RunServer(os.Args[2])
        }
    } else {
        RunBoth(os.Args[1], os.Args[2], os.Args[3])
    }
}

