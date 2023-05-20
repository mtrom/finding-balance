package main

import (
    "os"
    "path/filepath"
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
