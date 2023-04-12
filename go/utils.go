package main

import (
    "bufio"
    "bytes"
    "encoding/binary"
    "fmt"
    "io"
    "log"
    "os"
    "time"

    . "github.com/ahenzinger/simplepir/pir"
)

// size of a group element in bytes
const POINT_SIZE uint64 = 33

// size of a matrix element in bytes
const ELEMENT_SIZE = 4

/**
 * convert byte array to a hex string for debugging
 */
func toHex(bytes []byte, size uint64) string {
    const digits = string("0123456789ABCDEF")

    output := make([]byte, size * POINT_SIZE * 2)
    for i := range bytes {
        output[2 * i] = digits[bytes[i] & 0x0F]
        output[(2 * i) + 1] = digits[(bytes[i] >> 4) & 0x0F]
    }
    return string(output)
}

/**
 * read database from file
 */
func ReadDatabase(filename string) ([]uint64, uint64) {
    file, err := os.ReadFile(filename)
    if err != nil { panic(err) }

    reader := bytes.NewReader(file)

    var buckets, bucketSize uint64
    binary.Read(reader, binary.LittleEndian, &buckets)
    binary.Read(reader, binary.LittleEndian, &bucketSize)

    var values []uint64
    for {
        var value byte
        err := binary.Read(reader, binary.LittleEndian, &value)
        if err == io.EOF {
            break
        } else if err != nil {
            panic(err)
        }
        values = append(values, uint64(value))
    }

    return values, bucketSize
}

func WriteDatabase(filename string, values []uint64) {
    log.Printf("[ go/pir ] writing %d values to %s\n", len(values), filename)
    file, err := os.Create(filename)
    if err != nil { panic(err) }
    defer file.Close()

    buffer := bufio.NewWriter(file)

    for _, value := range values {
        /* why???
        if (value == 0) {
            continue;
        }
        */
        binary.Write(buffer, binary.LittleEndian, uint8(value))
    }
    buffer.Flush()
}

func ReadQueries(filename string) []uint64 {
    file, err := os.ReadFile(filename)
    if err != nil { panic(err) }

    reader := bytes.NewReader(file)

    var queries []uint64
    for {
        var index uint64
        err := binary.Read(reader, binary.LittleEndian, &index)
        if err == io.EOF {
            break
        } else if err != nil {
            panic(err)
        }

        queries = append(queries, index)
    }
    return queries
}


/**
 * serializes matrix into []byte for sending over network
 */
func MatrixToBytes(matrix *Matrix) []byte {
    output := make([]byte, len(matrix.Data) * ELEMENT_SIZE)
    for i := range matrix.Data {
        binary.LittleEndian.PutUint32(
            output[i * ELEMENT_SIZE:i * ELEMENT_SIZE + 4],
            uint32(matrix.Data[i]),
        )
    }
    return output
}

/**
 * deserializes matrix from []byte after receiving over network
 */
func BytesToMatrix(input []byte, rows, cols uint64) *Matrix {
    matrix := MatrixNew(rows, cols)
    for i := uint64(0); i < rows * cols; i++ {
        matrix.Set(
            uint64(binary.LittleEndian.Uint32(input[i*4:(i+1)*4])),
            i / cols, i % cols,
        )
    }
    return matrix
}

/**
 * makes it easy to track timing execution
 */
type Timer struct {
    message string
    start time.Time
    elapsed time.Duration
    unit string
}

func (t* Timer) End() {
    t.elapsed = time.Since(t.start)
    t.unit = "ns"
    if t.elapsed > 1000000 {
        t.unit = "ms"
        t.elapsed /= 1000000
    }

    t.Print()
}

func (t* Timer) Print() {
    fmt.Printf("%s\t%d%s\n", t.message, t.elapsed, t.unit)
}

func StartTimer(message string) *Timer {
    timer := new(Timer)
    timer.message = message
    timer.start = time.Now()
    return timer
}
