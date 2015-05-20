#!/bin/bash


# Synchronize and build libbibledit on OS X for OS X.


pushd webroot


# Sychronizes the libbibledit data files in the source tree to OS X and cleans them up.
rsync -av --delete ../../lib/ .
./configure
make distclean
rm config.h


# Xcode's toolchain for C and C++.
export CC="/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/clang"
export CXX="/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/clang++"


# Xcode's OS X SDK.
SDK=`xcrun --show-sdk-path`


# Configure Bibledit in client mode.
./configure --enable-client


# Update the Makefile.
sed -i.bak  's#\`xml2-config --cflags\`#-I/usr/include/libxml2#g' Makefile
sed -i.bak 's#-pedantic#-isysroot\ '$SDK'#g' Makefile
sed -i.bak  's#/opt/local/include#.#g' Makefile
sed -i.bak  's#\`xml2-config --libs\`#-lxml2 -lz -lpthread -liconv -lm#g' Makefile
sed -i.bak  's#/opt/local/lib#.#g' Makefile


# Build the Bibledit library.
make -j `sysctl -n hw.logicalcpu_max`


# Save the header file.
cp library/bibledit.h ../Bibledit


# Clean out stuff no longer needed.
# find . -name "*.h" -delete
# find . -name "*.cpp" -delete
# find . -name "*.c" -delete
# find . -name "*.o" -delete
rm *.m4
rm -r autom*cache
rm bibledit
rm compile
rm config.*
rm configure*
rm depcomp
rm dev
rm install-sh
rm missing
rm reconfigure
rm valgrind
find . -name ".deps" -exec rm -r "{}" \; > /dev/null 2>&1
find . -name ".dirstamp" -delete
rm Makefile*
# rm server
rm stamp-h1


popd


say Compile for OS X is ready
