#!/bin/sh

/c/Program\ Files/PostgreSQL/11/bin/pg_dump -U postgres -h localhost -p 5433 -f statcan-rawdata.sql -Fp -n data -n rawdata -O -x -d statcan 


