package main

import (
    "fmt"

    . "github.com/ahenzinger/simplepir/pir"
)

/**
 * recover entire db column from answer
 *  modified from pir.SimplePIR.Recover() to return entire column
 *
 * @param <offline>, <query>, <answer> previous protocol step outputs
 * @param <shared>, <client> protocol provided states
 * @param <p> protocol parameters
 * @param <info> database information
 */
func RecoverColumn(
    offline Msg,
    query Msg,
    answer Msg,
    shared State,
    client State,
    p Params,
    info DBinfo,
) []uint64 {
    secret := client.Data[0]
    H := offline.Data[0]
    ans := answer.Data[0]

    // TODO: what does this do?
    ratio := p.P / 2
    offset := uint64(0);
    for j := uint64(0); j < p.M; j++ {
        offset += ratio * query.Data[0].Get(j,0)
    }
    offset %= (1 << p.Logq)
    offset = (1 << p.Logq) - offset

    interm := MatrixMul(H, secret)
    ans.MatrixSub(interm)

    var vals []uint64

    fmt.Printf("len(ans) = %d, offset=%d\n", len(ans.Data), offset)

    // recover all Z_p elements in the queried column
    for j := 0; j < len(ans.Data); j++ {
        noised := uint64(ans.Data[j]) + offset
        denoised := p.Round(noised)
        vals = append(vals, denoised)
    }
    ans.MatrixAdd(interm)

    fmt.Printf("values are: ")
    for _, value := range vals {
        fmt.Printf("%d, ", value)
    }
    fmt.Println()

    // recover db entries from Z_p elements
    var column []uint64
    for i := uint64(0); i < uint64(len(vals)); i += info.Ne {
        column = append(
            column,
            ReconstructElem(vals[i*info.Ne:(i + 1)*info.Ne], 0, info),
        )
    }

    return column
}
