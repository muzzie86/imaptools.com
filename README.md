# imaptools.com

Various tools developed at imaptools and being open sourced. This is being provided *AS IS* with no warrantees or statements that it is fit for any purpose. It might be junk, but one person's junk might be anothers treasure. I hope you find it useful.

## What is Here?

* LICENSE - License file
* README.md - This file
* sql-scripts - Lots of PostgreSQL SQL scripts and function
* sql-scripts/geocoder - SQL scripts and function related to geocoding
* sql-scripts/statcan - SQL scripts for working with Canada data
* sql-scripts/statcan/rgeo - SQL for Reverse Geocoder for Canada
* sql-scripts/statcan/geocode - SQL for Geocoder for Canada
* sql-scripts/tiger - SQL for Tiger Data
* sql-scripts/rgeo - SQL for Reverse Geocoder
* shp2tile-1.14 - C code for spatial tiling of data in shapefile
* tiger-maps - Data and stuff used in building mapping for Tiger data
* tools - C tools to compile and used in most of my scripts
* tools/scripts - various scripts
* tools/maps - Tools to process Navteq or Tiger data for maps
* tools/shptools - C library to compile and install for tools
* tools/geo-rgeo - tools for processing Navteq or Tiger data for rgeo
* tools/strtools - C library to compile and install for tools



## Installation

A lot of my scripts rely on executables the I have written in ``strtools`` and ``shptools``. Build and install these. They will install into your ``~/bin, ~/lib, ~/include`` or you can change the ``Makefile`` to put them elsewhere.

**NOTE** make sure ``~/bin`` is included in your path.

### On Ubuntu 18.04, you will need to install the following packages:

```
sudo apt install build-essential libshp-dev libsqlite3-dev libpcre3-dev libgeos-dev libdbd-xbase-perl libicu-dev

# we mapserver-bin to get tile4ms if you are building maps
# you can ignore libapache2-mod-mapcache mapcache-tools if you do not
# plan to setup and test mapcache

sudo apt install mapserver-bin

sudo apt install apache2 libapache2-mod-php php php-cli php-pgsql libapache2-mod-mapcache mapcache-tools
sudo a2enmod cgi
sudo systemctl restart apache2

# optionally sqlite3 if you want to inspect any of the sqlite databases
sudo apt install sqlite3

# install postgresql and postgis and gdal/ogr
sudo apt install postgresql-all postgresql-contrib postgis gdal-bin

# build and install the tools that are needed
# sorry they throw a bunch of warnings which can be ignored
cd /path/to/imaptools.com/tools
./install.sh

```

### On CentOS, these packages might work:

I work and develop on Ubuntu, so ...

```
# basic dev tool environment
yum groupinstall 'Development Tools'
yum install shapelib-devel sqlite-devel pcre-devel geos-devel libicu-devel perl-DBD-XBase gdal

# we need mapserver and related programs (tile4ms)
# TODO

# install apache
yum install httpd

# postgresql database and postgis
# https://wiki.postgresql.org/wiki/YUM_Installation
# https://trac.osgeo.org/postgis/wiki/UsersWikiPostGIS21CentOS6pgdg
yum list postgresql*
yum install postgresql??-server
yum list postgis
yum install ???

# need Other stuff?

```






## User Guide

This is just a catch all for how you might use some of these tools. Fork and pull requests would be welcome to improve on this or any of the tools. I've tried to pull together 20+ years of stuff I've used in my consulting business and it is not well organized, but hopefully this is enough to get you started.



## Working With Tiger data

### Download the data

First step, you need to fetch the data. I store rawdata in ``/u/srcdata/NAME/`` where ``NAME`` identifies what it is. And processed data usually goes in ``/u/data/NAME/`` and it then loaded into the database if needed.

```
sudo mkdir /u
sudo chown USER.USER /u
mkdir /u/srcdata /u/data

cd /path/to/imaptools.com

# fetch all the Census Tiger data for 2019 - PATIENCE takes hours
# this took 28 hours to run on my comcast connection
tiger-maps/wget-tiger 2019
```
This leaves a log file which may contain errors in the download. It is safe to run the script multiple times as it will only download missing files.

### Prepare Tiger for Rgeo

* download Tiger data as above
* process the downloaded data for geo/rgeo
* install the rgeo schema and functions
* load the processed data into postgres database
* compute the instersections table
* test it

