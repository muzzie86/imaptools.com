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

On Ubuntu 18.04, you will need to install the following packages:

```
sudo apt install build-essential libshp-dev libsqlite3-dev libpcre3-dev libgeos-dev libdbd-xbase-perl

# optionally sqlite3 if you want to inspect any of the sqlite databases
sudo apt install sqlite3

# install postgresql and postgis and gdal/ogr
sudo apt install postgresql-all postgresql-contrib postgis gdal-bin

# build and install the tools that are needed
# sorry they throw a bunch of warnings which can be ignored
cd /path/to/imaptools.com/tools
./install.sh

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

### Prepare Tiger for Mapping

* download Tiger data as above or skip if done above
* process the downloaded data for mapping
* add your WMS service to OpenLayers or mapcache

### Prepare Tiger for Geocoder

* download Tiger data as above or skip if done
* process the downloaded data for geo/rgeo or skip if done above
* load the geocoder schema and function
* standardize load data
* good to go to access the geocoder via SQL queries



## Working with StatCan Data for Canada

### Download the Data

There is also [geobase](https://open.canada.ca/data/en/dataset?keywords=GeoBase) and [CavVec](https://open.canada.ca/data/en/dataset?q=canvec&sort=&collection=fgp) data for Canada which includes StatCan and other Canada specific GIS resources. These resource can augment the road network and boundary files when making maps.

* [StatCan Geography](https://www12.statcan.gc.ca/census-recensement/2016/geo/index-eng.cfm) which links to documentation and data.
* download [2019 NRN street data](http://www12.statcan.gc.ca/census-recensement/2011/geo/RNF-FRR/files-fichiers/lrnf000r19a_e.zip)
* download [2019 Boundary Files](http://www12.statcan.gc.ca/census-recensement/2011/geo/bound-limit/files-fichiers/lcsd000a19a_e.zip)

### Load Data into PostgreSQL

If you download the individual NRN files for each province, then you can look at and probably use [load-statcan-data.sh](https://github.com/woodbri/imaptools.com/blob/master/sql-scripts/statcan/load-statcan-data.sh). The new data, 2019, comes in a single file like:
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
    -nln rawdata.streets \
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


``


## Working with Navteq/HERE data

*To BE Done*

### Prepare Navteq/HERE for Rgeo


### Prepare Navteq/HERE for Mapping


### Prepare Navteq/HERE for Geocoder


