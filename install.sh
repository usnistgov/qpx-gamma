#!/bin/bash

echo "---------------------------"
echo "---=====================---"
echo "---=== QPX INSTALLER ===---"
echo "---=====================---"
echo "---------------------------"
echo " "
echo "This script will install all necessary dependencies and compile qpx. It"
echo "has been tested on Ubuntu distributions 14.04 and higher, but no guarantee"
echo "that all will work as intended. You will be asked if you wish to install"
echo "boost and Qt. At minimum, qpx requires boost 1.58 and full version of Qt 5.5."
echo "These are the recommended versions to use. You may be asked to enter your"
echo "password for sudo access a few times for installing required packages. Other"
echo "than boost and Qt, only standard ubuntu packages are used."
echo " "
read -n1 -rsp $'Press any key to continue or Ctrl+C to exit...\n'

PKG_OK=$(dpkg-query -W --showformat='${Status}\n' build-essential|grep "install ok installed")
if [ "" == "$PKG_OK" ]; then
  echo "Installing build-essential"
  sudo apt-get --yes install build-essential
fi

PKG_OK=$(dpkg-query -W --showformat='${Status}\n' cmake|grep "install ok installed")
if [ "" == "$PKG_OK" ]; then
  echo "Installing cmake"
  sudo apt-get --yes install cmake
fi

read -r -p "Install boost (>=1.58)? [Y/n]" getboost
getboost=${getboost,,} # tolower
if [[ $getboost =~ ^(yes|y| ) ]]; then
 ./bash/get-boost.sh 58
fi

read -r -p "Install Qt (>=5.5)? [Y/n]" getqt
getqt=${getqt,,} # tolower
if [[ $getqt =~ ^(yes|y| ) ]]; then
  ./bash/get-qt.sh 5
fi

read -r -p "Install ROOT (6.08.06)? [Y/n]" getroot
getroot=${getroot,,} # tolower
if [[ $getroot =~ ^(yes|y| ) ]]; then
  ./bash/get-ROOT.sh 6-08-06
fi


./bash/config.sh

SOURCEDIR=./data/*
DESTDIR=$HOME/qpx
mkdir $DESTDIR
cp -ur $SOURCEDIR $DESTDIR

read -r -p "Make release & install qpx? [Y/n]" mkrelease
mkrelease=${mkrelease,,} # tolower

if [[ $mkrelease =~ ^(yes|y| ) ]]; then
  mkdir build
  cd build
  cmake ../src
  make -j$(nproc)
  if [ $? -eq 0 ]
  then
    sudo make install
    echo " "
    echo "Release version compiled and installed. To start qpx, you may run 'qpx' from anywhere in the system."
  fi
fi
