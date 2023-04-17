rm ./out/*
set -e
./bin/datagen --server-n $1 --client-n $2 --overlap $3
./bin/offline --client --server-n $1
./bin/offline --server
./bin/pir client $1 &
./bin/pir server $2 &
wait
./bin/online --server &
./bin/online --client --expected $3 &
wait
set +e
