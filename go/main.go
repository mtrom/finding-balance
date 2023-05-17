package main

import (
    "bytes"
    "flag"
    "fmt"
    "io/ioutil"
    "log"
    "net"
    "os"
    "runtime"
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
    threads       := flag.Uint("threads", 1, "number of threads to run at once")

    cuckooN       := flag.Int64("cuckoo-n", -1, "total number of buckets in server's hash table")

    // client-only flag
    expected := flag.Int64("expected", -1, "expected size of intersection")

    // server-only flag
    queries := flag.Int64("queries-log", -1, "log of the number of pir queries")

    flag.Parse()

    if *bucketN == -1 { fmt.Println("expected --bucket-n argument"); os.Exit(1) }
    if *bucketSize == -1 { fmt.Println("expected --bucket-size argument"); os.Exit(1) }
    if *bucketsPerCol == -1 { fmt.Println("expected --bucket-per-col argument"); os.Exit(1) }

    psiParams := PSIParams{
        CuckooN: uint64(*cuckooN),
        BucketN: uint64(*bucketN),
        BucketSize: uint64(*bucketSize),
        BucketsPerCol: uint64(*bucketsPerCol),
        Threads: *threads,
    }

    // limit the number of threads
    runtime.GOMAXPROCS(int(*threads))

    if *client {
        if *expected == -1 { fmt.Println("need --expected argument"); os.Exit(1) }

        if *cuckooN == -1 {
            RunClient(&psiParams, *expected)
            os.Exit(0)
        }

        // connect to server
        connection, err := net.Listen(SERVER_TYPE, SERVER_HOST)
        if err != nil { panic(err) }
        defer connection.Close()

        server, err := connection.Accept()
        if err != nil { panic(err) }

        queries  := make([][]uint64, *cuckooN)
        oprfs    := make([][]byte, *cuckooN)

        for i := int64(0); i < *cuckooN; i++ {
            queries[i] = ReadQueries(
                fmt.Sprintf("%s%d%s", CLIENT_QUERY_PREFIX, i, CLIENT_QUERY_SUFFIX),
            )
            _, oprfs[i] = ReadDatabase[byte](
                fmt.Sprintf("%s%d%s", CLIENT_OPRF_PREFIX, i, CLIENT_OPRF_SUFFIX),
            )
        }

        timer := StartTimer("[ client ] pir offline", BLUE)
        states := make([]ClientState, *cuckooN)
        for i := int64(0); i < *cuckooN; i++ {
            states[i] = RunClientOffline(&psiParams, server)
        }
        timer.End()

        timer = StartTimer("[ client ] pir online", BLUE)
        indices := make([]uint64, *cuckooN)
        for i, query := range queries {
            // translate the hash value into the column by dividing by the number
            // of buckets per column
            indices[i] = query[0] / psiParams.BucketsPerCol
        }

        var results [][]byte
        if psiParams.Threads == 1 {
            results = RunClientOnline(
                &psiParams,
                states,
                indices,
                server,
                false,
            )
        } else {
            results = RunClientOnlineMultiThread(
                &psiParams,
                states,
                indices,
                server,
                false,
            )
        }

        // find intersection between OPRF and PIR results
        found := int64(0)
        for c := int64(0); c < *cuckooN; c++ {
            for i := 0; i < len(results[c]); i += ENTRY_SIZE {
                for j := 0; j < len(oprfs[c]); j += ENTRY_SIZE {
                    if bytes.Equal(results[c][i:i+ENTRY_SIZE], oprfs[c][j:j+ENTRY_SIZE]) {
                        found++
                    }
                }
            }
        }
        timer.End()

        if found == *expected {
            fmt.Printf("%s>>>>>>>>>>>>>>> SUCCESS <<<<<<<<<<<<<<<%s\n", GREEN, RESET)
        } else {
            fmt.Printf("%s>>>>>>>>> FAILURE [%d vs %d] >>>>>>>>>%s\n", RED, found, *expected, RESET)
        }

    } else if *server {
        if *cuckooN == -1 {
            if *queries == -1 { fmt.Println("expected --queries-log argument"); os.Exit(1) }
            RunServer(1 << *queries, &psiParams)
            os.Exit(0)
        }

        // connect to client
        client, err := net.Dial(SERVER_TYPE, SERVER_HOST)
        if err != nil { panic(err) }
        defer client.Close()

        datasets := ReadServerInputs(*cuckooN, &psiParams)

        timer := StartTimer("[ server ] pir offline", RED)
        states := make([]ServerState, *cuckooN)

        if psiParams.Threads == 1 {
            payloads := make([][]byte, *cuckooN)
            for i, dataset := range datasets {
                result := RunServerOffline(&psiParams, dataset)
                states[i] = result.state
                payloads[i] = result.payload
            }
            for _, payload := range payloads {
                WriteOverNetwork(client, payload)
            }
        } else {
            channels := make([]chan ServerOfflineResult, *cuckooN)
            for i := range datasets {
                channels[i] = make(chan ServerOfflineResult)
                go func(i int) {
                    channels[i] <- RunServerOffline(&psiParams, datasets[i])
                }(i)
            }
            for i, channel := range channels {
                result := <-channel
                states[i] = result.state
                WriteOverNetwork(client, result.payload)
            }
        }
        timer.End()

        timer = StartTimer("[ server ] pir online", RED)
        RunServerOnline(&psiParams, states, client)
        timer.End()

    } else {
        fmt.Println("expected `client` or `server` subcommand")
        os.Exit(1)
    }
}

