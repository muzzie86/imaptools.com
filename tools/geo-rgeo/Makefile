#
#  $Id: Makefile,v 1.21 2012-12-28 19:02:02 woodbri Exp $
#
#  $Log: Makefile,v $
#  Revision 1.21  2012-12-28 19:02:02  woodbri
#  Changes dealing with how to evaluate place names when we have multiple
#  Census names that could be used.
#
#  Revision 1.20  2012-06-03 14:07:37  woodbri
#  Adding new utility program build-tiger-places to build a sqlite3 db of
#  all the Tiger faces, cousub, concity, and place records to be used with
#  tgr2pagc-sqlite to deal with county adjacency issues.
#
#  Revision 1.19  2011/02/09 04:49:37  woodbri
#  Added ALT_NAMES to rgeo and updated Makefile help target to reflect
#  this.
#
#  Revision 1.18  2011/02/08 20:25:43  woodbri
#  iAdded new script to make install
#
#  Revision 1.17  2011/01/29 16:26:52  woodbri
#  Adding files to support PAGC data generation for Tiger and Navteq.
#  Adding options to domaps to support carring, speed, and address ranges.
#
#  Revision 1.16  2010/10/18 02:51:13  woodbri
#  Changes to nt2rgeo-sqlite.c to make Zones optional.
#  Changes to tgr2rgeo-sqlite.c to optionally include mtcfcc in the data.
#  Changes to doNtPolygons.c to include DISP_CLASS to water polygons.
#  Changes to Makefile to support the above.
#
#  Revision 1.14  2009/01/09 05:17:31  woodbri
#  Corrected copy/paster error.
#
#  Revision 1.13  2009/01/09 05:14:04  woodbri
#  Added timezone.shp file and removed usps-actual.db from nt which does not need it.
#
#  Revision 1.12  2009/01/09 04:33:28  woodbri
#  Modified Makefile to support targets tar-nt, tar-tgr, tar-all.
#  Added additional Makefile.* specific to tgr or nt releases.
#
#  Revision 1.11  2008/10/09 20:19:28  woodbri
#  Added install target and removed -ldb from the link targets.
#
#  Revision 1.10  2008/09/30 20:30:15  woodbri
#  Fixed a typo in the help target.
#
#  Revision 1.9  2008/09/30 20:14:30  woodbri
#  Added an option to include the FR_SPD_LIM and TO_SPD_LIM from the Streets.deb
#  to the output files.
#
#  Revision 1.8  2008/09/30 18:30:20  woodbri
#  Adding geo-sqlite.c and tgr2rgeo-sqlite.c, updates to Makefile for same.
#  Filter out tileindexes when loading data to pg.
#
#  Revision 1.7  2008/05/15 14:08:24  woodbri
#  Adding load-nt2pg
#
#  Revision 1.6  2008/05/13 18:40:06  woodbri
#  Added optional parameter nt2rgeo-sqlite to allow you to split the output file
#  because the dbf files were getting over 2GB. Added dbname as required argument
#  to rgeo2pg.
#
#  Revision 1.5  2008/04/11 13:51:30  woodbri
#  Updates the make tar target to work.
#
#  Revision 1.4  2008/04/10 15:23:42  woodbri
#  Cleanup files and Makefile.
#
#  Revision 1.3  2008/04/10 14:56:54  woodbri
#  Adding new files based on sqlite as underlying data store.
#
#  Revision 1.2  2008/03/25 18:43:09  woodbri
#  Updated Makefile to add libshp to the include search path
#  Updated nt2rgeo.c to be for libdb4.2 API
#
#  Revision 1.1  2008/02/04 03:38:59  woodbri
#  Updating cvs local changes.
#
#  Revision 2.6  2005/08/20 13:11:26  woodbri
#  Added code to calcuate the area of water polygons and add the value to
#  the dbf attribute file.
#
#  Revision 2.5  2005/02/25 01:06:25  woodbri
#  Changes required to support freeBSD.
#  Removed "libshp/" fromt the #include <shapelib.h>
#  Added -I/usr/local/include in the Makefiles
#
#  Revision 2.4  2005/01/15 13:16:45  woodbri
#  Add header comments.
#  Updated make file so it can be run with dmalloc via:
#  make DMALLOC=1
#
#
#  Revision 1.1  2001/12/04 21:12:18  woodbri
#  Initial revision
#

CC = gcc

DATE=`date +%Y%m%d`

CFLAGS+=-g -DUNIX
gebug : CFLAGS+=-g -DUNIX 

INCLUDE=-I. -I.. -I /usr/include/libshp -I/home/woodbri/include -I/usr/local/include

LIBS=-lpcre -lshp -lm
debug : LIBS=-lpcre -lshp -lm

LIBDIRS=-L. -L/home/woodbri/lib -L/usr/lib -L/usr/local/lib

OBJS=nt2rgeo-sqlite.o dbf2sqlite.o tgr2rgeo-sqlite.o rgeo2geo-sqlite.o geo-sqlite.o build-tiger-places.o
EXES=dbf2sqlite nt2rgeo-sqlite nt2pagc-sqlite tgr2rgeo-sqlite tgr2pagc-sqlite rgeo2geo-sqlite build-tiger-places # geo-sqlite
SCRIPTS=load-nt2pg process-nt process-tgr process-tgr-mtfcc do-nt2rgeo rgeo2pg load-nt-hwys load-nt-pois process-tgr2pagc

