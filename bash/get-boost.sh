#!/bin/bash

read -r -p "What version of boost would you like to install? (default:1.58)   1." vnum

if [ -z "$vnum" ]; then
  vnum="58"
fi

cl1="1."
cl2=".0"
cl=$cl1$vnum$cl2

fp="apt-cache search libboost1.${vnum}-all-dev"
res=$(eval $fp)

if [ "" != "$res" ]; then
  fe="sudo apt-get --yes install libboost1.${vnum}-all-dev"
  eval $fe
  exit
fi

if ldconfig -p | grep boost | grep -q $cl; then
  echo System appears to already have  boost $cl.
  exit
fi

zl1="boost_1_"
zl2="_0"
ext=".tar.bz2"
zl=$zl1$vnum$zl2
zlext=$zl$ext

echo Will download for $zlext

if [ -f ./$zlext ]; then
  echo deleting old zip
  rm ./$zlext
fi

wget http://downloads.sourceforge.net/project/boost/boost/$cl/$zlext


if [ -e ./$zl ]; then
  echo deleting old dir
  rm -R ./$zl
fi

tar jxf $zlext

PKG_OK=$(dpkg-query -W --showformat='${Status}\n' build-essential|grep "install ok installed")
if [ "" == "$PKG_OK" ]; then
  echo "Installing build-essential"
  sudo apt-get --yes install build-essential
fi

cd $zl
./bootstrap.sh
./b2
sudo ./b2 install
cd ..
rm $zlext
rm -R $zl

sudo ldconfig
