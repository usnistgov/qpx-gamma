#!/bin/bash

root_version=$1

if [ -z "$root_version" ]; then
  exit
fi

cd "$( dirname "${BASH_SOURCE[0]}" )/../.."

sudo apt-get --yes install git dpkg-dev cmake g++ gcc binutils libx11-dev libxpm-dev libxft-dev libxext-dev

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
printf '%s\n' "$LINE" > ~/.profile
