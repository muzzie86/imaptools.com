/*********************************************************************
*
*  $Id: fixpoly.c,v 1.2 2007/02/01 05:38:30 woodbri Exp $
*
*  fixpoly
*  Copyright 2005, Stephen Woodbridge, All rights Reserved
*  Redistribition without written permission strictly prohibited
*
**********************************************************************
*
*  fixpoly.c
*
*  $Log: fixpoly.c,v $
*  Revision 1.2  2007/02/01 05:38:30  woodbri
*  Fixed some bugs, so it now works.
*
*  Revision 1.1  2007/02/01 01:24:02  woodbri
*  Adding fixploy.c a utility to decompose a polygon and reconstruct it with GEOS.
*
*
*  2005-12-27 sew, created
*
**********************************************************************
*
*  Utility to read polygons from shapefile and attempt to fix them
*  using GEOS and to write them to another file.
*
**********************************************************************/

#include <getopt.h>
#include <assert.h>
#include <errno.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <shapefil.h>
#include <geos_c.h>

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
  printf("fixpoly <src base name> <dest base name> [<list of objects>]\n");
  exit(0);
}

void notice(const char *fmt, ...)
{
    va_list ap;

        fprintf( stdout, "NOTICE: ");

    va_start (ap, fmt);
        vfprintf( stdout, fmt, ap);
        va_end(ap);
        fprintf( stdout, "\n" );
}

void log_and_exit(const char *fmt, ...)
{
    va_list ap;

        fprintf( stdout, "ERROR: ");

    va_start (ap, fmt);
        vfprintf( stdout, fmt, ap);
        va_end(ap);
        fprintf( stdout, "\n" );
    exit(1);
}

int copyCoordinates(int nv, double *X, double *Y, GEOSGeom g)
{
    int          n, i;
    GEOSCoordSeq s;

    s = GEOSGeom_getCoordSeq(g);
    GEOSCoordSeq_getSize(s, &n);

    for (i=0; i<n; i++) {
        GEOSCoordSeq_getX(s, i, X+nv);
        GEOSCoordSeq_getY(s, i, Y+nv);
        nv++;
    }

    return nv;
}

SHPObject *GeomToList(GEOSGeom g)
{
    int      nVertices = 0;
    int      nParts    = 0;
    int      nv = 0;
    int      np = 0;
    int      i, j;
    int     *S;
    double  *X;
    double  *Y;
    GEOSGeom gg;
    SHPObject *shp;

    /* count the number of vertices we need */

    switch (GEOSGeomTypeId(g)) {
        case GEOS_POLYGON:
            nVertices += GEOSGetNumCoordinates(GEOSGetExteriorRing(g));
            nParts++;
            for (i=0; i<GEOSGetNumInteriorRings(g); i++) {
                nVertices += GEOSGetNumCoordinates(GEOSGetInteriorRingN(g, i));
                nParts++;
            }
            break;
        case GEOS_MULTIPOLYGON:
            for (j=0; j<GEOSGetNumGeometries(g); j++) {
                gg = GEOSGetGeometryN(g, j);
                nVertices += GEOSGetNumCoordinates(GEOSGetExteriorRing(gg));
                nParts++;
                for (i=0; i<GEOSGetNumInteriorRings(gg); i++) {
                    nVertices += GEOSGetNumCoordinates(GEOSGetInteriorRingN(gg, i));
                    nParts++;
                }
            }
            break;
        default:
            printf("GeomToList got and unexpected geom type(%d)\n",
                GEOSGeomTypeId(g));
            return NULL;
            break;
    }

    /* allocate space for the vertices */

    X = calloc(nVertices, sizeof(double));
    assert(X);
    Y = calloc(nVertices, sizeof(double));
    assert(Y);
    if (nParts > 1) {
        S = calloc(nParts, sizeof(int));
        assert(S);
    }
    else
        S = NULL;

    /* transfer the vertix information */

    switch (GEOSGeomTypeId(g)) {
        case GEOS_POLYGON:
            if (S) S[np++] = nv;
            nv = copyCoordinates(nv, X, Y, GEOSGetExteriorRing(g));
            for (i=0; i<GEOSGetNumInteriorRings(g); i++) {
                if (S) S[np++] = nv;
                nv = copyCoordinates(nv, X, Y, GEOSGetInteriorRingN(g, i));
            }
            break;
        case GEOS_MULTIPOLYGON:
            for (j=0; j<GEOSGetNumGeometries(g); j++) {
                gg = GEOSGetGeometryN(g, j);
                if (S) S[np++] = nv;
                nv = copyCoordinates(nv, X, Y, GEOSGetExteriorRing(gg));
                for (i=0; i<GEOSGetNumInteriorRings(gg); i++) {
                    if (S) S[np++] = nv;
                    nv = copyCoordinates(nv, X, Y, GEOSGetInteriorRingN(gg, i));
                }
            }
            break;
        default:
            printf("GeomToList got and unexpected geom type(%d)\n",
                GEOSGeomTypeId(g));
            return NULL;
            break;
    }

    assert( nv == nVertices );

    if (np == 0 && nParts == 1)
        nParts = 0;

    assert( np == nParts );

    /* create a shp object */

    shp = SHPCreateObject(SHPT_POLYGON, -1, nParts, S, NULL, nVertices, X, Y, NULL, NULL);

    return shp;
}


