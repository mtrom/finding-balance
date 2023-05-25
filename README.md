# Unbalanced PSI from SimplePIR

## Deploy on an AWS Machine

To deploy remotely, scrape over `deploy.sh`, make sure the machine has access to
the private repository by creating a key pair, then run `deploy.sh`.

```bash
AWS=<ip address>
scp deploy.sh ec2-user@$AWS:/home/ec2-user/
ssh ec2-user@$AWS
ssh-keygen -t ed25519 -C "<your@email>" # & add public key to github
./deploy.sh
```

## Build Dependencies Locally

If you want to build the project locally, the `deploy.sh` script is still a
helpful reference but you can also perform the following:

Install OpenSSL:
```bash
git clone git@github.com:openssl/openssl.git
cd openssl/
./Configure
make install
```

Install Coproto:
```bash
git clone git@github.com:Visa-Research/coproto.git
cd coproto/
python build.py --setup --install=/usr/local/ -D COPROTO_CPP_VER=14 -D COPROTO_ENABLE_BOOST=true -D COPROTO_ENABLE_OPENSSL=false -D COPROTO_FETCH_AUTO=true
python build.py --install=/usr/local/ -D COPROTO_CPP_VER=14 -D COPROTO_ENABLE_BOOST=true -D COPROTO_ENABLE_OPENSSL=false -D COPROTO_FETCH_AUTO=true
```

Install CryptoTools:
```bash
git clone git@github.com:ladnir/cryptoTools.git
cd cryptoTools
python3 build.py --install --relic --boost
```

Install APSI:
```bash
./vcpkg/bootstrap-vcpkg.sh
./vcpkg/vcpkg install apsi
```

## Use

To run benchmarks, create an `.ini` file in `params/` with the parameters you
want to benchmark. See existing files for reference. Then run:
```bash
python3 benchmark.py params/filename.ini
```
This will run each parameter configuration `trials` times and report back
performance and communication. It will also save the stdout of each run in the
`logs/` directory under the config file name and parameter set name. If you've
already run a file and have log files in `logs/`, you can just report out the
statistics by running:
```bash
python3 benchmark.py params/filename.ini --from-logs
```

*Note to Friends:* If you want to run a potentially very long benchmark, it's
probably best to run it in the background and pipe the output somewhere:
```
python3 benchmark.py params/filename.ini &> /tmp/benchmark.log &
```

*If you've been running a parameter file and hit a failure,* I'd suggest using
`--from-logs` to get the results from the runs that have already completed and
update the `.ini` file to include only configs you haven't run.

_Generally_, I'd say its just easier to have smaller `.ini` files (i.e., with
fewer configurations).

To run a correctness test, you can either run `./run/correctness.sh` or
```bash
python3 benchmark.py params/correctness.ini
```
which will run quick tests for a variety of situations.

To run manually, run:
```
./bin/datagen --server-log $SERVER_LOG --client $CLIENT_SIZE --overlap $OVERLAP

./bin/oprf --server --cuckoo-size $CUCKOO_SIZE --cuckoo-hashes $CUCKOO_HASHES --hashtable-size $HASHTABLE_SIZE --threads $THREADS &
./bin/oprf --client --cuckoo-size $CUCKOO_SIZE --cuckoo-hashes $CUCKOO_HASHES --hashtable-size $HASHTABLE_SIZE &
wait

./bin/pir  --client --cuckoo-size $CUCKOO_SIZE --hashtable-size $HASHTABLE_SIZE --buckets-per-col $BUCKETS_PER_COL --expected $OVERLAP &
./bin/pir  --server  --cuckoo-size $CUCKOO_SIZE --hashtable-size $HASHTABLE_SIZE --buckets-per-col $BUCKETS_PER_COL --queries $CLIENT_SIZE --threads $THREADS &
wait
```
