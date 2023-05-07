rm -r ./out/* 2> /dev/null

BUCKET_N=$((2**$1))
BUCKET_SIZE=$1
BUCKETS_PER_COL=2

QUERIES=$((2**$2))

set -e
./bin/datagen --server-log $1 --client-log $2 --overlap $3
./bin/oprf --client --cuckoo-n 1 --bucket-n $BUCKET_N &
./bin/oprf --server --cuckoo-n 1 --bucket-n $BUCKET_N --bucket-size $BUCKET_SIZE --queries $QUERIES &
wait
./bin/pir  --client --bucket-n $BUCKET_N --bucket-size $BUCKET_SIZE --buckets-per-col $BUCKETS_PER_COL --expected $3 &
./bin/pir  --server --bucket-n $BUCKET_N --bucket-size $BUCKET_SIZE --buckets-per-col $BUCKETS_PER_COL --queries-log $2 &
wait
set +e