SHPObject *fixpoly(int *npoly, SHPObject *sobj)
{
  GEOSGeom       *geoms;
  GEOSGeom        g  = NULL;
  GEOSGeom        g2 = NULL;
  GEOSGeom        g3;
  GEOSGeom        g4;
  GEOSGeom        g5;
  GEOSGeom        shp;
  GEOSCoordSeq    pnts;
  SHPObject      *this;
  int             nedge;
  int             ngeoms;
  int             i, j, n, na, nb, np;

  *npoly = 0;

  np = sobj->nParts;
  if (sobj->nParts == 0)
    np = 1;
  nedge = sobj->nVertices - np;
  geoms = calloc(nedge, sizeof(GEOSGeom));
  assert(geoms);

  n = 0;
  for (i=0; i<np; i++) {
    if (sobj->nParts) {
      na = sobj->panPartStart[i];
      if (i<np-1)
        nb = sobj->panPartStart[i+1] - 1;
      else
        nb = sobj->nVertices - 1;
    }
    else {
      na = 0;
      nb = sobj->nVertices - 1;
    }

    for (j=na; j<nb; j++) {
      pnts = GEOSCoordSeq_create(2, 2);
      GEOSCoordSeq_setX(pnts, 0, sobj->padfX[j]);
      GEOSCoordSeq_setY(pnts, 0, sobj->padfY[j]);
      GEOSCoordSeq_setX(pnts, 1, sobj->padfX[j+1]);
      GEOSCoordSeq_setY(pnts, 1, sobj->padfY[j+1]);
      geoms[n] = GEOSGeom_createLineString(pnts);
/*
printf("%d,%d,'LINESTRING(%.6f %.6f, %.6f %.6f)'\n",
    n, j, sobj->padfX[j], sobj->padfY[j], sobj->padfX[j+1], sobj->padfY[j+1]);
*/
      assert(n <= nedge);
      n++;
    }
  }
fflush(stdout);

  g = GEOSPolygonize(geoms, nedge);

  if (! g || GEOSGeomTypeId(g) != GEOS_GEOMETRYCOLLECTION)
    goto DONE;

  ngeoms = GEOSGetNumGeometries(g);

printf("GEOSPolygonize returned %d geoms IsValid(%d)\n", ngeoms, GEOSisValid(g));
/*
printf("%s\n", GEOSGeomToWKT(g));
*/

  if (ngeoms == 1) {
    g2 = GEOSGetGeometryN(g, 0);
    this = GeomToList(g2);
  }
  else if (ngeoms > 1) {
    shp = NULL;
    for (i=0; i<ngeoms; i++) {

printf("Processing Geom %d\n", i);

      GEOSGeom extring, tmp;

/***
      extring = GEOSGeom_createPolygon(
        GEOSGeom_clone(GEOSGetExteriorRing(GEOSGetGeometryN(g, i))),
        NULL, 0);
***/

      g3 = GEOSGetGeometryN(g, i);
printf("Geom(%d) is type (%d), IsValid(%d)\n", i, GEOSGeomTypeId(g3), GEOSisValid(g3));
/*
printf("%s\n", GEOSGeomToWKT(g3));
*/
      g4 = GEOSGetExteriorRing(g3);
printf("ExteriorRing(%d) is type (%d), IsValid(%d)\n", i, GEOSGeomTypeId(g4), GEOSisValid(g4));
/*
printf("%s\n", GEOSGeomToWKT(g4));
*/
/*
      g5 = GEOSGeom_clone(g4);
printf("Cloned ExteriorRing(%d) is type (%d), IsValid(%d)\n", i, GEOSGeomTypeId(g5), GEOSisValid(g5));
printf("%s\n", GEOSGeomToWKT(g5));
*/
      g5 = GEOSGeom_createLinearRing(GEOSCoordSeq_clone(GEOSGeom_getCoordSeq(g4)));
printf("Cloned ExteriorRing(%d) is type (%d), IsValid(%d)\n", i, GEOSGeomTypeId(g5), GEOSisValid(g5));

      extring = GEOSGeom_createPolygon(g5, NULL, 0);

      if (! extring) {
        fprintf(stderr, "GEOSGeom_createPolygon threw and exception!\n");
        goto DONE;
      }
printf("Got an extring\n");

      if (shp == NULL) {
        shp = extring;
      }
      else {
        tmp = GEOSSymDifference(shp, extring);
        GEOSGeom_destroy(shp);
        GEOSGeom_destroy(extring);
        shp = tmp;
      }
    }
printf("Result is type (%d), IsValid(%d)\n", GEOSGeomTypeId(shp), GEOSisValid(shp));
//printf("ngeom=%d\n", GEOSGetNumGeometries(shp));
/*
printf("%s\n", GEOSGeomToWKT(shp));
*/
    this = GeomToList(shp);
    GEOSGeom_destroy(shp);
  }

DONE:

  *npoly = ngeoms;

  /* destory GEOS objects */
  if (g)
    GEOSGeom_destroy(g);

  for (i=0; i<nedge; i++) {
    if (geoms[i])
      GEOSGeom_destroy(geoms[i]);
  }

  free(geoms);

  return this;
}

