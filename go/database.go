package main

import "math"
import "fmt"

type DBinfo struct {
	Num        uint64 // number of DB entries.
	Row_length uint64 // number of bits per DB entry.

	Packing uint64 // number of DB entries per Z_p elem, if log(p) > DB entry size.
	Ne      uint64 // number of Z_p elems per DB entry, if DB entry size > log(p).

	X uint64 // tunable param that governs communication,
	         // must be in range [1, ne] and must be a divisor of ne;
	         // represents the number of times the scheme is repeated.
	P    uint64 // plaintext modulus.
	Logq uint64 // (logarithm of) ciphertext modulus.

	// For in-memory DB compression
	Basis     uint64
	Squishing uint64
	Cols      uint64
}

type Database struct {
	Info DBinfo
	Data *Matrix
}

func (DB *Database) Squish() {
	//fmt.Printf("Original DB dims: ")
	//DB.Data.Dim()

	DB.Info.Basis = 10
	DB.Info.Squishing = 3
	DB.Info.Cols = DB.Data.Cols
	DB.Data.Squish(DB.Info.Basis, DB.Info.Squishing)

	//fmt.Printf("After squishing, with compression factor %d: ", DB.Info.Squishing)
	//DB.Data.Dim()

	// Check that params allow for this compression
	if (DB.Info.P > (1 << DB.Info.Basis)) || (DB.Info.Logq < DB.Info.Basis * DB.Info.Squishing) {
		panic("Bad params")
	}
}

func (DB *Database) Unsquish() {
	DB.Data.Unsquish(DB.Info.Basis, DB.Info.Squishing, DB.Info.Cols)
}

// Store the database with entries decomposed into Z_p elements, and mapped to [-p/2, p/2]
// Z_p elements that encode the same database entry are stacked vertically below each other.
func ReconstructElem(vals []uint64, index uint64, info DBinfo) uint64 {
	q := uint64(1 << info.Logq)

	for i, _ := range vals {
		vals[i] = (vals[i] + info.P/2) % q
		vals[i] = vals[i] % info.P
	}

	val := Reconstruct_from_base_p(info.P, vals)

	if info.Packing > 0 {
		val = Base_p((1 << info.Row_length), val, index%info.Packing)
	}

	return val
}

func (DB *Database) GetElem(i uint64) uint64 {
	if i >= DB.Info.Num {
		panic("Index out of range")
	}

	col := i % DB.Data.Cols
	row := i / DB.Data.Cols

	if DB.Info.Packing > 0 {
		new_i := i / DB.Info.Packing
		col = new_i % DB.Data.Cols
		row = new_i / DB.Data.Cols
	}

	var vals []uint64
	for j := row * DB.Info.Ne; j < (row+1)*DB.Info.Ne; j++ {
		vals = append(vals, DB.Data.Get(j, col))
	}

	return ReconstructElem(vals, i, DB.Info)
}

// Find smallest l, m such that l*m >= N*ne and ne divides l, where ne is
// the number of Z_p elements per DB entry determined by row_length and p.
func ApproxSquareDatabaseDims(N, row_length, p uint64) (uint64, uint64) {
	db_elems, elems_per_entry, _ := Num_DB_entries(N, row_length, p)
	l := uint64(math.Floor(math.Sqrt(float64(db_elems))))

	rem := l % elems_per_entry
	if rem != 0 {
		l += elems_per_entry - rem
	}

	m := uint64(math.Ceil(float64(db_elems) / float64(l)))

	return l, m
}

// Find smallest l, m such that l*m >= N*ne and ne divides l, where ne is
// the number of Z_p elements per DB entry determined by row_length and p, and m >=
// lower_bound_m.
func ApproxDatabaseDims(N, row_length, p, lower_bound_m uint64) (uint64, uint64) {
	l, m := ApproxSquareDatabaseDims(N, row_length, p)
	if m >= lower_bound_m {
		return l, m
	}

	m = lower_bound_m
	db_elems, elems_per_entry, _ := Num_DB_entries(N, row_length, p)
	l = uint64(math.Ceil(float64(db_elems) / float64(m)))

	rem := l % elems_per_entry
	if rem != 0 {
		l += elems_per_entry - rem
	}

	return l, m
}

