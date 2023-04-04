package main

import (
    "fmt"
    "os"
)

func main() {
    protocol, params, db, bucketSize := SetupProtocol(os.Args[1])

    fmt.Printf(
        "Packing = %d, Ne = %d, Basis = %d, Squishing = %d, Cols = %d\n",
        db.Info.Packing, db.Info.Ne, db.Info.Basis, db.Info.Squishing, db.Info.Cols,
    )
    queries := ReadQueries(os.Args[2])
    results := RunProtocol(&protocol, db, *params, queries, bucketSize)
    WriteDatabase(os.Args[3], results)
}
