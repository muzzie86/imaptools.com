/*********************************************************************
*
*  $Id: shpegrep.c,v 1.2 2010/07/25 00:42:19 woodbri Exp $
*
*  shpgrep
*  Copyright 2005, Stephen Woodbridge, All rights Reserved
*  Redistribition without written permission strictly prohibited
*
**********************************************************************
*
*  shpegrep.c
*
*  $Log: shpegrep.c,v $
*  Revision 1.2  2010/07/25 00:42:19  woodbri
*  Added -v option.
*
*  Revision 1.1  2009/04/11 04:49:06  woodbri
*  Adding shpegrep.c
*
*
*  Initial revision
*
*
*  2008-04-20 sew, created from shpgrep.c
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
#include <pcre.h>

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
  printf("shpegrep <src base name> <dest base name> <column> [-v] <pcrepatt> ...\n");
  printf("         regex only works on string fields, list int or float values\n");
  printf("         -v negates the matching and returns those that do not match\n");
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
  pcre        **re;
  const char   *str;
  const char   *error;
  int           erroffset;
  int           NOT;
  int           dowrite;


  if (argc < 5) Usage();

  fin  = argv[1];
  fout = argv[2];
  cnam = argv[3];
  argc -= 4;
  argv += 4;

  NOT = 0;
  if (!strcmp(argv[0], "-v")) {
    NOT = 1;
    argc--;
    argv++;
  }

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

  re = malloc(sizeof(pcre *)*argc);
  if (!re) {
      printf("Out of memory!\n");
      exit(1);
  }
  
  for (j=0; j<argc; j++) {
      re[j] = pcre_compile(argv[j], PCRE_NO_AUTO_CAPTURE, &error, &erroffset, NULL);
      if (!re[j]) {
          printf("Pattern '%s' failed to compile.\n%s at char %d.\n",
                  argv[j], error, erroffset);
          exit(1);
      }
  }

  rec = 0;
  for (i=0; i<nEnt; i++) {

    if (NOT) {
        dowrite = 1;
        for (j=0; j<argc; j++) {
            switch (ctyp) {
                case FTString:
                    str = DBFReadStringAttribute( dIN, i, cnum);
                    if (pcre_exec(re[j], NULL, str, strlen(str), 0, 0, NULL, 0) >= 0)
                        dowrite = 0;;
                    break;
                case FTInteger:
                    if (strtol(argv[j], NULL, 10) == DBFReadIntegerAttribute(dIN, i, cnum))
                        dowrite = 0;;
                    break;
                case FTDouble:
                    if (strtol(argv[j], NULL, 10) == DBFReadIntegerAttribute(dIN, i, cnum))
                        dowrite = 0;;
                    break;
            }
        }
        if (dowrite)
            goto writeit;
    }
    else {
        for (j=0; j<argc; j++) {
            switch (ctyp) {
                case FTString:
                    str = DBFReadStringAttribute( dIN, i, cnum);
                    if (pcre_exec(re[j], NULL, str, strlen(str), 0, 0, NULL, 0) >= 0)
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
        }
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

  for (i=0; i<argc; i++)
      if (re[i]) pcre_free(re[i]);

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


