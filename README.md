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
py build.py --setup --install=/usr/local/ -D COPROTO_CPP_VER=14 -D COPROTO_ENABLE_BOOST=true -D COPROTO_ENABLE_OPENSSL=false -D COPROTO_FETCH_AUTO=true
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

## TODO
- random seeds aren't very random

## Notes to Self
Check for memory leaks: `valgrind --leak-check=yes ./main`

## AWS Commands

```bash
AWS=<ip address>
scp deploy.sh ec2-user@$AWS:/home/ec2-user/
ssh ec2-user@$AWS
ssh-keygen -t ed25519 -C "mtromanhauser@gmail.com" # & add public key to github
./deploy.sh
```
