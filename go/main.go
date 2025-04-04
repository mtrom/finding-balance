package main

import (
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
    client := flag.Bool("client", false, "run the pir protocol as the client")
    server := flag.Bool("server", false, "run the pir protocol as the server")

    hashtableSize := flag.Int64("hashtable-size", -1, "total number of buckets in server's hash table")
    bucketsPerCol := flag.Int64("buckets-per-col", -1, "number of buckets in a col of the database")
    threads       := flag.Uint("threads", 1, "number of threads to run at once")
    cuckooSize    := flag.Int64("cuckoo-size", 1, "total number of buckets in server's hash table")

    // optional lwe options
    lweN     := flag.Int64("lwe-n", -1, "lwe secret dimension")
    lweSigma := flag.Float64("lwe-sigma", -1, "lwe error distribution std dev")
    modulus  := flag.Int64("mod", -1, "lwe plaintext modulus")

    // client-only flag
    expected := flag.Int64("expected", -1, "expected size of intersection")

    // server-only flag
    queries_log := flag.Int64("queries-log", -1, "log of the number of pir queries")
    queries     := flag.Int64("queries", -1, "the number of pir queries")

    debug := flag.Bool("debug", false, "print slightly more robust output")

    flag.Parse()

    if !*debug {
        log.SetOutput(ioutil.Discard)
    } else {
        // remove timestamp prefix from logs
        log.SetFlags(log.Flags() &^ (log.Ldate | log.Ltime))
    }

    if *hashtableSize == -1 { fmt.Println("expected --hashtable-size argument"); os.Exit(1) }
    if *bucketsPerCol == -1 { fmt.Println("expected --bucket-per-col argument"); os.Exit(1) }

    psiParams := PSIParams{
        CuckooSize: uint64(*cuckooSize),
        HashtableSize: uint64(*hashtableSize),
        BucketSize: 0,
        BucketsPerCol: uint64(*bucketsPerCol),
        Threads: *threads,
        LweN: *lweN,
        LweSigma: *lweSigma,
        Modulus: *modulus,
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
                RED, actual, *expected, RESET,
            )
            os.Exit(1)
        }

    } else if *server {
        // connect to client
        var client net.Conn
        for i := 0; i < CONNECTION_RETRIES; i++ {
            conn, err := net.Dial(SERVER_TYPE, SERVER_HOST)
            if err == nil { client = conn; break; }
            if i + 1 == CONNECTION_RETRIES { panic(err) }
            time.Sleep((2 << i) * time.Millisecond)
        }
        defer client.Close()

        if *queries_log != -1 {
            *queries = 1 << *queries_log
        }

        // run protocol
        RunServer(&psiParams, client, uint64(*queries))
    } else {
        fmt.Println("expected `client` or `server` subcommand")
        os.Exit(1)
    }
}

