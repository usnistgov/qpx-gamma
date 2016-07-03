#!/bin/bash

echo "---------------------------"
echo "---=====================---"
echo "---=== QPX INSTALLER ===---"
echo "---=====================---"
echo "---------------------------"
echo " "
echo "This script will install all necessary dependencies and compile qpx. It has been tested on Ubuntu distributions 14.04 and higher, but no guarantee that all will work as intended."
echo "You will be asked if you wish to install boost and Qt. At minimum, qpx requires boost 1.58 and full version of Qt 5.5. These are the recommended versions to use."
echo "You will be asked to enter your password for sudo access a number of times for installing required packages. Other than boost and Qt, only standard ubuntu packages are used."
echo " "
read -n1 -rsp $'Press any key to continue or Ctrl+C to exit...\n'

PKG_OK=$(dpkg-query -W --showformat='${Status}\n' build-essential|grep "install ok installed")
if [ "" == "$PKG_OK" ]; then
  echo "Installing build-essential"
  sudo apt-get --yes install build-essential
fi

read -r -p "Install boost? [Y/n]" getboost
getboost=${getboost,,} # tolower

if [[ $getboost =~ ^(yes|y| ) ]]; then
 ./get-boost.sh
fi

read -r -p "Install Qt? [Y/n]" getqt
getqt=${getqt,,} # tolower

if [[ $getqt =~ ^(yes|y| ) ]]; then
  sudo apt-get -y install qtchooser libgl1-mesa-dev
  if [ `getconf LONG_BIT` = "64" ]
  then
    wget http://download.qt.io/official_releases/online_installers/qt-unified-linux-x64-online.run
    chmod +x qt-unified-linux-x64-online.run
    ./qt-unified-linux-x64-online.run
    rm qt-unified-linux-x64-online.run    
  else
    wget http://download.qt.io/official_releases/online_installers/qt-unified-linux-x86-online.run
    chmod +x qt-unified-linux-x86-online.run
    ./qt-unified-linux-x86-online.run
    rm qt-unified-linux-x86-online.run    
  fi

  read -r -p "Configure qtchooser for newly installed version of Qt? [Y/n]" cfgchooser
  cfgchooser=${cfgchooser,,} # tolower

  if [[ $cfgchooser =~ ^(yes|y| ) ]]; then
    read -p "Installed version (default 5.5): " qtversion
    if [ -z "$qtversion" ]; then
      qtversion="5.5"
    fi

    t1="~/Qt/${qtversion}/gcc"
    t2="~/Qt/${qtversion}/gcc"
    if [ `getconf LONG_BIT` = "64" ]
    then
      t1="${t1}_64"
      t2="${t2}_64"
    fi
    t1="${t1}/bin"
    t2="${t2}/lib"
    text="${t1}"$'\n'"${t2}"
    mkdir ~/.config/qtchooser
    echo "$text" > ~/.config/qtchooser/default.conf
  fi
fi


#make distclean
./config.sh
SOURCEDIR=./data/*
DESTDIR=$HOME/qpx
mkdir $DESTDIR
cp -ur $SOURCEDIR $DESTDIR

read -r -p "Make release & install (else debug)? [Y/n]" mkrelease
mkrelease=${mkrelease,,} # tolower

if [[ $mkrelease =~ ^(yes|y| ) ]]; then
  make release
  if [ $? -eq 0 ]
  then
    sudo make install
    echo " "
    echo "Release version compiled and installed. To start qpx, you may run 'qpx' from anywhere in the system."
  fi
else
  make debug
  if [ $? -eq 0 ]
  then
    echo " "
    echo "Debug version compiled. You should run qpx as 'gdb ./qpxd' followed by 'run'. Report any problems to authors."
  fi
fi
