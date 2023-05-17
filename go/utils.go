package main

import (
    "bufio"
    "bytes"
    "encoding/binary"
    "fmt"
    "io"
    "log"
    "net"
    "os"
    "time"

    . "github.com/ahenzinger/simplepir/pir"
)

// size of pir input hash in bytes
const ENTRY_SIZE = 10

// size of a matrix element in bytes
const ELEMENT_SIZE = 4

// largest datasize to send at once
const CHUNK_SIZE = 512

const RED    = "\033[0;31m"
const GREEN  = "\033[0;32m"
const YELLOW = "\033[0;33m"
const BLUE   = "\033[0;34m"
const CYAN   = "\033[0;36m"
const WHITE  = "\033[0;37m"
const RESET  = "\033[0m"


/**
 * convert byte array to a hex string for debugging
 */
func toHex(bytes []byte, size uint64) string {
    const digits = string("0123456789ABCDEF")

    output := make([]byte, size * ENTRY_SIZE * 2)
    for i := range bytes {
        output[2 * i] = digits[bytes[i] & 0x0F]
        output[(2 * i) + 1] = digits[(bytes[i] >> 4) & 0x0F]
    }
    return string(output)
}

/**
 * modified version of Params.PrintParams() that's more consistantly formatted
 */
func PrintParams(params *Params) {
    fmt.Printf(
        "[ params ] db=%dx%d, hint=%dx%d, p=%d\n",
        params.M, params.L, params.L, params.N, params.P,
    )
}

/**
 * send data over a network connection, chunking if neccesary
 */
func WriteOverNetwork(conn net.Conn, data []byte) {
    size := uint64(len(data))
    if size < CHUNK_SIZE {
        conn.Write(data)
        return
    }

    i := uint64(0)
    for {
        var chunk []byte

        if i >= size  {
            break
        } else if i + CHUNK_SIZE > size {
            chunk = data[i:]
        } else {
            chunk = data[i:i+CHUNK_SIZE]
        }

        n, err := conn.Write(chunk)
        if err != nil { panic(err) }
        i += uint64(n)
    }
}

/**
 * read data from a network connection, chunking if neccesary
 */
func ReadOverNetwork(conn net.Conn, size uint64) ([]byte) {

    if size < CHUNK_SIZE {
        data := make([]byte, size)
        conn.Read(data)
        return data
    }

    var data []byte
    i := uint64(0)
    for {
        var chunk []byte

        if i >= size {
            break
        } else if i + CHUNK_SIZE > size {
            chunk = make([]byte, size - i)
        } else {
            chunk = make([]byte, CHUNK_SIZE)
        }
        n, err := conn.Read(chunk)
        if err != nil { return []byte{} } // panic(err) }
        data = append(data, chunk[:n]...)
        i += uint64(n)
    }

    if uint64(len(data)) != size {
        panic("somehow read incorrect size of data")
    }

    return data
}

/**
 * read database from file including any metadata
 *
 * @params <filename>
 * @params <metadata> name for each uint64 metadata value at the beginning of the database
 * @return map of name to metadata value and the database values
 */
func ReadDatabase[T ~uint64 | ~byte](
    filename string,
    metadata ...string,
) (map[string]uint64, []T) {
    file, err := os.ReadFile(filename)
    if err != nil { panic(err) }

    reader := bytes.NewReader(file)

    metamap := make(map[string]uint64)
    for _, name := range metadata {
        var value uint64
        binary.Read(reader, binary.LittleEndian, &value);
        metamap[name] = value
    }

    var values []T
    for {
        var value byte
        err := binary.Read(reader, binary.LittleEndian, &value)
        if err == io.EOF {
            break
        } else if err != nil {
            panic(err)
        }
        values = append(values, T(value))
    }

    return metamap, values
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
    color string
    start time.Time
    elapsed time.Duration
}

func (t* Timer) Start() {
    t.start = time.Now()
}

func (t* Timer) Stop() {
    t.elapsed += time.Since(t.start)
}

func (t* Timer) End() {
    t.Stop()
    t.Print()
}

func (t* Timer) Print() {
    fmt.Printf(
        "%s%s (ms)\t: %.3f\t%s\n",
        t.color, t.message, t.elapsed.Seconds() * 1000, RESET,
    )
}

func CreateTimer(message, color string) *Timer {
    timer := new(Timer)
    timer.message = message
    timer.color = color
    timer.elapsed = 0
    return timer
}

func StartTimer(message, color string) *Timer {
    timer := new(Timer)
    timer.message = message
    timer.color = color
    timer.start = time.Now()
    return timer
}