```
# process the downloaded data for geo/rgeo

cd /path/to/imaptools.com/tools/geo-rgeo

# edit ./tgr2rgeo-sqlite.c and change #define YEAR to be "2019" 

vi ./tgr2rgeo-sqlite.c
make && make install && make clean

# process the data (~7hr)
# path for usps-actual.db needs to be absolute not relative

./process-tgr-2011 /u/srcdata/TIGER2019 /u/data/tiger2019-rgeo /path/to/imaptools.com/tools/geo-rgeo/usps-actual.db &> /u/data/tiger2019-rgeo.log

# drop and recreate the databasee and
# load the processed data into postgres database (~1 hr)

rgeo2pg tiger2019_rgeo /u/data/tiger2019-rgeo/

# install the RGeo functions

psql -U postgres -h localhost tiger2019_rgeo -f /path/to/imaptools.com/sql-scripts/rgeo/imt-rgeo-pgis-2.0.sql


# compute the instersections table and run some trivial tests
# which output some "NOTICE: NNN out of MMM edges processed"
# it is normal for NNN to exceed MMM, this is just an indicator its working
# (~7.5 hr)

psql -U postgres -h localhost tiger2019_rgeo -f /path/to/imaptools.com/sql-scripts/rgeo/imt-rgeo-pgis-2.0-part2.sql

# Dump the database so youcan move it to another system


```

### Prepare Tiger for Mapping

* download Tiger data as above or skip if done above
* process the downloaded data for mapping

```
cd /path/to/imaptools.com/tiger-maps
cat README.md

./prep-tiger-maps 2019
./prep-tiger-maps-2 2019
cp tiger2019-tmp.map /u/data/tiger2019-maps/.
./copy-maps 2019 /u/data/tiger2019-maps/
(cd data ; cp map-*.inc /u/data/tiger2019-maps/ )
rsync -a data/etc /u/data/tiger2019-maps/.
rsync -a data/not-nt2 /u/data/tiger2019-maps/.
cd /u/data/tiger2019-maps
/path/to/impatools.com/sql-scripts/tiger-maps/process-state 2019
/path/to/impatools.com/sql-scripts/tiger-maps/process-pointlm POINTLM/
find . -name \*.shp -exec shptree {} \;

```
You should now be able to access a WMS service using mapserver at:
http://localhost/cgi-bin/mapserv?map=/u/data/tiger2019-maps/tiger2019-mc.map


* add your WMS service to OpenLayers or mapcache

### Prepare Tiger for Geocoder

* download Tiger data as above or skip if done
* process the downloaded data for geo/rgeo or skip if done above
* load the geocoder schema and function
* standardize load data
* good to go to access the geocoder via SQL queries



## Working with StatCan Data for Canada

**I Recommend that you read ALL the scripts and make adjustments to them as needed. Data and formats change from year to year and these scripts are what I actually executed in a prior year and may no longer be valid. Use them as a recipe for how you MIGHT be able to do it for the current year.**

### Download the Data

