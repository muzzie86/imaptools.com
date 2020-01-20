/*********************************************************************
*
*  $Id: shpcpy2D.c,v 1.4 2009/08/21 17:25:45 woodbri Exp $
*
*  shpcpy
*  Copyright 2001, Stephen Woodbridge, All rights Reserved
*  Redistribition without written permission strictly prohibited
*
**********************************************************************
*
*  shpcpy.c
*
*  $Log: shpcpy2D.c,v $
*  Revision 1.4  2009/08/21 17:25:45  woodbri
*  Added code to force fieldnames to upper case.
*
*  Revision 1.3  2009/08/21 17:15:57  woodbri
*  Added code to work around a change in behavior of the libshp
*  SHPWriteObject().
*
*  Revision 1.2  2009/06/02 04:25:13  woodbri
*  Fixed Usage statement.
*
*  Revision 1.1  2009/06/02 04:22:04  woodbri
*  Adding shpcpy2D and updating Makefile to support it.
*
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
  printf("shpcpy2D <src base name> <dest base name> [<list of objects>]\n");
  printf("   Creates a 2D shapefile, by stripping away the Z and/or M components.\n");
  exit(0);
}

const char* str2uc(const char *s)
{
    static char t[1025];
    int i;

    for (i=0; i<strlen(s) && i < 1024; i++) {
        if (islower(s[i]))
            t[i] = toupper(s[i]);
        else
            t[i] = s[i];
    }
    if (t[i+1] != '\0')
        t[i+1] = '\0';

    return t;
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
  int           nEnt, sType, fw, fd, tmp;
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

  SHPGetInfo(sIN, &nEnt, &sType, NULL, NULL);
  sMin[3] = sMin[4] = sMax[3] = sMax[4] = 0.0;
  if (sType == 31) {
      printf("Failed: MULTIPATCH shapefiles not supported!\n");
      exit(1);
  }
  sType = sType % 10; 

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
    DBFAddField(dOUT, str2uc(fldname), fType, fw, fd);
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
      tmp = sobj->nSHPType;
      sobj->nSHPType = sType;
      n = SHPWriteObject(sOUT, -1, sobj);
      sobj->nSHPType = tmp;
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
      tmp = sobj->nSHPType;
      sobj->nSHPType = sType;
      n = SHPWriteObject(sOUT, -1, sobj);
      sobj->nSHPType = tmp;
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

  /*
  for (j=0; j<4; j++) {
    sOUT->adBoundsMin[j] = sMin[j];
    sOUT->adBoundsMax[j] = sMax[j];
  }
  */

  DBFClose(dIN);
  DBFClose(dOUT);
  SHPClose(sIN);
  SHPClose(sOUT);
}


