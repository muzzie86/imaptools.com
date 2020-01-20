/*********************************************************************
*
*  $Id: geo-sqlite.c,v 1.1 2008/09/30 18:30:20 woodbri Exp $
*
*  geo-sqlite
*  Copyright 2008, Stephen Woodbridge, All rights Reserved
*  Redistribition without written permission strictly prohibited
*
**********************************************************************
*
*  geo-sqlite.c
*
*  $Log: geo-sqlite.c,v $
*  Revision 1.1  2008/09/30 18:30:20  woodbri
*  Adding geo-sqlite.c and tgr2rgeo-sqlite.c, updates to Makefile for same.
*  Filter out tileindexes when loading data to pg.
*
*
**********************************************************************
*
* This utility will open the specified database and create schema if needed
* Then it will load all the rgeo Streets.dbf files into it.
*
* This the data is stored into FTS3 tables. Tokens might be added for
* fuzzy searching ot these might be added to a separate related column.
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
#include <fts3_tokenizer.h>

#define MAX(a,b) ((a)>(b)?(a):(b))
#define MIN(a,b) ((a)<(b)?(a):(b))

#define ERR_LOAD_DBF -1
#define ERR_LOAD_INDEX -2
#define ERR_DBF_SQLPREP -3
#define ERR_DBFOPEN_FAILED -4
#define ERR_DBF_SQLRESET -5
#define ERR_DBF_SQLEXEC -6
#define ERR_DBF_SQLBIND -7
#define ERR_DBF_SQLSTEP -8
#define ERR_DBF_SQLFINALIZE -9
#define ERR_DBF_SQLCOMMIT -10

int DEBUG = 0;


void Usage()
{
    printf("Usage: geo-sqlite name.db \"string to tokenize\"\n");
    exit(1);
}


void checkNewDB(sqlite3 *db)
{
    sqlite3_stmt    *stmt;
    char    *zErrMsg;
    char    *sql;
    int     rc;
    int     tc;

    sql = "select count(*) from sqlite_master;";
    rc = sqlite3_prepare(db, sql, strlen(sql), &stmt, NULL);
    if( rc!=SQLITE_OK ){
    	fprintf(stderr, "SQL: %s\n", sql);
        fprintf(stderr, "SQL error: %s\n", sqlite3_errmsg(db));
        exit(1);
    }

    rc = sqlite3_step(stmt);
    if( rc!=SQLITE_ROW ){
        fprintf(stderr, "SQL: %s\n", sql);
        fprintf(stderr, "Failed to return a result!\n");
        exit(1);
    }

    tc = sqlite3_column_int(stmt, 0);
    if( tc == 0 ) {
        /* new db without tables, so add the schema */
        sql = "create virtual table geocode using fts3 (record, tokenize porter);";
        rc = sqlite3_exec(db, sql, NULL, NULL, &zErrMsg);
        if( rc!=SQLITE_OK ){
            fprintf(stderr, "SQL: %s\n", sql);
            fprintf(stderr, "SQL error: %s\n", zErrMsg);
            exit(1);
        }
    }
    sqlite3_finalize(stmt);
}

/*
    woodbri@carto:~/dev/navteq$ dbfdump -info /u/data/rgeo-tgr3/01/01001/Streets
    Filename:       /u/data/rgeo-tgr3/01/01001/Streets.dbf
    Version:        0x03 (ver. 3)
    Num of records: 7268
    Header length:  801
    Record length:  591
    Last change:    1995/7/26
    Num fields:     24
    Field info:
    Num     Name            Type    Len     Decimal
    1.      LINK_ID         N       10      0
    2.      NAME            C       80      0
    3.      L_REFADDR       N       10      0
    4.      L_NREFADDR      N       10      0
    5.      L_SQLNUMF       C       12      0
    6.      L_SQLFMT        C       16      0
    7.      L_CFORMAT       C       16      0
    8.      R_REFADDR       N       10      0
    9.      R_NREFADDR      N       10      0
    10.     R_SQLNUMF       C       12      0
    11.     R_SQLFMT        C       16      0
    12.     R_CFORMAT       C       16      0
    13.     L_AC5           C       35      0
    14.     L_AC4           C       35      0
    15.     L_AC3           C       35      0
    16.     L_AC2           C       35      0
    17.     L_AC1           C       35      0
    18.     R_AC5           C       35      0
    19.     R_AC4           C       35      0
    20.     R_AC3           C       35      0
    21.     R_AC2           C       35      0
    22.     R_AC1           C       35      0
    23.     L_POSTCODE      C       11      0
    24.     R_POSTCODE      C       11      0
*/

