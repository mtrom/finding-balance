rm ./out/*
set -e
./bin/datagen --server-log $1 --client-log $2 --overlap $3
./bin/offline --client --server-log $1
./bin/offline --server
./bin/pir client $1 &
./bin/pir server $2 &
wait
./bin/online --server &
./bin/online --client --expected $3 &
wait
set +e
