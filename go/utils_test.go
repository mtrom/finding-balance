package main

import (
    "os"
    "path/filepath"
    "testing"
)

func TestReadDatabase(t *testing.T) {
    cwd, _ := os.Getwd()
    db := ReadDatabase[uint64](filepath.Join(cwd, "test.edb"))

    for index, value := range db {
        if uint64(index) != value {
            t.Errorf("%dth item mismatch, found: %d\n", index, value)
        }
    }
}
