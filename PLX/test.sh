if [ -z $(lsmod | grep Plx9054 | xargs | tr -d ' ') ]; then
	echo "Plx driver failed to load (as indicated by lsmod). Examine driver debug output. Consult documentation."
	exit 1
fi

if [ -z $(cat /proc/iomem | grep Plx9054 | xargs | tr -d ' ') ]; then
	echo "Plx driver failed to load (as indicated in /proc/iomem). Examine driver debug output. Consult documentation."
	exit 1
fi

if [ -z $(ls /dev/plx* | xargs | tr -d ' ') ]; then
	echo "Plx devices not mounted. Examine driver debug output. Consult documentation."
	exit 1
fi
