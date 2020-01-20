/*********************************************************************
*
*  $Id: shpsplit.c,v 1.1 2006/09/28 15:27:29 woodbri Exp $
*
*  shpsplit
*  Copyright 2001, Stephen Woodbridge, All rights Reserved
*  Redistribition without written permission strictly prohibited
*
**********************************************************************
*
*  shpsplit.c
*
*  $Log: shpsplit.c,v $
*  Revision 1.1  2006/09/28 15:27:29  woodbri
*  Adding shpcpy2.c shpcpy3.c shpsplit.c
*
*
*  2006-09-19 sew, created from shpcpy
*
**********************************************************************
*
*  Utility to copy objects from one shapefile to a new shapefiles
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
#include <libshp/shapefil.h>
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
  printf("shpsplit <src base name> <dest base name> [number_of_files]\n");
  exit(0);
}

int main( int argc, char ** argv )
{
  DBFHandle     dIN, dOUT;
  SHPHandle     sIN, sOUT;
  SHPObject    *sobj;
  DBFFieldType  fType;
  char         *fin;
  char         *fout;
  char          fldname[12];
  char          fname[256];
  int           step, start, end, cnt;
  int           nEnt, sType, fw, fd;
  int           i, j, k, rec, n;
  double        zX[2] = {0.0, 0.0};
  double        zY[2] = {0.0, 0.0};
  double        sMin[4];
  double        sMax[4];

  if (argc < 3) Usage();

  fin  = argv[1];
  fout = argv[2];
  argc -= 3;
  argv += 3;

  if (!(dIN  = DBFOpen(fin, "rb"))) {
    printf("Failed to open dbf %s for read\n", fin);
    exit(1);
  }

  if (!(sIN  = SHPOpen(fin, "rb"))) {
    printf("Failed to open shp %s for read\n", fin);
    exit(1);
  }

  if (argc > 0) {
      cnt = strtol(argv[0], NULL, 10);
      if (cnt < 2) {
          printf("Number of files must be greater than 1.\n");
          exit(1);
      }
  } else {
      cnt = 2;
  }

  SHPGetInfo(sIN, &nEnt, &sType, sMin, sMax);

  step = nEnt/cnt;
  
  for (k=0; k<cnt; k++) {
      sprintf(fname, "%s-%02d", fout, k);
      
      start = k * step;
      end   = (k + 1) * step;
      if (k == cnt -1) end = nEnt;
      
      if (!(dOUT = DBFCreate(fname))) {
        printf("Failed to create dbf %s\n", fout);
        exit(1);
      }

      if (!(sOUT = SHPCreate(fname, sType))) {
        printf("Failed to create shp %s\n", fout);
        exit(1);
      }

      for (i=0; i<DBFGetFieldCount(dIN); i++) {
        fType = DBFGetFieldInfo(dIN, i, fldname, &fw, &fd);
        DBFAddField(dOUT, fldname, fType, fw, fd);
      }

    for (i=start; i<end; i++) {
      rec = i;
      sobj = SHPReadObject(sIN, rec);
      if (sobj->nSHPType < 1 || sobj->nVertices < 1) {
        SHPDestroyObject(sobj);
#ifdef NULLTOPOINT
        sobj = SHPCreateSimpleObject(sType, 1, zX, zY, NULL);
        sobj->dfXMin = sMin[0];
        sobj->dfYMin = sMin[1];
        sobj->dfZMin = sMin[2];
        sobj->dfMMin = sMin[3];
        sobj->dfXMax = sMax[0];
        sobj->dfYMax = sMax[1];
        sobj->dfZMax = sMax[2];
        sobj->dfMMax = sMax[3];
#else
        continue;
#endif
      }
      n = SHPWriteObject(sOUT, -1, sobj);
      SHPDestroyObject(sobj);
      for (j=0; j<DBFGetFieldCount(dIN); j++) {
        fType = DBFGetFieldInfo(dIN, j, NULL, NULL, NULL);
        switch (fType) {
          case FTString:
            DBFWriteStringAttribute( dOUT, n, j, DBFReadStringAttribute( dIN, rec, j));
            break;
          case FTInteger:
            DBFWriteIntegerAttribute(dOUT, n, j, DBFReadIntegerAttribute(dIN, rec, j));
            break;
          case FTDouble:
            DBFWriteDoubleAttribute( dOUT, n, j, DBFReadDoubleAttribute( dIN, rec, j));
            break;
        }
      }
    }
    DBFClose(dOUT);
    SHPClose(sOUT);
  }

  DBFClose(dIN);
  SHPClose(sIN);
}


