package main

import (
    "flag"
    "io/ioutil"
    "fmt"
    "log"
    "os"
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

    if len(os.Args) < 2 {
        fmt.Println("expected `client` or `server` subcommand")
        os.Exit(1)
    }

    flag.Bool("client", false, "")
    flag.Bool("server", false, "")

    bucketN       := flag.Uint64("bucket-n", 0, "total number of buckets in server's hash table")
    bucketSize    := flag.Uint64("bucket-size", 0, "size of each bucket in server's hash table")
    bucketsPerCol := flag.Uint64("buckets-per-col", 0, "number of buckets in a col of the database")

    if os.Args[1] == "-client" {
        serverLog     := flag.Int64("server-log", -1, "log the size of the server's dataset")
        input         := flag.String("input", "out/queries.db", "file with pir queries")
        output        := flag.String("output", "out/answer.edb", "file to output pir answer")

        flag.Parse()

        if *serverLog == -1 { fmt.Println("expected --server-log argument"); os.Exit(1) }
        if *bucketN == 0 { fmt.Println("expected --bucket-n argument"); os.Exit(1) }
        if *bucketSize == 0 { fmt.Println("expected --bucket-size argument"); os.Exit(1) }
        if *bucketsPerCol == 0 { fmt.Println("expected --bucket-per-col argument"); os.Exit(1) }

        psiParams := PSIParams{
            BucketN: *bucketN,
            BucketSize: *bucketSize,
            BucketsPerCol: *bucketsPerCol,
        }

        RunClient(*input, *output, &psiParams)
    } else if os.Args[1] == "-server" {
        queries := flag.Int64("queries-log", -1, "number of pir queries")
        input   := flag.String("input", "out/server.edb", "file with server database")

        flag.Parse()

        if *queries == -1 { fmt.Println("expected --queries-log argument"); os.Exit(1) }
        if *bucketN == 0 { fmt.Println("expected --bucket-n argument"); os.Exit(1) }
        if *bucketSize == 0 { fmt.Println("expected --bucket-size argument"); os.Exit(1) }
        if *bucketsPerCol == 0 { fmt.Println("expected --bucket-per-col argument"); os.Exit(1) }

        psiParams := PSIParams{
            BucketN: *bucketN,
            BucketSize: *bucketSize,
            BucketsPerCol: *bucketsPerCol,
        }
        RunServer(*input, 1 << *queries, &psiParams)
    } else {
        fmt.Println("expected `client` or `server` subcommand")
        os.Exit(1)
    }
}

