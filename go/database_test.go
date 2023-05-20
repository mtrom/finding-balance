package main

import (
    "fmt"
    "testing"
)

func TestDatabaseDims(t *testing.T) {

    var tests = []struct {
        bucketN, bucketSize, bucketsPerCol, entryBits, ptMod uint64
        expectedRows, expectedCols uint64
    }{
        {32, 4, 2, 8, 991, 8, 16},
        // TODO: add more tests
    }

    for _, test := range tests {

        t.Run(
            fmt.Sprintf("%d,%d,%d", test.bucketN, test.bucketSize, test.bucketsPerCol),
            func(t *testing.T) {
                psiParams := PSIParams{
                    BucketN: test.bucketN,
                    BucketSize: test.bucketSize,
                    BucketsPerCol: test.bucketsPerCol,
                }
                actualRows, actualCols := GetDatabaseDims(&psiParams, test.entryBits, test.ptMod)

                if actualRows != test.expectedRows {
                    t.Errorf("expected %d rows, found %d rows", test.expectedRows, actualRows)
                }

                if actualCols != test.expectedCols {
                    t.Errorf("expected %d rows, found %d rows", test.expectedCols, actualCols)
                }
            },
        )
    }
}