func SetupDB(Num, row_length uint64, p *Params) *Database {
	if (Num == 0) || (row_length == 0) {
		panic("Empty database!")
	}

	D := new(Database)

	D.Info.Num = Num
	D.Info.Row_length = row_length
	D.Info.P = p.P
	D.Info.Logq = p.Logq

	db_elems, elems_per_entry, entries_per_elem := Num_DB_entries(Num, row_length, p.P)
	D.Info.Ne = elems_per_entry
	D.Info.X = D.Info.Ne
	D.Info.Packing = entries_per_elem

	for D.Info.Ne%D.Info.X != 0 {
		D.Info.X += 1
	}

	D.Info.Basis = 0
	D.Info.Squishing = 0

	fmt.Printf("Total packed DB size is ~%f MB\n",
		float64(p.L*p.M)*math.Log2(float64(p.P))/(1024.0*1024.0*8.0))

	if db_elems > p.L*p.M {
		panic("Params and database size don't match")
	}

	if p.L%D.Info.Ne != 0 {
		panic("Number of DB elems per entry must divide DB height")
	}

	return D
}

func MakeRandomDB(Num, row_length uint64, p *Params) *Database {
	D := SetupDB(Num, row_length, p)
	D.Data = MatrixRand(p.L, p.M, 0, p.P)

	// Map DB elems to [-p/2; p/2]
	D.Data.Sub(p.P / 2)

	return D
}

func MakeDB(Num, row_length uint64, p *Params, vals []uint64) *Database {
	D := SetupDB(Num, row_length, p)
	D.Data = MatrixZeros(p.L, p.M)

	if uint64(len(vals)) != Num {
		panic("Bad input DB")
	}

	if D.Info.Packing > 0 {
		// Pack multiple DB elems into each Z_p elem
		at := uint64(0)
		cur := uint64(0)
		coeff := uint64(1)
		for i, elem := range vals {
			cur += (elem * coeff)
			coeff *= (1 << row_length)
			if ((i+1)%int(D.Info.Packing) == 0) || (i == len(vals)-1) {
				D.Data.Set(cur, at/p.M, at%p.M)
				at += 1
				cur = 0
				coeff = 1
			}
		}
	} else {
		// Use multiple Z_p elems to represent each DB elem
		for i, elem := range vals {
			for j := uint64(0); j < D.Info.Ne; j++ {
				D.Data.Set(Base_p(D.Info.P, elem, j), (uint64(i)/p.M)*D.Info.Ne+j, uint64(i)%p.M)
			}
		}
	}

	// Map DB elems to [-p/2; p/2]
	D.Data.Sub(p.P / 2)

	return D
}

/////////////////////// UPDATED ///////////////////////

/**
 * get the db dimensions given db size and plaintext modulo
 *  modified from pir.Database.ApproxSquareDatabaseDims() to factor in bucket size
 *
 * @params <psiParams> params of the greater psi protocol
 * @params <entryBits> number of bits to represent an entry
 * @params <ptMod> plaintext modulo (or p)
 */
func GetDatabaseDims(psiParams *PSIParams, entryBits, ptMod uint64) (uint64, uint64) {

    ptElems, ptPerEntry, _ := Num_DB_entries(psiParams.DBBytes(), entryBits, ptMod)

    rows := psiParams.BucketsPerCol * psiParams.BucketSize * ptPerEntry
    cols := psiParams.DBBytes() * ptPerEntry / rows

    if entryBits != uint64(8) { panic("unsupported for now") }
    if rows * cols < ptElems { panic("don't expect this to happen") }

    return rows, cols
}


/**
 * create the matrix database with the given parameters
 *  modified from pir.Database.MakeDB() to stack into columns rather than rows
 *
 * @params <dbSize> number of entries in the database
 * @params <entryBits> number of bits to represent an entry
 * @params <params> protocol parameters
 * @params <values> values to populate database
 */
func CreateDatabase(entryBits uint64, params *Params, values []uint64) *Database {
	db := SetupDB(uint64(len(values)), entryBits, params)
	db.Data = MatrixZeros(params.L, params.M)

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
 *  modified from pir.database.SetupDB()
 *
 * @params <psiParams> params of the greater psi protocol
 * @params <entryBits> number of bits to represent an entry
 * @params <params> protocol parameters
 */
func SetupDBInfo(psiParams *PSIParams, entryBits uint64, params *Params) (DBinfo) {

    var info DBinfo

	info.Num = psiParams.DBBytes()
	info.Row_length = entryBits
	info.P = params.P
	info.Logq = params.Logq

	ptElems, ptPerEntry, entryPerPt := Num_DB_entries(psiParams.DBBytes(), entryBits, params.P)
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
