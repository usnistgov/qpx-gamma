#!/bin/bash

PKG_OK=$(dpkg-query -W --showformat='${Status}\n' build-essential|grep "install ok installed")
if [ "" == "$PKG_OK" ]; then
  echo "Installing build-essential"
  sudo apt-get --force-yes --yes install build-essential
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
    echo "Version number (i.e. 5.5): "
    read qtversion
    t1="~/Qt/${qtversion}/gcc_64/bin"
    t2="~/Qt/${qtversion}/gcc_64/lib"
    text="${t1}"$'\n'"${t2}"
    mkdir ~/.config/qtchooser
    echo "$text" > ~/.config/qtchooser/default.conf
  fi
fi


make distclean
./config.sh
SOURCEDIR=./data/*
DESTDIR=$HOME/qpx
mkdir $DESTDIR
cp -ur $SOURCEDIR $DESTDIR

read -r -p "Make release & install (else debug)? [Y/n]" mkrelease
mkrelease=${mkrelease,,} # tolower

if [[ $mkrelease =~ ^(yes|y| ) ]]; then
  make release
  sudo make install
else
  make debug
fi

