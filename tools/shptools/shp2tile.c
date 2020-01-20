/*********************************************************************
*
*  $Id: shp2tile.c,v 1.14 2006/05/30 04:02:04 woodbri Exp $
*
*  shp2tile
*  Copyright 2002, Stephen Woodbridge
*
*  tools@swoodbridge.com
*  http://swoodbridge.com/tools/
*
*  Development sponsored by:
*  Luca Pescatore <info@pescatoreluca.com>
*
*   This program is free software; you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation; either version 2 of the License, or
*   (at your option) any later version.
*
*   This program is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with this program; if not, write to the Free Software
*   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*
**********************************************************************
*
*  shp2tile.c
*
*  2004-04-12 Stephen Woodbridge, added global tile to quadtree and
*             fixed a few bugs.
*  2003-03-30 Daniel Morissette, added square and quadtree tiling
*  2002-03-16 sew, created
*
**********************************************************************
*
*  Utility to copy objects from one shapefile to a row X col
*  tiled set of shapefiles
*
**********************************************************************
*
* TODO:
*
*  o tile into a global grid x0, y0, dx, dy
*  o better control over output file numbering/naming
*  o generate tileindex
*  o wildcard on input files
*
**********************************************************************/

#include <getopt.h>
#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <shapefil.h>

#ifdef DMALLOC
#include "dmalloc.h"
#endif

#define ONE_GLOBAL_CELL 1

typedef struct cellStruct {
  double      xmin;
  double      ymin;
  double      xmax;
  double      ymax;
  int         count;
  DBFHandle   dH;
  SHPHandle   sH;
} CELL;

extern int DEBUG;
int DEBUG  = 0;

static char *version = "$Id: shp2tile.c,v 1.14 2006/05/30 04:02:04 woodbri Exp $";


void Usage()
{
  printf("Usage: shp2tile <options> <shapefile> <dest name>\n");
  printf("  where <options> are:\n");
  printf("   [-v|--version]      print version string and exit\n");
  printf("   [-s|--no-stats]     do not print stats\n");
  printf("   [-w|--no-write]     do not write output files\n");
  printf("   [-r|--row] <n>      generate <n> rows in Y\n");
  printf("   [-c|--col] <m>      generate <m> cols in X\n");
  printf("   [-e|--square-ext]   adjust extent to produce square tiles\n");
  printf("   [-q|--quadtree] <n> quadtree type of index (auto split tiles > n objects)\n");
  printf("   [-m|--maxdepth] <n> maximum depth for --quadtree option\n");
  printf("  <shapefile>          name of file to read and split into tiles\n");
  printf("  <dest name>          output file names: <dest name>-<num>.<ext>\n");
  exit(0);
}

char * BuildTileFilename(const char *FOUT, int cid)
{
    char *fname;
    
    assert(cid >= 0);
    fname = (char *) malloc(strlen(FOUT)+20);
    assert(fname);
    sprintf(fname, "%s-%d", FOUT, cid);

    return fname;
}

void WriteObjectToFile(DBFHandle dIN, SHPObject *o, CELL *c, int cid, const char *FOUT)
{
  int           i, j, n;
  DBFFieldType  fType;
  char          fldname[12];
  char         *fname;
  int           fw, fd;

  if (!c->dH) {

    fname = BuildTileFilename(FOUT, cid);

    if (!(c->dH = DBFCreate(fname))) {
      printf("Failed to create dbf %s\n", fname);
      exit(1);
    }
  
    if (!(c->sH = SHPCreate(fname, o->nSHPType))) {
      printf("Failed to create shp %s\n", fname);
      exit(1);
    }

    free(fname);
  
    for (i=0; i<DBFGetFieldCount(dIN); i++) {
      fType = DBFGetFieldInfo(dIN, i, fldname, &fw, &fd);
      DBFAddField(c->dH, fldname, fType, fw, fd);
    }
  }

  n = SHPWriteObject(c->sH, -1, o);

  for (j=0; j<DBFGetFieldCount(dIN); j++) {
    fType = DBFGetFieldInfo(dIN, j, NULL, NULL, NULL);
    switch (fType) {
      case FTString:
        DBFWriteStringAttribute( c->dH, n, j, DBFReadStringAttribute( dIN, o->nShapeId, j));
        break;
      case FTInteger:
        DBFWriteIntegerAttribute(c->dH, n, j, DBFReadIntegerAttribute(dIN, o->nShapeId, j));
        break;
      case FTDouble:
        DBFWriteDoubleAttribute( c->dH, n, j, DBFReadDoubleAttribute( dIN, o->nShapeId, j));
        break;
      default:
        break;
    }
  }


}


