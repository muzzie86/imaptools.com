#!/bin/sh

(cd ~ ; mkdir -p bin include lib)
cd tools/strtools
make
make install
cd ../shptools
make
make install

