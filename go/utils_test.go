package main

import (
    "os"
    "path/filepath"
    "strconv"
    "testing"
)

func TestReadDatabase(t *testing.T) {
    cwd, _ := os.Getwd()
    metadata, db := ReadDatabase[byte, uint64](
        filepath.Join(cwd, "test.edb"),
        "hashtableSize", "bucketSize",
    )

    if metadata["hashtableSize"] != uint64(5) {
        t.Errorf("expected bucketSize = 4, found bucketSize = %d\n", metadata["hashtableSize"])
    }

    if metadata["bucketSize"] != uint64(4) {
        t.Errorf("expected bucketSize = 4, found bucketSize = %d\n", metadata["bucketSize"])
    }

    for index, value := range db {
        if uint64(index) != value {
            t.Errorf("%dth item mismatch, found: %d\n", index, value)
        }
    }
}

func TestModuloSwitching(t *testing.T) {
    N := uint64(512)
    vector := FasterMatrixRand(N, 1)
    switched := ModuloSwitchBack(ModuloSwitch(vector), N, 1)
    for i := range vector.Data {
        expected := uint32(vector.Data[i]) / uint32(1 << (32 - MOD_SWITCH_BITS))
        actual := uint32(switched.Data[i])

        // it could be exact or 1 more
        if actual - expected > 1 || actual - expected < 0 {
            t.Errorf(
                "expected %s (%d) at i=%d, found %s (%d)\n",
                strconv.FormatUint(uint64(expected), 2), expected, i,
                strconv.FormatUint(uint64(actual), 2), actual,
            )
        }
    }
}

func TestModuloSwitchingZeros(t *testing.T) {
    N := uint64(512)
    vector := MatrixNew(N, 1)
    switched := ModuloSwitchBack(ModuloSwitch(vector), N, 1)
    for i := range vector.Data {
        expected := uint32(0)
        actual := uint32(switched.Data[i])

        // it could be exact or 1 more
        if actual - expected > 1 || actual - expected < 0 {
            t.Errorf(
                "expected %s (%d) at i=%d, found %s (%d)\n",
                strconv.FormatUint(uint64(expected), 2), expected, i,
                strconv.FormatUint(uint64(actual), 2), actual,
            )
        }
    }
}