int loadStreets(sqlite3 *db, char *fin)
{
    sqlite3_stmt    *stmt;
    int         rc;
    int         i, j;
    int         cnt;
    char        *zErrMsg;
    char        *sql;
    char        *doc;
    char        *f[14];
    DBFHandle   dH;

    sql = "insert into geocode (record) values (?);";
    rc = sqlite3_prepare(db, sql, strlen(sql), &stmt, NULL);
    if( rc != SQLITE_OK ){
        fprintf(stderr, "SQL prepare error (%s)\n", sql);
        return ERR_DBF_SQLPREP;
    }

    dH = DBFOpen(fin, "rb");
    if (!dH) {
        fprintf(stderr, "Failed to open dbf file %s\n", fin);
        return ERR_DBFOPEN_FAILED;
    }

    cnt = DBFGetRecordCount(dH);
    
    rc = sqlite3_exec(db, "BEGIN;\n", NULL, NULL, &zErrMsg);
    if( rc!=SQLITE_OK ){
        fprintf(stderr, "SQL BEGIN; error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
        return ERR_DBF_SQLCOMMIT;
    }
    for (i=0; i<cnt; i++) {
        f[ 0] = strdup(DBFReadStringAttribute(dH, i,  0)); /* LINK_ID */
        f[ 1] = strdup(DBFReadStringAttribute(dH, i,  1)); /* LINK_ID */
        f[ 2] = strdup(DBFReadStringAttribute(dH, i, 12)); /* LINK_ID */
        f[ 3] = strdup(DBFReadStringAttribute(dH, i, 13)); /* LINK_ID */
        f[ 4] = strdup(DBFReadStringAttribute(dH, i, 14)); /* LINK_ID */
        f[ 5] = strdup(DBFReadStringAttribute(dH, i, 15)); /* LINK_ID */
        f[ 6] = strdup(DBFReadStringAttribute(dH, i, 16)); /* LINK_ID */
        f[ 7] = strdup(DBFReadStringAttribute(dH, i, 17)); /* LINK_ID */
        f[ 8] = strdup(DBFReadStringAttribute(dH, i, 18)); /* LINK_ID */
        f[ 9] = strdup(DBFReadStringAttribute(dH, i, 19)); /* LINK_ID */
        f[10] = strdup(DBFReadStringAttribute(dH, i, 20)); /* LINK_ID */
        f[11] = strdup(DBFReadStringAttribute(dH, i, 21)); /* LINK_ID */
        f[12] = strdup(DBFReadStringAttribute(dH, i, 22)); /* LINK_ID */
        f[13] = strdup(DBFReadStringAttribute(dH, i, 23)); /* LINK_ID */
        doc = sqlite3_mprintf(
            /*
            "%s | %s | %s | %s | %s | %s | %s | %s | %s | %s | %s | %s | %s | %s",
            */
            "%s|%s|%s|%s|%s|%s|%s|%s|%s|%s|%s|%s|%s|%s",
            f[0], f[1], f[2], f[3], f[4], f[5], f[6], f[7], f[8], f[9],
            f[10], f[11], f[12], f[13] );

        for (j=0; j<14; j++) free(f[j]);

        if (i <10) printf("%s\n", doc);

        rc = sqlite3_bind_text(stmt, 1, doc, strlen(doc), SQLITE_STATIC);
        if( rc!=SQLITE_OK ) {
            fprintf(stderr, "SQL bind_text error\n");
            return ERR_DBF_SQLBIND;
        }

        rc = sqlite3_step(stmt);
        if( rc != SQLITE_DONE && rc != SQLITE_ROW ) {
            fprintf(stderr, "SQL step error (%d)\n", rc);
            return ERR_DBF_SQLSTEP;
        }

        rc = sqlite3_reset(stmt);
        if ( rc!=SQLITE_OK ) {
            fprintf(stderr, "SQL reset error (%d)\n", rc);
            return ERR_DBF_SQLRESET;
        }

        sqlite3_free(doc);
    }

    rc = sqlite3_finalize(stmt);
    if ( rc!=SQLITE_OK ) {
        fprintf(stderr, "SQL finalize error (%d)\n", rc);
        return ERR_DBF_SQLFINALIZE;
    }
    
    rc = sqlite3_exec(db, "COMMIT;\n", NULL, NULL, &zErrMsg);
    if( rc!=SQLITE_OK ){
        fprintf(stderr, "SQL COMMIT; error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
        return ERR_DBF_SQLCOMMIT;
    }

    DBFClose(dH);
    
    return cnt;
}


int getTokenizer(sqlite3 *db, char *zName, const sqlite3_tokenizer_module **pp)
{
    int rc;
    sqlite3_stmt *pStmt;
    const char zSql[] = "select fts3_tokenizer(?)";

    *pp = 0;
    rc = sqlite3_prepare(db, zSql, strlen(zSql), &pStmt, 0);
    if (rc != SQLITE_OK)
        return rc;

    sqlite3_bind_text(pStmt, 1, zName, -1, SQLITE_STATIC);
    if ( SQLITE_ROW == sqlite3_step(pStmt) ) {
        if ( sqlite3_column_type(pStmt, 0) == SQLITE_BLOB ) 
            memcpy(pp, sqlite3_column_blob(pStmt, 0), sizeof(*pp));
    }

    return sqlite3_finalize(pStmt);
}


int main( int argc, char **argv ) {
    sqlite3 *db;

    sqlite3_tokenizer *ppTokenizer;
    const sqlite3_tokenizer_module *porter;
    sqlite3_tokenizer_cursor *ppCursor;
    
    const char    *token;
    char    *tt;
    int     pnBytes;
    int     piStartOffset;
    int     piEndOffset;
    int     piPosition;
    
    static char buf[2048];

    char    *zErrMsg = 0;
    char    *sql;
    int     rc;
    int     i;
    int     ret = 0;
    time_t  s0, s1;

    if (argc < 3)
        Usage();

    s0 = time(NULL);

    /* open our working database that we will load everything into */
    rc = sqlite3_open(argv[1], &db);
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

    fprintf(stderr, "sqlite3 version: %s\n", sqlite3_libversion());

    /* check if this is a new database and add the required schema */
    checkNewDB(db);

    rc = getTokenizer(db, "porter", &porter);
    // sqlite3Fts3PorterTokenizerModule(&porter);
    //porter = &porterTokenizerModule;
    if (!porter) {
        printf("Failed to get the porter tokenizer!\n");
        exit(1);
    }

    rc = porter->xCreate(0, NULL, &ppTokenizer);
    if (rc != SQLITE_OK) {
        printf("porterCreate Failed! (%d)\n", rc);
        exit(1);
    }

    for (i=2; i<argc; i++) {

        /*
        s1 = time(NULL);
        rc = loadStreets(db, argv[i]);
        s1 = time(NULL)-s1;
        printf("loadStreets: file=%s, time=%d, nrec=%d, rec/sec=%.2f\n",
                argv[i], s1, rc, (float)rc/(float)(s1?s1:1));

        */
        
        rc = porter->xOpen(ppTokenizer, argv[i], strlen(argv[i]), &ppCursor);
        if (rc != SQLITE_OK) {
            printf("porterOpen Failed!\n");
            exit(1);
        }

        while ( (rc = porter->xNext(ppCursor, &token, &pnBytes,
                    &piStartOffset, &piEndOffset, &piPosition)) == SQLITE_OK ) {
            strncpy(buf, token, pnBytes);
            buf[pnBytes] = '\0';
            tt = calloc(sizeof(char), piEndOffset-piStartOffset+2);
            assert(tt);
            strncpy(tt, argv[i]+piStartOffset, piEndOffset-piStartOffset+1);
            printf("Token: '%s', %d, '%s', %d, %d, %d\n", buf, pnBytes,
                    tt, piStartOffset, piEndOffset, piPosition);
            free(tt);
        }

        if (rc != SQLITE_DONE)
            printf("rc = %d\n", rc);

        rc = porter->xClose(ppCursor);
    }

    rc = porter->xDestroy(ppTokenizer);
    
    /* clean up and exit */
err:

    sqlite3_close(db);
    printf("Total time: %d sec.\n", time(NULL)-s0);
    
    exit(ret);
}
