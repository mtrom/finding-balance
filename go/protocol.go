package main

import (
    "fmt"
    "log"
    "time"

    . "github.com/ahenzinger/simplepir/pir"
)

/**
 * parameters of the greater psi protocol
 */
type PSIParams struct {
    BucketN       uint64 // number of buckets in server's hash table
    BucketSize    uint64 // size of each bucket _in group elements_
    BucketsPerCol uint64 // number of buckets in a col of the pir database
}

/**
 * given psi params, size of each bucket in the database in bytes
 */
func (p* PSIParams) BucketBytes() uint64 {
    return p.BucketSize * ENTRY_SIZE
}

/**
 * given psi params, size of the encrypted database in bytes
 */
func (p* PSIParams) DBBytes() uint64 {
    return p.BucketN * p.BucketSize * ENTRY_SIZE
}

/**
 * given psi params, dimensions of the encrypted database in bytes
 */
// func (p* PSIParams) DBDims() (uint64, uint64) { }


/**
 * get appropriate parameters based on database & lwe information
 *  modified from pir.SimplePIR.PickParams() to factor in bucket size
 *
 * @param <psiParams> params of the greater psi protocol
 * @param <entryBits> number of bits to represent an entry
 * @param <lweParam> security parameter for lwe
 * @param <lweMod> modulo for lwe and used to help find error distribution
 */
func GetParams(psiParams *PSIParams, entryBits, lweParam, lweMod uint64) (*Params) {
    var params *Params = nil

    // iteratively refine plaintext modulo to find tight values
    for ptMod := uint64(2); ; ptMod += 1 {
        rows, cols := GetDatabaseDims(psiParams, entryBits, ptMod)

        candidate := Params{
            N:    lweParam,
            Logq: lweMod,
            L:    rows,
            M:    cols,
        }
        candidate.PickParams(false, cols)

        if candidate.P < ptMod {
            if params == nil {
                // we don't expect this to be possible
                panic("could not find tight params")
            }
            return params
        }

        params = &candidate
    }

    // don't expect this to happen either
    panic("could not find any params")
    return params
}

/**
 * setup parameters for a SimplePIR protocol
 *
 * @param <psiParams> params of the greater psi protocol
 * @returns protocol, parameters, and entry bits (so it can be used below)
 */
func SetupProtocol(psiParams *PSIParams) (SimplePIR, *Params, uint64) {

    // LWE params
    const LOGQ = uint64(32)
    const SEC_PARAM = uint64(1 << 10)

    // bits per database `element'
    const ENTRY_BITS = uint64(8)

    params := GetParams(psiParams, ENTRY_BITS, SEC_PARAM, LOGQ)
    protocol := SimplePIR{}

    return protocol, params, ENTRY_BITS
}

/**
 * setup db & parameters for a SimplePIR protocol
 *  note that a db `element' here is not a 1:1 corresponding to a ddh
 *  element, but only makes up part of it
 *
 * @param <dbFn> filename for the database
 * @returns protocol, database, & parameters (& bucket size in bytes)
 */
func SetupProtocolAndDB(dbFn string) (SimplePIR, *Params, *Database, uint64) {

    return SimplePIR{}, nil, nil, 0

    /*
    // raw binary database
    values, bucketSize := ReadDatabase(dbFn)
    dbSize := uint64(len(values))

    protocol, params, ENTRY_BITS := SetupProtocol(dbSize, bucketSize)
    db := CreateDatabase(dbSize, ENTRY_BITS, params, values)

    return protocol, params, db, bucketSize
    */
}

/**
 * run protocol on given database and return the results
 *
 * @param <protocol> SimplePIR object
 * @param <db> server's database
 * @param <params> protcol parameters
 * @param <queries> client's queries as the hash value of an elemnet
 * @param <bucketSize> number of buckets for hash value
 */
func RunProtocol(
    protocol PIR,
    db *Database,
    params Params,
    queries []uint64,
    bucketSize uint64,
) ([]uint64) {

    start := time.Now()
    // sample lwe random matrix A
    shared_state := protocol.Init(db.Info, params)

    // calculate hint
    server_state, offline := protocol.Setup(db, shared_state, params)
    elapsed := time.Since(start)
	comm := float64(offline.Size() * uint64(params.Logq) / (8.0 * 1024.0))
    fmt.Printf("[ go/pir ] offline:\t%dms,\t%.2fKB\n", elapsed / 1000000, comm)

    var results []uint64
    queried := map[uint64]struct{}{}
    start = time.Now()
    elapsedQuery := time.Since(start)
    elapsedAnswer := time.Since(start)
    elapsedRecover := time.Since(start)
    comm = float64(0)
    for _, col := range queries {
        // translate the hash value into the column by dividing by the number
        // of buckets per column
        translated := col / (params.L / bucketSize)

        // this should not happen
        if translated % params.M != translated {
            panic("our translated query larger then the number of columns")
        }

        log.Printf("[ go/pir ] translated query %d to %d\n", col, translated)

        // don't query the same column twice
        if _, already_queried := queried[translated]; already_queried {
            continue;
        } else {
            queried[translated] = struct{}{}
        }

        log.Printf("[ go/pir ] querying column %d\n", translated)

        // create query vector
        startQuery := time.Now()
        client_state, query := protocol.Query(translated, shared_state, params, db.Info)

        // original protocol supported batch querying so need to put query in a slice
        querySlice := MakeMsgSlice(query)
        elapsedQuery += time.Since(startQuery)
        comm += float64(querySlice.Size() * uint64(params.Logq) / (8.0 * 1024.0))

        log.Printf("[ go/pir ] db size   : %d x %d\n", db.Data.Rows, db.Data.Cols)

        // create answer vector
        startAnswer := time.Now()
        answer := protocol.Answer(db, querySlice, server_state, shared_state, params)
        elapsedAnswer += time.Since(startAnswer)
        comm += float64(answer.Size() * uint64(params.Logq) / (8.0 * 1024.0))

        // recover all values in the queried column
        startRecover := time.Now()
        column := RecoverColumn(
            offline,
            querySlice.Data[0],
            answer,
            client_state,
            params,
            db.Info,
        )
        results = append(results, column...)
        elapsedRecover += time.Since(startRecover)
    }
    elapsed = time.Since(start)

    fmt.Printf("[ go/pir ] query:\t%dms\n", elapsedQuery / 1000000)
    fmt.Printf("[ go/pir ] answer:\t%dms\n", elapsedAnswer / 1000000)
    fmt.Printf("[ go/pir ] recover:\t%dms\n", elapsedRecover / 1000000)
    fmt.Printf("[ go/pir ] online:\t%dms,\t%.2fKB\n", elapsed / 1000000, comm)

    protocol.Reset(db, params)

    return results
}
