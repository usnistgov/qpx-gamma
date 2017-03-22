#!/bin/bash

minimum_qt_version=$1

if [ -z "$minimum_qt_version" ]; then
  exit
fi


sudo apt-get -y install libgl1-mesa-dev

exp="apt-cache show qt5-default | grep -Po '(?<=Version: \d.)\d'"
default_qt_version=$(eval $exp)

echo default qt version on system = $default_qt_version

if [ "$default_qt_version" -ge "$minimum_qt_version" ]; then
  sudo apt-get --yes install qt5-default
  exit
fi;

if [ `getconf LONG_BIT` = "64" ]
then
  wget http://download.qt.io/official_releases/online_installers/qt-unified-linux-x64-online.run
  chmod +x qt-unified-linux-x64-online.run
  echo Will now install Qt using the online installer. Make sure to chose Qt5.5.
  read -n1 -rsp $'Press any key to continue or Ctrl+C to exit...\n'
  ./qt-unified-linux-x64-online.run
  rm qt-unified-linux-x64-online.run
  LINE="export CMAKE_PREFIX_PATH=$HOME/Qt/5.5/gcc_64"
else
  wget http://download.qt.io/official_releases/online_installers/qt-unified-linux-x86-online.run
  chmod +x qt-unified-linux-x86-online.run
  echo Will now install Qt using the online installer. Make sure to chose Qt5.5.
  read -n1 -rsp $'Press any key to continue or Ctrl+C to exit...\n'
  ./qt-unified-linux-x86-online.run
  rm qt-unified-linux-x86-online.run    
  LINE="export CMAKE_PREFIX_PATH=$HOME/Qt/5.5/gcc"
fi

eval $LINE
printf '%s\n' "$LINE" >> ~/.profile