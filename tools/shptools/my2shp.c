/*********************************************************************
*
*  $Id: my2shp.c,v 1.2 2004/11/14 02:04:10 woodbri Exp $
*
*  my2shp
*  Copyright 2001, Stephen Woodbridge, All rights Reserved
*  Redistribition without written permission strictly prohibited
*
**********************************************************************
*
*  my2shp.c
*
*  $Log: my2shp.c,v $
*  Revision 1.2  2004/11/14 02:04:10  woodbri
*  Changed path for shapelib.h for Debian.
*
*  Revision 1.1  2004/02/09 15:00:12  woodbri
*  Migrating unchecked in changes from RCS.
*
*
*  2002-09-22 sew, created
*
**********************************************************************
*
*  Exact records from MySQL and save as shape file(s)
*
**********************************************************************/

#include <getopt.h>
#include <assert.h>
#include <errno.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <mysql.h>
#include <shapefil.h>

#ifdef DMALLOC
#include "dmalloc.h"
#endif



void open_shape(char *file, DBFHandle *dH, SHPHandle *sH)
{

  *sH = SHPCreate( file, SHPT_POINT );
  if (!*sH) {
    printf("Failed to create shape file for %s!\n", file);
    exit(1);
  }

  *dH = DBFCreate( file );
  if (!*dH) {
    printf("Failed to create dbf file for %s!\n", file);
    exit(1);
  }

  DBFAddField( *dH, "NAME",   FTString, 125, 0 );
  DBFAddField( *dH, "STATE",  FTString,   2, 0 );
  DBFAddField( *dH, "CC",     FTString,   2, 0 );
  DBFAddField( *dH, "TYPE",   FTString,   9, 0 );

}



int main( int argc, char ** argv )
{
  MYSQL       *conn;
  MYSQL_RES   *res_set;
  MYSQL_ROW    row;

  DBFHandle    dH;
  SHPHandle    sH;
  SHPObject   *sobj;

  char        *file;
  double       X, Y;
  int          num;

  conn = mysql_init(NULL);
  if (!conn) {
    printf("mysql_init failed: %s\n", mysql_error(conn));
    exit( 1 );
  }

  conn = mysql_real_connect(conn, NULL, "scott", "tiger", "geocode", 0, NULL, 0);
  if (!conn) {
    printf("mysql_real_connect failed: %s\n", mysql_error(conn));
    exit( 1 );
  }

  if (argc > 1)
    file = argv[1];
  else
    file = "geonames1";

  file = strdup(file);

  open_shape(file, &dH, &sH);

  /***********  Read ANTARCTICA data **************/

  if (mysql_query(conn, "select NAME, LAT, LON from ANTARCTICA")) {
    printf("mysql_query failed: %s\n", mysql_error(conn));
    exit( 1 );
  }

  if ((res_set = mysql_use_result(conn)) == NULL) {
    printf("mysql_use_result failed: %s\n", mysql_error(conn));
    exit( 1 );
  }

  while ((row = mysql_fetch_row( res_set )) != NULL) {

    // printf("%s:%s:%s\n", row[0], row[1], row[2]);

    X = strtod(row[2], NULL);
    Y = strtod(row[1], NULL);

    sobj = SHPCreateSimpleObject(SHPT_POINT, 1, &X, &Y, NULL);
    num  = SHPWriteObject(sH, -1, sobj);
    SHPDestroyObject(sobj);

    DBFWriteStringAttribute(dH, num, 0, row[0]);
    DBFWriteNULLAttribute(  dH, num, 1);
    DBFWriteStringAttribute(dH, num, 2, "AY");
    DBFWriteNULLAttribute(  dH, num, 3);

  }
  mysql_free_result(res_set);

  DBFClose(dH);
  SHPClose(sH);

  file[strlen(file)-1] = file[strlen(file)-1] + 1;
  open_shape(file, &dH, &sH);

  /************ Read GNPS table and write to Shapefile **************/

  if (mysql_query(conn, "select FULL_NAME_ND, LAT, LON, CC1, DSG from GNPS")) {
    printf("mysql_query failed: %s\n", mysql_error(conn));
    exit( 1 );
  }

  if ((res_set = mysql_use_result(conn)) == NULL) {
    printf("mysql_use_result failed: %s\n", mysql_error(conn));
    exit( 1 );
  }

  while ((row = mysql_fetch_row( res_set )) != NULL) {

    // printf("%s:%s:%s\n", row[0], row[1], row[2]);

    X = strtod(row[2], NULL);
    Y = strtod(row[1], NULL);

    sobj = SHPCreateSimpleObject(SHPT_POINT, 1, &X, &Y, NULL);
    num  = SHPWriteObject(sH, -1, sobj);
    SHPDestroyObject(sobj);

    DBFWriteStringAttribute(dH, num, 0, row[0]);
    DBFWriteNULLAttribute(  dH, num, 1);
    DBFWriteStringAttribute(dH, num, 2, row[3]);
    DBFWriteStringAttribute(dH, num, 3, row[4]);

  }
  mysql_free_result(res_set);

  DBFClose(dH);
  SHPClose(sH);

  file[strlen(file)-1] = file[strlen(file)-1] + 1;
  open_shape(file, &dH, &sH);

  /************ Read STATES table and write to Shapefile **************/

  if (mysql_query(conn, "select NAME, LAT, LON, STATE, TYPE from STATES")) {
    printf("mysql_query failed: %s\n", mysql_error(conn));
    exit( 1 );
  }

  if ((res_set = mysql_use_result(conn)) == NULL) {
    printf("mysql_use_result failed: %s\n", mysql_error(conn));
    exit( 1 );
  }

  while ((row = mysql_fetch_row( res_set )) != NULL) {
  
    // printf("%s:%s:%s\n", row[0], row[1], row[2]);

    X = strtod(row[2], NULL);
    Y = strtod(row[1], NULL);

    sobj = SHPCreateSimpleObject(SHPT_POINT, 1, &X, &Y, NULL);
    num  = SHPWriteObject(sH, -1, sobj);
    SHPDestroyObject(sobj);

    DBFWriteStringAttribute(dH, num, 0, row[0]);
    DBFWriteStringAttribute(dH, num, 1, row[3]);
    DBFWriteStringAttribute(dH, num, 2, "US");
    DBFWriteStringAttribute(dH, num, 3, row[4]);

  }
  mysql_free_result(res_set);


  DBFClose(dH);
  SHPClose(sH);

  free(file);
  mysql_close(conn);

  return 0;
}
