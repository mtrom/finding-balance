package main

import (
    "os"
    "path/filepath"
    "testing"
)

func TestReadDatabase(t *testing.T) {
    cwd, _ := os.Getwd()
    db, bucketSize := ReadDatabase(filepath.Join(cwd, "test.edb"))

    if bucketSize != 4 {
        t.Errorf("expected bucketSize = 4, found bucketSize = %d\n", bucketSize)
    }

    for index, value := range db {
        if uint64(index) != value {
            t.Errorf("%dth item mismatch, found: %d\n", index, value)
        }
    }
}
