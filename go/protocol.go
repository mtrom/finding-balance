package main

import (
	"runtime"
	"runtime/debug"
	"time"
    "fmt"
    "math"

    . "github.com/ahenzinger/simplepir/pir"
)


func printTime(start time.Time) time.Duration {
	elapsed := time.Since(start)
	fmt.Printf("\tElapsed: %s\n", elapsed)
	return elapsed
}

func printRate(p Params, elapsed time.Duration, batch_sz int) float64 {
	rate := math.Log2(float64((p.P))) * float64(p.L*p.M) * float64(batch_sz) /
		float64(8*1024*1024*elapsed.Seconds())
	fmt.Printf("\tRate: %f MB/s\n", rate)
	return rate
}

/**
 * get appropriate parameters based on database & lwe information
 *  modified from pir.SimplePIR.PickParams() to factor in bucket size
 *
 * @param <dbSize> number of entries in database
 * @param <bucketSize> number of entires per bucket
 * @param <entryBits> number of bits to represent an entry
 * @param <lweParam> security parameter for lwe
 * @param <lweMod> modulo for lwe and used to help find error distribution
 */
func GetParams(dbSize, bucketSize, entryBits, lweParam, lweMod uint64) (*Params) {
    var params *Params = nil

    // iteratively refine plaintext modulo to find tight values
    for ptMod := uint64(2); ; ptMod += 1 {
        rows, cols := GetDatabaseDims(dbSize, bucketSize, entryBits, ptMod)

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
            params.PrintParams()
            return params
        }

        params = &candidate
    }

    // don't expect this to happen either
    panic("could not find any params")
    return params
}

/**
 * setup db & parameters for a SimplePIR protocol
 *  note that a db `element' here is not a 1:1 corresponding to a ddh
 *  element, but only makes up part of it
 *
 * @param <dbFn> filename for the database
 * @returns protocol, database, & parameters
 */
func SetupProtocol(dbFn string) (SimplePIR, *Params, *Database) {

    // LWE params
    const LOGQ = uint64(32)
    const SEC_PARAM = uint64(1 << 10)

    // bits per database `element'
    const ENTRY_BITS = uint64(8)

    // raw binary database
    values, BUCKET_SIZE := ReadDatabase(dbFn)

    // number of `elements' in database
    DB_SIZE := uint64(len(values))

    params := GetParams(
        DB_SIZE, BUCKET_SIZE, ENTRY_BITS, SEC_PARAM, LOGQ,
    )
    protocol := SimplePIR{}
    db       := CreateDatabase(DB_SIZE, ENTRY_BITS, params, values)

    return protocol, params, db
}


// mostly taken straight from their impl
func RunProtocol(pi PIR, DB *Database, p Params, i []uint64) (float64, float64, []uint64) {
	fmt.Printf("Executing %s\n", pi.Name())
	//fmt.Printf("Memory limit: %d\n", debug.SetMemoryLimit(math.MaxInt64))
	debug.SetGCPercent(-1)

	num_queries := uint64(len(i))
	if DB.Data.Rows/num_queries < DB.Info.Ne {
		panic("Too many queries to handle!")
	}
	batch_sz := DB.Data.Rows / (DB.Info.Ne * num_queries) * DB.Data.Cols
	bw := float64(0)

	shared_state := pi.Init(DB.Info, p)

	fmt.Println("Setup...")
	start := time.Now()
	server_state, offline_download := pi.Setup(DB, shared_state, p)
	printTime(start)
	comm := float64(offline_download.Size() * uint64(p.Logq) / (8.0 * 1024.0))
	fmt.Printf("\t\tOffline download: %f KB\n", comm)
	bw += comm
	runtime.GC()

	fmt.Println("Building query...")
	start = time.Now()
	var client_state []State
	var query MsgSlice
	for index, _ := range i {
		index_to_query := i[index] + uint64(index)*batch_sz
		cs, q := pi.Query(index_to_query, shared_state, p, DB.Info)
		client_state = append(client_state, cs)
		query.Data = append(query.Data, q)
	}
	runtime.GC()
	printTime(start)
	comm = float64(query.Size() * uint64(p.Logq) / (8.0 * 1024.0))
	fmt.Printf("\t\tOnline upload: %f KB\n", comm)
	bw += comm
	runtime.GC()

	fmt.Println("Answering query...")
	start = time.Now()
	answer := pi.Answer(DB, query, server_state, shared_state, p)
	elapsed := printTime(start)
	rate := printRate(p, elapsed, len(i))
	comm = float64(answer.Size() * uint64(p.Logq) / (8.0 * 1024.0))
	fmt.Printf("\t\tOnline download: %f KB\n", comm)
	bw += comm
	runtime.GC()

	pi.Reset(DB, p)
	fmt.Println("Reconstructing...")
	start = time.Now()

    var results []uint64 = make([]uint64, len(i))
    for index, _ := range i {
        index_to_query := i[index] + uint64(index) * batch_sz
        column := RecoverColumn(
            offline_download,
            query.Data[index],
            answer,
            shared_state,
            client_state[index],
            p,
            DB.Info,
        )
        expected := DB.GetElem(index_to_query)
        fmt.Printf("expected := %d\n", expected)
        found := false
        for _, actual := range column {
            if expected == actual {
                found = true
            }
        }

		if !found {
            fmt.Printf("querying column %d failed; %d not found in column\n",
                index_to_query / p.M, expected,
            )
			// panic("Reconstruct failed!")
		}

        results = append(results, column...)
	}
	fmt.Println("Success!")
	printTime(start)

	runtime.GC()
	debug.SetGCPercent(100)
	return rate, bw, results
}
