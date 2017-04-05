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

hdf5="off"

fitter_none="on"
fitter_ROOT="off"
fitter_Ceres="off"

cmd="off"
gui="off"

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )/../src" && pwd )/config"
FILE=$DIR"/CMakeLists.txt"

mkdir -p $DIR

if [ ! -f ${FILE} ]; then
  parser_raw="on"
  simulator2d="on"
  gui="on"
else
  if grep -q QPX_CMD ${FILE}; then
    cmd="on"
  fi
  if grep -q QPX_GUI ${FILE}; then
    gui="on"
  fi
  if grep -q QPX_USE_HDF5 ${FILE}; then
    hdf5="on"
  fi
  if grep -q QPX_PARSER_RAW ${FILE}; then
    parser_raw="on" 
  fi
  if grep -q QPX_SIMULATOR2D ${FILE}; then
    simulator2d="on" 
  fi
  if grep -q QPX_PIXIE4 ${FILE}; then
    pixie4="on" 
  fi
  if grep -q QPX_HV8 ${FILE}; then
    hv8="on" 
  fi
  if grep -q QPX_VME ${FILE}; then
    vme="on" 
  fi
  if grep -q QPX_PARSER_EVT ${FILE}; then
    parser_evt="on" 
  fi
  if grep -q QPX_FITTER_ROOT ${FILE}; then
    fitter_none="off" 
    fitter_ROOT="on"
    fitter_Ceres="off"
  elif grep -q QPX_FITTER_CERES ${FILE}; then
    fitter_none="off" 
    fitter_ROOT="off"
    fitter_Ceres="on"
  else
    fitter_none="on" 
    fitter_ROOT="off"
    fitter_Ceres="off"
  fi
fi

cmd1=(--title Options --checklist "Base program options:" 10 60 16)
options1=(
         1 "QPX Graphical Interface" "$gui"
         2 "Command line tool" "$cmd"
         3 "Use HDF5 (experimental)" "$hdf5"
        )

cmd2=(--and-widget --title Producers --checklist "Build the following data producer plugins:" 14 60 16)
options2=(
         4 "Parser for QPX list output" "$parser_raw"
         5 "Simulator2D" "$simulator2d"
         6 "XIA Pixie-4" "$pixie4"
         7 "Radiation Technologies HV-8" "$hv8"
         8 "VME (Wiener, Mesytec, Iseg)" "$vme"
         9 "Parser for NSCL *.evt" "$parser_evt"
        )

cmd3=(--and-widget --title Fitter --radiolist "Fitter:" 14 60 16)
options3=(
         10 "None" "$fitter_none"
         11 "ROOT" "$fitter_ROOT"
         12 "Ceres Solver" "$fitter_Ceres"
        )

cmd=(dialog --backtitle "QPX BUILD OPTIONS" --separate-output)
choices=$("${cmd[@]}" "${cmd1[@]}" "${options1[@]}" "${cmd2[@]}" "${options2[@]}" "${cmd3[@]}" "${options3[@]}" 2>&1 >/dev/tty)

if test $? -ne 0
then
  clear 
  exit
fi

clear

if [[ $choices == *"9"* ]] && [[ $choices != *"8"* ]]; then
  echo auto-enabling VMM module as prereq fro *.evt parser
  choices+=$' 8'
fi

text=''
for choice in $choices
do
    case $choice in
        1)
            text+=$'set(QPX_GUI TRUE PARENT_SCOPE)\n'
            ;;
        2)
            text+=$'set(QPX_CMD TRUE PARENT_SCOPE)\n'
            ;;
        3)
            text+=$'set(QPX_USE_HDF5 TRUE PARENT_SCOPE)\n'
            PKG_OK=$(dpkg-query -W --showformat='${Status}\n' libhdf5-dev|grep "install ok installed")
            if [ "" == "$PKG_OK" ]; then
              echo "Installing libhdf5"
              sudo apt-get --yes install libhdf5-dev
            fi
            ;;
        4)
            text+=$'set(QPX_PARSER_RAW TRUE PARENT_SCOPE) \n'
            ;;
        5)
            text+=$'set(QPX_SIMULATOR2D TRUE PARENT_SCOPE) \n'
            ;;
        6)
            text+=$'set(QPX_PIXIE4 TRUE PARENT_SCOPE)\n'
            ;;
        7)
            text+=$'set(QPX_HV8 TRUE PARENT_SCOPE)\n'
            ;;
        8)
            text+=$'set(QPX_VME TRUE PARENT_SCOPE)\n'
            PKG_OK=$(dpkg-query -W --showformat='${Status}\n' libusb-dev|grep "install ok installed")
            if [ "" == "$PKG_OK" ]; then
              echo "Installing libusb"
              sudo apt-get --yes install libusb-dev
            fi
            ;;
        9)
            text+=$'set(QPX_PARSER_EVT TRUE PARENT_SCOPE)\n'
            ;;
        11)
            text+=$'set(QPX_FITTER_ROOT TRUE PARENT_SCOPE)\n'
            ;;
        12)
            text+=$'set(QPX_FITTER_CERES TRUE PARENT_SCOPE)\n'
            ;;
    esac
done

touch $FILE
printf '%s\n' "$text" > ${FILE}

