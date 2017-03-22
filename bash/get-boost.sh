#!/bin/bash

minimum_boost_version=$1

if [ -z "$minimum_boost_version" ]; then
  exit
fi

exp="apt-cache show libboost-all-dev | grep -Po '(?<=Version: \d.)\d.'"
default_boost_version=$(eval $exp)

echo default boost version on system = $default_boost_version

if [ "$default_boost_version" -ge "$minimum_boost_version" ]; then
  exp="sudo apt-get --yes install libboost-all-dev"
  eval $exp
  exit
fi;

exp="apt-cache search libboost1 | grep -Po '(?<=libboost\d.)\d.(?=-all-dev)'"
other_boost_versions=($(eval $exp))

max_boost_version=${other_boost_versions[0]}
for n in "${other_boost_versions[@]}" ; do
    ((n > max_boost_version)) && max_boost_version=$n
done

echo highest available boost version = $max_boost_version

if [ "$max_boost_version" -ge "$minimum_boost_version" ]; then
  exp="sudo apt-get --yes install libboost\d.${max_boost_version}-all-dev"
  eval $exp
  exit
fi;


zl1="boost_1_"
zl2="_0"
ext=".tar.bz2"
zl=$zl1$minimum_boost_version$zl2
zlext=$zl$ext

echo Will download $zlext

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
rm -rf $zl

sudo ldconfig
