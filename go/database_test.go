package main

import (
    "fmt"
    "testing"
)

func TestDatabaseDims(t *testing.T) {

    var tests = []struct {
        dbSize, bucketSize, entryBits, ptMod uint64
        expectedRows, expectedCols uint64
    }{
        {32, 4, 8, 991, 4, 8},
        // TODO: add more tests
    }

    for _, test := range tests {

        t.Run(fmt.Sprintf("%d,%d", test.dbSize, test.bucketSize), func(t *testing.T) {
            actualRows, actualCols := GetDatabaseDims(
                test.dbSize,
                test.bucketSize,
                test.entryBits,
                test.ptMod,
            )

            if actualRows != test.expectedRows {
                t.Errorf("expected %d rows, found %d rows", test.expectedRows, actualRows)
            }

            if actualCols != test.expectedCols {
                t.Errorf("expected %d rows, found %d rows", test.expectedCols, actualCols)
            }
        })
    }
}
