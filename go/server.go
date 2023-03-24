package main

import (
    "fmt"
    "os"
)

func main() {
    protocol, params, db := SetupProtocol(os.Args[1])
    queries := ReadQueries(os.Args[2])
    _, _, results := RunProtocol(&protocol, db, *params, queries)

    fmt.Printf("\n\nfinal results: ")
    for _, result := range results {
        fmt.Printf("%d, ", result)
    }
    fmt.Println()

}
