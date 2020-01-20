#
#  $Id: Makefile,v 2.9 2009/10/18 01:03:51 woodbri Exp $
#
#  $Log: Makefile,v $
#  Revision 2.9  2009/10/18 01:03:51  woodbri
#  Added target "ter" to make a source tarball.
#
#  Revision 2.8  2009/02/24 18:20:49  woodbri
#  Added install target to copy files to ~/lib and ~/include
#
#  Revision 2.7  2005/08/20 13:13:47  woodbri
#  Changes the Makefiles so the default build is no dmalloc and not DB4.
#  These can be activated now using    make DMALLOC=yes DB4=yes
#
#  Revision 2.6  2005/02/25 01:06:25  woodbri
#  Changes required to support freeBSD.
#  Removed "libshp/" fromt the #include <shapelib.h>
#  Added -I/usr/local/include in the Makefiles
#
#  Revision 2.5  2005/02/07 13:44:19  woodbri
#  Chages to better handle compile options.
#
#  Revision 2.4  2005/01/15 13:16:40  woodbri
#  Add header comments.
#  Updated make file so it can be run without dmalloc via:
#  make NO_DMALLOC=1
#
#
#  Revision 1.1  2001/12/04 21:12:18  woodbri
#  Initial revision
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

DATE=`date +%Y%m%d`

opt   : CFLAGS2=-O2 -DUNIX -I.. $(CFLAGS)
debug : CFLAGS2=-g -DUNIX $(DMALLOC_CFLAGS) -I.. $(CFLAGS)

AR=ar -cq
RANLIB=ranlib

INCLUDE=-I. -I/usr/local/include

opt   : LIBS=-lshp -lm
debug : LIBS=-lshp $(DMALLOC_LIBS) -lm

LIBDIRS=-L. -L.. -L/usr/lib -L/usr/local/lib

OBJS=strtools.o double_metaphone.o index.o states.o rgxtools.o getpath.o hash.o

all:   debug
debug: strtools.a
opt:   strtools.a

.c.o:   Makefile strtools.h rgxtools.h index.h hash.h
	$(CC) $(CFLAGS2) -c $(INCLUDE) $<

strtools.a:	$(OBJS)
	-rm -f strtools.a
	$(AR) strtools.a $(OBJS)
	$(RANLIB) strtools.a

install:
	cp -fp strtools.a ~/lib/
	cp -fp *.h ~/include/

tar:    inventory.txt Makefile *.c *.h
	mkdir -p imt-strtools-$(DATE)
	cat inventory.txt | xargs --replace={} cp -fp {} imt-strtools-$(DATE)/.
	cp -fp *.[ch] Makefile imt-strtools-$(DATE)
	tar  czf imt-strtools-$(DATE).tgz imt-strtools-$(DATE)
	rm -rf imt-strtools-$(DATE)

clean:
	rm -f *.o strtools.a core
	

