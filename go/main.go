package main

import (
    "io/ioutil"
    "log"
    "os"
    "strconv"
)

const (
    SERVER_HOST = "localhost:1122"
    SERVER_TYPE = "tcp"
)

func RunBoth(dbFn, queriesFn, outputFn string) {
    protocol, params, db, bucketSize := SetupProtocolAndDB(dbFn)

    queries := ReadQueries(queriesFn)
    results := RunProtocol(&protocol, db, *params, queries, bucketSize)
    WriteDatabase(outputFn, results)
}

func main() {
    log.SetOutput(ioutil.Discard)

    if os.Args[1] == "client" {
        serverLog, _ := strconv.ParseUint(os.Args[2], 10, 64)

        bucketSize := POINT_SIZE * serverLog
        dbSize := uint64(1 << serverLog) * bucketSize

        if len(os.Args) < 5 {
            RunClient("out/queries.db", "out/answer.edb", dbSize, bucketSize)
        } else {
            RunClient(os.Args[3], os.Args[4], dbSize, bucketSize)
        }
    } else if os.Args[1] == "server" {
        queries, _ := strconv.ParseUint(os.Args[2], 10, 64)
        if len(os.Args) < 4 {
            RunServer("out/server.edb", 1 << queries)
        } else {
            RunServer(os.Args[3], 1 << queries)
        }
    } else {
        RunBoth(os.Args[1], os.Args[2], os.Args[3])
    }
}

