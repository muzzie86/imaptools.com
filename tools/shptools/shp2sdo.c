/*********************************************************************
*
*  $Id: shp2sdo.c,v 1.1 2006/09/28 15:26:15 woodbri Exp $
*
*  shp2sdo
*  Copyright 2006, Stephen Woodbridge, All rights Reserved
*  Redistribition without written permission strictly prohibited
*
**********************************************************************
*
*  shp2sdo.c
*
*  $Log: shp2sdo.c,v $
*  Revision 1.1  2006/09/28 15:26:15  woodbri
*  Adding shp2sdo.c
*
*
*  2006-09-26 sew, created from shpcpy.c
*
**********************************************************************
*
*  Utility to convert shapefiles into Oracle sdo loader files
*
**********************************************************************/

#define USE_GETOPT_LONG 1
#ifdef USE_GETOPT_LONG
#define _GNU_SOURCE
#include <getopt.h>
#else
#include <unistd.h>
#endif
#include <assert.h>
#include <errno.h>
#include <ctype.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#ifdef DEBIAN_LIBSHP
#include <shapefil.h>
#else
#include <libshp/shapefil.h>
#endif

// Debug print out
#define DBGOUT(s) (s)
//#define DBGOUT(s)

extern int DEBUG;
int DEBUG  = 0;


void Usage()
{
printf("Usage: shp2sdo [-o] <shapefile> <tablename> -g <geometry column>\n"
"               -i <id column> -n <start_id> -p -d\n"
"               -x (xmin,xmax) -y (ymin,ymax) -s <srid>\n"
"   or\n"
"       shp2sdo -r <shapefile> <outlayer> -c <ordcount> -n <start_gid> -a -d\n"
"               -x (xmin,xmax) -y (ymin,ymax)\n"
"               shapefile    - name of input shape file\n"
"                              (Do not include suffix .shp .dbf or .shx)\n"
"               tablename    - spatial table name\n"
"                              if not specified: same as input file name\n"
"   Generic options:\n"
"               -o           - Convert to object/relational format (default)\n"
"               -r           - Convert to the relational format\n"
"               -d           - store data in the control file\n"
"                              if not specified: keep data in separate files\n"
"               -x           - bounds for the X dimension\n"
"               -y           - bounds for the Y dimension\n"
"               -v           - verbose output\n"
"               -h or -?     - print this message\n"
"   Options valid for the object model only:\n"
"               -g geom_col  - Name of the column used for the SDO_GEOMETRY object\n"
"                              if not specified: GEOMETRY\n"
"               -i id_column - Name of the column used for numbering the geometries\n"
"                              if not specified, no key column will be generated\n"
"                              if specified without name, use ID\n"
"               -n start_id  - Start number for IDs\n"
"                              if not specified, start at 1\n"
"               -p           - Store points in the SDO_ORDINATES array\n"
"                              if not specified, store in SDO_POINT\n"
"               -s           - Load SRID field in geometry and metadata\n"
"                              if not specified, SRID field is NULL\n"
"               -t           - Load tolerance fields (x and y) in metadata\n"
"                              if not specified, tolerance fields are 0.00000005\n"
"               -8           - Write control file in 8i format\n"
"                              if not specified, file written in 9i format\n"
"               -f           - Write geometry data with 10 digits of precision\n"
"                              if not specified, 6 digits of precision is used\n"
"   Options valid for the relational model only:\n"
"               -c ordcount  - Number of ordinates in _SDOGOEM table\n"
"                              if not specified: 16 ordinates\n"
"               -n start_gid - Start number for GIDs\n"
"                              if not specified, start at 1\n"
"               -a           - attributes go in _SDOGEOM table\n"
"                              if not specified, attributes are in separate table\n");
  exit(0);
}

char *strtoupper(char *str)
{
    char *s = strdup(str);
    int i;
    
    for (i=0; i<strlen(s); i++)
        s[i] = toupper(s[i]);

    return s;
}

