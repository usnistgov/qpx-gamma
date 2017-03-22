#!/bin/bash

cd "$( dirname "${BASH_SOURCE[0]}" )/../.."

sudo apt-get install git dpkg-dev cmake g++ gcc binutils libx11-dev libxpm-dev libxft-dev libxext-dev

git clone http://root.cern.ch/git/root.git
cd root
git checkout -b v6-08-06 v6-08-06

cd ..
mkdir root6.08
cd root6.08
cmake ../root
make -j$(nproc)

LINE="source $( pwd )/bin/thisroot.sh"
eval $LINE
printf '%s\n' "$LINE" > ~/.profile
