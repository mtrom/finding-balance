package main

import (
    "fmt"
    "testing"
)

func TestFasterMatrixRand(t *testing.T) {
    const LOGQ = uint64(32)
    const M = uint64(1 << 12)
    const N = uint64(1024)

	seed := RandomPRGKey()
    reader := NewPRG(seed)
    bufPrgReader = NewBufPRG(reader)

    timer := StartTimer("library", WHITE)
    A := MatrixRand(M, N, LOGQ, 0)
    timer.End()
    fmt.Printf("%d x %d\n", A.Rows, A.Cols)

    zeros := 0
    nonzeros := 0
    timer = StartTimer("faster", WHITE)
    B := FasterMatrixRand(M, N)
    for _, val : range B.Data {
        if val == 0 { zeros++ }
        else { nonzeros++ }
    }
    timer.End()
    fmt.Printf("%d x %d\n", B.Rows, B.Cols)
    fmt.Printf("%d zeros vs %d nonzeros\n", zeros, nonzeros)

    timer = StartTimer("raw cipher", WHITE)
    var buf [M * N * 4]byte
    n, err := reader.Read(buf[:])
    timer.End()
    if err != nil { panic(err) }
    if uint64(n) != M * N * 4 { panic(fmt.Sprintf("%d != %d * %d * 4\n", n, M, N))}
}