int main( int argc, char ** argv )
{
  DBFHandle     dIN, dOUT;
  SHPHandle     sIN, sOUT;
  SHPObject    *sobj;
  SHPObject    *shp;
  DBFFieldType  fType;
  char         *fin;
  char         *fout;
  char          fldname[12];
  int           nEnt, sType, fw, fd;
  int           i, j, rec, n;
  int           np, npoly;
  double        zX[2] = {0.0, 0.0};
  double        zY[2] = {0.0, 0.0};
  double        sMin[4];
  double        sMax[4];

  if (argc < 3) Usage();

  fin  = argv[1];
  fout = argv[2];
  argc -= 3;
  argv += 3;

  if (!(sIN  = SHPOpen(fin, "rb"))) {
    printf("Failed to open shp %s for read\n", fin);
    exit(1);
  }

  SHPGetInfo(sIN, &nEnt, &sType, sMin, sMax);

  if ( sType % 10 != 5 ) {
    printf("Shapefile is not polygon type (%d)\n", sType);
    exit(1);
  }

  if (!(dIN  = DBFOpen(fin, "rb"))) {
    printf("Failed to open dbf %s for read\n", fin);
    exit(1);
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
  }

  initGEOS(notice, log_and_exit);
  if (1 || DEBUG)
    printf("%s\n", GEOSversion());

  if (argc > 0)
    for (i=0; i<argc; i++) {
      rec = strtol(argv[i], NULL, 10);
      if (rec >= nEnt) continue;
      sobj = SHPReadObject(sIN, rec);
      if (sobj->nSHPType != sType || sobj->nVertices < 4) { 
        printf("Skipping: %d, type=%d, nvert=%d\n", i, sobj->nSHPType, sobj->nVertices);
        SHPDestroyObject(sobj);
        continue;
      }
      shp = fixpoly(&npoly, sobj);
      if (! shp) continue;
      n = SHPWriteObject(sOUT, -1, shp);
      SHPDestroyObject(shp);
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
      if (sobj->nSHPType != sType || sobj->nVertices < 4) {
        printf("Skipping: %d, type=%d, nvert=%d\n", i, sobj->nSHPType, sobj->nVertices);
        SHPDestroyObject(sobj);
        continue;
      }
      shp = fixpoly(&npoly, sobj);
      if (! shp) continue;
      n = SHPWriteObject(sOUT, -1, shp);
      SHPDestroyObject(shp);
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

  finishGEOS();

  DBFClose(dIN);
  DBFClose(dOUT);
  SHPClose(sIN);
  SHPClose(sOUT);
}


