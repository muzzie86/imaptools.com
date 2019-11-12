#
#  $Id: Makefile,v 1.1 2001/12/27 16:11:18 woodbri Exp woodbri $
#
#  $Log: Makefile,v $
#  Revision 1.1  2001/12/27 16:11:18  woodbri
#  Initial revision
#
#  Revision 1.1  2001/12/04 21:12:18  woodbri
#  Initial revision
#
#

CC = gcc

CFLAGS=-O2 -DUNIX
debug : CFLAGS=-g -Wall

INCLUDE=-I.

LIBS=-lshp -lm
debug : LIBS=-lshp -lm

LIBDIRS=-L. -L/usr/lib -L/usr/local/lib

PROGS=shp2tile

all:   debug
debug: $(PROGS)
opt:   $(PROGS)

.c.o:   Makefile
	$(CC) $(CFLAGS) -c $(INCLUDE) $<

shp2tile: shp2tile.o
	$(CC) $(CFLAGS) -o shp2tile shp2tile.o \
              $(INCLUDE) $(LIBDIRS) $(LIBS)

install:
	cp $(PROGS) $(HOME)/bin

clean:
	rm -f *.o $(PROGS) core
