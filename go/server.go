package main

import (
    "fmt"

    "github.com/ahenzinger/simplepir/pir"
)

func main() {

    p := uint64(5)
    m := uint64(2092384)
    i := uint64(3)

    based := pir.Base_p(p, m, i)

    fmt.Println("based: ", based)
}