all:   $(EXES)
opt:   $(EXES)

help:
	@echo "Usage: [CFLAGS='-DSPEED_LIMIT -DALTNAMES'] [DMALLOC=1] make clean|tar|all|opt|debug"

.c.o:   Makefile dbf2sqlite.h
	$(CC) $(CFLAGS) -c $(INCLUDE) $<

nt2rgeo-sqlite:    nt2rgeo-sqlite.o dbf2sqlite.o
	$(CC) $(CFLAGS) -o nt2rgeo-sqlite nt2rgeo-sqlite.o dbf2sqlite.o $(INCLUDE) $(LIBDIRS) -lsqlite3 $(LIBS) 

nt2pagc-sqlite:    nt2pagc-sqlite.o dbf2sqlite.o
	$(CC) $(CFLAGS) -o nt2pagc-sqlite nt2pagc-sqlite.o dbf2sqlite.o $(INCLUDE) $(LIBDIRS) -lsqlite3 $(LIBS) 

tgr2rgeo-sqlite:    tgr2rgeo-sqlite.o dbf2sqlite.o
	$(CC) $(CFLAGS) -o tgr2rgeo-sqlite tgr2rgeo-sqlite.o dbf2sqlite.o $(INCLUDE) $(LIBDIRS) -lsqlite3 $(LIBS) 

tgr2pagc-sqlite:    tgr2pagc-sqlite.o dbf2sqlite.o
	$(CC) $(CFLAGS) -o tgr2pagc-sqlite tgr2pagc-sqlite.o dbf2sqlite.o $(INCLUDE) $(LIBDIRS) -lsqlite3 $(LIBS) 

build-tiger-places:    build-tiger-places.o dbf2sqlite.o
	$(CC) $(CFLAGS) -o build-tiger-places build-tiger-places.o dbf2sqlite.o $(INCLUDE) $(LIBDIRS) -lsqlite3 $(LIBS) 

rgeo2geo-sqlite:    rgeo2geo-sqlite.o
	$(CC) $(CFLAGS) -o rgeo2geo-sqlite rgeo2geo-sqlite.o $(INCLUDE) $(LIBDIRS) -lsqlite3 $(LIBS) 

geo-sqlite:    geo-sqlite.o
	$(CC) $(CFLAGS) -o geo-sqlite geo-sqlite.o $(INCLUDE) -Wl,-rpath,/home/woodbri/lib $(LIBDIRS) -lsqlite3 $(LIBS) 

dbf2sqlite:    dbf2sqlite-main.o dbf2sqlite.o
	$(CC) $(CFLAGS) -o dbf2sqlite dbf2sqlite-main.o dbf2sqlite.o $(INCLUDE) $(LIBDIRS) -lsqlite3 $(LIBS) 

tar:
	@echo :Use one of the following targets: tar-nt, tar-tgr, tar-all

tar-all:
	mkdir -p imt-navteq-$(DATE)/data
	cat inventory.tgr | xargs --replace={} cp -fp {} imt-navteq-$(DATE)/.
	cat inventory.nt  | xargs --replace={} cp -fp {} imt-navteq-$(DATE)/.
	cp -fp *.[ch] Makefile imt-navteq-$(DATE)
	cp -fp usps-actual.db imt-navteq-$(DATE)
	cp -fp data/*.sql imt-navteq-$(DATE)/data
	cp -fp data/*.txt imt-navteq-$(DATE)/data
	cp -fp data/timeznp020.* imt-navteq-$(DATE)/data
	tar  czf imt-navteq-$(DATE).tgz imt-navteq-$(DATE)
	rm -rf imt-navteq-$(DATE)

tar-nt:
	mkdir -p nt2rgeo-$(DATE)/data
	cat inventory.nt  | xargs --replace={} cp -p {} nt2rgeo-$(DATE)/.
	(cd nt2rgeo-$(DATE) ; ln -s Makefile.nt Makefile)
	cp -p usps-actual.db nt2rgeo-$(DATE)
	cp -fp data/timeznp020.* nt2rgeo-$(DATE)/data
	tar  czf nt2rgeo-$(DATE).tgz nt2rgeo-$(DATE)
	rm -rf nt2rgeo-$(DATE)

tar-tgr:
	mkdir -p tgr2rgeo-$(DATE)/data
	cat inventory.tgr | xargs --replace={} cp -p {} tgr2rgeo-$(DATE)/.
	(cd tgr2rgeo-$(DATE) ; ln -s Makefile.tgr Makefile)
	cp -p data/*.sql tgr2rgeo-$(DATE)/data
	cp -p data/*.txt tgr2rgeo-$(DATE)/data
	cp -fp data/timeznp020.* tgr2rgeo-$(DATE)/data
	tar  czf tgr2rgeo-$(DATE).tgz tgr2rgeo-$(DATE)
	rm -rf tgr2rgeo-$(DATE)

clean:
	rm -f *.o $(EXES) core nt2rgeo-sqlite.db tgr2rgeo-sqlite.db 
	
install:
	cp -fp $(EXES) $(SCRIPTS) ~/bin
