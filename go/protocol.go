package main

import (
    . "github.com/ahenzinger/simplepir/pir"
)

/**
 * parameters of the greater psi protocol
 */
type PSIParams struct {
    CuckooSize    uint64 // number of buckets in cuckoo hashing table
    HashtableSize uint64 // number of buckets in server's hash table
    BucketSize    uint64 // size of each bucket in _bytes_
    BucketsPerCol uint64 // number of buckets in a col of the pir database
    Threads       uint   // number of threads to run at a time

    // (optional) provide to manually choose lwe params
    LweN     int64
    LweSigma float64
    Modulus  int64
}

/**
 * given psi params, size of the encrypted database in bytes
 */
func (p* PSIParams) DBBytes() uint64 {
    return p.HashtableSize * p.BucketSize
}

/**
 * get appropriate parameters based on database & lwe information
 *  modified from pir.SimplePIR.PickParams() to factor in bucket size
 *  and allow manual parameter setting
 *
 * @param <psiParams> params of the greater psi protocol
 * @param <entryBits> number of bits to represent an entry
 */
func GetParams(psiParams *PSIParams, entryBits uint64) (*Params) {
    var params *Params = nil

    // default LWE params
    const LOGQ = uint64(32)
    const SEC_PARAM = uint64(1 << 10)

    // if we're manually setting the lwe params
    if psiParams.LweN != -1 {
        rows, cols := GetDatabaseDims(psiParams, entryBits, uint64(psiParams.Modulus))
        return &Params{
            N: uint64(psiParams.LweN),
            Sigma: psiParams.LweSigma,
            L: rows,
            M: cols,
            Logq: LOGQ,
            P: uint64(psiParams.Modulus),
        }
    }

    // iteratively refine plaintext modulo to find tight values
    for ptMod := uint64(2); ; ptMod += 1 {
        rows, cols := GetDatabaseDims(psiParams, entryBits, ptMod)

        candidate := Params{
            N:    SEC_PARAM,
            Logq: LOGQ,
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

    // bits per database `element'
    const ENTRY_BITS = uint64(8)

    params := GetParams(psiParams, ENTRY_BITS)
    protocol := SimplePIR{}

    return protocol, params, ENTRY_BITS
}

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
    client State,
    p Params,
    info DBinfo,
) []byte {
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

    // recover all Z_p elements in the queried column
    for j := 0; j < len(ans.Data); j++ {
        noised := uint64(ans.Data[j]) + offset
        denoised := p.Round(noised)
        vals = append(vals, denoised)
    }
    ans.MatrixAdd(interm)

    // recover db entries from Z_p elements
    var column []byte
    for i := uint64(0); i < uint64(len(vals)); i += info.Ne {
        column = append(
            column,
            byte(ReconstructElem(vals[i*info.Ne:(i + 1)*info.Ne], 0, info)),
        )
    }

    return column
}
