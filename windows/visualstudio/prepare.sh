#!/bin/bash


# This script runs on Linux or macOS.
# It populates a folder with files for Visual Studio.
# Visual Studio can then build Bibledit for Windows.


# Work in a directory separate from the source tree.
SCRIPTDIR=`dirname $0`
cd $SCRIPTDIR
WORKDIR=~/dev/visualstudio
mkdir -p $WORKDIR


echo Synchronizing relevant source code to the temporal directory.
rsync --archive --exclude '.DS_Store' . $WORKDIR/
rsync --archive --exclude '.deps' --exclude '*.o' --exclude '*.a' --exclude '.dirstamp' --exclude 'server' --exclude 'unittest' --exclude '.DS_Store' --exclude 'autom4te.cache' --exclude 'bibledit' --exclude '*~' --exclude 'dev' --exclude 'generate' --exclude 'valgrind' ../../lib/* $WORKDIR/server


cd $WORKDIR/server


# Configure the Bibledit source.
./configure --enable-windows --with-network-port=9876 --enable-visualstudio
EXIT_CODE=$?
if [ $EXIT_CODE != 0 ]; then
  exit
fi


# Remove the .deps folders.
find . -name ".deps" -type d -prune -exec rm -rf '{}' '+'


# Fix g++.exe: error: unrecognized command line option '-rdynamic'
# Fix undefined reference to `_imp__*' by adding required library to the linker.
# sed -i 's/-rdynamic/-lws2_32/' Makefile.am
