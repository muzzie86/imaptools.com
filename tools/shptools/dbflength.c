/*********************************************************************
*
*  $Id: dbflength.c,v 1.1 2009/04/11 04:51:48 woodbri Exp $
*
*  shplength
*  Copyright 2009, Stephen Woodbridge, All rights Reserved
*  Redistribition without written permission strictly prohibited
*
**********************************************************************
*
*  dbflength.c
*
*  $Log: dbflength.c,v $
*  Revision 1.1  2009/04/11 04:51:48  woodbri
*  Adding dbflength.c
*
*
*  2009-01-01 sew, created
*
**********************************************************************
*
*  Utility to read a shapefile, compute the length of the shape
*  and add it to a copy of the dbf file with a new field for it.
*
**********************************************************************/

#include <unistd.h>
#include <assert.h>
#include <errno.h>
#include <ctype.h>
#include <stdlib.h>
#include <math.h>
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

#define EARTH_RADIUS_KM 6371.0
#define TO_RADIAN(a) ((a)/57.295779513982)

extern int DEBUG;
int DEBUG  = 0;

extern char *optarg;
extern int optind, opterr, optopt;

void Usage()
{
  printf("dbflength [-h|-i] [-p] [-f <length field>] <src file> [<new dbf file>]\n");
  printf("        -h  display this help message\n");
  printf("        -i  display dbf field info\n");
  printf("        -p  print the length of the shape in meters\n");
  printf("        -t  print the cumulative length of the shape in meters\n");
  printf("        -f  dbf field name not in file for length to be placed in\n");
  printf("        <length field>   - name of new attribute field the will be\n");
  printf("                         added and populated with the computed length\n");
  printf("        <src file>     - polygon a shape file\n");
  printf("        <new dbf file> - name of new dbf file that will be a copy\n");
  printf("                         the <src file> with <length field> added\n");
  printf("   NOTE: you must move the <new dbf file> to replace the <src file>.dbf\n");
  printf("   NOTE: This utility ASSUMES the shape data is in decimal degrees!!!\n");
  printf("   NOTE: This utility ASSUMES the shape is an arc type!!!\n");
  exit(1);
}

double spherical_distance_km(double x1, double y1, double x2, double y2)
{
    double dx, dy, a, c, d;

    dx = TO_RADIAN(x2 - x1);
    dy = TO_RADIAN(y2 - y1);
    a = pow(sin(dy/2.0), 2.0) + 
        cos(TO_RADIAN(y1)) * cos(TO_RADIAN(y2)) *
        pow(sin(dx/2.0), 2.0);
    c = 2.0 * atan2(sqrt(a), sqrt(1.0 - a));
    return EARTH_RADIUS_KM * c;
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
  char         *newfld = NULL;
  char          fldname[12];
  int           nEnt, sType, fw, fd;
  int           i, j, k, rec, n, partEnd;
  int           nf;
  double        zX[2] = {0.0, 0.0};
  double        zY[2] = {0.0, 0.0};
  double        sMin[4];
  double        sMax[4];
  double        length;
  double        tlength = 0.0;
  double        dx, dy;
  int           doinfo = 0;
  int           print = 0;
  int           tprint = 0;
  int           last = 0;

  while (!last) {
    switch (getopt(argc, argv, "hitpf:")) {
      case -1:
        last = 1;
        break;
      case 'h':
        Usage();
        break;
      case 'i':
        doinfo = 1;
        break;
      case 't':
        tprint = 1;
        break;
      case 'p':
        print = 1;
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

  if (argc-optind < 2) {
      fout   = NULL;
  } else {
      fout   = argv[optind+1];
  }

  if (!newfld && fout) {
      printf("New field is require if you want a new dbf!\n");
      printf("otherwise just use -p without -f and <new dbf file>\n");
      exit(1);
  }
  
  if (fout)
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

  if (sType != SHPT_ARC &&
      sType != SHPT_ARCZ &&
      sType != SHPT_ARCM ) {
    printf("The shapefile is not ARC type\n");
    exit(1);
  }

  if (fout && !(dOUT = DBFCreate(fout))) {
    printf("Failed to create dbf %s\n", fout);
    exit(1);
  }

  if (fout) {
      for (i=0; i<DBFGetFieldCount(dIN); i++) {
        fType = DBFGetFieldInfo(dIN, i, fldname, &fw, &fd);
        DBFAddField(dOUT, fldname, fType, fw, fd);
      }
      nf = DBFAddField(dOUT, newfld, FTDouble, 12, 3);
  }

  for (i=0; i<nEnt; i++) {
    rec = i;
    sobj = SHPReadObject(sIN, rec);
    length = 0.0;
    if (sobj->nParts)
        for (k=0; k<sobj->nParts; k++) {
            partEnd = sobj->nVertices;
            if (k < sobj->nParts-1)
                partEnd = sobj->panPartStart[k+1];
            for (j=sobj->panPartStart[k]; j<partEnd-1; j++) {
                /*
                dx = sobj->padfX[j+1] - sobj->padfX[j];
                dy = sobj->padfY[j+1] - sobj->padfY[j];
                length += sqrt(dx*dx + dy*dy) * METERS_PER_DEGREE;
                */
                length += spherical_distance_km(
                        sobj->padfX[j], sobj->padfY[j],
                        sobj->padfX[j+1], sobj->padfY[j+1]) * 1000.0;
            }
        }
    else
        for (j=0; j<sobj->nVertices-1; j++) {
            /*
            dx = sobj->padfX[j+1] - sobj->padfX[j];
            dy = sobj->padfY[j+1] - sobj->padfY[j];
            length += sqrt(dx*dx + dy*dy) * METERS_PER_DEGREE;
            */
            length += spherical_distance_km(
                    sobj->padfX[j], sobj->padfY[j],
                    sobj->padfX[j+1], sobj->padfY[j+1]) * 1000.0;
        }
    SHPDestroyObject(sobj);
    if (fout) {
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
        DBFWriteDoubleAttribute( dOUT, rec, nf, length);
    }
    tlength += length;
    if (print) printf("%8d\t%12.2f\n", i+1, length);
  }
  if (tprint) printf("Cumulative length:\t%12.2f\n", tlength);

  DBFClose(dIN);
  if (fout) DBFClose(dOUT);
  SHPClose(sIN);
}


