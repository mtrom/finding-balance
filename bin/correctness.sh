#!/bin/bash

run_protocol() {
    rm -r ./out/* 2> /dev/null
    if [ $1 -gt 1 ]; then
        (cd out/; seq 0 1 $1 | xargs -n 1 mkdir)
    fi
    set -e
    ./bin/datagen --server-log $SERVER_LOG --client-log $CLIENT_LOG --overlap $OVERLAP


    ./bin/oprf --server --cuckoo-n $1 --cuckoo-pad $2 --cuckoo-hashes $3 --hashtable-n $4 --hashtable-pad $5 --threads $7 &
    ./bin/oprf --client --cuckoo-n $1 --cuckoo-pad $2 --cuckoo-hashes $3 --hashtable-n $4 &
    wait
    if [ $1 -gt 1 ]; then
        ./bin/pir  --client --cuckoo-n $1 --bucket-n $4 --bucket-size $5 --buckets-per-col $6 --expected $OVERLAP --threads $7 &
        ./bin/pir  --server --cuckoo-n $1 --bucket-n $4 --bucket-size $5 --buckets-per-col $6 --queries-log $CLIENT_LOG --threads $7 &
    else
        ./bin/pir  --client --bucket-n $4 --bucket-size $5 --buckets-per-col $6 --expected $OVERLAP --threads $7 &
        ./bin/pir  --server --bucket-n $4 --bucket-size $5 --buckets-per-col $6 --queries-log $CLIENT_LOG --threads $7 &
    fi
    wait
    set +e
}

SERVER_LOG=10

CLIENT_LOG=0
OVERLAP=1
# single thread, no cuckoo, positive result
echo ""
echo ">>>>>>>>>>>>>>> T=1 C=1 E=1 <<<<<<<<<<<<<<<"
run_protocol 1 0 0 1024 0 2 1

CLIENT_LOG=0
OVERLAP=0
# single thread, no cuckoo, negative result
echo ""
echo ">>>>>>>>>>>>>>> T=1 C=1 E=0 <<<<<<<<<<<<<<<"
run_protocol 1 0 0 1024 0 2 1

CLIENT_LOG=3
OVERLAP=3
# single thread, no cuckoo, multiple result
echo ""
echo ">>>>>>>>>>>>>>> T=1 C=1 E=3 <<<<<<<<<<<<<<<"
run_protocol 1 0 0 1024 0 2 1

CLIENT_LOG=0
OVERLAP=1
# multi thread, no cuckoo, positive result
echo ""
echo ">>>>>>>>>>>>>>> T=4 C=1 E=1 <<<<<<<<<<<<<<<"
run_protocol 1 0 0 1024 0 2 4

CLIENT_LOG=0
OVERLAP=0
# multi thread, no cuckoo, negative result
echo ""
echo ">>>>>>>>>>>>>>> T=4 C=1 E=0 <<<<<<<<<<<<<<<"
run_protocol 1 0 0 1024 0 2 4

CLIENT_LOG=3
OVERLAP=5
# multi thread, no cuckoo, multiple result
echo ""
echo ">>>>>>>>>>>>>>> T=4 C=1 E=5 <<<<<<<<<<<<<<<"
run_protocol 1 0 0 1024 0 2 4

CLIENT_LOG=0
OVERLAP=1
# single thread, cuckoo, positive result
echo ""
echo ">>>>>>>>>>>>>>> T=1 C=8 E=1 <<<<<<<<<<<<<<<"
run_protocol 8 512 2 512 10 2 1

CLIENT_LOG=0
OVERLAP=0
# single thread, cuckoo, negative result
echo ""
echo ">>>>>>>>>>>>>>> T=1 C=8 E=0 <<<<<<<<<<<<<<<"
run_protocol 8 512 2 512 10 2 1

CLIENT_LOG=1
OVERLAP=2
# single thread, cuckoo, multiple result
echo ""
echo ">>>>>>>>>>>>>>> T=1 C=8 E=2 <<<<<<<<<<<<<<<"
run_protocol 8 512 2 512 10 2 1

CLIENT_LOG=0
OVERLAP=1
# multi thread, cuckoo, positive result
echo ""
echo ">>>>>>>>>>>>>>> T=4 C=8 E=1 <<<<<<<<<<<<<<<"
run_protocol 8 512 2 512 10 2 4

CLIENT_LOG=0
OVERLAP=0
# multi thread, cuckoo, negative result
echo ""
echo ">>>>>>>>>>>>>>> T=4 C=8 E=0 <<<<<<<<<<<<<<<"
run_protocol 8 512 2 512 10 2 4

CLIENT_LOG=1
OVERLAP=2
# multi thread, cuckoo, multiple result
echo ""
echo ">>>>>>>>>>>>>>> T=4 C=8 E=2 <<<<<<<<<<<<<<<"
run_protocol 8 512 2 512 10 2 4