int main( int argc, char ** argv )
{
  FILE         *SQL;
  FILE         *CTL;
  FILE         *DAT;
  DBFHandle     dIN;
  SHPHandle     sIN;
  SHPObject    *sobj;
  DBFFieldType  fType;
  char         *fin;
  char         *table;
  char          fldname[12];
  char          fsql[256];
  char          fctl[256];
  char          fdat[256];
  int           c;
  int           nEnt, sType, fw, fd;
  int           i, j, rec, n;
  double        zX[2] = {0.0, 0.0};
  double        zY[2] = {0.0, 0.0};
  double        sMin[4];
  double        sMax[4];

#ifdef USE_GETOPT_LONG
  static struct option long_options[] = {
    {"help", 0, 0, 'h'},
    {0, 0, 0, 0}
  };
#endif

  int odatactl   = 0;
  int mode       = 'o';
  int verbose    = 0;
  int start_id   = 1;
  int oraformat  = 9;
  int percision  = 6;
  int ordcount   = 16;
  int start_gid  = 1;
  long tm;
  char *xextents = NULL;
  char *yextents = NULL;
  char *geom_col = "GEOMETRY";
  char *id_column = NULL;
  char *sdo_ordinates = "SDO_POINT";
  char *srid = "NULL";
  char *attributes = NULL;
  double tolerance = 0.00000005;
  double xmin = -180.0;
  double xmax =  180.0;
  double ymin =  -90.0;
  double ymax =   90.0;

    while (1) {
      int option_index = 0;
              
      /*
      c = getopt_long( argc, argv, "ordx:y:vhg:i:n:p:s:t:8fc:a:",
                       long_options, &option_index);
       */           
      c = getopt( argc, argv, "ordx:y:vhg:i:n:p:s:t:8fc:a:");
                  
      /* Detect the end of options. */
      if (c == -1)
        break;
                      
      switch (c) {
        case 'o':
          mode = 'o';
          break;
        case 'r':
          mode = 'r';
          break;
        case 'd':
          odatactl = 1;
          break;
        case 'x':
          xextents = optarg;
          break;
        case 'y':
          yextents = optarg;
          break;
        case 'v':
          verbose += 1;
          break;
        case '?':
        case 'h':
          Usage();
          break;
        case 'g':
          geom_col = optarg;
          break;
        case 'i':
          id_column = optarg;
          break;
        case 'n':
          start_id = strtol(optarg, NULL, 10);
          break;
        case 'p':
          sdo_ordinates = optarg;
          break;
        case 's':
          srid = optarg;
          break;
        case 't':
          tolerance = strtod(optarg, NULL);
          break;
        case '8':
          oraformat = 8;
          break;
        case 'f':
          percision = 10;
          break;
        case 'c':
          ordcount = strtol(optarg, NULL, 10);
          break;
        case 'a':
          start_gid = strtol(optarg, NULL, 10);
          break;
        default:
          Usage();
        }
  }

  if ((argc - optind) != 2) Usage();

  fin   = argv[optind];
  table = argv[optind+1];

  /* do some sanity checking of arguments */

  if (mode == 'r') {
      printf("Sorry relational format is not currently supported\n");
      exit(1);
  }

  if (xextents) {
    if (sscanf(xextents, "(%f,%f)", &xmin, &xmax) == EOF) {
        printf("Error parsing parameter -x '%s' as (%%f,%%f)\n", xextents);
        exit(1);
    }
  }

  if (yextents) {
    if (sscanf(yextents, "(%f,%f)", &ymin, &ymax) == EOF) {
        printf("Error parsing parameter -y '%s' as (%%f,%%f)\n", xextents);
        exit(1);
    }
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

  if (sType%10 != 1 && sType%10 != 3) {
      printf("Only point and arc type shapefiles are currently supported!\n");
      exit(1);
  }

  strcpy(fsql, table);
  strcat(fsql, ".sql");
  if (!(SQL  = fopen(fsql, "wb"))) {
      printf("Failed to create %s for write\n", fsql);
      exit(1);
  }

  strcpy(fctl, table);
  strcat(fctl, ".ctl");
  if (!(CTL  = fopen(fctl, "wb"))) {
      printf("Failed to create %s for write\n", fctl);
      exit(1);
  }

  strcpy(fdat, table);
  strcat(fdat, ".dat");
  if (!(DAT  = fopen(fdat, "wb"))) {
      printf("Failed to create %s for write\n", fdat);
      exit(1);
  }

  /* this return a strdup of the input string, so free it */
  table = strtoupper(table);

  /* write the .sql and .ctl file */
  fprintf(CTL, "LOAD DATA\n");
  fprintf(CTL, " INFILE %s\n", fdat);
  fprintf(CTL, " TRUNCATE\n");
  fprintf(CTL, " CONTINUEIF NEXT(1:1) = '#'\n");
  fprintf(CTL, " INTO TABLE %s\n", table);
  fprintf(CTL, " FIELDS TERMINATED BY '|'\n");
  fprintf(CTL, " TRAILING NULLCOLS (\n");
  
  fprintf(SQL, "-- %s\n", fsql);
  fprintf(SQL, "--\n");
  fprintf(SQL, "--   This script creates the spatial table.\n");
  fprintf(SQL, "--\n");
  fprintf(SQL, "--   Execute this script before attempting to use SQL*Loader\n");
  fprintf(SQL, "--   to load the %s file\n", fctl);
  fprintf(SQL, "--\n");
  fprintf(SQL, "--   This script will also populate the USER_SDO_GEOM_METADATA table.\n");
  fprintf(SQL, "--   Loading the .ctl file will populate %s table.\n", table);
  fprintf(SQL, "--\n");
  fprintf(SQL, "--   To load the .ctl file, run SQL*Loader as follows\n");
  fprintf(SQL, "--   substituting appropriate values\n");
  fprintf(SQL, "--       sqlldr username/password %s\n", fctl);
  fprintf(SQL, "--\n");
  fprintf(SQL, "--   After the data is loaded in the %s table, you should\n", table);
  fprintf(SQL, "--   migrate polygon data and create the spatial index\n");
  fprintf(SQL, "--\n");
  tm = time(NULL);
  fprintf(SQL, "--   CreationDate : %s\n", ctime(&tm));
  fprintf(SQL, "--   Copyright 2006, Where2getit, Inc.\n");
  fprintf(SQL, "--   Copyright 2006, Stephen Woodbridge\n");
  fprintf(SQL, "--   All rights reserved\n");
  fprintf(SQL, "--\n");
  fprintf(SQL, "DROP TABLE %s;\n\n", table);
  fprintf(SQL, "CREATE TABLE %s (\n", table);
  for (i=0; i<DBFGetFieldCount(dIN); i++) {
    fType = DBFGetFieldInfo(dIN, i, fldname, &fw, &fd);
    if (fType == FTInteger) {
      fprintf(CTL, "   %s,\n", fldname);
      fprintf(SQL, "  %s \tNUMBER,\n", fldname);
    } else if (fType == FTString) {
      fprintf(CTL, "   %-15s NULLIF %s = BLANKS,\n", fldname, fldname);
      fprintf(SQL, "  %s \tVARCHAR2(%d),\n", fldname, fw);
    } else if (fType == FTDouble) {
      fprintf(CTL, "   %s,\n", fldname);
      fprintf(SQL, "   %s \tFLOAT,\n", fldname);
    } else {
      printf("Invalid or unknown column type in shapefile (%s).\n", fldname);
      exit(1);
    }
  }
  fprintf(CTL, "   %s COLUMN OBJECT\n", geom_col);
  fprintf(CTL, "   (\n");
  fprintf(CTL, "     SDO_GTYPE       INTEGER EXTERNAL,\n");
  fprintf(CTL, "     SDO_SRID        INTEGER EXTERNAL,\n");
  if (sType%10 == 1) {  /* point objects here */
  fprintf(CTL, "     SDO_POINT COLUMN OBJECT\n");
  fprintf(CTL, "       (X            FLOAT EXTERNAL,\n");
  fprintf(CTL, "        Y            FLOAT EXTERNAL)\n");
  }
  else if (sType%10 == 3) {  /* line/arc objects here */
  fprintf(CTL, "     SDO_ELEM_INFO   VARRAY TERMINATED BY '|/'\n");
  fprintf(CTL, "       (X            FLOAT EXTERNAL),\n");
  fprintf(CTL, "     SDO_ORDINATES   VARRAY TERMINATED BY '|/'\n");
  fprintf(CTL, "       (X            FLOAT EXTERNAL)\n");
  }
  fprintf(CTL, "   )\n");
  fprintf(CTL, ")\n");

  fprintf(SQL, "  %s \tMDSYS.SDO_GEOMETRY);\n\n", geom_col);
  fprintf(SQL, "DELETE FROM USER_SDO_GEOM_METADATA\n");
  fprintf(SQL, "  WHERE TABLE_NAME = '%s' AND COLUMN_NAME = '%s' ;\n\n", table, geom_col);
  fprintf(SQL, "INSERT INTO USER_SDO_GEOM_METADATA (TABLE_NAME, COLUMN_NAME, DIMINFO, SRID)\n");
  fprintf(SQL, "  VALUES ('%s', '%s',\n", table, geom_col);
  fprintf(SQL, "    MDSYS.SDO_DIM_ARRAY\n");
  fprintf(SQL, "      (MDSYS.SDO_DIM_ELEMENT('X', %.9f, %.9f, %.9f),\n", xmin, xmax, tolerance);
  fprintf(SQL, "       MDSYS.SDO_DIM_ELEMENT('Y', %.9f, %.9f, %.9f)\n", ymin, ymax, tolerance);
  fprintf(SQL, "     ),\n");
  fprintf(SQL, "     %s);\n", srid);
  fprintf(SQL, "COMMIT;\n");

  for (i=0; i<nEnt; i++) {
    rec = i;
    fprintf(DAT, " ");
    for (j=0; j<DBFGetFieldCount(dIN); j++) {
      fType = DBFGetFieldInfo(dIN, j, NULL, &fw, NULL);
      switch (fType) {
        case FTString:
          if (strlen(DBFReadStringAttribute( dIN, rec, j)) == 0)
            fprintf(DAT, "%*s", fw, DBFReadStringAttribute( dIN, rec, j));
          else
            fprintf(DAT, "%s", DBFReadStringAttribute( dIN, rec, j));
          break;
        case FTInteger:
          fprintf(DAT, "%d", DBFReadIntegerAttribute(dIN, rec, j));
          break;
        case FTDouble:
          fprintf(DAT, "%.9f", DBFReadDoubleAttribute( dIN, rec, j));
          break;
      }
      fprintf(DAT, "|");
    }
    /* write the shape data here */
    sobj = SHPReadObject(sIN, rec);
    if (sType%10 == 1) {
        if (sobj->nSHPType < 1 || sobj->nVertices < 1)
            fprintf(DAT, " 2001|%s|%.9f|%.9f|", srid, 0.0, 0.0);
        else
            fprintf(DAT, " 2001|%s|%.9f|%.9f|", srid, sobj->padfX[0], sobj->padfY[0]);
    } else if (sType%10 == 3) {
        if (!(sobj->nSHPType < 1 || sobj->nVertices < 1)) {
            fprintf(DAT, "\n#2002|%s|\n", srid);
            fprintf(DAT, "#1|2|1|/");
            for (j=0; j<sobj->nVertices; j++) {
                if (j%2 == 0) fprintf(DAT, "\n#");
                fprintf(DAT, "%.*f|%.*f|", percision, sobj->padfX[j],
                                           percision, sobj->padfY[j]);
            }
            fprintf(DAT, "/");
        }
    } 
    SHPDestroyObject(sobj);
    fprintf(DAT, "\n");
  }

  free(table);

  DBFClose(dIN);
  SHPClose(sIN);
  fclose(SQL);
  fclose(CTL);
  fclose(DAT);
}


