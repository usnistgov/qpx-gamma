#!/bin/bash

echo "-------------------------"
echo "---===================---"
echo "---=== QPX UPDATER ===---"
echo "---===================---"
echo "-------------------------"
echo " "
echo "This script will update qpx from latest git version. It will also"
echo "rerun the configure script. It will also copy (and replace) any"
echo "config and sample data files in your HOME/qpx directory. If you"
echo "have uncommitted changes, then maybe you shouldn't do this."
echo " "
read -r -p "Update and build qpx now? [Y/n]" mkrelease
mkrelease=${mkrelease,,} # tolower

if [[ $mkrelease =~ ^(yes|y| ) ]]; then

  git pull
  git submodule update

  ./bash/config.sh

  SOURCEDIR=./data/*
  DESTDIR=$HOME/qpx
  mkdir -p $DESTDIR
  cp -ur $SOURCEDIR $DESTDIR

  mkdir -p build
  cd build
  cmake -DCMAKE_BUILD_TYPE=Release ../src
  make -j$(nproc)
  if [ $? -eq 0 ]
  then
    read -r -p "Built successfully. Install qpx to system? [Y/n]" mkinstall
    mkinstall=${mkinstall,,} # tolower
    if [[ $mkinstall =~ ^(yes|y| ) ]]; then
      sudo make install
      echo " "
      echo "Release version compiled and installed. To start qpx, you may run 'qpx' from anywhere in the system."
    fi
  fi
fi
