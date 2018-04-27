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

git submodule init
git submodule update

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

echo "---¿¿¿ install boost ???---"
echo "qpx requires boost, version 1.58 at the minimum."
echo "If you know that you already have boost installed, you may not need to install it now."
read -r -p "Do you want to install boost? [Y/n]" getboost
getboost=${getboost,,} # tolower
if [[ $getboost =~ ^(yes|y| ) ]]; then
 ./bash/get-boost.sh 58
fi

echo "---¿¿¿ install Qt ???---"
echo "qpx uses Qt for the graphical user interface. If you intend to use the graphical interface, you"
echo "will need Qt. If you know that you already have Qt installed, you may not need to install it now."
read -r -p "Do you want to install Qt? [Y/n]" getqt
getqt=${getqt,,} # tolower
if [[ $getqt =~ ^(yes|y| ) ]]; then
  ./bash/get-qt.sh 5
fi

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
mkdir $DESTDIR
cp -ur $SOURCEDIR $DESTDIR

read -r -p "Build qpx now? [Y/n]" mkrelease
mkrelease=${mkrelease,,} # tolower

if [[ $mkrelease =~ ^(yes|y| ) ]]; then
  mkdir build
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
