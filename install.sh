#!/bin/bash
qmake cpx.pro
make release
qmake qpx.pro
make release
SOURCEDIR=./samples/*
DESTDIR=$HOME/qpxdata
mkdir $DESTDIR
cp -ur $SOURCEDIR $DESTDIR

