package main

import (
	"runtime"
	"runtime/debug"
	"time"
    "bytes"
    "encoding/binary"
    "fmt"
    "io/ioutil"
    "math"

    . "github.com/ahenzinger/simplepir/pir"
)

const POINT_SIZE uint64 = 33
const LOGQ = uint64(32)
const SEC_PARAM = uint64(1 << 10)

func to_hex(bytes []byte, size uint64) string {
    const digits = string("0123456789ABCDEF")

    output := make([]byte, size * POINT_SIZE * 2)
    for i := range bytes {
        output[2 * i] = digits[bytes[i] & 0x0F]
        output[(2 * i) + 1] = digits[(bytes[i] >> 4) & 0x0F]
    }
    return string(output)
}

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


func ReadDatabase(filename string) (SimplePIR, *Database, Params) {
    file, err := ioutil.ReadFile(filename)

    if err != nil {
        panic(err)
    }

    reader := bytes.NewReader(file)

    var buckets, size uint64
    binary.Read(reader, binary.LittleEndian, &buckets)
    binary.Read(reader, binary.LittleEndian, &size)
    fmt.Printf("buckets = %d, size = %d\n", buckets, size)
    size = uint64(math.Pow(2, float64(size)))

    var bytes []byte = make([]byte, size * POINT_SIZE)
    for i := range bytes {
        if err = binary.Read(reader, binary.BigEndian, &bytes[i]); err != nil {
            panic(err)
        }
    }
    // fmt.Println(to_hex(bytes, size))

    if len(bytes) % 8 > 0 {
        panic("Database won't evenly pack into uint64s")
    }

    // TODO: each entry fits in a uint64 + 1 byte, how can we pack them together?
    var vals []uint64 = make([]uint64, len(bytes) / 8)
    fmt.Printf("len(bytes) = %d\n", len(bytes))
    for i := range vals {
        vals[i] = binary.BigEndian.Uint64(bytes[i * 8: (i + 1) * 8])
    }
    fmt.Printf("len(vals) = %d, val[100] = %d\n", len(vals), vals[100])

    row_length := uint64(64)

    protocol := SimplePIR{}
    params := protocol.PickParams(uint64(len(vals)), row_length, SEC_PARAM, LOGQ)


    fmt.Printf("Num = %d, row_length = %d, len(vals) = %d\n", len(vals), row_length, len(vals))
    db := MakeDB(uint64(len(vals)), row_length, &params, vals)

    return protocol, db, params
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
		index_to_query := i[index] + uint64(index)*batch_sz
		val := pi.Recover(index_to_query, uint64(index), offline_download,
		                  query.Data[index], answer, shared_state,
			          client_state[index], p, DB.Info)

		if DB.GetElem(index_to_query) != val {
			fmt.Printf("Batch %d (querying index %d -- row should be >= %d): Got %d instead of %d\n",
				index, index_to_query, DB.Data.Rows/4, val, DB.GetElem(index_to_query))
			panic("Reconstruct failed!")
		}

        results[index] = val
	}
	fmt.Println("Success!")
	printTime(start)

	runtime.GC()
	debug.SetGCPercent(100)
	return rate, bw, results
}


func main() {

    p := uint64(5)
    m := uint64(2092384)
    i := uint64(3)

    based := Base_p(p, m, i)

    fmt.Println("based: ", based)

    protocol, db, params := ReadDatabase("/Users/max/research/ddh-unbalanced-psi/bin/server.db")

    var indexes []uint64 = make([]uint64, 1)
    indexes[0] = uint64(100)

    _, _, val := RunProtocol(&protocol, db, params, indexes)
    fmt.Printf("final answer: %d\n", val[0])
}
