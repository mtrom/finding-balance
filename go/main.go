package main

import (
    "encoding/binary"
    "flag"
    "fmt"
    "io/ioutil"
    "log"
    "net"
    "os"
    "runtime"
    "time"
)

func main() {
    log.SetOutput(ioutil.Discard)

    if len(os.Args) < 2 {
        fmt.Println("expected `client` or `server` subcommand")
        os.Exit(1)
    }

    client := flag.Bool("client", false, "run the pir protocol as the client")
    server := flag.Bool("server", false, "run the pir protocol as the server")

    bucketN       := flag.Int64("bucket-n", -1, "total number of buckets in server's hash table")
    bucketsPerCol := flag.Int64("buckets-per-col", -1, "number of buckets in a col of the database")
    threads       := flag.Uint("threads", 1, "number of threads to run at once")

    cuckooN       := flag.Int64("cuckoo-n", 1, "total number of buckets in server's hash table")

    // client-only flag
    expected := flag.Int64("expected", -1, "expected size of intersection")

    // server-only flag
    queries_log := flag.Int64("queries-log", -1, "log of the number of pir queries")

    flag.Parse()

    if *bucketN == -1 { fmt.Println("expected --bucket-n argument"); os.Exit(1) }
    if *bucketsPerCol == -1 { fmt.Println("expected --bucket-per-col argument"); os.Exit(1) }

    psiParams := PSIParams{
        CuckooN: uint64(*cuckooN),
        BucketN: uint64(*bucketN),
        BucketSize: 0,
        BucketsPerCol: uint64(*bucketsPerCol),
        Threads: *threads,
    }

    // limit the number of threads
    runtime.GOMAXPROCS(int(*threads))

    if *client {
        if *expected == -1 { fmt.Println("need --expected argument"); os.Exit(1) }

        // connect to server
        connection, err := net.Listen(SERVER_TYPE, SERVER_HOST)
        if err != nil { panic(err) }
        defer connection.Close()

        server, err := connection.Accept()
        if err != nil { panic(err) }
        server.SetDeadline(time.Time{})

        // run protocol
        actual := RunClient(&psiParams, server)

        // report results
        if actual == *expected {
            fmt.Printf("%s>>>>>>>>>>>>>>> SUCCESS <<<<<<<<<<<<<<<%s\n", GREEN, RESET)
            os.Exit(0)
        } else {
            fmt.Printf(
                "%s>>>>>>>>> FAILURE [%d vs %d] >>>>>>>>>%s\n",
                RED, actual, expected, RESET,
            )
            os.Exit(1)
        }

    } else if *server {
        if *cuckooN == 1 {
            if *queries_log == -1 { fmt.Println("expected --queries-log argument"); os.Exit(1) }
            RunServer(1 << *queries_log, &psiParams)
            os.Exit(0)
        }

        // connect to client
        var client net.Conn
        for i := 0; i < CONNECTION_RETRIES; i++ {
            conn, err := net.Dial(SERVER_TYPE, SERVER_HOST)
            if err == nil { client = conn; break; }
            if i + 1 == CONNECTION_RETRIES { panic(err) }
            time.Sleep((2 << i) * time.Millisecond)
        }
        defer client.Close()

        datasets, params := ReadServerInputs(*cuckooN, psiParams)

        timer := StartTimer("[ server ] pir offline", RED)
        states := make([]ServerState, *cuckooN)

        if psiParams.Threads == 1 {
            payloads := make([][]byte, *cuckooN)
            for i, dataset := range datasets {
                result := RunServerOffline(&params[i], dataset)
                states[i] = result.state
                payloads[i] = result.payload
            }

            // let the client know we're ready for offline
            ready := []byte{1}
            client.Write(ready)

            for i, payload := range payloads {
                err := binary.Write(client, binary.LittleEndian, &params[i].BucketSize)
                if err != nil { panic(err) }
                WriteOverNetwork(client, payload)
            }
        } else {
            channels := make([]chan ServerOfflineResult, *cuckooN)
            for i := range datasets {
                channels[i] = make(chan ServerOfflineResult)
                go func(i int) {
                    channels[i] <- RunServerOffline(&params[i], datasets[i])
                }(i)
            }

            // let the client know we're ready for offline
            ready := []byte{1}
            client.Write(ready)

            for i, channel := range channels {
                result := <-channel
                states[i] = result.state
                err := binary.Write(client, binary.LittleEndian, &params[i].BucketSize)
                if err != nil { panic(err) }
                WriteOverNetwork(client, result.payload)
            }
        }
        timer.End()

        timer = StartTimer("[ server ] pir online", RED)
        RunServerOnline(states, client, psiParams.Threads)
        timer.End()

    } else {
        fmt.Println("expected `client` or `server` subcommand")
        os.Exit(1)
    }
}

