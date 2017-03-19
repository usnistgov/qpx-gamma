#!/bin/bash
#script to install Plx9054 driver for Pixie-4

if [ $(id -u) != "0" ]; then
	echo "You must be the superuser to run this script" >&2
	exit 1
fi

export PLX_SDK_DIR=$(pwd)

cd Driver
./builddriver 9054 cleanall
./builddriver 9054 d

echo "Plx driver (debug build) compiled. Loading..."

cd ../Bin
./Plx_load 9054 d

if [ -z $(lsmod | grep Plx9054 | xargs | tr -d ' ') ]; then
	echo "Plx driver failed to load. Examine driver debug output. Consult documentation."
	exit 0
fi

if [ -z $(cat /proc/iomem | grep Plx9054 | xargs | tr -d ' ') ]; then
	echo "Plx driver failed to load. Examine driver debug output. Consult documentation."
	exit 0
fi

if [ -z $(ls /dev/plx* | xargs | tr -d ' ') ]; then
	echo "Plx devices not mounted. Examine driver debug output. Consult documentation."
	exit 0
fi

cd ../Driver
./builddriver 9054 cleanall
./builddriver 9054

echo "Plx driver (release build) compiled. Configuring system to load upon bootup..."

chmod 644 ./Plx9054/Plx9054.ko
cp ./Plx9054/Plx9054.ko /lib/modules/$(uname -r)/kernel/drivers/iio/dac/
if [ -n $(grep Plx9054 /etc/modules | xargs | tr -d ' ') ]
then echo 'Plx9054' >> /etc/modules
fi
depmod

cd ..
cp ./load-plx9054 /etc/init.d
chown root /etc/init.d/load-plx9054
chgrp root /etc/init.d/load-plx9054
chmod 755 /etc/init.d/load-plx9054
update-rc.d load-plx9054 defaults
