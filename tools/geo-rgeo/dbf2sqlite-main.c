/*********************************************************************
*
*  $Id: dbf2sqlite-main.c,v 1.2 2012-06-03 14:02:21 woodbri Exp $
*
*  dbf2sqlite
*  Copyright 2008, Stephen Woodbridge, All rights Reserved
*  Redistribition without written permission strictly prohibited
*
**********************************************************************
*
*  dbf2sqlite.c
*
*  $Log: dbf2sqlite-main.c,v $
*  Revision 1.2  2012-06-03 14:02:21  woodbri
*  Changed the behavior to droptable=1 so it drops and recreates the table.
*  There is no option to append to an existing table, but it would be easy
*  to add.
*
*  Revision 1.1  2008/04/10 15:24:29  woodbri
*  Split this out of dbf2sqlite.c
*
*  Revision 1.1  2008/04/10 14:56:54  woodbri
*  Adding new files based on sqlite as underlying data store.
*
*
**********************************************************************
*
* This utility function will load DBF files into a SQLite Database
* It optionally be passed in a list of column names to be loaded.
* It returns 0 on success of an error code.
* 
**********************************************************************
*/

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <shapefil.h>
#include <sqlite3.h>
#include "dbf2sqlite.h"


void Usage()
{
    printf("Usage: dbf2sqlite database dbffile [columns]\n");
    exit(1);
}


int main( int argc, char **argv ) {
    int     rc;
    int     i;
    char    *dbname;
    char    *dbfname;
    sqlite3 *db;
    int     droptable = 1;
    char    *tablename;
    const char    **colnames = NULL;
    char    *p;
    int     ncol;

    if (argc < 3)
        Usage();

    dbname  = argv[1];
    dbfname = argv[2];

    rc = sqlite3_open(dbname, &db);
    if( rc ){
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        exit(1);
    }
    
    p = strrchr(dbfname, '/');
    if (p)
        tablename = strdup(p+1);
    else
        tablename = strdup(dbfname);
    
    for (i=0; i<strlen(tablename); i++)
        if (tablename[i] == '.') {
            tablename[i] = '\0';
            break;
        }
        else
            tablename[i] = tolower(tablename[i]);

    ncol = 0;
    if (argc>3) {
        ncol = argc-3;
        colnames = (const char **) &argv[3];
    }
        
    rc = loadDBF(db, dbfname, tablename, ncol, colnames, "RECNO", droptable);
    if (rc) {
        fprintf(stderr, "Failed to load DBF file: %s\n", dbfname);
        goto err;
    }

err:
    sqlite3_close(db);
    exit(0);
}

