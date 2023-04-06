# Unbalanced PSI from SimplePIR

## Use

```bash
./bin/datagen --server-n |X| --client-n |Y| --overlap ?
./bin/offline --client --database out/server.db
./bin/offline --server --database out/client.db
./bin/pir --database out/server.edb --queries out/queries.db
./bin/online --server out/server.edb
./bin/online --client out/client.db
```


## Build

Install OpenSSL:
```bash
git clone git@github.com:openssl/openssl.git
cd openssl/
./Configure
make install
```

Install Coproto:
```bash
py build.py --setup --install=/usr/local/ -D COPROTO_CPP_VER=14 -D COPROTO_ENABLE_BOOST=true -D
COPROTO_ENABLE_OPENSSL=false -D COPROTO_FETCH_AUTO=true
py build.py --install=/usr/local/ -D COPROTO_CPP_VER=14 -D COPROTO_ENABLE_BOOST=true -D COPROTO_ENABLE_OPENSSL=false -D COPROTO_FETCH_AUTO=true
```

```bash
cd cryptoTools
python3 build.py --install --relic --boost
cd ..
cmake -B build/ .
make -C build/
./bin/main -flags ...
```

Go installation:
```bash
cd go/
go get github.com/ahenzinger/simplepir
go build -o bin/serverpir go/server.go
```

I think `go get` should be okay now that its in the `go.mod` file.

## TODO
- test changes to parameter picking in go
- make pir communicate over network
- fix padding elements
- random seeds aren't very random
- cuckoo hashing for multiple queries?
- randomly permute elements in hashtable (or sort?)

## Notes to Self
Check for memory leaks: `valgrind --leak-check=yes ./main`

SimplePIR uses `uint32_t` from `stdin.h` for its database entries --- see `simplepir/pir/pir.h`.

The `src/tests/test.db` file was generated using:
```bash
echo -n -e '\xD2\x04\x00\x00\x40\xE2\x01\x00\x4E\x61\xBC\x00\xD2\x02\x96\x49' > src/tests/test.db
```
```bash
echo -n -e $(cat test.edb.desc | tr -d '\n') > go/test.edb
```
