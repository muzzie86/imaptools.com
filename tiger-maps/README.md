# Preparing US Census Tiger data for years 2012+ for mapping

This process assumes the following directory structure, where YYYY is the year of the Tiger data you are working with.

* /u/data/tigerYYYY-maps/   - output directory for mapping data
* /u/srcdata/TIGERYYYY/     - downloaded tiger shapefiles
* ~/work/tiger-maps/        - this directory

There is a ``wget-tiger`` for downloading the Census tiger data.

I recommend using the above directory structure, you can use symlinks. Feel free to move things around and edit the scripts as needed for your own use.

```
#!/bin/sh

# set the year you are processing
YEAR=2018

./wget-tiger $YEAR
./prep-tiger-maps $YEAR
./prep-tiger-maps-2 $YEAR
cp tiger$YEAR-tmp.map /u/data/tiger$YEAR-maps/.
./copy-maps $YEAR /u/data/tiger$YEAR-maps/
(cd data ; for x in map-*.inc ; do ln -sf ~/work/tiger-maps/data/$x /u/data/tiger$YEAR-maps/$x ; done )
rsync -a data/etc /u/data/tiger$YEAR-maps/.
rsync -a data/not-nt2 /u/data/tiger$YEAR-maps/.
cd /u/data/tiger$YEAR-maps
~/work/tiger-maps/process-state $YEAR
~/work/tiger-maps/process-pointlm POINTLM/
find . -name \*.shp -exec shptree {} \;
```
