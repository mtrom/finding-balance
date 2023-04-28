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
    RunProtocol(&protocol, db, *params, queries, bucketSize)
}

func main() {
    log.SetOutput(ioutil.Discard)

    if len(os.Args) < 2 {
        fmt.Println("expected `client` or `server` subcommand")
        os.Exit(1)
    }

    client := flag.Bool("client", false, "run the pir protocol as the client")
    server := flag.Bool("server", false, "run the pir protocol as the server")

    bucketN       := flag.Int64("bucket-n", -1, "total number of buckets in server's hash table")
    bucketSize    := flag.Int64("bucket-size", -1, "size of each bucket in server's hash table")
    bucketsPerCol := flag.Int64("buckets-per-col", -1, "number of buckets in a col of the database")

    //client-only flag
    expected := flag.Int64("expected", -1, "expected size of intersection")

    // server-only flag
    queries := flag.Int64("queries-log", -1, "log of the number of pir queries")

    flag.Parse()

    if *bucketN == -1 { fmt.Println("expected --bucket-n argument"); os.Exit(1) }
    if *bucketSize == -1 { fmt.Println("expected --bucket-size argument"); os.Exit(1) }
    if *bucketsPerCol == -1 { fmt.Println("expected --bucket-per-col argument"); os.Exit(1) }

    psiParams := PSIParams{
        BucketN: uint64(*bucketN),
        BucketSize: uint64(*bucketSize),
        BucketsPerCol: uint64(*bucketsPerCol),
    }

    if *client {
        if *expected == -1 { fmt.Println("need --expected argument"); os.Exit(1) }

        RunClient(&psiParams, *expected)
    } else if *server {
        if *queries == -1 { fmt.Println("expected --queries-log argument"); os.Exit(1) }

        RunServer(1 << *queries, &psiParams)
    } else {
        fmt.Println("expected `client` or `server` subcommand")
        os.Exit(1)
    }
}

