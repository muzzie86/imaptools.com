/*********************************************************************
*
*  $Id: shpmove.c,v 1.2 2005/10/18 23:09:50 woodbri Exp $
*
*  shpmove
*  Copyright 2005, Stephen Woodbridge, All rights Reserved
*  Redistribition without written permission strictly prohibited
*
**********************************************************************
*
*  shpcpy.c
*
*  $Log: shpmove.c,v $
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
*  Utility to copies a shapefile and allow you to move all the verticies
*  of a one or more shape objects by a dx, dy offset.
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
  printf("shpmove <src base name> <dest base name> <dx> <dy> [<list of objects>]\n");
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
  char         *sdx;
  char         *sdy;
  char          fldname[12];
  int           nEnt, sType, fw, fd;
  int           i, j, rec, n;
  double        zX[2] = {0.0, 0.0};
  double        zY[2] = {0.0, 0.0};
  double        sMin[4];
  double        sMax[4];
  double        dx, dy;

  if (argc < 5) Usage();

  fin  = argv[1];
  fout = argv[2];
  sdx  = argv[3];
  sdy  = argv[4];
  argc -= 5;
  argv += 5;

  dx = strtod(sdx, NULL);
  dy = strtod(sdy, NULL);

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

  /* copy all the objects to the new file */
  
  for (i=0; i<nEnt; i++) {
    rec = i;
    sobj = SHPReadObject(sIN, rec);
    if (sobj->nSHPType < 1 || sobj->nVertices < 1) {
      printf("ABORT: Found a bad shape (%d), please clean file first\n", i);
      exit(1);
    }
    if (argc == 0) {
        for (j=0; j<sobj->nVertices; j++) {
            sobj->padfX[j] += dx;
            sobj->padfY[j] += dy;
        }
        SHPComputeExtents(sobj);
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

  if (argc > 0) {
    SHPClose(sOUT);
    if (!(sOUT  = SHPOpen(fout, "rb+"))) {
      printf("Failed to re-open shp %s for read/write\n", fout);
      exit(1);
    }
    SHPGetInfo(sOUT, &nEnt, &sType, sMin, sMax);
    
    for (i=0; i<argc; i++) {
      rec = strtol(argv[i], NULL, 10);
      if (rec >= nEnt) continue;
      sobj = SHPReadObject(sOUT, rec);
      if (sobj->nSHPType < 1 || sobj->nVertices < 1) { 
        printf("ABORT: Found a bad shape (%d), please clean file first\n", i);
        exit(1);
      }
      for (j=0; j<sobj->nVertices; j++) {
          sobj->padfX[j] += dx;
          sobj->padfY[j] += dy;
      }
      SHPComputeExtents(sobj);
      n = SHPWriteObject(sOUT, rec, sobj);
      SHPDestroyObject(sobj);
    }
  }

  DBFClose(dIN);
  DBFClose(dOUT);
  SHPClose(sIN);
  SHPClose(sOUT);
}


