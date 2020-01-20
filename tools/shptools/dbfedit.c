/*********************************************************************
*
*  $Id: dbfedit.c,v 1.2 2005/10/18 23:09:50 woodbri Exp $
*
*  dbfedit
*  Copyright 2005, Stephen Woodbridge, All rights Reserved
*  Redistribition without written permission strictly prohibited
*
**********************************************************************
*
*  dbfedit.c
*
*  $Log: dbfedit.c,v $
*  Revision 1.2  2005/10/18 23:09:50  woodbri
*  Changing location of libshape.h
*
*  Revision 1.1  2005/09/02 18:47:54  woodbri
*  Initial check in of dbfedit.c shpmove.c
*
*
*  2005-09-02 sew, created from shpcpy.c
*
**********************************************************************
*
*  Utility to set a dbf attribute for one of more rows.
*
**********************************************************************/

#include <getopt.h>
#include <assert.h>
#include <errno.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#ifdef DEBIAN_LIBSHP
#include <shapefil.h>
#else
#include <shapefil.h>
#endif


#ifdef DMALLOC
#include "dmalloc.h"
#endif

// Debug print out
#define DBGOUT(s) (s)
//#define DBGOUT(s)

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

extern int DEBUG;
int DEBUG  = 0;


void Usage()
{
  printf("dbfedit <src name> <field_name> <new_value> [<list of objects>]\n");
  exit(0);
}

int main( int argc, char ** argv )
{
  DBFHandle     dIN, dOUT;
  DBFFieldType  fType;
  char         *fin;
  char         *fnam;
  char         *fval;
  char          fldname[12];
  int           nEnt;
  int           fw, fd, ftype;
  int           i, j, rec, n;
  int           fldnum;

  if (argc < 4) Usage();

  fin  = argv[1];
  fnam = argv[2];
  fval = argv[3];
  argc -= 4;
  argv += 4;

  if (!(dIN  = DBFOpen(fin, "rb+"))) {
    printf("Failed to open dbf %s for read/write\n", fin);
    exit(1);
  }

  nEnt   = DBFGetRecordCount(dIN);
  fldnum = DBFGetFieldIndex(dIN, fnam);
  if (fldnum < 0) {
      printf("Failed to find field (%s)\n", fnam);
      exit(1);
  }
  ftype  = DBFGetFieldInfo(dIN, fldnum, NULL, &fw, &fd);

  if (argc > 0)
    for (i=0; i<argc; i++) {
      rec = strtol(argv[i], NULL, 10);
      if (rec >= nEnt) continue;
      switch (ftype) {
          case FTString:
            DBFWriteStringAttribute(dIN, rec, fldnum, fval);
            break;
          case FTInteger:
            DBFWriteIntegerAttribute(dIN, rec, fldnum, strtol(fval, NULL, 10));
            break;
          case FTDouble:
            DBFWriteDoubleAttribute(dIN, rec, fldnum, strtod(fval, NULL));
            break;
          default:
            break;
      }
    }
  else 
    for (i=0; i<nEnt; i++) 
      switch (ftype) {
          case FTString:
            DBFWriteStringAttribute(dIN, rec, fldnum, fval);
            break;
          case FTInteger:
            DBFWriteIntegerAttribute(dIN, rec, fldnum, strtol(fval, NULL, 10));
            break;
          case FTDouble:
            DBFWriteDoubleAttribute(dIN, rec, fldnum, strtod(fval, NULL));
            break;
          default:
            break;
      }

  DBFClose(dIN);
}


