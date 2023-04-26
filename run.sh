rm -r ./out/* 2> /dev/null

BUCKET_N=$((2**$1))
BUCKET_SIZE=$1
BUCKETS_PER_COL=2

set -e
./bin/datagen --server-log $1 --client-log $2 --overlap $3
./bin/offline --client --server-log $1 --cuckoo-n 1
./bin/offline --server --cuckoo-n 1 --bucket-n $BUCKET_N --bucket-size $BUCKET_SIZE
./bin/pir -client --server-log $1 --bucket-n $BUCKET_N --bucket-size $BUCKET_SIZE --buckets-per-col $BUCKETS_PER_COL &
./bin/pir -server --queries-log $2 --bucket-n $BUCKET_N --bucket-size $BUCKET_SIZE --buckets-per-col $BUCKETS_PER_COL &
wait
./bin/online --server &
./bin/online --client --expected $3 &
wait
set +e
