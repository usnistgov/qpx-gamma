#!/bin/bash

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
