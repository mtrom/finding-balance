# rm -r ./out/* 2> /dev/null
set -e

if [ -z ${CUCKOO_N+x} ]; then
    ./bin/datagen --server-log $1 --client-log $2 --overlap $3
    BUCKET_N=$((2**$1))
    BUCKET_SIZE=0
    BUCKETS_PER_COL=2
    QUERIES=$((2**$2))

    ./bin/oprf --server --cuckoo-n 1 --cuckoo-pad 0 --cuckoo-hashes 3 --hashtable-n $BUCKET_N --hashtable-pad $BUCKET_SIZE --threads $4 &
    ./bin/oprf --client --cuckoo-n 1 --cuckoo-pad 0 --cuckoo-hashes 3 --hashtable-n $BUCKET_N &
    wait
    ./bin/pir  --client --bucket-n $BUCKET_N --bucket-size $BUCKET_SIZE --buckets-per-col $BUCKETS_PER_COL --expected $3 &
    ./bin/pir  --server --bucket-n $BUCKET_N --bucket-size $BUCKET_SIZE --buckets-per-col $BUCKETS_PER_COL --queries-log $2 &
    wait
else
    CLIENT_LOG=3
    EXPECTED=4
    # ./bin/datagen --server-log 10 --client-log $CLIENT_LOG --overlap $EXPECTED
    CUCKOO_N=12
    CUCKOO_PAD=512
    CUCKOO_HASHES=3
    BUCKET_N=$CUCKOO_PAD
    BUCKET_SIZE=8
    BUCKETS_PER_COL=2

    # (cd out/; seq 0 1 $CUCKOO_N | xargs -n 1 mkdir)
    ./bin/oprf --client --cuckoo-n $CUCKOO_N --cuckoo-pad $CUCKOO_PAD --cuckoo-hashes $CUCKOO_HASHES --hashtable-n $BUCKET_N &
    ./bin/oprf --server --cuckoo-n $CUCKOO_N --cuckoo-pad $CUCKOO_PAD --cuckoo-hashes $CUCKOO_HASHES --hashtable-n $BUCKET_N --hashtable-pad $BUCKET_SIZE --threads $1 &
    wait
    ./bin/pir  --client --cuckoo-n $CUCKOO_N --bucket-n $BUCKET_N --bucket-size $BUCKET_SIZE --buckets-per-col $BUCKETS_PER_COL --expected $EXPECTED --threads $1 &
    ./bin/pir  --server --cuckoo-n $CUCKOO_N --bucket-n $BUCKET_N --bucket-size $BUCKET_SIZE --buckets-per-col $BUCKETS_PER_COL --queries-log $CLIENT_LOG --threads $1 &
    wait
fi

set +e
