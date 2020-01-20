/*********************************************************************
*
*  $Id: shpgrep.c,v 1.3 2012-03-04 19:39:59 woodbri Exp $
*
*  shpgrep
*  Copyright 2005, Stephen Woodbridge, All rights Reserved
*  Redistribition without written permission strictly prohibited
*
**********************************************************************
*
*  shpgrep.c
*
*  $Log: shpgrep.c,v $
*  Revision 1.3  2012-03-04 19:39:59  woodbri
*  Adding shpgtlt.c and missing inventpry.txt
*
*  Revision 1.2  2008/02/04 03:38:59  woodbri
*  Updating cvs local changes.
*
*  Revision 1.1  2005/10/18 23:08:29  woodbri
*  Added shpgrep.c a new utility.
*
*
*  Initial revision
*
*
*  2005-10-17 sew, created
*
**********************************************************************
*
*  Utility to copy objects from one shapefile to a new shapefile
*  based on selecting an attribute column and value
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
  printf("shpgrep <src base name> <dest base name> <column> <list of values>\n");
  exit(0);
}

int main( int argc, char ** argv )
{
  DBFHandle     dIN, dOUT;
  SHPHandle     sIN, sOUT;
  SHPObject    *sobj;
  DBFFieldType  fType, ctyp;
  char         *fin;
  char         *fout;
  char         *cnam;
  char          fldname[12];
  int           cnum = -1;
  int           nEnt, sType, fw, fd;
  int           i, j, rec, n;
  double        zX[2] = {0.0, 0.0};
  double        zY[2] = {0.0, 0.0};
  double        sMin[4];
  double        sMax[4];

  if (argc < 5) Usage();

  fin  = argv[1];
  fout = argv[2];
  cnam = argv[3];
  argc -= 4;
  argv += 4;

  if (!(dIN  = DBFOpen(fin, "rb"))) {
    printf("Failed to open dbf %s for read\n", fin);
    exit(1);
  }

  if (!(sIN  = SHPOpen(fin, "rb"))) {
    printf("Failed to open shp %s for read\n", fin);
    exit(1);
  }

  SHPGetInfo(sIN, &nEnt, &sType, sMin, sMax);
  if (DBFGetRecordCount(dIN) != nEnt) {
    printf("WARNING: record counts for shp(%d) and dbf(%d) do not match!\n",
            nEnt, DBFGetRecordCount(dIN));
    if (DBFGetRecordCount(dIN) < nEnt) nEnt = DBFGetRecordCount(dIN);
  }

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
    printf("Column: '%s'\n", fldname);
    if (!strcmp(fldname, cnam)) {
        cnum = i;
        ctyp = fType;
    }
  }

  if (cnum == -1) {
    printf("Failed to find column '%s'!\n", cnam);
    exit(1);
  }

  rec = 0;
  for (i=0; i<nEnt; i++) {

    for (j=0; j<argc; j++)
        switch (ctyp) {
            case FTString:
                if (!strcmp(argv[j], DBFReadStringAttribute( dIN, i, cnum)))
                    goto writeit;
                break;
            case FTInteger:
                if (strtol(argv[j], NULL, 10) == DBFReadIntegerAttribute(dIN, i, cnum))
                    goto writeit;
                break;
            case FTDouble:
                if (strtol(argv[j], NULL, 10) == DBFReadIntegerAttribute(dIN, i, cnum))
                    goto writeit;
                break;
        }
    continue;

    writeit:

    sobj = SHPReadObject(sIN, i);
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
          DBFWriteStringAttribute( dOUT, n, j, DBFReadStringAttribute( dIN, i, j));
          break;
        case FTInteger:
          DBFWriteIntegerAttribute(dOUT, n, j, DBFReadIntegerAttribute(dIN, i, j));
          break;
        case FTDouble:
          DBFWriteDoubleAttribute( dOUT, n, j, DBFReadDoubleAttribute( dIN, i, j));
          break;
      }
    }
    rec++;
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


