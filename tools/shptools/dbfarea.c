/*********************************************************************
*
*  $Id: dbfarea.c,v 1.2 2005/05/18 23:43:04 woodbri Exp $
*
*  shpcpy
*  Copyright 2001, Stephen Woodbridge, All rights Reserved
*  Redistribition without written permission strictly prohibited
*
**********************************************************************
*
*  dbfarea.c
*
*  $Log: dbfarea.c,v $
*  Revision 1.2  2005/05/18 23:43:04  woodbri
*  Fixed a message typo.
*
*  Revision 1.1  2005/05/16 02:29:20  woodbri
*  Adding dbfarea.c and Makefile changes for same.
*
*
*  2005-05-11 sew, created
*
**********************************************************************
*
*  Utility to read a shapefile, compute the bbox area of the shape
*  nad add it to a copy of the dbf file with a new field for it.
*
**********************************************************************/

#include <unistd.h>
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

/* METERS_PER_DEGREE = EARTH_CIRCUMFRENCE_IN_METERS / 360 */
#define METERS_PER_DEGREE (40075160/360)
#define KILOMETERS_PER_DEGREE (40075.16/360)

extern int DEBUG;
int DEBUG  = 0;

extern char *optarg;
extern int optind, opterr, optopt;

void Usage()
{
  printf("dbfarea [-h|-i] [-b|-s] -f <area field> <src file> <new dbf file>\n");
  printf("        -h  display this help message\n");
  printf("        -i  display dbf field info\n");
  printf("        -b  compute the area as the area of the shape bbox in sq. meters\n");
  printf("        -s  compute the area of the shape polygon in sq. meters\n");
  printf("            ASSUMES simple shapes without holes\n");
  printf("        -f  dbf field name not in file for area to be placed in\n");
  printf("        <area field>   - name of new attribute field the will be\n");
  printf("                         added and populated with the computed area\n");
  printf("        <src file>     - polygon a shape file\n");
  printf("        <new dbf file> - name of new dbf file that will be a copy\n");
  printf("                         the <src file> with <area field> added\n");
  printf("   NOTE: you must move the <new dbf file> to replace the <src file>.dbf\n");
  printf("   NOTE: This utility ASSUMES the shape data is in decimal degrees!!!\n");
  exit(1);
}

void print_fields(DBFHandle dIN) {
    int           i, fw, fd;
    char          fldname[12];
    DBFFieldType  fType;

    fprintf(stderr, "DBF Records = %d\nNum Fieldname    Typ Wid Dec\n",
             DBFGetRecordCount(dIN));
    for (i=0; i<DBFGetFieldCount(dIN); i++) {
        fType = DBFGetFieldInfo(dIN, i, fldname, &fw, &fd);
        fprintf(stderr, " %2d %-12s  %c  %3d %3d\n", i+1, fldname,
                DBFGetNativeFieldType(dIN, i), fw, fd);
    }
}


