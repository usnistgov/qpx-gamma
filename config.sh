#!/bin/bash

PKG_OK=$(dpkg-query -W --showformat='${Status}\n' dialog|grep "install ok installed")
if [ "" == "$PKG_OK" ]; then
  echo "Installing dialog"
  sudo apt-get --yes install dialog
fi

parser_raw="off"
simulator2d="off"
pixie4="off"
hv8="off"
vme="off"
parser_evt="off"
fitter_ceres="off"

if [ ! -f config.pri ]; then
  parser_raw="on"
  simulator2d="on"
else
  if grep -q parser_raw config.pri; then 
    parser_raw="on" 
  fi
  if grep -q simulator2d config.pri; then 
    simulator2d="on" 
  fi
  if grep -q pixie4 config.pri; then 
    pixie4="on" 
  fi
  if grep -q hv8 config.pri; then 
    hv8="on" 
  fi
  if grep -q vme config.pri; then 
    vme="on" 
  fi
  if grep -q parser_evt config.pri; then 
    parser_evt="on" 
  fi
  if grep -q fitter_ceres config.pri; then 
    fitter_ceres="on" 
  fi
fi

cmd=(dialog --separate-output --checklist "Compile QPX with support for the following data sources:" 22 76 16)
options=(
         1 "Parser for QPX list output" "$parser_raw"
         2 "Simulator2D" "$simulator2d"
         3 "XIA Pixie-4" "$pixie4"
         4 "Radiation Technologies HV-8" "$hv8"
         5 "VME (Wiener, Mesytec, Iseg)" "$vme"
         6 "Parser for NSCL *.evt" "$parser_evt"
         7 "Ceres solver (fitter, experimental)" "$fitter_ceres"
        )
choices=$("${cmd[@]}" "${options[@]}" 2>&1 >/dev/tty)

if test $? -ne 0
then
  clear 
  exit
fi

clear 

text='DAQ_SOURCES +='
for choice in $choices
do
    case $choice in
        1)
            text="${text} parser_raw"
            ;;
        2)
            text="${text} simulator2d"
            ;;
        3)
            text="${text} pixie4"
            ;;
        4)
            text="${text} hv8"
            ;;
        5)
            text="${text} vme"
            PKG_OK=$(dpkg-query -W --showformat='${Status}\n' libusb-dev|grep "install ok installed")
            if [ "" == "$PKG_OK" ]; then
              echo "Installing libusb"
              sudo apt-get --yes install libusb-dev
            fi
            ;;
        6)
            text="${text} parser_evt"
            ;;
        7)
            text="${text} fitter_ceres"
            ;;
    esac
done

echo $text > config.pri

PKG_OK=$(dpkg-query -W --showformat='${Status}\n' libnlopt-dev|grep "install ok installed")
if [ "" == "$PKG_OK" ]; then
  echo "Installing libnlopt"
  sudo apt-get --yes install libnlopt-dev
fi

echo "Preparing makefiles..."
./prep.sh


