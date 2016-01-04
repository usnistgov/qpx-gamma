#!/bin/bash
qmake
make release
SOURCEDIR=./samples/*
DESTDIR=$HOME/qpx
mkdir $DESTDIR
cp -ur $SOURCEDIR $DESTDIR

