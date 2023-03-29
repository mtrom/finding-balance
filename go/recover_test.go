package main

import (
    "fmt"
    "os"
    "path/filepath"
    "testing"

    . "github.com/ahenzinger/simplepir/pir"
)

func TestRecoverColumn(t *testing.T) {
    cwd, _ := os.Getwd()
    protocol, p, db := SetupProtocol(filepath.Join(cwd, "test.edb"))
    params := *p

    var tests = []struct {
        col uint64
        expected []uint64
    }{
        { 0, []uint64{ 0, 1, 2, 3 } },
        { 1, []uint64{ 4, 5, 6, 7 } },
        { 2, []uint64{ 8, 9, 10, 11 } },
        { 3, []uint64{ 12, 13, 14, 15 } },
        { 4, []uint64{ 16, 17, 18, 19 } },
        { 5, []uint64{ 20, 21, 22, 23 } },
        { 6, []uint64{ 24, 25, 26, 27 } },
        { 7, []uint64{ 28, 29, 30, 31 } },
    }

    for _, test := range tests {

        t.Run(fmt.Sprintf("col=%d", test.col), func(t *testing.T) {
            shared_state := protocol.Init(db.Info, params)
            server_state, offline := protocol.Setup(db, shared_state, params)

            var querySlice MsgSlice
            client_state, query := protocol.Query(test.col, shared_state, params, db.Info)
            querySlice.Data = append(querySlice.Data, query)

            answer := protocol.Answer(db, querySlice, server_state, shared_state, params)
            protocol.Reset(db, params)
            column := RecoverColumn(
                offline,
                querySlice.Data[0],
                answer,
                shared_state,
                client_state,
                params,
                db.Info,
            )
            for i := range column {
                if column[i] != test.expected[i] {
                    t.Errorf("%dth value expected %d, found %d", i, test.expected[i], column[i])
                }

            }
        })
    }
}
