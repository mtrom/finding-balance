package main

import (
    "io/ioutil"
    "log"
    "os"
)

func main() {
    log.SetOutput(ioutil.Discard)

    protocol, params, db, bucketSize := SetupProtocol(os.Args[1])

    log.Printf(
        "Packing = %d, Ne = %d, Basis = %d, Squishing = %d, Cols = %d\n",
        db.Info.Packing, db.Info.Ne, db.Info.Basis, db.Info.Squishing, db.Info.Cols,
    )
    queries := ReadQueries(os.Args[2])
    results := RunProtocol(&protocol, db, *params, queries, bucketSize)
    WriteDatabase(os.Args[3], results)
}
