#!/bin/bash

DBCONN="-U postgres -p 5433 -h 192.168.99.1"

DBNAME=statcan

if false ; then
dropdb $DBCONN $DBNAME
createdb $DBCONN $DBNAME
psql $DBCONN $DBNAME <<EOF

create extension postgis;
create extension address_standardizer2;
create schema data;
create schema rawdata;
alter database $DBNAME set search_path to data, rawdata, public;

EOF

mkdir -p shape
rm -f shape/*

for x in NRN*.zip ; do
  unzip -d shape -j $x *ADDRANGE.* *ALTNAMLINK.* *STRPLANAME.* *ROADSEG.*
done

fi

for x in shape/*ADDRANGE.dbf shape/*ALTNAMLINK.dbf shape/*STRPLANAME.dbf ; do
  TABLE=`basename $x .dbf`
  echo "shp2pgsql -d -D -n $x rawdata.$TABLE"
#  shp2pgsql -d -D -n $x rawdata.$TABLE | psql $DBCONN $DBNAME
  sleep 20
  echo "psql $DBCONN -d $DBNAME -c \"create index ${TABLE}_nid_idx on $TABLE using btree(nid)\" "
  psql $DBCONN -d $DBNAME -c "create index ${TABLE}_nid_idx on $TABLE using btree(nid)"
done

for x in shape/*ROADSEG.shp ; do
  TABLE=`basename $x .shp`
  echo "shp2pgsql -s 4617 -d -g geom -D -t 2D -N skip $x rawdata.$TABLE"
#  shp2pgsql -s 4617 -d -g geom -D -t 2D -N skip $x rawdata.$TABLE | psql $DBCONN $DBNAME
  # have to sleep here because pg11 crashes if you don't give it time to 
  # recover and do housekeeping after loading a shapefile
  sleep 60
  echo "psql $DBCONN -d $DBNAME -c \"create index ${TABLE}_nid_idx on $TABLE using btree(nid)\" "
  psql $DBCONN -d $DBNAME -c "create index ${TABLE}_nid_idx on $TABLE using btree(nid)"
done