#define PERCENT   0.05
#define MNBX(i,j) (MINB[0]+dx*((j)-PERCENT))
#define MXBX(i,j) (MINB[0]+dx*((j)+1+PERCENT))
#define MNBY(i,j) (MINB[1]+dy*((i)-PERCENT))
#define MXBY(i,j) (MINB[1]+dy*((i)+1+PERCENT))

int BestCell(SHPObject *o, double *MINB, double dx, double dy, int nr, int nc)
{
    int    i, j;
    int    ret;

    ret = nr * nc;

    for (i=0; i<nr; i++)
      for (j=0; j<nc; j++) 
        if (o->dfXMin > MNBX(i,j) && o->dfXMax < MXBX(i,j) &&
            o->dfYMin > MNBY(i,j) && o->dfYMax < MXBY(i,j) ) return i*nc+j;

    return ret;
}


/* ProcessFile()
**
** This is where all the magic happens!
*/

int ProcessFile(const char * fin, 
                const char * FOUT,
                int NROW, int NCOL,
                int dostats, int dowrite, 
                int square_ext, int quadtree_max,
                CELL *gcell, int maxdepth )
{
  DBFHandle     dIN;
  SHPHandle     sIN;
  SHPObject    *sobj;
  int           nEnt2, nEnt, sType;
  int           i, j, u, v, indx, rec;
  int           total = 0;
  double        MINB[4], MAXB[4];
  double        dx, dy;
  CELL         *cells;

  if (DEBUG)
    printf("ProcessFile('%s', '%s', %d, %d, %d, %d, %d, %d, %d)\n",
      fin, FOUT, NROW, NCOL, dostats, dowrite, square_ext, quadtree_max, maxdepth);

  if (!(dIN  = DBFOpen(fin, "rb"))) {
    printf("Failed to open dbf %s for read\n", fin);
    exit(1);
  }

  if (!(sIN  = SHPOpen(fin, "rb"))) {
    printf("Failed to open shp %s for read\n", fin);
    exit(1);
  }

  SHPGetInfo(sIN, &nEnt, &sType, MINB, MAXB);

  dx = (MAXB[0] - MINB[0]) / NCOL;
  dy = (MAXB[1] - MINB[1]) / NROW;

  if (dx == 0.0 || dy == 0.0) {
    printf("File: %s can not be tiled. Spatial width=%.4f and height=%.4f\n must be greater than 0.0. File has %d entities\n", fin, dx, dy, nEnt);
    return 99999;
  }

  if (square_ext) {
      /* Square tiles requested.  Adjust file extents to keep the 
       * grid centered on the original extents */
      if (dx > dy) {
          int dy2;
          dy2= (dx-dy) * NROW / 2.0;
          MINB[1] -= dy2;
          MAXB[1] += dy2;
      } else {
          int dx2;
          dx2= (dy-dx) * NCOL / 2.0;
          MINB[0] -= dx2;
          MAXB[0] += dx2;
      } 

      dx = (MAXB[0] - MINB[0]) / NCOL;
      dy = (MAXB[1] - MINB[1]) / NROW;
  }

  cells = (CELL *) calloc(NROW*NCOL+1, sizeof(CELL));
  assert(cells);

  for (i=0; i<NROW; i++)
    for (j=0; j<NCOL; j++) {
      cells[i*NCOL+j].xmin = MINB[0] + dx*j;
      cells[i*NCOL+j].ymin = MINB[1] + dy*i;
      cells[i*NCOL+j].xmax = MINB[0] + dx*(j+1);
      cells[i*NCOL+j].ymax = MINB[1] + dy*(i+1);
    }
  cells[NCOL*NROW].xmin = MINB[0];
  cells[NCOL*NROW].ymin = MINB[1];
  cells[NCOL*NROW].xmax = MAXB[0];
  cells[NCOL*NROW].ymax = MAXB[1];

  /*  Cycle through all the objects and write them to the files */

  for (rec=0; rec<nEnt; rec++) {
    sobj = SHPReadObject(sIN, rec);
    if (sobj->nSHPType < 1 || sobj->nVertices < 1) {
      SHPDestroyObject(sobj);
      continue;
    }

    i = (sobj->dfYMin - MINB[1]) / dy;
    j = (sobj->dfXMin - MINB[0]) / dx;
    u = (sobj->dfYMax - MINB[1]) / dy;
    v = (sobj->dfXMax - MINB[0]) / dx;

    if ( i >= NCOL ) i = NCOL - 1;
    if ( j >= NROW ) j = NROW - 1;
    if ( u >= NCOL ) u = NCOL - 1;
    if ( v >= NROW ) v = NROW - 1;

    if (i != u || j != v) {
      if (abs(i-u) < 2 && abs(j-v) < 2)
        indx = BestCell(sobj, MINB, dx, dy, NROW, NCOL);
      else
        indx = NROW * NCOL;
    } else
        indx = i*NCOL+j;

    cells[indx].count++;

    if (dowrite) {
      if (gcell && indx == NROW*NCOL)
        WriteObjectToFile(dIN, sobj, gcell, indx, FOUT);
      else
        WriteObjectToFile(dIN, sobj, &cells[indx], indx, FOUT);
    }

    SHPDestroyObject(sobj);
  }

  /*  Update each cell extents with the actual from the shape file */
  /*  and close all the open files.                                */

  if (dowrite)
    for (i=0; i<NROW*NCOL+1; i++) {
  
      if (cells[i].sH && cells[i].count) {
        SHPGetInfo(cells[i].sH, &nEnt2, &sType, MINB, MAXB);
        cells[i].xmin = MINB[0];
        cells[i].ymin = MINB[1];
        cells[i].xmax = MAXB[0];
        cells[i].ymax = MAXB[1];
      }
  
      if (cells[i].dH) DBFClose(cells[i].dH);
      if (cells[i].sH) SHPClose(cells[i].sH);
    }

  if (dostats) {
    printf("Shapefile Type: %s   # of Shapes: %d\n\n", SHPTypeName(sType), nEnt);
    printf("File Bounds: (%12.6f,%12.6f)\n",   MINB[0], MINB[1]);
    printf("         to  (%12.6f,%12.6f)\n\n", MAXB[0], MAXB[1]);
    printf("Divided into %d rows X %d columns (dx, dy) = (%12.6f,%12.6f)\n\n", NROW, NCOL, dx, dy);
    printf(" Index   Cell     Count      XMin         YMin         YMax         YMax\n");
  
    for (i=0; i<NROW; i++)
      for (j=0; j<NCOL; j++) {
        total += cells[i*NCOL+j].count;
        printf("%6d [%3d,%3d] %6d %12.6f %12.6f %12.6f %12.6f\n", i*NCOL+j, i, j, cells[i*NCOL+j].count, 
                cells[i*NCOL+j].xmin, cells[i*NCOL+j].ymin,
                cells[i*NCOL+j].xmax, cells[i*NCOL+j].ymax);
      }
  
    total += cells[NROW*NCOL].count;
    printf("%6d [ global] %6d %12.6f %12.6f %12.6f %12.6f\n", NROW*NCOL, cells[NROW*NCOL].count, 
            cells[NROW*NCOL].xmin, cells[NROW*NCOL].ymin,
            cells[NROW*NCOL].xmax, cells[NROW*NCOL].ymax);
  
    printf("               --------\n               %8d\n", total);
  }

  DBFClose(dIN);
  SHPClose(sIN);

  if (dostats && !dowrite && quadtree_max > 0) 
  {
      printf("\n\nWARNING: Cannot split tiles (--quadtree) in 'no-write' mode.\n\n");
  }
  else if (dowrite && quadtree_max > 0 && (maxdepth < 0 || maxdepth > 1))
  {
      /* Reprocess any tile that contains more objects than the
       * specified maximum. This will result in a quadtree-type of
       * organization
       */
      for (i=0; i<NROW; i++) {

          for (j=0; j<NCOL; j++) {
              if (dowrite && cells[i*NCOL+j].count > quadtree_max) 
              {
                  char *src_fname, *dst_fname;
                  int err;

                  src_fname = BuildTileFilename(FOUT, i*NCOL+j);
                  dst_fname = strdup(src_fname);
                  strcat(src_fname, ".shp"); /* Yes this is safe, buffer is large enough */
                  if (cells[i*NCOL+j].count == nEnt)
                  {
                      printf("File: %s can not be tiled any farther all src objects are being output to the same file\n", src_fname);
                  }
                  else
                  {
                      err = ProcessFile(src_fname, dst_fname, NROW, NCOL, dostats, dowrite, square_ext, quadtree_max, gcell, maxdepth-1);
                      if (err && err != 99999)
                          return err;
                      else if (err == 0) {

                          /* We don't need the original .shp/.shx/.dbf any more */
                          if (unlink(src_fname))
                              printf("Could not remove file: %s\n", src_fname);
                          strcpy(src_fname + strlen(src_fname) - 4, ".shx");
                          if (unlink(src_fname))
                              printf("Could not remove file: %s\n", src_fname);
                          strcpy(src_fname + strlen(src_fname) - 4, ".dbf");
                          if (unlink(src_fname))
                              printf("Could not remove file: %s\n", src_fname);
                      }
                  }
                  free(src_fname);
                  free(dst_fname);
              } 
          }
      }
  }

  free(cells);

  return 0;
}



