#!/bin/bash
cmd=(dialog --separate-output --checklist "Compile QPX with support for the following data sources:" 22 76 16)
options=(1 "Parser for QPX list output" on    # any option can be set to default to "on"
         2 "Simulator2D" on
         3 "XIA Pixie-4" off
         4 "Radiation Technologies HV-8" off
         5 "VME (Wiener, Mesytec, Iseg)" off
         6 "Parser for NSCL *.evt" off
         7 "Ceres solver (fitter, experimental)" off)
choices=$("${cmd[@]}" "${options[@]}" 2>&1 >/dev/tty)
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
echo "Preparing makefiles..."
./prep.sh

