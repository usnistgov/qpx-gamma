#!/bin/bash

#ensure that we are in the script directory
pushd $(dirname "${BASH_SOURCE[0]}")

. ./bash/numcpus.sh

echo "---------------------------"
echo "---=====================---"
echo "---=== QPX INSTALLER ===---"
echo "---=====================---"
echo "---------------------------"
echo " "
echo "This script will install all necessary dependencies and compile qpx. It"
echo "has been tested on Ubuntu 18.04. At minimum, qpx requires boost and Qt5."
echo "You may be asked to enter your password for sudo access a few times for"
echo "installing required packages."
echo " "
read -n1 -rsp $'Press any key to continue or Ctrl+C to exit...\n'

git submodule init || exit 1
git submodule update || exit 1

sudo apt-get install -y cmake qt5-default libboost-all-dev || exit 1

echo "---¿¿¿ install ROOT ???---"
echo "qpx can do some experimental peak fitting using CERN's ROOT framework for nonlinear optimization."
echo "If you already have ROOT, you may not need to install it. If you don't intend to use qpx's"
echo "experimental peak fitting, then you may also not need this."
read -r -p "Do you want to install ROOT (6.12.06)? [Y/n]" getroot
getroot=${getroot,,} # tolower
if [[ $getroot =~ ^(yes|y| ) ]]; then
  ./bash/get-ROOT.sh 6-12-06
fi

./bash/config.sh

SOURCEDIR=./data/*
DESTDIR=$HOME/qpx
mkdir -p $DESTDIR
cp -ur $SOURCEDIR $DESTDIR

read -r -p "Build qpx now? [Y/n]" mkrelease
mkrelease=${mkrelease,,} # tolower

if [[ $mkrelease =~ ^(yes|y| ) ]]; then
  mkdir -p build
  cd build
  cmake -DCMAKE_BUILD_TYPE=Release ../src || exit 1
  make -j$NUMCPUS || exit 1
fi
