/*********************************************************************
*
*  $Id: build-tiger-places.c,v 1.1 2012-06-03 14:07:37 woodbri Exp $
*
*   build-tiger-places
*  Copyright 2012, Stephen Woodbridge, All rights Reserved
*  Redistribition without written permission strictly prohibited
*
**********************************************************************
*
*  build-tiger-places.c
*
*  $Log: build-tiger-places.c,v $
*  Revision 1.1  2012-06-03 14:07:37  woodbri
*  Adding new utility program build-tiger-places to build a sqlite3 db of
*  all the Tiger faces, cousub, concity, and place records to be used with
*  tgr2pagc-sqlite to deal with county adjacency issues.
*
*
*
**********************************************************************
*
* This utility is part of tgr2pagc suite. I builds the tiger-places.db
* file and requires the following files:
*   *_faces.*
*   *_cousub.dbf
*   *_place.dbf
*   *_concity.dbf
*
* It will read this files into a SQLite DB file that wiil be used by
* tgr2pagc-sqlite when loading data into PAGC.
* 
**********************************************************************
*/

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <time.h>
#include <math.h>
#include <shapefil.h>
#include <sqlite3.h>
#include "dbf2sqlite.h"


#define YEAR2008 0
#define YEAR2010 0
#define YEAR2011 1

#define ERR_LOAD_DBF -1
#define ERR_LOAD_INDEX -2

int DEBUG = 1;


void Usage()
{
    printf("Usage: build-tiger-places -c dbfile table dbffile\n");
    printf("       build-tiger-places -a dbfile table dbffile\n");
    printf("       build-tiger-places -i dbfile\n");
    printf("       -c  Will drop and create table\n");
    printf("       -a  Will append to existing table\n");
    printf("       -i  Will create indexes on table(s)\n");
#if YEAR2008
    printf("       Built for YEAR2008 Tiger file formats.\n");
#elif YEAR2010
    printf("       Built for YEAR2010 Tiger file formats.\n");
#elif YEAR2011
    printf("       Built for YEAR2011 Tiger file formats.\n");
#endif
    exit(1);
}


int createTable(sqlite3 *db, char *table, int ncol, const char **cols, char *recno)
{
    static char sql[16*1024];
    char        *zErrMsg;
    int     i;
    int     rc;

    strcpy(sql, "create table ");
    strcat(sql, table);
    strcat(sql, " (");

    for (i=0; i<ncol; i++) {
        if (!strcasecmp(cols[i], "tlid"))
            sprintf(sql+strlen(sql), "%s integer,", cols[i]);
        else
            sprintf(sql+strlen(sql), "%s text,", cols[i]);
    }
    
    if (recno) {
        sprintf(sql+strlen(sql), "%s integer", recno);
    }
    else {
        sql[strlen(sql)-1] = '\0';
    }
    strcat(sql, ");\n");

    if (DEBUG)
        printf(sql);

    rc = sqlite3_exec(db, sql, NULL, NULL, &zErrMsg);
    if( rc != SQLITE_OK ){
        fprintf(stderr, "SQL: %s\n", sql);
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
        return ERR_DBF_SQLEXEC;
    }

    return 0;
}


int loadCousub(sqlite3 *db, char *fin, int create)
{
    int     rc;
    char        *zErrMsg;
    char        *sql;
    const char  *cols11[] = {"GEOID","NAME"};
    const char  *cols10[] = {"GEOID10","NAME10"};
    const char  *cols09[] = {"COSBIDFP","NAME"};
    
    /* create the tables */

#if YEAR2008
    rc = loadDBF(db, fin, "cousub", 2, cols09, NULL, create);
#elif YEAR2010
    rc = loadDBFrename(db, fin, "cousub", 2, cols10, cols09, NULL, create);
#elif YEAR2011
    rc = loadDBFrename(db, fin, "cousub", 2, cols11, cols09, NULL, create);
#endif
    if (rc) {
        fprintf(stderr, "Failed to load DBF file (%s)\n", fin);
        return ERR_LOAD_DBF;
    }

    return 0;
}


int loadConcity(sqlite3 *db, char *fin, int create)
{
    int     rc;
    char        *zErrMsg;
    char        *sql;
    const char  *cols11[] = {"GEOID","NAME"};
    const char  *cols10[] = {"GEOID10","NAME10"};
    const char  *cols09[] = {"CCTYIDFP","NAME"};
    
    /* create the tables */

#if YEAR2008
    rc = loadDBF(db, fin, "concty", 2, cols09, NULL, create);
#elif YEAR2010
    rc = loadDBFrename(db, fin, "concty", 2, cols10, cols09, NULL, create);
#elif YEAR2011
    rc = loadDBFrename(db, fin, "concty", 2, cols11, cols09, NULL, create);
#endif
    if (rc) {
        fprintf(stderr, "Failed to load DBF file (%s)\n", fin);
        return ERR_LOAD_DBF;
    }

    return 0;
}


