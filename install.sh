#!/bin/bash
make distclean
./config.sh
make release
SOURCEDIR=./data/*
DESTDIR=$HOME/qpx
mkdir $DESTDIR
cp -ur $SOURCEDIR $DESTDIR

