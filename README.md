# Unbalanced PSI from SimplePIR

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
./build/bin/main
```

Go installation:
```bash
cd go/
go get github.com/ahenzinger/simplepir
go build -o ../bin/serverpir server.go
```

I think `go get` should be okay now that its in the `go.mod` file.

## Open Questions
 - how to call into SimplePIR?
 - can we do an actual stateless server-client architecture rather than two-way communication channel?
   - how does SimplePIR interact with this?


## Notes to Self
Check for memory leaks: `valgrind --leak-check=yes ./main`

SimplePIR uses `uint32_t` from `stdin.h` for its database entries --- see `simplepir/pir/pir.h`.