int loadFaces(sqlite3 *db, char *fin, int create)
{
    int     rc;
    char        *zErrMsg;
    char        *sql;
    const char  *cols11[] = {"TFID","STATEFP10","COUNTYFP10","COUSUBFP",
        "PLACEFP","CONCTYFP","LWFLAG","ZCTA5CE10"};
    const char  *cols10[] = {"TFID","STATEFP10","COUNTYFP10","COUSUBFP10",
        "PLACEFP10","CONCTYFP10","LWFLAG","ZCTA5CE10"};
    const char  *cols09[] = {"TFID","STATEFP","COUNTYFP","COUSUBFP",
        "PLACEFP","CONCTYFP","LWFLAG","ZCTA5CE"};
    
    /* create the tables */

#if YEAR2008
    rc = loadDBF(db, fin, "faces", 8, cols09, NULL, create);
#elif YEAR2010
    rc = loadDBFrename(db, fin, "faces", 8, cols10, cols09, NULL, create);
#elif YEAR2011
    rc = loadDBFrename(db, fin, "faces", 8, cols11, cols09, NULL, create);
#endif
    if (rc) {
        fprintf(stderr, "Failed to load DBF file (%s)\n", fin);
        return ERR_LOAD_DBF;
    }

    return 0;
}


int loadPlace(sqlite3 *db, char *fin, int create)
{
    int     rc;
    char        *zErrMsg;
    char        *sql;
    const char  *cols11[] = {"GEOID","NAME"};
    const char  *cols10[] = {"GEOID10","NAME10"};
    const char  *cols09[] = {"PLCIDFP","NAME"};
    
    /* create the tables */

#if YEAR2008
    rc = loadDBF(db, fin, "place", 2, cols09, NULL, create);
#elif YEAR2010
    rc = loadDBFrename(db, fin, "place", 2, cols10, cols09, NULL, create);
#elif YEAR2011
    rc = loadDBFrename(db, fin, "place", 2, cols11, cols09, NULL, create);
#endif
    if (rc) {
        fprintf(stderr, "Failed to load DBF file (%s)\n", fin);
        return ERR_LOAD_DBF;
    }

    return 0;
}



int main( int argc, char **argv ) {
    sqlite3 *db;
    char    *zErrMsg = 0;
    int     rc;
    time_t  s0, s1;
    char    mode;
    char    *sql;
    int     ret = 0;

    if (argc < 3)
        Usage();

    mode = argv[1][1];
    if (argv[1][0] != '-' ||
        !(mode == 'c' || mode == 'a' || mode == 'i') ||
        (mode != 'i' && argc < 4)) Usage();

    s0 = time(NULL);

    rc = sqlite3_open(argv[2], &db);
    if( rc ){
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        exit(1);
    }
    rc = sqlite3_exec(db, "PRAGMA page_size = 8192;PRAGMA cache_size = 8000;",
            NULL, NULL, &zErrMsg);
    if ( rc != SQLITE_OK ) {
        if (zErrMsg) {
            fprintf(stderr, "Setting PRAGMAs Failed: %s\n", zErrMsg);
            sqlite3_free(zErrMsg);
        }
        exit(1);
    }
        
    if (mode == 'i') {

        sql = "create unique index place_plcidfp_idx on place (PLCIDFP);\n";
        rc = sqlite3_exec(db, sql, NULL, NULL, &zErrMsg);
        if( rc!=SQLITE_OK ){
            fprintf(stderr, "SQL: %s", sql);
            fprintf(stderr, "SQL error: %s\n", zErrMsg);
        }

        sql = "create unique index faces_tfid_idx on faces (TFID);\n";
        rc = sqlite3_exec(db, sql, NULL, NULL, &zErrMsg);
        if( rc!=SQLITE_OK ){
            fprintf(stderr, "SQL: %s", sql);
            fprintf(stderr, "SQL error: %s\n", zErrMsg);
        }

        sql = "create unique index concty_cctyidfp_idx on concty (CCTYIDFP);\n";
        rc = sqlite3_exec(db, sql, NULL, NULL, &zErrMsg);
        if( rc!=SQLITE_OK ){
            fprintf(stderr, "SQL: %s", sql);
            fprintf(stderr, "SQL error: %s\n", zErrMsg);
        }

        sql = "create unique index cousub_cosbidfp_idx on cousub (COSBIDFP);\n";
        rc = sqlite3_exec(db, sql, NULL, NULL, &zErrMsg);
        if( rc!=SQLITE_OK ){
            fprintf(stderr, "SQL: %s", sql);
            fprintf(stderr, "SQL error: %s\n", zErrMsg);
        }

    }
    else {
        int i;
        char *tab = strdup(argv[3]);
        for (i=0; i<strlen(tab); i++) tab[i] = tolower(tab[i]);
        int create = (mode=='c')?1:0;

        if (!strcmp(tab, "cousub"))
            ret = loadCousub(db, argv[4], create);
        else if (!strcmp(tab, "concity"))
            ret = loadConcity(db, argv[4], create);
        else if (!strcmp(tab, "faces"))
            ret = loadFaces(db, argv[4], create);
        else if (!strcmp(tab, "place"))
            ret = loadPlace(db, argv[4], create);
        else {
            fprintf(stderr, "Error: table must be one of [faces|cousub|concity|place]!\n");
            sqlite3_close(db);
            exit(1);
        }
        free(tab);
    }

    sqlite3_close(db);

    printf("Total time: %d sec.\n", time(NULL)-s0);
    
    exit(ret);
}
