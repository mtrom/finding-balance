package main

import (
    "bytes"
    "encoding/binary"
    "fmt"
    "io"
    "os"
)

const POINT_SIZE uint64 = 33


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

func ReadQueries(filename string) []uint64 {
    file, err := os.ReadFile(filename)
    if err != nil { panic(err) }

    reader := bytes.NewReader(file)

    fmt.Printf("queries: ")

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
        fmt.Printf("%d, ", index)
    }
    return queries
}