int main( int argc, char ** argv )
{
  char         *fin, *FOUT=NULL;
  int           NROW = 2, NCOL = 2;
  int           dostats = 1;
  int           dowrite = 1;
  int           square_ext = 0;
  int           quadtree_max = 0;
  int           maxdepth = -1;
  char          c;
  int           err = 0;
  CELL         *gcell;

  static struct option long_options[] = {
    {"help",       no_argument,       0, 'h'},
    {"version",    no_argument,       0, 'v'},
    {"debug",      no_argument,       0, 'd'},
    {"no-stats",   no_argument,       0, 's'},
    {"no-write",   no_argument,       0, 'w'},
    {"row",        required_argument, 0, 'r'},
    {"col",        required_argument, 0, 'c'},
    {"square-ext", no_argument,       0, 'e'},
    {"quadtree",   required_argument, 0, 'q'},
    {"maxdepth",   required_argument, 0, 'm'},
    {0,0,0,0}
  };

  if (argc > 1) {
    while (1) {
      int option_index = 0;

      c = getopt_long( argc, argv, "hvdswr:c:eq:m:",
                       long_options, &option_index);

      /* Detect the end of options. */
      if (c == -1)
        break;

      switch (c) {
        case 0:
          break;
        case 'h':
          Usage();
          break;
        case 'v':
          printf("%s\n", version);
          exit(0);
          break;
        case 'd':
          DEBUG = !DEBUG;
          break;
        case 's':
          dostats = 0;
          break;
        case 'w':
          dowrite = 0;
          break;
        case 'r':
          NROW = strtol(optarg, NULL, 10);
          if (NROW < 1) {
            printf("Error: row must be greater than 0\n");
            err++;
          }
          break;
        case 'c':
          NCOL = strtol(optarg, NULL, 10);
          if (NCOL < 1) {
            printf("Error: col must be greater than 0\n");
            err++;
          }
          break;
        case 'e':
          square_ext = 1;
          break;
        case 'q':
          quadtree_max = strtol(optarg, NULL, 10);
          if (quadtree_max < 1) {
            printf("Error: quadtree threshold must be greater than 0\n");
            err++;
          }
          break;
        case 'm':
          maxdepth = strtol(optarg, NULL, 10);
          if (maxdepth < 1) {
            printf("Error: maxdepth value must be greater than 0\n");
            err++;
          }
          break;
        case '?':
          break;
        default:
          abort();
      }
    }
  } else
    Usage();

  if (err || !((argc == optind+2) || (argc == optind+1 && !dowrite)) )
    Usage();

  fin  = argv[optind++];
  if (dowrite)
    FOUT = argv[optind];

#if ONE_GLOBAL_CELL
  gcell = (CELL *) calloc(1, sizeof(CELL));
  assert(gcell);
#else
  gcell = NULL;
#endif
  
  err = ProcessFile(fin, FOUT, NROW, NCOL, dostats, dowrite, square_ext, quadtree_max, gcell, maxdepth);

#if ONE_GLOBAL_CELL
  if (dostats) {
    if (gcell->sH) SHPGetInfo(gcell->sH, &gcell->count, NULL, NULL, NULL);
    printf("Global cell contains: %d objects.\n", gcell->count);
  }
  if (gcell->dH) DBFClose(gcell->dH);
  if (gcell->sH) SHPClose(gcell->sH);
  free(gcell);
#endif

  return err;
}
