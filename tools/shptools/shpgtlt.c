/*********************************************************************
*
*  $Id: shpgtlt.c,v 1.2 2013-11-01 15:09:52 woodbri Exp $
*
*  shpgtlt
*  Copyright 2005, Stephen Woodbridge, All rights Reserved
*  Redistribition without written permission strictly prohibited
*
**********************************************************************
*
*  shpegtlt.c
*
*  $Log: shpgtlt.c,v $
*  Revision 1.2  2013-11-01 15:09:52  woodbri
*  Syncing changes to the server.
*
*  Revision 1.1  2012-03-04 19:39:59  woodbri
*  Adding shpgtlt.c and missing inventpry.txt
*
*
*  Initial revision
*  2008-04-20 sew, created.
*  2012-03-04 sew, created from shpgrep.c
*
**********************************************************************
*
*  Utility to copy objects from one shapefile to a new shapefile
*  based on an attribute be GT ot LT a value
*
**********************************************************************/

#include <getopt.h>
#include <assert.h>
#include <errno.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <shapefil.h>

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
  printf("shpegtlt <src base name> <dest base name> [gt|lt] <column> <value>\n");
  printf("         Only works on int or float attribute columns\n");
  printf("         of string based columns if the value is a number.\n");
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
  char         *gtlt;
  char         *cnam;
  char         *end;
  char          fldname[12];
  int           cnum = -1;
  int           nEnt, sType, fw, fd;
  int           i, j, rec, n;
  double        zX[2] = {0.0, 0.0};
  double        zY[2] = {0.0, 0.0};
  double        sMin[4];
  double        sMax[4];
  double        value;
  double        val;
  const char   *str;
  const char   *error;
  int           erroffset;
  int           dowrite;
  int           gt;
  int           lt;


  if (argc != 6) Usage();

  fin  = argv[1];
  fout = argv[2];
  gtlt = argv[3];
  cnam = argv[4];

  gt = strcasecmp(gtlt, "gt")?0:1;
  lt = strcasecmp(gtlt, "lt")?0:1;
  if (!gt && !lt) {
    printf("Error: condition must be 'gt' or 'lt'! '%s' is not valid!\n", gtlt);
    exit(1);
  }

  value = strtod(argv[5], NULL);

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
    if (!strcmp(fldname, cnam)) {
        cnum = i;
        ctyp = fType;
    }
  }

  rec = 0;
  for (i=0; i<nEnt; i++) {

    dowrite = 0;
    switch (ctyp) {
        case FTString:
            str = DBFReadStringAttribute( dIN, i, cnum);
            val = strtod(str, &end);
            if (str != end && ((gt && val > value) || (!gt && val < value)))
                dowrite = 1;
            break;
        case FTInteger:
            val = (double) DBFReadIntegerAttribute(dIN, i, cnum);
            if ((gt && val > value) || (!gt && val < value))
                dowrite = 1;
            break;
        case FTDouble:
            val = DBFReadDoubleAttribute(dIN, i, cnum);
            if ((gt && val > value) || (!gt && val < value))
                dowrite = 1;
            break;
    }
    if (!dowrite)  continue;

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

  DBFClose(dIN);
  DBFClose(dOUT);
  SHPClose(sIN);
  SHPClose(sOUT);
}


