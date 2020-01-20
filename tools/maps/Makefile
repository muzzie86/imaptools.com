#
#  $Id: Makefile,v 1.6 2012-02-25 20:57:04 woodbri Exp $
#
#  $Log: Makefile,v $
#  Revision 1.6  2012-02-25 20:57:04  woodbri
#  Turn on UTF8 libraries.
#
#  Revision 1.5  2012-02-25 20:47:22  woodbri
#  *** empty log message ***
#
#  Revision 1.4  2011/01/30 17:58:18  woodbri
#  Added support to carry speed and house numbers into map data.
#  Added some scripts to help automate things.
#
#  Revision 1.3  2011/01/29 16:26:52  woodbri
#  Adding files to support PAGC data generation for Tiger and Navteq.
#  Adding options to domaps to support carring, speed, and address ranges.
#
#  Revision 1.2  2010/07/08 18:58:53  woodbri
#  Adding some minor changes to the Makefiles and scripts.
#
#  Revision 1.1  2009/10/18 00:53:12  woodbri
#  Adding domaps to cvs.
#
#

CC = gcc

DMALLOC_CFLAGS=
DMALLOC_LIBS=
ifdef DMALLOC
DMALLOC_CFLAGS=-DDMALLOC -DDMALLOC_FUNC_CHECK
DMALLOC_LIBS=-ldmalloc
endif
ifdef CYGWIN
DMALLOC_CFLAGS=
DMALLOC_LIBS=
endif

UNICODE=1
ifdef UNICODE
UNICODE_CFLAGS=-DUNICODE
UNICODE_LIBS=-licui18n -licuuc
else
UNICODE_CFLAGS=
UNICODE_LIBS=
endif

DATE=`date +%Y%m%d`

CFLAGS+=-g -DUNIX $(UNICODE_CFLAGS)
debug : CFLAGS+=-g -DUNIX $(UNICODE_CFLAGS) $(DMALLOC_CFLAGS)

INCLUDE=-I. -I.. -I /usr/include/libshp -I$(HOME)/include -I/usr/local/include

LIBS=-lshp $(UNICODE_LIBS) -lm
debug : LIBS=-lshp $(DMALLOC_LIBS) -lm

LIBDIRS=-L. -L~/lib -L/usr/lib -L/usr/lib/x86_64-linux-gnu/ -L/usr/local/lib

EXE=doNtPolygons doNtMisc doNtHwys doNtStreets doTgrRoads
OBJS=doNtPolygons.o doNtMisc.o doNtUtils.o doNtHwys.o doNtStreets.o doTgrRoads.o

all:   $(EXE)
opt:   $(EXE)

help:
	@echo 'Usage: make clean|tar|all|opt|debug'

.c.o:   Makefile
	$(CC) $(CFLAGS) -c $(INCLUDE) $<

doNtPolygons:    doNtPolygons.o doNtUtils.o ~/lib/strtools.a
	$(CC) $(CFLAGS) -o doNtPolygons doNtPolygons.o doNtUtils.o ~/lib/strtools.a $(INCLUDE) $(LIBDIRS) $(LIBS) 

doNtMisc:    doNtMisc.o doNtUtils.o ~/lib/strtools.a
	$(CC) $(CFLAGS) -o doNtMisc doNtMisc.o doNtUtils.o ~/lib/strtools.a $(INCLUDE) $(LIBDIRS) $(LIBS) 

doNtHwys: doNtHwys.o doNtUtils.o doNtUtils.h
	$(CC) $(CFLAGS) -o doNtHwys doNtHwys.o doNtUtils.o ~/lib/strtools.a $(INCLUDE) $(LIBDIRS) $(LIBS)

doNtStreets: doNtStreets.o doNtUtils.o doNtUtils.h
	$(CC) $(CFLAGS) -o doNtStreets doNtStreets.o doNtUtils.o ~/lib/strtools.a $(INCLUDE) $(LIBDIRS) $(LIBS)

doNtUtils.o:	doNtUtils.c doNtUtils.h
	$(CC) $(CFLAGS) -c $(INCLUDE) doNtUtils.c

doTgrRoads:     doTgrRoads.o
	$(CC) $(CFLAGS) -o doTgrRoads doTgrRoads.o ~/lib/strtools.a $(INCLUDE) $(LIBDIRS) -lpcre $(LIBS)

tar:	inventory.txt Makefile *.c *.h
	mkdir -p imt-navteq-domaps-$(DATE)
	cat inventory.txt | xargs --replace={} cp -fp {} imt-navteq-domaps-$(DATE)/.
	cp -fp *.[ch] Makefile imt-navteq-domaps-$(DATE)
	tar  czf imt-navteq-domaps-$(DATE).tgz imt-navteq-domaps-$(DATE)
	rm -rf imt-navteq-domaps-$(DATE)

clean:
	rm -f *.o $(EXE)
	
install:
	cp $(EXE) install-poi mk-nt-maps-tindex mk-poi-mapfile process-nt-maps ~/bin
