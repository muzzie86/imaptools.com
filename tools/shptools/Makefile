#
#  $Id: Makefile,v 1.20 2013-11-01 15:09:52 woodbri Exp $
#

DATE=`date +%Y%m%d`

CC = gcc

DLIBSHP=-DDEBIAN_LIBSHP

CFLAGS=-O2  $(DLIBSHP)
debug : CFLAGS=-g -DUNIX -DNULLTOPOINT  $(DLIBSHP)
#debug : CFLAGS=-g -DUNIX -DDMALLOC -DDMALLOC_FUNC_CHECK -DNULLTOPOINT  $(DLIBSHP)

INCLUDE=-I. -I/usr/local/include -I/usr/include/mysql

LIBS=-lshp -lm
debug : LIBS=-lshp -lm

LIBDIRS=-L. -L/usr/lib -L/usr/local/lib -L/usr/lib/mysql

PROJS_BROKEN=prj2prj prj4326to900913

PROGS=shp2sdo shpsplit shpgrep shpcpy shpcpy2 shpcpy2D shpcpy3 shp2tile shpmove dbf_add_col dbf_drop_col dbfedit txt2shp  dbfarea dbflength fixpoly shpegrep dbfcase dbfrename shpgtlt

all:   debug
debug: $(PROGS)
opt:   $(PROGS)

.c.o:   Makefile
	$(CC) $(CFLAGS) -c $(INCLUDE) $<

dbfcase:  dbfcase.o
	$(CC) $(CFLAGS) -o dbfcase dbfcase.o \
              $(INCLUDE) $(LIBDIRS) $(LIBS) 

dbfrename:  dbfrename.o
	$(CC) $(CFLAGS) -o dbfrename dbfrename.o \
              $(INCLUDE) $(LIBDIRS) $(LIBS) 

dbfarea:  dbfarea.o
	$(CC) $(CFLAGS) -o dbfarea dbfarea.o \
              $(INCLUDE) $(LIBDIRS) $(LIBS) 

prj4326to900913:  prj4326to900913.o
	$(CC) $(CFLAGS) -o prj4326to900913 prj4326to900913.o \
              $(INCLUDE) $(LIBDIRS) $(LIBS) -lproj

dbflength:  dbflength.o
	$(CC) $(CFLAGS) -o dbflength dbflength.o \
              $(INCLUDE) $(LIBDIRS) $(LIBS) 

txt2shp:  txt2shp.o
	$(CC) $(CFLAGS) -o txt2shp txt2shp.o \
              $(INCLUDE) $(LIBDIRS) $(LIBS) 

prj2prj: prj2prj.o
	$(CC) $(CFLAGS) -o prj2prj prj2prj.o \
              $(INCLUDE) $(LIBDIRS) $(LIBS) -lproj

dbfedit:  dbfedit.o
	$(CC) $(CFLAGS) -o dbfedit dbfedit.o \
              $(INCLUDE) $(LIBDIRS) $(LIBS) 

shpmove:  shpmove.o
	$(CC) $(CFLAGS) -o shpmove shpmove.o \
              $(INCLUDE) $(LIBDIRS) $(LIBS) 

shpsplit:  shpsplit.o
	$(CC) $(CFLAGS) -o shpsplit shpsplit.o \
              $(INCLUDE) $(LIBDIRS) $(LIBS) 

shpgrep:  shpgrep.o
	$(CC) $(CFLAGS) -o shpgrep shpgrep.o \
              $(INCLUDE) $(LIBDIRS) $(LIBS) 

shpgtlt:  shpgtlt.o
	$(CC) $(CFLAGS) -o shpgtlt shpgtlt.o \
              $(INCLUDE) $(LIBDIRS) $(LIBS) 

shpegrep:  shpegrep.o
	$(CC) $(CFLAGS) -o shpegrep shpegrep.o \
              $(INCLUDE) $(LIBDIRS) -lpcre $(LIBS) 

shp2sdo:  shp2sdo.o
	$(CC) $(CFLAGS) -o shp2sdo shp2sdo.o \
              $(INCLUDE) $(LIBDIRS) $(LIBS) 

shpcpy:  shpcpy.o
	$(CC) $(CFLAGS) -o shpcpy shpcpy.o \
              $(INCLUDE) $(LIBDIRS) $(LIBS) 

shpcpy2:  shpcpy2.o
	$(CC) $(CFLAGS) -o shpcpy2 shpcpy2.o \
              $(INCLUDE) $(LIBDIRS) $(LIBS) 

shpcpy2D:  shpcpy2D.o
	$(CC) $(CFLAGS) -o shpcpy2D shpcpy2D.o \
              $(INCLUDE) $(LIBDIRS) $(LIBS) 

shpcpy3:  shpcpy3.o
	$(CC) $(CFLAGS) -o shpcpy3 shpcpy3.o \
              $(INCLUDE) $(LIBDIRS) $(LIBS) 

dbf_add_col:    dbf_add_col.o
	$(CC) $(CFLAGS) -o dbf_add_col dbf_add_col.o \
	      $(INCLUDE) $(LIBDIRS) $(LIBS)

dbf_drop_col:    dbf_drop_col.o
	$(CC) $(CFLAGS) -o dbf_drop_col dbf_drop_col.o \
	      $(INCLUDE) $(LIBDIRS) $(LIBS)

shp2tile: shp2tile.o
	$(CC) $(CFLAGS) -o shp2tile shp2tile.o \
              $(INCLUDE) $(LIBDIRS) $(LIBS)

my2shp:  my2shp.o
	$(CC) $(CFLAGS) -o my2shp my2shp.o \
              $(INCLUDE) $(LIBDIRS) -lmysqlclient $(LIBS) 

fixpoly:  fixpoly.o
	$(CC) $(CFLAGS) -o fixpoly fixpoly.o \
			  $(INCLUDE) $(LIBDIRS) -lgeos_c $(LIBS)

install:
	cp $(PROGS) $(HOME)/bin

clean:
	rm -f *.o $(PROGS) core test.{dbf,shp,shx}
	
tar:    inventory.txt Makefile *.c
	mkdir -p imt-shptools-$(DATE)
	cat inventory.txt | xargs --replace={} cp -fp {} imt-shptools-$(DATE)/.
	cp -fp *.[ch] Makefile imt-shptools-$(DATE)
	tar  czf imt-shptools-$(DATE).tgz imt-shptools-$(DATE)
	rm -rf imt-shptools-$(DATE)

