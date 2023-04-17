set -e
# install tools
sudo yum install git -y
sudo yum install openssl-devel
sudo yum install goalng -y

# install cmake
sudo yum groupinstall "Development Tools"
wget https://cmake.org/files/v3.18/cmake-3.18.0.tar.gz
tar -xvzf cmake-3.18.0.tar.gz
(cd cmake-3.18.0; ./bootstrap)
(cd cmake-3.18.0; make)
(cd cmake-3.18.0; sudo make install)

# install libraries
git clone git@github.com:Visa-Research/coproto.git
git clone git@github.com:mtrom/ddh-unbalanced-psi.git
git clone git@github.com:ladnir/cryptoTools.git

sudo mkdir -p /usr/local/lib64/cmake/coproto
sudo chown -R $(whoami) /usr/local/lib64/
sudo mkdir -p /usr/local/include/coproto
sudo chown $(whoami) /usr/local/include/coproto
(cd coproto; python3 build.py --setup --install=/usr/local/ -D COPROTO_CPP_VER=14 -D COPROTO_ENABLE_BOOST=true -D COPROTO_ENABLE_OPENSSL=false -D COPROTO_FETCH_AUTO=true)
(cd coproto; python3 build.py --install=/usr/local/ -D COPROTO_CPP_VER=14 -D COPROTO_ENABLE_BOOST=true -D COPROTO_ENABLE_OPENSSL=false -D COPROTO_FETCH_AUTO=true)

(cd cryptoTools; python3 build.py --install --relic --boost)

set +e
