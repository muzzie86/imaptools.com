#!/bin/sh

(cd ~ ; mkdir bin include lib)
cd tools/strtools
make && make install && make clean
cd ../shptools
make && make install && make clean
cd ../geo-rgeo
make && make install && make clean
cd ..
cp -fp dbfdump ~/bin

