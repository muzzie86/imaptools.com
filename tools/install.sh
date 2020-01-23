#!/bin/sh

(cd ~ ; mkdir -p bin include lib)

# compile and install tools
for x in strtools shptools geo-rgeo maps ; do
    (cd $x && make && make install)
done

# after all is done clean up
for x in strtools shptools geo-rgeo maps ; do
    (cd $x && make clean)
done

# copy other scripts
cp -fp bin/*  ~/bin/.

