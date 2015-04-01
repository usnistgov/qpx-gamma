#!/bin/bash
qmake
make release
SOURCEDIR=./samples/*
DESTDIR=$HOME/qpxdata
mkdir $DESTDIR
cp -ur $SOURCEDIR $DESTDIR

