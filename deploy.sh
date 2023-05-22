set -e
# download tools
sudo yum install openssl-devel -y
sudo yum install golang -y
sudo yum install git -y
sudo yum groupinstall "Development Tools" -y

# build cmake
wget https://cmake.org/files/v3.18/cmake-3.18.0.tar.gz
tar -xvzf cmake-3.18.0.tar.gz
(cd cmake-3.18.0; ./bootstrap)
(cd cmake-3.18.0; make)
(cd cmake-3.18.0; sudo make install)

# download libraries
git clone git@github.com:Visa-Research/coproto.git
git clone git@github.com:ladnir/cryptoTools.git
git clone git@github.com:microsoft/vcpkg.git

# make sure install locations are set up
sudo mkdir -p /usr/local/lib64/cmake/coproto
sudo chown -R $(whoami) /usr/local/lib64/
sudo mkdir -p /usr/local/include/coproto
sudo chown $(whoami) /usr/local/include/

# build coproto
(cd coproto; python3 build.py --setup --install=/usr/local/ -D COPROTO_CPP_VER=14 -D COPROTO_ENABLE_BOOST=true -D COPROTO_ENABLE_OPENSSL=false -D COPROTO_FETCH_AUTO=true)
(cd coproto; python3 build.py --install=/usr/local/ -D COPROTO_CPP_VER=14 -D COPROTO_ENABLE_BOOST=true -D COPROTO_ENABLE_OPENSSL=false -D COPROTO_FETCH_AUTO=true)

# build cryptoTools
(cd cryptoTools; python3 build.py --install --relic --boost)

# build apsi
./vcpkg/bootstrap-vcpkg.sh
./vcpkg/vcpkg install apsi

# download the repo
git clone git@github.com:mtrom/ddh-unbalanced-psi.git --recursive

# make necessary directories
mkdir ddh-unbalanced-psi/out
mkdir ddh-unbalanced-psi/logs

# build project
(cd ddh-unbalanced-psi; make)

set +e
