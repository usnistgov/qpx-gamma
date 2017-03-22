#!/bin/bash

root_version=$1

if [ -z "$root_version" ]; then
  exit
fi

cd "$( dirname "${BASH_SOURCE[0]}" )/../.."

sudo apt-get --yes install git dpkg-dev cmake \
g++ gcc binutils libx11-dev libxpm-dev libxft-dev \
libxext-dev gfortran libssl-dev libpcre3-dev \
xlibmesa-glu-dev libglew1.5-dev libftgl-dev \
libmysqlclient-dev libfftw3-dev libcfitsio-dev \
graphviz-dev libavahi-compat-libdnssd-dev \
libldap2-dev python-dev libxml2-dev libkrb5-dev \
libgsl0-dev libqt4-dev

git clone http://root.cern.ch/git/root.git
cd root
git checkout -b v$root_version v$root_version

cd ..
mkdir root$root_version
cd root$root_version
cmake ../root
make -j$(nproc)

LINE="source $( pwd )/bin/thisroot.sh"
eval $LINE
printf '%s\n' "$LINE" >> ~/.profile
