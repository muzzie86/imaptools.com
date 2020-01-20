/*********************************************************************
*
*  $Id: shpcpy.c,v 1.3 2004/11/14 02:04:10 woodbri Exp $
*
*  shpcpy
*  Copyright 2001, Stephen Woodbridge, All rights Reserved
*  Redistribition without written permission strictly prohibited
*
**********************************************************************
*
*  shpcpy.c
*
*  $Log: shpcpy.c,v $
*  Revision 1.3  2004/11/14 02:04:10  woodbri
*  Changed path for shapelib.h for Debian.
*
*  Revision 1.2  2004/03/12 14:42:28  woodbri
*  Consolidating various changes from different systems into cvs.
*
*  Revision 1.1  2002/03/18 16:23:52  woodbri
*  Initial revision
*
*
*  2002-02-12 sew, created
*
**********************************************************************
*
*  Utility to copy objects from one shapefile to a new shapefile
*
**********************************************************************/

#include <getopt.h>
#include <assert.h>
#include <errno.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <shapefil.h>

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
  printf("shpcpy <src base name> <dest base name> [<list of objects>]\n");
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
  int           nEnt, sType, fw, fd;
  int           i, j, rec, n;
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

  SHPGetInfo(sIN, &nEnt, &sType, sMin, sMax);

  if (!(dOUT = DBFCreate(fout))) {
    printf("Failed to create dbf %s\n", fout);
    exit(1);
  }

  if (!(sOUT = SHPCreate(fout, sType))) {
    printf("Failed to create shp %s\n", fout);
    exit(1);
  }

  for (i=0; i<DBFGetFieldCount(dIN); i++) {
    fType = DBFGetFieldInfo(dIN, i, fldname, &fw, &fd);
    DBFAddField(dOUT, fldname, fType, fw, fd);
  }

  if (argc > 0)
    for (i=0; i<argc; i++) {
      rec = strtol(argv[i], NULL, 10);
      if (rec >= nEnt) continue;
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
  else
    for (i=0; i<nEnt; i++) {
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

  for (j=0; j<4; j++) {
    sOUT->adBoundsMin[j] = sMin[j];
    sOUT->adBoundsMax[j] = sMax[j];
  }

  DBFClose(dIN);
  DBFClose(dOUT);
  SHPClose(sIN);
  SHPClose(sOUT);
}


