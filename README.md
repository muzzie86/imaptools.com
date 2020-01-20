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
sudo apt install build-essential libshp-dev libsqlite3-dev libpcre3-dev libgeos-dev
# optionally sqlite3 if you want to inspect any of the sqlite databases
sudo apt install sqlite3

# build and install the tools that are needed
# sorry they throw a bunch of warnings which can be ignored

(cd ~ ; mkdir bin include lib)
cd tools/strtools
make
make install
cd ../shptools
make
make install
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


### Prepare Tiger for Mapping


### Prepare Tiger for Geocoder


## Working with Navteq/HERE data

### Prepare Navteq/HERE for Rgeo


### Prepare Navteq/HERE for Mapping


### Prepare Navteq/HERE for Geocoder


