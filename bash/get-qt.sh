#!/bin/bash

minimum_qt_version=$1

qtversion="5.$1"

if [ `getconf LONG_BIT` = "64" ]
then
  dest_dir="~/Qt/${qtversion}/gcc_64"
  download_file="qt-unified-linux-x64-online.run"
else
  dest_dir="~/Qt/${qtversion}/gcc"
  download_file="qt-unified-linux-x86-online.run"
fi

echo $qtversion
echo $dest_dir
echo $download_file

exit

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

wget http://download.qt.io/official_releases/online_installers/$download_file
chmod +x $download_file
echo Will now install Qt using the online installer. Make sure to chose $qtversion.
read -n1 -rsp $'Press any key to continue or Ctrl+C to exit...\n'
./$download_file
rm $download_file

text="${dest_dir}/bin"$'\n'"${dest_dir}/lib"
mkdir ~/.config/qtchooser
echo "$text" > ~/.config/qtchooser/default.conf

LINE="export CMAKE_PREFIX_PATH=$dest_dir"
eval $LINE
printf '%s\n' "$LINE" >> ~/.profile