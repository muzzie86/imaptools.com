# iMaptools - Tools for Building Geocoders, Reverse Geocoders and Mapping

First step is to compile and install the tools with:

```
./install.sh
```

This will compile C code and install is under ``~/bin/`` along with various scripts. Once this is done, you can proceed to download Tiger data and prepare is for Geocodering, Reverse Geocoding and/or mapping.

## Geocoding

Adjust paths as appropriate to your environment. Copy the following into a script file.

```
#!/bin/bash

YEAR=2019
DBPORT=5432
DBNAME=tiger${YEAR}_geo
DBUSER=postgres
DBHOST=localhost

# TODO edit path
SQL_SCRIPTS=~/work/imaptools/sql-scripts/geocoder

process-build-places tiger${YEAR}-places.db /u/srcdata/TIGER${YEAR}/
process-tgr2pagc-2011-shp /u/data/tiger${YEAR}-geo/ /u/srcdata/TIGER${YEAR}/ . &> tiger${YEAR}-geo.log

load-tiger-geo tiger${YEAR}_geo ${DBPORT} ${YEAR} /u/data/tiger${YEAR}-geo/ &> tiger${YEAR}-geo-load.log

psql -U $DBUSER -h $DBHOST -p $DBPORT tiger${YEAR}_geo -f ${SQL_SCRIPTS}/us-gaz.sql
psql -U $DBUSER -h $DBHOST -p $DBPORT tiger${YEAR}_geo -f ${SQL_SCRIPTS}/us-lex.sql
psql -U $DBUSER -h $DBHOST -p $DBPORT tiger${YEAR}_geo -f ${SQL_SCRIPTS}/us-rules.sql

# look through the iger${YEAR}-geo-load.log
# for "Unable to convert data value to UTF-8"
# copy those commands to a file and change "UTF-8" to "LATIN1"
# and execute those commands to load the data that failed to load
# for example the following:

shp2pgsql -s 4326 -a -g the_geom -D -t 2D -W LATIN1 -N skip '/u/data/tiger2019-geo/72/115/Streets.shp' data.streets | psql -U postgres -h localhost -p 5432 tiger2019_geo
shp2pgsql -s 4326 -a -g the_geom -D -t 2D -W LATIN1 -N skip '/u/data/tiger2019-geo/72/031/Streets.shp' data.streets | psql -U postgres -h localhost -p 5432 tiger2019_geo
shp2pgsql -s 4326 -a -g the_geom -D -t 2D -W LATIN1 -N skip '/u/data/tiger2019-geo/72/035/Streets.shp' data.streets | psql -U postgres -h localhost -p 5432 tiger2019_geo
shp2pgsql -s 4326 -a -g the_geom -D -t 2D -W LATIN1 -N skip '/u/data/tiger2019-geo/72/037/Streets.shp' data.streets | psql -U postgres -h localhost -p 5432 tiger2019_geo


#
# The final prep is done by manually running the SQL commands in
# sql-scripts/geocoder/prep-tiger-geo-new.sql
#
# I do NOT reccommend blindly running the whole script
# read it and copy and paste the commands or break it up into pieces
# that can be run as a group.
#
```

## Reverse Geocoding

NOTE: use absolution paths on the command lines.

```
cd geo-rgeo
# edit tgr2rgeo-sqlite.c and change YEAR
make all install
process-tgr-2011 [-mtfcc] /path/to/srcdir /path/to/dstdir /path/to/usps-actual.db
rgeo2pg
cd ..
```


## Mapping

```
cd ../tiger-maps
# see README.md
```