int main( int argc, char ** argv )
{
  DBFHandle     dIN, dOUT;
  SHPHandle     sIN, sOUT;
  SHPObject    *sobj;
  DBFFieldType  fType;
  char         *fin;
  char         *fout;
  char         *newfld;
  char          fldname[12];
  int           nEnt, sType, fw, fd;
  int           i, j, k, rec, n, partEnd;
  int           nf;
  double        zX[2] = {0.0, 0.0};
  double        zY[2] = {0.0, 0.0};
  double        sMin[4];
  double        sMax[4];
  double        area;
  double        dx, dy;
  int           doinfo = 0;
  int           dobbox = 1;
  int           last = 0;

  while (!last) {
    switch (getopt(argc, argv, "hibsf:")) {
      case -1:
        last = 1;
        break;
      case 'h':
        Usage();
        break;
      case 'i':
        doinfo = 1;
        break;
      case 'b':
        dobbox = 1;
        break;
      case 's':
        dobbox = 0;
        break;
      case 'f':
        newfld = optarg;
        break;
    }
  }

  if (argc-optind < 1) Usage();
  fin    = argv[optind];

  if (!(dIN  = DBFOpen(fin, "rb"))) {
    printf("Failed to open dbf %s for read\n", fin);
    exit(1);
  }

  if (doinfo) {
    print_fields(dIN);
    DBFClose(dIN);
    exit(0);
  }

  if (argc-optind < 2) Usage();
  fout   = argv[optind+1];

  for (i=0; i<DBFGetFieldCount(dIN); i++) {
    fType = DBFGetFieldInfo(dIN, i, fldname, &fw, &fd);
    if (!strcmp(fldname, newfld)) {
        printf("New field '%s' is already in use!\n", newfld);
        exit(1);
    }
  }

  if (!(sIN  = SHPOpen(fin, "rb"))) {
    printf("Failed to open shp %s for read\n", fin);
    exit(1);
  }

  SHPGetInfo(sIN, &nEnt, &sType, sMin, sMax);

  if (sType != SHPT_POLYGON &&
      sType != SHPT_POLYGONZ &&
      sType != SHPT_POLYGONM ) {
    printf("The shapefile is not POLYGON type\n");
    exit(1);
  }

  if (!(dOUT = DBFCreate(fout))) {
    printf("Failed to create dbf %s\n", fout);
    exit(1);
  }

  for (i=0; i<DBFGetFieldCount(dIN); i++) {
    fType = DBFGetFieldInfo(dIN, i, fldname, &fw, &fd);
    DBFAddField(dOUT, fldname, fType, fw, fd);
  }
  nf = DBFAddField(dOUT, newfld, FTDouble, 12, 3);

  for (i=0; i<nEnt; i++) {
    rec = i;
    sobj = SHPReadObject(sIN, rec);
    if (dobbox) {
        area = (sobj->dfXMax - sobj->dfXMin) * KILOMETERS_PER_DEGREE *
               (sobj->dfYMax - sobj->dfYMin) * KILOMETERS_PER_DEGREE;
    } else {
        area = 0.0;
        if (sobj->nParts)
            for (k=0; k<sobj->nParts; k++) {
                partEnd = sobj->nVertices;
                if (k < sobj->nParts-1)
                    partEnd = sobj->panPartStart[k+1];
                for (j=sobj->panPartStart[k]; j<partEnd-1; j++) {
                    dx = sobj->padfX[j+1] - sobj->padfX[j];
                    dy = (sobj->padfY[j+1] + sobj->padfY[j]) / 2.0;
                    area += dx * KILOMETERS_PER_DEGREE * 
                            dy * KILOMETERS_PER_DEGREE;
                }
            }
        else
            for (j=0; j<sobj->nVertices-1; j++) {
                dx = sobj->padfX[j+1] - sobj->padfX[j];
                dy = (sobj->padfY[j+1] + sobj->padfY[j]) / 2.0;
                area += dx * KILOMETERS_PER_DEGREE * 
                        dy * KILOMETERS_PER_DEGREE;
            }
        if (area < 0.0) area = -area;
    }
    SHPDestroyObject(sobj);
    for (j=0; j<DBFGetFieldCount(dIN); j++) {
      fType = DBFGetFieldInfo(dIN, j, NULL, NULL, NULL);
      switch (fType) {
        case FTString:
          DBFWriteStringAttribute( dOUT, rec, j, DBFReadStringAttribute( dIN, rec, j));
          break;
        case FTInteger:
          DBFWriteIntegerAttribute(dOUT, rec, j, DBFReadIntegerAttribute(dIN, rec, j));
          break;
        case FTDouble:
          DBFWriteDoubleAttribute( dOUT, rec, j, DBFReadDoubleAttribute( dIN, rec, j));
          break;
      }
    }
    DBFWriteDoubleAttribute( dOUT, rec, nf, area);
  }

  DBFClose(dIN);
  DBFClose(dOUT);
  SHPClose(sIN);
}


