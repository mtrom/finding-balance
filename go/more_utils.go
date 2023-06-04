package main

import (
    "bytes"
    "encoding/binary"
    "fmt"
    "io"
    "net"
    "os"
    "math/rand"
    "time"
)

const (
    // size of pir input hash in bytes
    ENTRY_SIZE = 10

    // size of a matrix element in bytes
    ELEMENT_SIZE = 4

    // largest datasize to send at once
    CHUNK_SIZE = 512

    // number of bits in modulo switch output
    MOD_SWITCH_BITS = 19

    // number of bits in a byte
    BYTE_BITS = 8
)

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
 *  note R is the type we're reading in from the file and W is the type we're returning
 *
 * @params <filename>
 * @params <metadata> name for each uint64 metadata value at the beginning of the database
 * @return map of name to metadata value and the database values
 */
func ReadDatabase[R ~uint64 | ~byte, W ~uint64 | ~byte](
    filename string,
    metadata ...string,
) (map[string]uint64, []W) {
    file, err := os.ReadFile(filename)
    if err != nil { panic(err) }

    reader := bytes.NewReader(file)

    metamap := make(map[string]uint64)
    for _, name := range metadata {
        var value uint64
        binary.Read(reader, binary.LittleEndian, &value);
        metamap[name] = value
    }

    var values []W
    for {
        var value R
        err := binary.Read(reader, binary.LittleEndian, &value)
        if err == io.EOF {
            break
        } else if err != nil {
            panic(err)
        }
        values = append(values, W(value))
    }

    return metamap, values
}

func ModuloSwitchLength(elements uint64) uint64 {
    length := elements * MOD_SWITCH_BITS / BYTE_BITS
    if (elements * MOD_SWITCH_BITS) % BYTE_BITS != 0 { length++ }

    // want to be a multiple of uint32s
    length += 4 - (length % 4)

    return length
}

/**
 * serialize a matrix to bytes, but first switch the modulo to 2^MOD_SWITCH_BITS
 *  allowing us to compress each element into MOD_SWITCH_BITS bits
 */
func ModuloSwitch(matrix *Matrix) []byte {
    output := make([]byte, ModuloSwitchLength(uint64(len(matrix.Data))))

    floating := uint32(0)
    floatingBits := uint(0)
    outputIndex := 0
    for i := range matrix.Data {
        // switch to new modulus
        integer := uint32(matrix.Data[i]) / uint32(1 << (32 - MOD_SWITCH_BITS))

        // decimal remainder of the division
        decimal := float64(matrix.Data[i]) / float64(1 << (32 - MOD_SWITCH_BITS))
        decimal -= float64(integer)

        // TODO: is this rand slow?
        // round according to decimal remainder to floor() or ceil() of division
        if rand.Float64() < decimal { integer++ }

        // add new bits to our floating bits
        floating = (integer << floatingBits) | floating
        floatingBits += MOD_SWITCH_BITS

        // if we have a full 32 bits to send, do so
        if floatingBits >= 32 {
            binary.LittleEndian.PutUint32(
                output[outputIndex:outputIndex + 4], floating,
            )
            floatingBits -= 32
            floating = integer >> (MOD_SWITCH_BITS - floatingBits)
            outputIndex += 4
        }
    }

    // include final floating bits
    if floatingBits > 0 {
        binary.LittleEndian.PutUint32(output[outputIndex:], floating)
    }

    return output
}

/**
 * deserializes matrix from bytes that are compressed from MOD_SWITCH_BITS bit integers
 */
func ModuloSwitchBack(inputs []byte, rows, cols uint64) *Matrix {
    INPUT_BITS := rows * cols * MOD_SWITCH_BITS

    matrix := MatrixNew(rows, cols)
    i := uint64(0)
    for bit := uint64(0); bit < INPUT_BITS; bit += MOD_SWITCH_BITS {
        // get relevant 32 bits
        input := binary.LittleEndian.Uint32(inputs[bit/BYTE_BITS:(bit/BYTE_BITS)+4])

        // truncate any least significant bits not used for this element
        input = input >> (bit % BYTE_BITS)

        // place the bits as the most significant
        input = input << (32 - MOD_SWITCH_BITS)

        matrix.Set(uint64(input), i / cols, i % cols)
        i++
    }

    return matrix
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
        "%s%s (s)\t: %.3f\t%s\n",
        t.color, t.message, t.elapsed.Seconds(), RESET,
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