There is also [geobase](https://open.canada.ca/data/en/dataset?keywords=GeoBase) and [CanVec](https://open.canada.ca/data/en/dataset?q=canvec&sort=&collection=fgp) data for Canada which includes StatCan and other Canada specific GIS resources. These resource can augment the road network and boundary files when making maps.

* [StatCan Geography](https://www12.statcan.gc.ca/census-recensement/2016/geo/index-eng.cfm) which links to documentation and data.
* download [2019 NRN street data](http://www12.statcan.gc.ca/census-recensement/2011/geo/RNF-FRR/files-fichiers/lrnf000r19a_e.zip)
* download [2019 Boundary Files](http://www12.statcan.gc.ca/census-recensement/2011/geo/bound-limit/files-fichiers/lcsd000a19a_e.zip)

### Load Data into PostgreSQL

If you download the individual NRN files for each province, then you can look at and probably use [load-statcan-data.sh](https://github.com/woodbri/imaptools.com/blob/master/sql-scripts/statcan/load-statcan-data.sh).

The new data, 2019, comes in a single file like:
```
woodbri@mappy:~/work/imaptools.com/sql-scripts$ unzip -l /u/srcdata/statcan-2019/lrnf000r19a_e.zip
Archive:  /u/srcdata/statcan-2019/lrnf000r19a_e.zip
  Length      Date    Time    Name
---------  ---------- -----   ----
   225541  2019-10-24 07:16   92-500-g2019001-eng.pdf
1188031154  2019-10-01 11:39   lrnf000r19a_e.dbf
      503  2019-10-01 11:03   lrnf000r19a_e.prj
416084580  2019-10-01 11:39   lrnf000r19a_e.shp
 17865220  2019-10-01 11:39   lrnf000r19a_e.shx
    32809  2019-10-23 15:34   lrnf000r19a_e.xml
---------                     -------
1622239807                     6 files


# And has a table structure like:

woodbri@mappy:/u/srcdata/statcan-2019$ dbfdump --info data/lrnf000r19a_e.dbf
Filename:       data/lrnf000r19a_e.dbf
Version:        0x03 (ver. 3)
Num of records: 2233140
Header length:  673
Record length:  532
Last change:    2019/10/1
Num fields:     20
Field info:
Num     Name            Type    Len     Decimal
1.      NGD_UID         C       10      0
2.      NAME            C       50      0
3.      TYPE            C       6       0
4.      DIR             C       2       0
5.      AFL_VAL         C       9       0
6.      ATL_VAL         C       9       0
7.      AFR_VAL         C       9       0
8.      ATR_VAL         C       9       0
9.      CSDUID_L        C       7       0
10.     CSDNAME_L       C       100     0
11.     CSDTYPE_L       C       3       0
12.     CSDUID_R        C       7       0
13.     CSDNAME_R       C       100     0
14.     CSDTYPE_R       C       3       0
15.     PRUID_L         C       2       0
16.     PRNAME_L        C       100     0
17.     PRUID_R         C       2       0
18.     PRNAME_R        C       100     0
19.     RANK            C       1       0
20.     CLASS           C       2       0

woodbri@mappy:/u/srcdata/statcan-2019$ unzip -j -d data lcsd000a19a_e.zip

# So we can load this like:

# create and initialize database statcan2019

createdb -U postgres -h localhost statcan2019
psql -U postgres -h localhost statcan2019 <<EOF
create extension postgis;
create schema rawdata;
create schema data;
alter database statcan2019 set search_path to data, rawdata, public;
EOF

ogr2ogr -f PostgreSQL PG:"dbname=statcan2019 user=postgres host=localhost" \
    -lco LAUNDER=YES \
    -lco PRECISION=NO \
    -lco DIM=2 \
    -lco GEOMETRY_NAME=geom \
    -lco FID=gid \
    -nln rawdata.roadseg \
    -t_srs EPSG:4326 \
    data/lrnf000r19a_e.shp

ogr2ogr -f PostgreSQL PG:"dbname=statcan2019 user=postgres host=localhost" \
    -lco LAUNDER=YES \
    -lco PRECISION=NO \
    -lco DIM=2 \
    -lco GEOMETRY_NAME=geom \
    -lco FID=gid \
    -nln rawdata.places \
    -t_srs EPSG:4326 \
    -nlt PROMOTE_TO_MULTI \
    data/lcsd000a19a_e.shp

cd /path/to/imaptools.com/sql-scripts

# load the reverse geocoder functions and types
psql -U postgres -h localhost statcan2019 -f rgeo/imt-rgeo-pgis-2.0-streets.sql

# prep the statcan data 
psql -U postgres -h localhost statcan2019 -f rgeo/statcan-rgeo-prep-2019.sql
psql -U postgres -h localhost statcan2019 -f rgeo/fix-canada-rgeo-with-hn-formats.sql


# finish the data prep with
psql -U postgres -h localhost statcan2019 -f rgeo/imt-rgeo-pgis-2.0-part2.sql

# run some test queries

psql -U postgres -h localhost -a -A statcan2019 <<EOF
-- 2nd AVE|Saskatoon|||Saskatchewan|CA||1.22208694|6076200
select * from imt_reverse_geocode_flds(-106.663771,52.128759);

-- 22nd ST, 2nd AVE|Saskatoon|||Saskatchewan|CA||2.9|35539
select * from imt_rgeo_intersections_flds(-106.663771,52.128759);

-- 1962 Lockhead RD|Ottawa|||Ontario|CA||263.91757826|2120803
select * from imt_reverse_geocode_flds(-75.663771,45.128759);

-- Third Line RD, Lockhead RD|Ottawa|||Ontario|CA||889.9|1273516
select * from imt_rgeo_intersections_flds(-75.663771,45.128759);

-- des Laurentides AUT|Montréal|||Quebec / Québec|CA||27.55289667|5492621
select * from imt_reverse_geocode_flds(-73.663771,45.53);

-- Deslauriers RUE, Wright RUE|Montréal|||Quebec / Québec|CA||46.1|1343364
select * from imt_rgeo_intersections_flds(-73.663771,45.53);
EOF

``


## Working with Navteq/HERE data

*To BE Done*

### Prepare Navteq/HERE for Rgeo


### Prepare Navteq/HERE for Mapping


### Prepare Navteq/HERE for Geocoder


