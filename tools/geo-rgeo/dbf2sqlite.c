/*********************************************************************
*
*  $Id: dbf2sqlite.c,v 1.3 2012-06-03 14:00:20 woodbri Exp $
*
*  dbf2sqlite
*  Copyright 2008, Stephen Woodbridge, All rights Reserved
*  Redistribition without written permission strictly prohibited
*
**********************************************************************
*
*  dbf2sqlite.c
*
*  $Log: dbf2sqlite.c,v $
*  Revision 1.3  2012-06-03 14:00:20  woodbri
*  Added function loadDBFrename so dbf column names can be renamed as they
*  are loaded. Also changed the symantic for the "drop" parameter, now it
*  will drop AND recreate the database table.
*
*  Revision 1.2  2008/04/10 15:23:42  woodbri
*  Cleanup files and Makefile.
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

#define DEBUG 0

int loadDBF(sqlite3 *db, char *dbfname, char *tablename, int ncol, const char **colnames, char *recno, int drop) {
    return loadDBFrename(db, dbfname, tablename, ncol, colnames, colnames, recno, drop);
}

int loadDBFrename(sqlite3 *db, char *dbfname, char *tablename, int ncol, const char **colsin, const char **colsout, char *recno, int drop) {
    sqlite3_stmt *ppStmt;
    DBFHandle   dH;
    static char sql[16*1024];
    static char sql2[1024];
    char        *columns = NULL;
    char        *binders = NULL;
    char        fname[12];
    char        *zErrMsg = NULL;
    int         rc;
    int         wdt;
    int         dec;
    int         i, j, cnt, n;
    int         fc, fc1, ft;
    const char *pzTail;
    char        **values;
    int         *fields;

    sql[0] = '\0';
    
    dH = DBFOpen(dbfname, "rb");
    if (!dH) {
        fprintf(stderr, "Failed to open dbf file %s\n", dbfname);
        return ERR_DBFOPEN_FAILED;
    }

    strcpy(sql, "create table ");
    strcat(sql, tablename);
    strcat(sql, " (");

    fc = ncol?ncol:DBFGetFieldCount(dH);
    fc1 = recno?fc+1:fc;
    
    columns = calloc(1, fc1*12+5);
    binders = calloc(1, fc1*2+5);

    if (ncol) {
        fields = malloc(sizeof(int)*ncol);
        for (i=0; i<ncol; i++)
            fields[i] = -1;
        
        for (i=0; i<ncol; i++) {
            n = DBFGetFieldIndex(dH, colsin[i]);
            if (n == -1) {
                fprintf(stderr, "Failed to find column: %s\n", colsin[i]);
                return ERR_DBF_UNKNOWN_COL;
            }
            fields[i] = n;
            ft = DBFGetFieldInfo(dH, n, NULL, &wdt, &dec);
            switch (ft) {
                case FTString:
                    sprintf(sql+strlen(sql), "%s text(%d),", colsout[i], wdt);
                    break;
                case FTInteger:
                    sprintf(sql+strlen(sql), "%s integer,", colsout[i]);
                    break;
                case FTDouble:
                    sprintf(sql+strlen(sql), "%s double,", colsout[i]);
                    break;
                default:
                    /* skip it */
                    break;
            }
            strcat(columns, colsout[i]);
            strcat(columns, ",");
            strcat(binders, "?,");
        }
        fc = ncol;
    }
    else {
        fields = malloc(sizeof(int)*fc);
        for (i=0; i<fc; i++) {
            fields[i] = i;
            ft = DBFGetFieldInfo(dH, i, fname, &wdt, &dec);
            switch (ft) {
                case FTString:
                    sprintf(sql+strlen(sql), "%s text(%d),", fname, wdt);
                    break;
                case FTInteger:
                    sprintf(sql+strlen(sql), "%s integer,", fname);
                    break;
                case FTDouble:
                    sprintf(sql+strlen(sql), "%s double,", fname);
                    break;
                default:
                    /* skip it */
                    break;
            }
            strcat(columns, fname);
            strcat(columns, ",");
            strcat(binders, "?,");
        }
    }
    
    if (recno) {
        sprintf(sql+strlen(sql), "%s integer", recno);
        strcat(columns, recno);
        strcat(binders, "?");
    }
    else {
        sql[strlen(sql)-1] = '\0';
        columns[strlen(columns)-1] = '\0';
        binders[strlen(binders)-1] = '\0';
    }
    strcat(sql, ");\n");

    if (drop) {
        strcpy(sql2, "drop table ");
        strcat(sql2, tablename);
        strcat(sql2, ";\n");
        
        rc = sqlite3_exec(db, sql2, NULL, NULL, &zErrMsg);
        if( rc!=SQLITE_OK ){
            fprintf(stderr, "SQL: %s\n", sql2);
            fprintf(stderr, "SQL error: %s\n", zErrMsg);
            sqlite3_free(zErrMsg);
        }

        if (DEBUG)
            printf(sql);
        
        rc = sqlite3_exec(db, sql, NULL, NULL, &zErrMsg);
        if( rc != SQLITE_OK ){
            fprintf(stderr, "SQL: %s\n", sql);
            fprintf(stderr, "SQL error: %s\n", zErrMsg);
            sqlite3_free(zErrMsg);
            return ERR_DBF_SQLEXEC;
        }
    }

    /* start a transaction */
    rc = sqlite3_exec(db, "BEGIN;\n", NULL, NULL, &zErrMsg);
    if( rc != SQLITE_OK ){
        fprintf(stderr, "SQL: BEGIN;\n");
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
        return ERR_DBF_SQLEXEC;
    }

    /* create sql for insert */
    sprintf(sql, "insert into %s (%s) values (%s)",
            tablename, columns, binders);

    free(columns);
    free(binders);

    rc = sqlite3_prepare(db, sql, strlen(sql), &ppStmt, &pzTail);
    if( rc != SQLITE_OK ){
        fprintf(stderr, "SQL prepare error (%s)\n", sql);
        return ERR_DBF_SQLPREP;
    }

    values = calloc(fc, sizeof(char *));
    cnt = DBFGetRecordCount(dH);
    for (i=0; i<cnt; i++) {
        for (j=0; j<fc; j++) {
            values[j] = strdup(DBFReadStringAttribute(dH, i, fields[j]));
            rc = sqlite3_bind_text(ppStmt, j+1, values[j], strlen(values[j]), free);
            if ( rc!=SQLITE_OK ) {
                fprintf(stderr, "SQL bind_text error\n");
                return ERR_DBF_SQLBIND;
            }
        }
        if (recno) {
            rc = sqlite3_bind_int(ppStmt, fc+1, i);
            if ( rc!=SQLITE_OK ) {
                fprintf(stderr, "SQL: %s (%d)\n", sql, fc+1);
                fprintf(stderr, "SQL bind_int error\n");
                return ERR_DBF_SQLBIND;
            }
        }
        rc = sqlite3_step(ppStmt);
        if (rc != SQLITE_DONE && rc != SQLITE_ROW) {
            fprintf(stderr, "SQL step error (%d)\n", rc);
            return ERR_DBF_SQLSTEP;
        }
        rc = sqlite3_reset(ppStmt);
        if ( rc!=SQLITE_OK ) {
            fprintf(stderr, "SQL reset error (%d)\n", rc);
            return ERR_DBF_SQLRESET;
        }
    }
    rc = sqlite3_finalize(ppStmt);
    if ( rc!=SQLITE_OK ) {
        fprintf(stderr, "SQL finalize error (%d)\n", rc);
        return ERR_DBF_SQLFINALIZE;
    }
    rc = sqlite3_exec(db, "COMMIT;\n", NULL, NULL, &zErrMsg);
    if( rc!=SQLITE_OK ){
        fprintf(stderr, "SQL commit error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
        return ERR_DBF_SQLCOMMIT;
    }

    free(fields);
    free(values);
    DBFClose(dH);
    return 0;
}

