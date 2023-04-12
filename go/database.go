package main

import (
    "math"

    . "github.com/ahenzinger/simplepir/pir"
)

/**
 * get the db dimensions given db size and plaintext modulo
 *  modified from pir.Database.ApproxSquareDatabaseDims() to factor in bucket size
 *
 * @param <dbSize> number of entries in the database
 * @param <bucketSize> number of entries per bucket
 * @param <entryBits> number of bits to represent an entry
 * @param <ptMod> plaintext modulo (or p)
 */
func GetDatabaseDims(dbSize, bucketSize, entryBits, ptMod uint64) (uint64, uint64) {

    ptElems, ptPerEntry, _ := Num_DB_entries(dbSize, entryBits, ptMod)

    bucketSize *= ptPerEntry

    rows := uint64(math.Floor(math.Sqrt(float64(ptElems))))

    // ensure the number of rows per column is a multiple of the bucket size
    if rows < bucketSize {
        rows = bucketSize
    } else if rows % bucketSize > bucketSize / 2 {
        rows += (bucketSize - (rows % bucketSize))
    } else {
        rows -= rows % bucketSize
    }

    if rows % ptPerEntry != 0 {
        panic("don't expect this to be possible with proper bucket size")
    }

	cols := uint64(math.Ceil(float64(ptElems) / float64(rows)))

	return rows, cols
}


/**
 * create the matrix database with the given parameters
 *  modified from pir.Database.MakeDB() to stack into columns rather than rows
 *
 * @param <dbSize> number of entries in the database
 * @param <entryBits> number of bits to represent an entry
 * @param <params> protocol parameters
 * @param <values> values to populate database
 */
func CreateDatabase(dbSize, entryBits uint64, params *Params, values []uint64) *Database {
	db := SetupDB(dbSize, entryBits, params)
	db.Data = MatrixZeros(params.L, params.M)

	if uint64(len(values)) != dbSize {
		panic("number of input database values does not match given dbSize")
	}

    // when multiple entries go into each Z_p elements
	if db.Info.Packing > 0 {
		at    := uint64(0)
		cur   := uint64(0)
		coeff := uint64(1)

		for i, elem := range values {
			cur += (elem * coeff)
			coeff *= (1 << entryBits)

			if ((i + 1) % int(db.Info.Packing) == 0) || (i == len(values) - 1) {
				db.Data.Set(cur, at % params.L, at / params.L)
				at += 1
				cur = 0
				coeff = 1
			}
		}
    // if one or more Z_p elements represents a single entry
	} else {
		for i, elem := range values {
			for j := uint64(0); j < db.Info.Ne; j++ {
				db.Data.Set(
                    Base_p(db.Info.P, elem, j),
                    uint64(i) % params.L,
                    (uint64(i) / params.L) * db.Info.Ne + j,
                )
			}
		}
	}

	// Map DB elems to [-p/2; p/2]
	db.Data.Sub(params.P / 2)

	return db
}

/**
 * setup the database info without needing the db itself for the client
 * modified from pir.database.SetupDB()
 *
 * @param <dbSize> number of entries in the database
 * @param <entryBits> number of bits to represent an entry
 * @param <params> protocol parameters
 */
func SetupDBInfo(dbSize, entryBits uint64, params *Params) (DBinfo) {

    var info DBinfo

	info.Num = dbSize
	info.Row_length = entryBits
	info.P = params.P
	info.Logq = params.Logq

	ptElems, ptPerEntry, entryPerPt := Num_DB_entries(dbSize, entryBits, params.P)
	info.Ne = ptPerEntry
	info.X = info.Ne
	info.Packing = entryPerPt

	for info.Ne % info.X != 0 {
		info.X += 1
	}

	if ptElems > params.L * params.M {
		panic("Params and database size don't match")
	}

	if params.L % info.Ne != 0 {
		panic("Number of DB elems per entry must divide DB height")
	}

    // this isn't in pir.database.SetupDB() but later in pir.database.Squish()
	info.Basis = 10
	info.Squishing = 3
    info.Cols = params.M

    return info
}

