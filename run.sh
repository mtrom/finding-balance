rm ./out/*
set -e
./bin/datagen --server-n $1 --client-n $2 --overlap $3
./bin/offline --client --server-n $1
./bin/offline --server
./bin/pir out/server.edb out/queries.db out/answer.edb
./bin/online --server &
./bin/online --client &
wait
set +e
