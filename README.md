# Unbalanced PSI from Single-Server PIR

## Overview

This repository contains the implementation accompanying the paper _Finding Balance in
Unbalanced PSI: A New Construction from Single-Server PIR_. It contains a
protocol for private set intersection (PSI) specifically tailored for the
unbalanced setting where a client holds a small dataset and interacts with a
server holding a much larger set to find their intersection.
Our construction achieves this by combining an oblivious pseudorandom function
(Microsoft's [APSI](https://github.com/microsoft/APSI)) with a private
information retrieval protocol
([SimplePIR](https://github.com/ahenzinger/simplepir)).

> [!WARNING]
> This repository is a research prototype intended for evaluating the
> protocol's performance. It is **not** production-ready and we would only
> advise it's use for research.

## Building the Project

> [!NOTE]
> The evaluations were run on a `c5.metal` AWS instance running Linux. The
> [deploy script](deploy.sh) was used to build the project on those machines so
> it may be a useful reference.

The project has several dependencies which should be built prior to compiling the main repository.

**OpenSSL**
```bash
git clone git@github.com:openssl/openssl.git && \
  (cd openssl/; ./Configure && make install)
```

**Coproto:**
```bash
git clone git@github.com:Visa-Research/coproto.git && cd coproto/ && \
  python3 build.py --setup --install=/usr/local/ -D COPROTO_CPP_VER=14 \
  -D COPROTO_ENABLE_BOOST=true -D COPROTO_ENABLE_OPENSSL=false -D \
  COPROTO_FETCH_AUTO=true && \
  python3 build.py --install=/usr/local/ -D COPROTO_CPP_VER=14 -D \
  COPROTO_ENABLE_BOOST=true -D COPROTO_ENABLE_OPENSSL=false -D \
  COPROTO_FETCH_AUTO=true
```

**CryptoTools:**
```bash
git clone git@github.com:ladnir/cryptoTools.git && \
  (cd cryptoTools; python3 build.py --install --relic --boost)
```

**APSI:**
```bash
./vcpkg/bootstrap-vcpkg.sh && ./vcpkg/vcpkg install apsi
```

> [!WARNING]
> The `go/` directory has a minimal, optimized implementation of SimplePIR
> pulled from [their repository](https://github.com/ahenzinger/simplepir). Thus
> it does not need to be fetched or built.

Once these have successfully been installed, you can build the project using `make`.


## Use

> [!NOTE]
> Because SimplePIR is written in Go and the APSI OPRF in C++, this project
> implements the protocol in three phases. The computation and communication
> costs of _all_ phases need to be accounted for. There will be time in between
> phases (reading and writing to disk) that should be ignored. The
> `benchmark.py` script handles this.

In order to manage a large number of parameters, the evaluation script
(`benchmark.py`) must be given an `.ini` file with the experiments to be run.

> [!TIP]
> See the existing files in `params/` for the formatting and parameters.

To run benchmark from a given parameter file:
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

To run a correctness test, you can either run `./run/correctness.sh` or
```bash
python3 benchmark.py params/correctness.ini
```
which will run quick tests for a variety of situations.
