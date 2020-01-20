/*********************************************************************
*
*  $Id: tgr2pagc-sqlite.c,v 1.4 2012-12-28 19:02:02 woodbri Exp $
*
*  tgr2pagc
*  Copyright 2010, Stephen Woodbridge, All rights Reserved
*  Redistribition without written permission strictly prohibited
*
**********************************************************************
*
*  tgr2pagc-sqlite.c
*
*  $Log: tgr2pagc-sqlite.c,v $
*  Revision 1.4  2012-12-28 19:02:02  woodbri
*  Changes dealing with how to evaluate place names when we have multiple
*  Census names that could be used.
*
*  Revision 1.3  2012-09-10 02:36:38  woodbri
*  Changes to support tiger2011-12
*
*  Revision 1.2  2012-06-03 13:45:59  woodbri
*  Changes to support TLID, TFID, SIDE attribute on output and Tiger 2011
*  Attribute changes.
*
*  Revision 1.1  2011/01/29 16:26:52  woodbri
*  Adding files to support PAGC data generation for Tiger and Navteq.
*  Adding options to domaps to support carring, speed, and address ranges.
*
*
**********************************************************************
*
* This utility requires the following Tiger2007 files for each county:
*   *_edges.*
*   *_addr.dbf
*   *_addrfn.dbf
*   *_featnames.dbf
*
* It will read this files into a SQLite DB file and then create
* a new Streets.* shapefile in the desination directory with the data
* denormalized and suitable for the PAGC Geocoder pagc_build_schema.
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
#include <pcre.h>
#include "dbf2sqlite.h"


/*
    these defines how the code will get built and with what features
    WITH_PLACENAMES - enable use of a global tiger placenames databse
    SINGLE_SIDED - Do not create right/left merged records
    PARSED_NAME  - create fields for the parsed Census road names
    MTFCC        - add MTFCC, PASSFLG, and DIVROAD fields to the output
    FOLD_PREQUAL_2_SUFQUAL - if PREQUAL and !SUFQUAL set SUFQUAL = PREQUAL
    YEAR2010
*/
#define WITH_PLACENAMES
#define SINGLE_SIDED
#define PARSED_NAME
#define FOLD_PREQUAL_2_SUFQUAL
#define YEAR2008 0
#define YEAR2010 0
#define YEAR2011 0
#define YEAR2017 1
//#define MTFCC

/*
    KEEP_NO_HN 1 - to keep streets with no house number ranges
                   This is useful for geocoding intersections

    KEEP_NO_HN 0 - to skip streets with no house number ranges
*/
#define KEEP_NO_HN 0

#define DATABASE "tgr2pagc-sqlite.db"
#define POSTALCODES "usps-actual.db"
#define MAX_NAME 36
#define MAX_NUMW 12

#define MAX(a,b) ((a)>(b)?(a):(b))
#define MIN(a,b) ((a)<(b)?(a):(b))

#define ERR_LOAD_DBF -1
#define ERR_LOAD_INDEX -2

int DEBUG = 0;

struct handle_struct {
    char    *fiStreets;
    DBFHandle   iDBF;
    SHPHandle   iSHP;
    char    *foStreets;
    DBFHandle   oDBF;
    SHPHandle   oSHP;
    int     cnt;
    int     nEnt;
    int     loaded;
    sqlite3 *db;
    sqlite3 *pc;
    sqlite3 *pl;
    sqlite3_stmt *stmt_getpc;
    sqlite3_stmt *stmt_getfips;
    sqlite3_stmt *stmt_getcousub;
    sqlite3_stmt *stmt_getplace;
    sqlite3_stmt *stmt_getzcta5;
    pcre *re_three;
    pcre *re_comma;
};

/*
    sql query results field positions
    if you change the parameters to the big join below 
    you need to update the list below
*/
#define N_tlid          0
#define N_name          1
#define N_paflag        2
#define N_mtfcc         3
#define N_passflg       4
#define N_divroad       5
#define N_fromhn        6
#define N_tohn          7
#define N_side          8
#define N_zip           9
#define N_plus4         10
#define N_statefp       11
#define N_countyfp      12
#define N_tfidl         13
#define N_tfidr         14
#ifdef PARSED_NAME
#define N_predirabrv    15
#define N_pretypabrv    16
#define N_prequalabr    17
#define N_sufdirabrv    18
#define N_suftypabrv    19
#define N_sufqualabr    20
#define N_recno         21
#else
#define N_recno         15
#endif

/* bit flags used to indicate various files have been loaded or not */
#define L_ADDR           1
#define L_ADDRFN         2
#define L_FEATNAMES      4
#define L_FACES          8
#define L_ZCTA5          8
#define L_COUSUB        16
#define L_PLACE         32
#define L_CONCTY        64

typedef struct out_record {
    const char *field_name;
    DBFFieldType field_type;
    int field_width;
    int dbf_field_index;
} OUTPUT_RECORD;

#ifdef PARSED_NAME
#define PARSED_NAME_RECS 6
#else
#define PARSED_NAME_RECS 0
#endif

#ifdef MTFCC
#define MTFCC_RECS 3
#else
#define MTFCC_RECS 0
#endif

#ifdef SINGLE_SIDED
#define SIDES_RECS 16
#else
#define SIDES_RECS 26
#endif

#define TOTAL_OUTPUT_RECORD (SIDES_RECS + PARSED_NAME_RECS + MTFCC_RECS)

#ifdef SINGLE_SIDED
OUTPUT_RECORD street_output_record[ TOTAL_OUTPUT_RECORD ] = {
    { "TLID",       FTInteger, 10, -1 },
    { "REFADDR",    FTInteger, 10, -1 },
    { "NREFADDR",   FTInteger, 10, -1 },
    { "SQLNUMF",    FTString,  12, -1 },
    { "SQLFMT",     FTString,  16, -1 },
    { "CFORMAT",    FTString,  16, -1 },
    { "NAME",       FTString,  80, -1 },
#ifdef PARSED_NAME
    { "PREDIRABRV", FTString,  15, -1 },
    { "PRETYPABRV", FTString,  50, -1 },
    { "PREQUALABR", FTString,  15, -1 },
    { "SUFDIRABRV", FTString,  15, -1 },
    { "SUFTYPABRV", FTString,  50, -1 },
    { "SUFQUALABR", FTString,  15, -1 },
#endif
    { "SIDE",       FTInteger,  1, -1 },
    { "TFID",       FTInteger, 10, -1 },
    { "USPS",       FTString,  35, -1 },
    { "AC5",        FTString,  35, -1 },
    { "AC4",        FTString,  35, -1 },
    { "AC3",        FTString,  35, -1 },
    { "AC2",        FTString,  35, -1 },
    { "AC1",        FTString,  35, -1 },
    { "POSTCODE",   FTString,  11, -1 }
#ifdef MTFCC
    ,
    { "MTFCC",      FTInteger,  5, -1 },
    { "PASSFLG",    FTString,   1, -1 },
    { "DIVROAD",    FTString,   1, -1 }
#endif
};

enum dbf_index{
    F_TLID = 0,
    F_L_REFADDR,
    F_L_NREFADDR,
    F_L_SQLNUMF,
    F_L_SQLFMT,
    F_L_CFORMAT,
    F_NAME,
#ifdef PARSED_NAME
    F_PREDIRABRV,
    F_PRETYPABRV,
    F_PREQUALABR,
    F_SUFDIRABRV,
    F_SUFTYPABRV,
    F_SUFQUALABR,
#endif
    F_SIDE,
    F_L_TFID,
    F_L_USPS,
    F_L_AC5,
    F_L_AC4,
    F_L_AC3,
    F_L_AC2,
    F_L_AC1,
    F_L_POSTCODE
#ifdef MTFCC
    ,
    F_MTFCC,
    F_PASSFLG,
    F_DIVROAD
#endif
};
#else

/*
    TODO: figure out how to do TFIDR and TFIDL and SIDE fields
    for dualsided records. This will likely require changes in PAGC
    to be able to return the SIDE that matched and to select the
    TFID[RL] associated to that SIDE.
*/

OUTPUT_RECORD street_output_record[ TOTAL_OUTPUT_RECORD ] = {
    { "TLID",       FTInteger, 10, -1 },
    { "L_REFADDR",  FTString,  10, -1 },
    { "L_NREFADDR", FTInteger, 10, -1 },
    { "L_SQLNUMF",  FTString,  12, -1 },
    { "L_SQLFMT",   FTString,  16, -1 },
    { "L_CFORMAT",  FTString,  16, -1 },
    { "R_REFADDR",  FTString,  10, -1 },
    { "R_NREFADDR", FTInteger, 10, -1 },
    { "R_SQLNUMF",  FTString,  12, -1 },
    { "R_SQLFMT",   FTString,  16, -1 },
    { "R_CFORMAT",  FTString,  16, -1 },
    { "NAME",       FTString,  80, -1 },
#ifdef PARSED_NAME
    { "PREDIRABRV", FTString,  15, -1 },
    { "PRETYPABRV", FTString,  50, -1 },
    { "PREQUALABR", FTString,  15, -1 },
    { "SUFDIRABRV", FTString,  15, -1 },
    { "SUFTYPABRV", FTString,  50, -1 },
    { "SUFQUALABR", FTString,  15, -1 },
#endif
    { "L_USPS",     FTString,  35, -1 },
    { "L_AC5",      FTString,  35, -1 },
    { "L_AC4",      FTString,  35, -1 },
    { "L_AC3",      FTString,  35, -1 },
    { "L_AC2",      FTString,  35, -1 },
    { "L_AC1",      FTString,  35, -1 },
    { "R_USPS",     FTString,  35, -1 },
    { "R_AC5",      FTString,  35, -1 },
    { "R_AC4",      FTString,  35, -1 },
    { "R_AC3",      FTString,  35, -1 },
    { "R_AC2",      FTString,  35, -1 },
    { "R_AC1",      FTString,  35, -1 },
    { "L_POSTCODE", FTString,  11, -1 },
    { "R_POSTCODE", FTString,  11, -1 }
#ifdef MTFCC
    ,
    { "MTFCC",      FTInteger,  5, -1 },
    { "PASSFLG",    FTString,   1, -1 },
    { "DIVROAD",    FTString,   1, -1 }
#endif
};

enum dbf_index{
    F_TLID = 0,
    F_L_REFADDR,
    F_L_NREFADDR,
    F_L_SQLNUMF,
    F_L_SQLFMT,
    F_L_CFORMAT,
    F_R_REFADDR,
    F_R_NREFADDR,
    F_R_SQLNUMF,
    F_R_SQLFMT,
    F_R_CFORMAT,
    F_NAME,
#ifdef PARSED_NAME
    F_PREDIRABRV,
    F_PRETYPABRV,
    F_PREQUALABR,
    F_SUFDIRABRV,
    F_SUFTYPABRV,
    F_SUFQUALABR,
#endif
    F_L_USPS,
    F_L_AC5,
    F_L_AC4,
    F_L_AC3,
    F_L_AC2,
    F_L_AC1,
    F_R_USPS,
    F_R_AC5,
    F_R_AC4,
    F_R_AC3,
    F_R_AC2,
    F_R_AC1,
    F_L_POSTCODE,
    F_R_POSTCODE
#ifdef MTFCC
    ,
    F_MTFCC,
    F_PASSFLG,
    F_DIVROAD
#endif
};
#endif

#define OINDX(a) street_output_record[a].dbf_field_index

struct streets_struct {
    int     recno;
    int     tlid;
    int     tfidl;
    int     tfidr;
    char    name[101];
    char    paflag[2];
    char    mtfcc[6];
    char    passflg[2];
    char    divroad[2];
    char    fromhn[13];
    char    tohn[13];
    char    side[2];
    char    zip[6];
    char    plus4[5];
    char    statefp[3];
    char    countyfp[4];
#ifdef PARSED_NAME
    char    predirabrv[16];
    char    pretypabrv[51];
    char    prequalabr[16];
    char    sufdirabrv[16];
    char    suftypabrv[51];
    char    sufqualabr[16];
#endif
};

struct current_struct {
    int     recno;
    int     tlid;
    int     tfidl;
    int     tfidr;
    int     sides;
    char    name[101];
    char    altname[101];
    char    sfips[3];
    char    cfips[4];
    char    mtfcc[6];
    char    passflg[2];
    char    divroad[2];
    char    lzip[6];
    char    lplus4[5];
    char    rzip[6];
    char    rplus4[5];
    char    lfromhn[13];
    char    ltohn[13];
    char    rfromhn[13];
    char    rtohn[13];
#ifdef PARSED_NAME
    char    predirabrv[16];
    char    pretypabrv[51];
    char    prequalabr[16];
    char    sufdirabrv[16];
    char    suftypabrv[51];
    char    sufqualabr[16];
#endif
};

int write_rec(struct handle_struct *H, long tlid, long tfid, struct streets_struct *c, char *city, char *st, char *zip, char *ocity, char *ac4, char *ac3, char *ac1);

void Usage()
{
#ifdef WITH_PLACENAMES
    printf("Usage: tgr2pagc src/edges.dbf dest_st_dir path/tiger-places.db path/usps-actual.db [num_split]\n");
#else
    printf("Usage: tgr2pagc src/edges.dbf dest_st_dir path/usps-actual.db [num_split]\n");
#endif
#if YEAR2008
    printf("       Built for YEAR2008 Tiger file formats.\n");
#elif YEAR2010
    printf("       Built for YEAR2010 Tiger file formats.\n");
#elif YEAR2011
    printf("       Built for YEAR2011 Tiger file formats.\n");
#elif YEAR2017
    printf("       Built for YEAR2017 Tiger file formats.\n");
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
        printf("%s\n", sql);

    rc = sqlite3_exec(db, sql, NULL, NULL, &zErrMsg);
    if( rc != SQLITE_OK ){
        fprintf(stderr, "SQL: %s\n", sql);
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
        return ERR_DBF_SQLEXEC;
    }

    return 0;
}


int loadAddr(sqlite3 *db, char *fin)
{
    int         rc;
    char        *zErrMsg;
    char        *sql;
    const char  *cols[] = {"TLID","FROMHN","TOHN","SIDE","ZIP","PLUS4",
        "FROMTYP","TOTYP","ARID","MTFCC"};
    
    /* create the tables */

    rc = loadDBF(db, fin, "addr", 10, cols, NULL, 1);
    if (rc) {
        fprintf(stderr, "Failed to load DBF file (%s)\n", fin);
        rc = createTable(db, "addr", 10, cols, NULL);
        return ERR_LOAD_DBF;
    }

    /* Create indexes that we will need */

    sql = "create index addr_tlid_idx on addr (TLID);\n";
    rc = sqlite3_exec(db, sql, NULL, NULL, &zErrMsg);
    if( rc!=SQLITE_OK ){
    	fprintf(stderr, "SQL: %s", sql);
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        return ERR_LOAD_INDEX;
    }
    
    sql = "create unique index addr_arid_idx on addr (ARID);\n";
    rc = sqlite3_exec(db, sql, NULL, NULL, &zErrMsg);
    if( rc!=SQLITE_OK ){
    	fprintf(stderr, "SQL: %s", sql);
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        return ERR_LOAD_INDEX;
    }

    return 0;
}


int loadAddrfn(sqlite3 *db, char *fin)
{
    int         rc;
    char        *zErrMsg;
    char        *sql;
    const char  *cols[] = {"ARID", "LINEARID"};

    /* create the tables */

    rc = loadDBF(db, fin, "addrfn", 2, cols, NULL, 1);
    if (rc) {
        fprintf(stderr, "Failed to load DBF file (%s)\n", fin);
        rc = createTable(db, "addrfn", 2, cols, NULL);
        return ERR_LOAD_DBF;
    }

    /* Create indexes that we will need */

    sql = "create index addrfn_arid_idx on addrfn (ARID);\n";
    rc = sqlite3_exec(db, sql, NULL, NULL, &zErrMsg);
    if( rc!=SQLITE_OK ){
    	fprintf(stderr, "SQL: %s", sql);
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        return ERR_LOAD_INDEX;
    }
    
    sql = "create index addrfn_linearid_idx on addrfn (LINEARID);\n";
    rc = sqlite3_exec(db, sql, NULL, NULL, &zErrMsg);
    if( rc!=SQLITE_OK ){
    	fprintf(stderr, "SQL: %s", sql);
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        return ERR_LOAD_INDEX;
    }
    
    return 0;
}

#ifndef WITH_PLACENAMES

int loadCousub(sqlite3 *db, char *fin)
{
    int     rc;
    char        *zErrMsg;
    char        *sql;
    const char  *cols11[] = {"GEOID","NAME"};
    const char  *cols10[] = {"GEOID10","NAME10"};
    const char  *cols09[] = {"COSBIDFP","NAME"};
    
    /* create the tables */

#if YEAR2008
    rc = loadDBF(db, fin, "cousub", 2, cols09, NULL, 1);
#elif YEAR2010
    rc = loadDBFrename(db, fin, "cousub", 2, cols10, cols09, NULL, 1);
#elif YEAR2011 || YEAR2017
    rc = loadDBFrename(db, fin, "cousub", 2, cols11, cols09, NULL, 1);
#endif
    if (rc) {
        fprintf(stderr, "Failed to load DBF file (%s)\n", fin);
        return ERR_LOAD_DBF;
    }

    /* Create indexes that we will need */

    sql = "create unique index cousub_cosbidfp_idx on cousub (COSBIDFP);\n";
    rc = sqlite3_exec(db, sql, NULL, NULL, &zErrMsg);
    if( rc!=SQLITE_OK ){
    	fprintf(stderr, "SQL: %s", sql);
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        return ERR_LOAD_INDEX;
    }

    return 0;
}


int loadConcity(sqlite3 *db, char *fin)
{
    int     rc;
    char        *zErrMsg;
    char        *sql;
    const char  *cols11[] = {"GEOID10","NAME"};
    const char  *cols10[] = {"GEOID10","NAME10"};
    const char  *cols09[] = {"CCTYIDFP","NAME"};
    
    /* create the tables */

#if YEAR2008
    rc = loadDBF(db, fin, "concty", 2, cols09, NULL, 1);
#elif YEAR2010
    rc = loadDBFrename(db, fin, "concty", 2, cols10, cols09, NULL, 1);
#elif YEAR2011 || YEAR2017
    rc = loadDBFrename(db, fin, "concty", 2, cols11, cols09, NULL, 1);
#endif
    if (rc) {
        fprintf(stderr, "Failed to load DBF file (%s)\n", fin);
        return ERR_LOAD_DBF;
    }

    /* Create indexes that we will need */

    sql = "create unique index concty_cctyidfp_idx on concty (CCTYIDFP);\n";
    rc = sqlite3_exec(db, sql, NULL, NULL, &zErrMsg);
    if( rc!=SQLITE_OK ){
    	fprintf(stderr, "SQL: %s", sql);
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        return ERR_LOAD_INDEX;
    }

    return 0;
}

#endif

int removeDuplicateEdges(sqlite3 *db)
{
    int     rc;
    int     cnt;
    int     tlid;
    int     recno;
    char    *sql_tlid;
    char    *sql_recno;
    char    *sql_delete;
    sqlite3_stmt *stmt_tlid;
    sqlite3_stmt *stmt_recno;
    sqlite3_stmt *stmt_delete;

    sql_tlid = "select TLID from edges group by TLID having count(*)>1;\n";
    rc = sqlite3_prepare(db, sql_tlid, strlen(sql_tlid), &stmt_tlid, NULL);
    if ( rc!=SQLITE_OK ) {
        fprintf(stderr, "SQL: %s", sql_tlid);
        fprintf(stderr, "SQL error(%d): %s\n", rc, sqlite3_errmsg(db));
        return 1;
    }

    sql_recno = "select RECNO from edges where TLID=?;\n";
    rc = sqlite3_prepare(db, sql_recno, strlen(sql_recno), &stmt_recno, NULL);
    if ( rc!=SQLITE_OK ) {
        fprintf(stderr, "SQL: %s", sql_recno);
        fprintf(stderr, "SQL error(%d): %s\n", rc, sqlite3_errmsg(db));
        return 1;
    }

    sql_delete = "delete from edges where TLID=? and RECNO=?;\n";
    rc = sqlite3_prepare(db, sql_delete, strlen(sql_delete), &stmt_delete, NULL);
    if ( rc!=SQLITE_OK ) {
        fprintf(stderr, "SQL: %s", sql_delete);
        fprintf(stderr, "SQL error(%d): %s\n", rc, sqlite3_errmsg(db));
        return 1;
    }

    while (sqlite3_step(stmt_tlid) == SQLITE_ROW) {
        tlid = sqlite3_column_int(stmt_tlid, 0);
        rc = sqlite3_bind_int(stmt_recno, 1, tlid);
        if ( rc!=SQLITE_OK ) {
            fprintf(stderr, "removeDuplicateEdges: SQL bind_int tlid error\n");
            return 1;
        }

        cnt=0;
        while (sqlite3_step(stmt_recno) == SQLITE_ROW) {
            if (cnt) {
                recno = sqlite3_column_int(stmt_recno, 0);
                rc = sqlite3_bind_int(stmt_delete, 1, tlid);
                if ( rc!=SQLITE_OK ) {
                    fprintf(stderr, "removeDuplicateEdges: SQL bind_int delete error\n");
                    return 1;
                }
                rc = sqlite3_bind_int(stmt_delete, 2, recno);
                if ( rc!=SQLITE_OK ) {
                    fprintf(stderr, "removeDuplicateEdges: SQL bind_int delete error\n");
                    return 1;
                }
                rc = sqlite3_step(stmt_delete);
                if ( rc != SQLITE_OK && rc != SQLITE_DONE ) {
                    fprintf(stderr, "SQL: %s", sql_delete);
                    fprintf(stderr, "SQL error(%d): %s\n", rc, sqlite3_errmsg(db));
                    return 1;
                }
                sqlite3_reset(stmt_delete);
            }
            cnt++;
        }
        sqlite3_reset(stmt_recno);
    }
    sqlite3_finalize(stmt_delete);
    sqlite3_finalize(stmt_recno);
    sqlite3_finalize(stmt_tlid);

    return 0;
}


int loadEdges(sqlite3 *db, char *fin)
{
    int     rc;
    char        *zErrMsg;
    char        *sql;
    char        *sql2;
    const char  *cols[] = {"STATEFP","COUNTYFP","TLID","TFIDL",
        "TFIDR","MTFCC","FULLNAME","SMID","LFROMADD","LTOADD","RFROMADD",
        "RTOADD","ZIPL","ZIPR","FEATCAT","HYDROFLG","RAILFLG","ROADFLG",
#if YEAR2017
        /* GCSEFLG is ignored, DIVROAD does not exist in 2017 data */
        "OLFFLG","PASSFLG","GCSEFLG","EXTTYP","TTYP","DECKEDROAD","ARTPATH"};
#else
        "OLFFLG","PASSFLG","DIVROAD","EXTTYP","TTYP","DECKEDROAD","ARTPATH"};
#endif

    /* create the tables */

    rc = loadDBF(db, fin, "edges", 25, cols, "RECNO", 1);
    if (rc) {
        fprintf(stderr, "Failed to load DBF file (%s)\n", fin);
        return ERR_LOAD_DBF;
    }

    /* Create indexes that we will need */

    sql = "create unique index edges_tlid_idx on edges (TLID);\n";
    rc = sqlite3_exec(db, sql, NULL, NULL, &zErrMsg);
    if( rc!=SQLITE_OK ){
        /* if not unique, get rid of segments we don't care about */
        if (!strcmp(zErrMsg, "indexed columns are not unique")) {
            rc = removeDuplicateEdges(db);
            if( rc ){
                fprintf(stderr, "Failed to remove duplicate edges!\n");
                return ERR_LOAD_INDEX;
            }
            /* try again to build the index */
            fprintf(stderr, "Removed duplicate edges, trying to reindex.\n");
            rc = sqlite3_exec(db, sql, NULL, NULL, &zErrMsg);
        }
        /* if the above index attempt failed throw the error here */
        if( rc!=SQLITE_OK ){
            fprintf(stderr, "SQL: %s", sql);
            fprintf(stderr, "SQL error: %s\n", zErrMsg);
            return ERR_LOAD_INDEX;
        }
    }

    return 0;
}

#ifndef WITH_PLACENAMES

int loadFaces(sqlite3 *db, char *fin)
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
    rc = loadDBF(db, fin, "faces", 8, cols09, NULL, 1);
#elif YEAR2010
    rc = loadDBFrename(db, fin, "faces", 8, cols10, cols09, NULL, 1);
#elif YEAR2011 || YEAR2017
    rc = loadDBFrename(db, fin, "faces", 8, cols11, cols09, NULL, 1);
#endif
    if (rc) {
        fprintf(stderr, "Failed to load DBF file (%s)\n", fin);
        return ERR_LOAD_DBF;
    }

    /* Create indexes that we will need */

    sql = "create unique index faces_tfid_idx on faces (TFID);\n";
    rc = sqlite3_exec(db, sql, NULL, NULL, &zErrMsg);
    if( rc!=SQLITE_OK ){
    	fprintf(stderr, "SQL: %s", sql);
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        return ERR_LOAD_INDEX;
    }

    return 0;
}

#endif

int loadFeatnames(sqlite3 *db, char *fin)
{
    int     rc;
    char        *zErrMsg;
    char        *sql;
    const char  *cols[] = {"TLID","FULLNAME","NAME","PREDIRABRV","PRETYPABRV","PREQUALABR","SUFDIRABRV","SUFTYPABRV","SUFQUALABR","PREDIR","PRETYP","PREQUAL","SUFDIR","SUFTYP","SUFQUAL","LINEARID","MTFCC","PAFLAG"};
    
    /* create the tables */

    rc = loadDBF(db, fin, "featnames", 18, cols, NULL, 1);
    if (rc) {
        fprintf(stderr, "Failed to load DBF file (%s)\n", fin);
        rc = createTable(db, "featnames", 18, cols, NULL);
        return ERR_LOAD_DBF;
    }

    /* Create indexes that we will need */

    sql = "create index featnames_tlid_idx on featnames (TLID);\n";
    rc = sqlite3_exec(db, sql, NULL, NULL, &zErrMsg);
    if( rc!=SQLITE_OK ){
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        return ERR_LOAD_INDEX;
    }

    sql = "create index featnames_linearid_idx on featnames (LINEARID);\n";
    rc = sqlite3_exec(db, sql, NULL, NULL, &zErrMsg);
    if( rc!=SQLITE_OK ){
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        return ERR_LOAD_INDEX;
    }

    return 0;
}

#ifndef WITH_PLACENAMES

int loadPlace(sqlite3 *db, char *fin)
{
    int     rc;
    char        *zErrMsg;
    char        *sql;
    const char  *cols11[] = {"GEOID","NAME"};
    const char  *cols10[] = {"GEOID10","NAME10"};
    const char  *cols09[] = {"PLCIDFP","NAME"};
    
    /* create the tables */

#if YEAR2008
    rc = loadDBF(db, fin, "place", 2, cols09, NULL, 1);
#elif YEAR2010
    rc = loadDBFrename(db, fin, "place", 2, cols10, cols09, NULL, 1);
#elif YEAR2011 || YEAR2017
    rc = loadDBFrename(db, fin, "place", 2, cols11, cols09, NULL, 1);
#endif
    if (rc) {
        fprintf(stderr, "Failed to load DBF file (%s)\n", fin);
        return ERR_LOAD_DBF;
    }

    /* Create indexes that we will need */

    sql = "create unique index place_plcidfp_idx on place (PLCIDFP);\n";
    rc = sqlite3_exec(db, sql, NULL, NULL, &zErrMsg);
    if( rc!=SQLITE_OK ){
    	fprintf(stderr, "SQL: %s", sql);
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        return ERR_LOAD_INDEX;
    }

    return 0;
}

#endif

char *mkFName(char *path, char *file, int addslash, int state)
{
    char *f;
    char *p = strdup(path);

    f = strstr(p, "edges");
    if (f) *f = '\0';
    if (f && state) {
        *(f-4) = '_';
        *(f-3) = '\0';
    }
    
    f = calloc(strlen(p)+strlen(file)+2, sizeof(char));
    assert(f);

    strcpy(f, p);
    if (addslash && path[strlen(p)-1] != '/')
        strcat(f, "/");
    strcat(f, file);

    free(p);

    return f;
}


int getNextShape(struct handle_struct *H, int seq)
{
    if (H->oDBF) DBFClose(H->oDBF);
    if (H->oSHP) SHPClose(H->oSHP);
    if (openShapefile(H->foStreets, seq, H)) {
        fprintf(stderr, "Failed to rotate to next shapefile of output.\n");
        return 1;
    }
    return 0;
}


int openShapefile(char *base, int seq, struct handle_struct *H)
{
    char *s;
    int i;
    int  q = seq>0?(int)log10(seq):0;

    printf("called openShapefile('%s', %d, ...)\n", base, seq);
    
    s = (char *) calloc(strlen(base)+q+10, 1);
    assert(s);
    strcpy(s, base);
    if (seq > 0) {
        if (s[strlen(s)-4] == '.') s[strlen(s)-4] = '\0';
        sprintf(s+strlen(s), "-%d.dbf", seq);
    }
    if (!(H->oDBF = DBFCreate(s))) {
        fprintf(stderr, "Failed to create dbf %s\n", s);
        free(s);
        return 1;
    }
    strcpy(s+strlen(s)-3, "shp");
    if (!(H->oSHP = SHPCreate(s, SHPT_ARC))) {
        fprintf(stderr, "Failed to create shp %s\n", s);
        free(s);
        return 1;
    }
    
    /* create output structure */
    if (DEBUG) printf("TOTAL_OUTPUT_RECORD=%d\n", TOTAL_OUTPUT_RECORD);
    for (i=0; i<TOTAL_OUTPUT_RECORD; i++) {
        if (DEBUG) printf("  Field %2d: %s\n", i+1, street_output_record[i].field_name);
        street_output_record[i].dbf_field_index =
            DBFAddField(H->oDBF,
                street_output_record[i].field_name,
                street_output_record[i].field_type,
                street_output_record[i].field_width, 0);
        if (street_output_record[i].dbf_field_index == -1) {
            printf("Error: failed to create output DBF field('%s', %d, %d)\n",
                street_output_record[i].field_name,
                street_output_record[i].field_type,
                street_output_record[i].field_width);
            exit(1);
        }
    }

    return 0;
}

#ifdef WITH_PLACENAMES
#define PDB pl
#define IS_LOADED(a) 1
#else
#define PDB db
#define IS_LOADED(a) (loaded & (a))
#endif
    

struct handle_struct *createHandle(sqlite3 *db, sqlite3 *pc, sqlite3 *pl, char *fiStreets, char *foStreets, int seq, int loaded)
{
    char *sin;
    char *sout;
    char *sql;
    int  rc;
    const char *re1;
    const char *re2;
    const char *err;
    int erroff;
    
    struct handle_struct *H;
    H = (struct handle_struct *) calloc(1, sizeof(struct handle_struct));
    assert(H);

    H->fiStreets  = fiStreets;
    H->foStreets  = foStreets;
    H->cnt        = 0;
    H->loaded     = loaded;

    /* prepare some sqlite queries that we need to reuse */

    H->db = db;
    H->pc = pc;
    H->pl = pl;
    
    sql = "select city, st from zipcode where zip=?;";
    rc = sqlite3_prepare(pc, sql, strlen(sql), &H->stmt_getpc, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL: %s\n", sql);
        fprintf(stderr, "getPostalCity: SQL Error: %s\n", sqlite3_errmsg(pc));
        exit(1);
    }

    sql = "select county, st from fipscodes where fips=? and fipc=?;";
    rc = sqlite3_prepare(pc, sql, strlen(sql), &H->stmt_getfips, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL: %s\n", sql);
        fprintf(stderr, "getStateCounty: SQL Error: %s\n", sqlite3_errmsg(pc));
        exit(1);
    }

    if (IS_LOADED(L_FACES) && IS_LOADED(L_COUSUB)) {
        sql = "select b.name from faces a, cousub b where TFID=? and b.COSBIDFP=a.STATEFP||a.COUNTYFP||a.COUSUBFP;";
        rc = sqlite3_prepare(PDB, sql, strlen(sql), &H->stmt_getcousub, NULL);
        if (rc != SQLITE_OK) {
            fprintf(stderr, "SQL: %s\n", sql);
            fprintf(stderr, "getFacesInfo: SQL Error: %s\n", sqlite3_errmsg(PDB));
            exit(1);
        }
    }

    if (IS_LOADED(L_FACES) && IS_LOADED(L_PLACE)) {
        sql = "select b.name from faces a, place b where tfid=? and b.PLCIDFP=a.STATEFP||a.PLACEFP;";
        rc = sqlite3_prepare(PDB, sql, strlen(sql), &H->stmt_getplace, NULL);
        if (rc != SQLITE_OK) {
            fprintf(stderr, "SQL: %s\n", sql);
            fprintf(stderr, "getFacesInfo: SQL Error: %s\n", sqlite3_errmsg(PDB));
            exit(1);
        }
    }

    if (IS_LOADED(L_FACES)) {
        sql = "select zcta5ce from faces where tfid=?;";
        rc = sqlite3_prepare(PDB, sql, strlen(sql), &H->stmt_getzcta5, NULL);
        if (rc != SQLITE_OK) {
            fprintf(stderr, "SQL: %s\n", sql);
            fprintf(stderr, "getFacesInfo: SQL Error: %s\n", sqlite3_errmsg(PDB));
            exit(1);
        }
    }

    /* open shapefiles */
    
    sin = strdup(H->fiStreets);
    strcpy(sin+strlen(sin)-3, "shp");
    if (!(H->iSHP  = SHPOpen(sin, "rb"))) {
        fprintf(stderr, "Failed to open shp %s for read\n", sin);
        exit(1);
    }
    free(sin);
    SHPGetInfo(H->iSHP, &H->nEnt, NULL, NULL, NULL);

    if (openShapefile(H->foStreets, seq, H)) {
        exit(1);
    }

    re1 = "^\\d+$|^I{0,3}$";
    H->re_three = pcre_compile(re1, PCRE_CASELESS, &err, &erroff, NULL);
    if (!H->re_three) {
        fprintf(stderr, "Regex '%s' failed to compile at char (%i) : %s\n", re1, erroff, err);
        exit(1);
    }
    
    re2 = "^\\d+,\\s+(.*)";
    H->re_comma = pcre_compile(re2, 0, &err, &erroff, NULL);
    if (!H->re_comma) {
        fprintf(stderr, "Regex '%s' failed to compile at char (%i) : %s\n", re2, erroff, err);
        exit(1);
    }
    
    return H;
}

void closeHandle(struct handle_struct *H)
{

    sqlite3_finalize(H->stmt_getpc);
    sqlite3_finalize(H->stmt_getfips);
    sqlite3_finalize(H->stmt_getcousub);
    sqlite3_finalize(H->stmt_getplace);
    
    /* close shapefiles */
    if (H->iDBF) DBFClose(H->iDBF);
    if (H->oDBF) DBFClose(H->oDBF);
    if (H->iSHP) SHPClose(H->iSHP);
    if (H->oSHP) SHPClose(H->oSHP);

    /* closr pcre regex */
    if (H->re_three) pcre_free(H->re_three);
    if (H->re_comma) pcre_free(H->re_comma);
    
    /* free memory */
    memset(H, 0, sizeof(struct handle_struct));
    free(H);
}


int getPostalName(struct handle_struct *H, char *zip, char *zcta, char *city, char *st)
{
    sqlite3_stmt *stmt;
    char *z;
    int rc;
    int ret = 0;

    if (zip && strlen(zip))
        z = zip;
    else if (zcta && strlen(zcta))
        z = zcta;
    else
        return ret;

    stmt = H->stmt_getpc;

    rc = sqlite3_bind_text(stmt, 1, z, strlen(z), SQLITE_STATIC);
    if ( rc!=SQLITE_OK ) {
        fprintf(stderr, "getPostalName: SQL bind_text error\n");
        exit(1);
    }

    rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
        strcpy(city, sqlite3_column_text(stmt, 0));
        strcpy(st,   sqlite3_column_text(stmt, 1));
        ret = 1;
    }
    sqlite3_reset(stmt);
    return ret;
}

int getCountyStateName(struct handle_struct *H, char *sfips, char *cfips, char *name, char *state)
{
    sqlite3_stmt *stmt;
    int rc;
    int ret = 0;

    if (!sfips || !cfips)
        return ret;

    stmt = H->stmt_getfips;

    rc = sqlite3_bind_text(stmt, 1, sfips, strlen(sfips), SQLITE_STATIC);
    if ( rc!=SQLITE_OK ) {
        fprintf(stderr, "getCountyName: SQL bind_text error\n");
        exit(1);
    }

    rc = sqlite3_bind_text(stmt, 2, cfips, strlen(cfips), SQLITE_STATIC);
    if ( rc!=SQLITE_OK ) {
        fprintf(stderr, "getCountyName: SQL bind_text error\n");
        exit(1);
    }

    rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
        strcpy(name, sqlite3_column_text(stmt, 0));
        strcpy(state, sqlite3_column_text(stmt, 1));
        ret = 1;
    }
    else {
        fprintf(stderr, "ERROR: getCountyName: failed to find names for (%s,%s)\n", sfips, cfips);
    }
    sqlite3_reset(stmt);
    return ret;
}


int getFacesInfo(struct handle_struct *H, int tfid, int indx, char *name)
{
    sqlite3_stmt *stmt;
    char *sql;
    int rc;
    int ret = 0;

#ifndef WITH_PLACENAMES
    if ( !tfid || ((H->loaded & indx) == 0) )
        return ret;
#endif

    if (indx & L_COUSUB)
        stmt = H->stmt_getcousub;
    else if (indx & L_PLACE)
        stmt = H->stmt_getplace;
    else
        return ret;

    rc = sqlite3_bind_int(stmt, 1, tfid);
    if ( rc!=SQLITE_OK ) {
        fprintf(stderr, "getFacesInfo: SQL bind_int error\n");
        exit(1);
    }

    rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
        strcpy(name, sqlite3_column_text(stmt, 0));
        ret = 1;
    }
    sqlite3_reset(stmt);
    return ret;
}

#define getCousubName(a,b,c) getFacesInfo(a,b,L_COUSUB,c)
#define getConctyName(a,b,c) getFacesInfo(a,b,L_CONCTY,c)
#define getPlaceName(a,b,c)  getFacesInfo(a,b,L_PLACE,c)
#define getZCTA5(a,b,c)      getFacesInfo(a,b,L_ZCTA5,c)
#define coalesce(a,b,c) (((a) && strlen(a))?(a):((b) && strlen(b))?(b):((c) && strlen(c))?(c):"")

/*
 * currently we are only interested in the following tokens:
 *   N = number
 *   Z = leadding zero number
 *   A = alpha
 *   - = hypthen
 * all other characters will be dropped
 */

int tokenize_num(char *s, char tok[][MAX_NUMW], char *typ) {
    int i;
    int j = 0;
    int k = 0;

    for (i=0; i<strlen(s); i++) {
        if (isdigit(s[i])) {
            if (!j || (typ[j-1] != 'N' && typ[j-1] != 'Z')) {
                if (s[i] == '0')
                    typ[j] = 'Z';
                else
                    typ[j] = 'N';
                j++;
                k = 0;
            }
            tok[j-1][k++] = s[i];
        }
        else if (isalpha(s[i])) {
            if (!j || typ[j-1] != 'A') {
                typ[j] = 'A';
                j++;
                k = 0;
            }
            tok[j-1][k++] = s[i];
        }
        else if (s[i] == '-') {
            if (!j || typ[j-1] != '-') {
                typ[j] = '-';
                j++;
                k = 0;
            }
            else if (!j || typ[j-1] == '-') {
                continue;
            }
            tok[j-1][k++] = s[i];
        }
    }

    return j;
}


char *scan_nt_nums(char *f, char *t, int *nf, int *nt) {
    static char fmt[20];
    static char bad[1] = "";
    char ftok[MAX_NUMW][MAX_NUMW];
    char ttok[MAX_NUMW][MAX_NUMW];
    char ftyp[MAX_NUMW];
    char ttyp[MAX_NUMW];
    int i, ntf, ntt;
    int w;

    *nf = 0;
    *nt = 0;
    if (strlen(f) == 0 && strlen(t) == 0) {
        return bad;
    }
    
    for (i=0; i<10; i++) {
        memset(ftok[i], 0,  MAX_NUMW);
        memset(ttok[i], 0,  MAX_NUMW);
    }
    memset(ftyp, 0,  MAX_NUMW);
    memset(ttyp, 0,  MAX_NUMW);
    memset(fmt,  0,  2*MAX_NUMW);

    ntf = tokenize_num(f, ftok, ftyp);
    ntt = tokenize_num(t, ttok, ttyp);

    if (ntf != ntt || ntf == 0) {
        return bad;
    }

    for (i=0; i<MAX_NUMW; i++) {
        if (ftyp[i] == '\0')
            break;

        /* from and to are not numbers and don't match return bad */
        if (!(strchr("NZ", ftyp[i]) && strchr("NZ", ttyp[i]))
                && (ftyp[i] != ttyp[i]))
            return bad;

        if (!strcmp(ftok[i], ttok[i])) {
            strcat(fmt, ftok[i]);
        }
        else {
            if (ftyp[i] == 'Z' || ttyp[i] == 'Z') {
                sprintf(fmt+strlen(fmt), "%%0%dd", 
                        (int)MAX(strlen(ftok[i]), strlen(ttok[i])));
                *nf = strtol(ftok[i], NULL, 10);
                *nt = strtol(ttok[i], NULL, 10);
            }
            else if (ftyp[i] == 'N') {
                sprintf(fmt+strlen(fmt), "%%%dd", 
                        (int)MAX(strlen(ftok[i]), strlen(ttok[i])));
                *nf = strtol(ftok[i], NULL, 10);
                *nt = strtol(ttok[i], NULL, 10);
            }
            else {
                return bad;
            }
        }
    }
    return fmt;
}

void mkSqlFmt(char *in, char *fmt1, char *fmt2) {
    int i;
    int j;
    int w;

    fmt1[0] = fmt2[0] = '\0';

    for (i=0; i<strlen(in); i++) {
        /* look for the C format string if it exists */
        if (in[i] == '%') {
            strcat(fmt1, "{");
            strcat(fmt1, "}");
            /* construct a sql to_char() format sting */
            w = atoi(in+i+1);
            if (in[i+1] == '0') {
                w--;
                strcat(fmt2, "0");
            }
            else
                strcat(fmt2, "FM");
            while (w--)
                strcat(fmt2, "9");
            /* skip over the rest of the C format string */
            while (i<strlen(in) && in[i] != 'd')
                i++;
        }
        else {
            j = strlen(fmt1);
            fmt1[j++] = in[i];
            fmt1[j] = '\0';
        }
    }
}


/*
 *  Simple logic follows, the trick is that af, at, bf, bt
 *  may be alphanumeric values! So so we will have to scan the values
 *  and then do the comparisions on the numeric parts and/or alpha parts
 *  as needed. Basically this is a pain as most if not all are numbers
 *  only. We might take a short cut and only treat them as numbers and
 *  throw an error if we hit alphas and fix it later.
 *  
 *  if bf < bt then
 *      bf = min(af, at, bf, bt)
 *      bt = max(af, at, bf, bt)
 *  else
 *      bf = max(af, at, bf, bt)
 *      bt = min(af, at, bf, bt)
 *  endif
 */

void merge_hn(int tlid, char *af, char *at, char *bf, char *bt)
{
    int naf, nat, nbf, nbt;
    char *afmt = scan_nt_nums(af, at, &naf, &nat);
    char *bfmt = scan_nt_nums(bf, bt, &nbf, &nbt);

    if (!strlen(afmt) && (strlen(af) || strlen(at)))
        printf("merge_hn: scan_nt_nums: tlid: %d a('%s', '%s')\n",
                tlid, af, at);
    if (!strlen(bfmt) && (strlen(bf) || strlen(bt)))
        printf("merge_hn: scan_nt_nums: tlid: %d b('%s', '%s')\n",
                tlid, bf, bt);

    int nmin = MIN(MIN(naf, nat), MIN(nbf, nbt));
    int nmax = MAX(MAX(naf, nat), MAX(nbf, nbt));

    if (strcmp(afmt, bfmt))
        printf("WARN: tlid: %d, merge_hn: fmt mismatch(%s, %s)\n", tlid, afmt, bfmt);
    
    if (nbf <= nbt && naf <= nat) {
        sprintf(bf, bfmt, nmin);
        sprintf(bt, bfmt, nmax);
    }
    else if (nbf >= nbt && naf >= nat) {
        sprintf(bf, bfmt, nmax);
        sprintf(bt, bfmt, nmin);
    }
    else {
        printf("WARN: tlid: %d, merge_hn(%s, %s, %s, %s)\n", tlid, af, at, bf, bt);
    }
}

/*
 *  The idea behind merge_RL() is to accumulate the data from multiple 
 *  records *s into the current record *c
 *
 *  1) do the simple things like copy data if the target is empty
 *  2) report collisions that matter
 *  3) merge address ranges or report issues that prevent merging
 *  
 */

void merge_RL(struct current_struct *c, struct streets_struct *s)
{
    assert(c->tlid == s->tlid);
    switch (s->side[0]) {
        case 'R':
            c->sides |= 1;
            if (!strlen(c->rzip)) {
                strcpy(c->rzip, s->zip);
                strcpy(c->rplus4, s->plus4);
            }
            else if (strlen(c->rzip) && strcmp(c->rzip, s->zip)) {
                printf("WARN: merge_RL: multiple zipcodes on same R side: %d, %s, %s\n",
                        s->tlid, c->rzip, s->zip);
            }
            
            if (!strlen(c->rfromhn)) {
                strcpy(c->rfromhn, s->fromhn);
                strcpy(c->rtohn, s->tohn);
            }
            else {
                merge_hn(s->tlid, s->fromhn, s->tohn, c->rfromhn, c->rtohn);
            }
            break;
        case 'L':
            c->sides |= 2;
            if (!strlen(c->lzip)) {
                strcpy(c->lzip, s->zip);
                strcpy(c->lplus4, s->plus4);
            }
            else if (strlen(c->lzip) && strcmp(c->lzip, s->zip)) {
                printf("WARN: merge_RL: multiple zipcodes on same L side: %d, %s, %s\n",
                        s->tlid, c->lzip, s->zip);
            }

            if (!strlen(c->lfromhn)) {
                strcpy(c->lfromhn, s->fromhn);
                strcpy(c->ltohn, s->tohn);
            }
            else {
                merge_hn(s->tlid, s->fromhn, s->tohn, c->lfromhn, c->ltohn);
            }
            break;
        case '\0':
            /* side will be null for records without names or address ranges */
            break;
        default:
            printf("WARN: merge_RL: side not R or L, tlid: %d\n", s->tlid);
            break;
    }
}


/*

The following table shows the various input conditions that we can have
based on a Census Record where the names are:

    city    - city name associated with record
    st      - state based on county
    zip     - zipcode associated with record
    ucity   - USPS city associated with zip
    ust     - USPS state associated with zip
    zcta    - zcta5 zipcode associated with record
    zcity   - USPS city associated with zcta
    zst     - USPS state associated with zcta
    "00000" - dummy zipcode mean we are unsure of the zipcode

The input conditions below are:

    have zip    - the record has a zipcode
    zip ok      - the record has a zipcode in the USPS database
    have zcta   - the record has a zcta zipcode
    zcta ok     - the record has a zcta zipcode in the USPS database
    zip == zcta - zip matches zcta
    st == ust   - state matches USPS zip state
    st == zst   - state matches USPS zcta state 

So for each set of input conditions we translate that to a set of
records that we want to output using this table:

          INPUTS
    +- st == zst
    | +- st == ust
    | | +- zip == zcta, implies ust == zst
    | | | + zcta ok
    | | | | +- have zcta
    | | | | | +- zip ok
    | | | | | | +- have zip
    | | | | | | |   in                 out
    | | | | | | | code                code
    1 1 1 1 1 1 1  127  0 0 1 0 0 1 0  18
    1 0 0 1 1 1 1   79  0 1 1 1 0 0 1  57
    1 0 0 1 1 0 1   77  0 1 0 1 1 0 1  45
    1 0 0 1 1 0 0   76  0 1 0 0 1 0 1  37
    0 1 0 0 1 1 1   39  0 1 0 1 1 1 0  46
    0 1 0 0 0 1 1   35  0 0 0 1 0 1 0  10
    0 0 1 1 1 1 1   31  0 0 1 1 1 0 0  28
    0 0 1 0 1 0 1   21  0 0 0 1 1 0 0  12
    0 0 0 1 1 1 1   15  1 0 1 1 1 0 0  92
    0 0 0 1 1 0 1   13  1 0 0 1 1 0 0  76
    0 0 0 1 1 0 0   12  1 0 0 0 1 0 0  68
    0 0 0 0 1 1 1    6  0 1 1 1 1 0 0  60
    0 0 0 0 1 0 1    5  0 1 0 1 1 0 0  44
    0 0 0 0 1 0 0    4  0 1 0 0 1 0 0  36
    0 0 0 0 0 1 1    3  0 0 1 1 1 0 0  28
    0 0 0 0 0 0 1    1  0 0 0 1 1 0 0  12
    0 0 0 0 0 0 0    0  0 0 0 0 1 0 0   4
                        | | | | | | |
                        | | | | | | |     OUTPUTS
                        | | | | | | +-  1 zcityalt zcity
                        | | | | | +---  2 ucityalt ucity
                        | | | | +-----  4 city, st, "00000" ## SKIP THIS ##
                        | | | +-------  8 city, st, zip, ucityalt
                        | | +--------- 16 ucity, ust, zip, ucityalt
                        | +----------- 32 city, st, zcta, zcityalt
                        +------------- 64 zcity, zst, zcta, zcityalt

I think we probably do NOT want to generate records of the
'city, st "00000"' case as this should probably be generated in PAGC
for ALL records because a user might know know the zipcode when
making a geocode_request.

*/


int analyze_cases(char *zip, char *st, char *ust, char *zcta, char *zst)
{
    int incode = 0;

    if (strlen(zip))        incode |=  1;
    if (strlen(ust))        incode |=  2;
    if (strlen(zcta))       incode |=  4;
    if (strlen(zst))        incode |=  8;
    if (!strcmp(zip, zcta)) incode |= 16;
    if (!strcmp(st, ust))   incode |= 32;
    if (!strcmp(st, zst))   incode |= 64;

    switch (incode) {
        case 127: return 18; break;
        case  79: return 57; break;
        case  77: return 45; break;
        case  76: return 37; break;
        case  39: return 46; break;
        case  35: return 10; break;
        case  31: return 28; break;
        case  21: return 12; break;
        case  15: return 92; break;
        case  13: return 76; break;
        case  12: return 68; break;
        case   6: return 60; break;
        case   5: return 44; break;
        case   4: return 36; break;
        case   3: return 28; break;
        case   1: return 12; break;
        case   0: return  4; break;
    }
    printf("ERROR: analyze_cases: got an unknown incode (%d)!\n", incode);
    return 0;
    //exit(1);
}


#define NPLACE 11

#ifdef SINGLE_SIDED
int output_cases(int code, struct handle_struct *H, long tlid, long tfid, struct streets_struct *c, char *lname)
{
    int cnt = 0;
    char *city   = coalesce(&lname[5*35], &lname[4*35], &lname[3*35]);
    char *st     = &lname[1*35];
    char *zip    = c->zip;
    char *ucity  = &lname[6*35];
    char *ust    = &lname[8*35];
    char *zcta   = &lname[7*35];
    char *zcity  = &lname[9*35];
    char *zst    = &lname[10*35];
    //char *ocity = coalesce((code & 2) ? ucity : NULL, (code & 1) ? zcity : NULL, NULL);
    char *ocity = coalesce((code & 2) ? ucity : "", (code & 1) ? zcity : "", "");

/*  we changed the indexes in PAGC so this is not needed
    if (code &  4)
        cnt += write_rec(H, tlid, tfid, c, city,  st,  "00000", "", &lname[3*35], &lname[2*35], &lname[0]);
*/
    if (code &  8)
        cnt += write_rec(H, tlid, tfid, c, city,  st,  zip,     ocity, &lname[3*35], &lname[2*35], &lname[0]);
    if (code & 16)
        cnt += write_rec(H, tlid, tfid, c, ucity, ust, zip,     ocity, &lname[3*35], &lname[2*35], &lname[0]);
    if (code & 32)
        cnt += write_rec(H, tlid, tfid, c, city,  st,  zcta,    ocity, &lname[3*35], &lname[2*35], &lname[0]);
    if (code & 64)
        cnt += write_rec(H, tlid, tfid, c, zcity, zst, zcta,    ocity, &lname[3*35], &lname[2*35], &lname[0]);
    return cnt;
}
#endif


void checkCousub(struct handle_struct *H, char *names)
{
    int rc;
    int ovec[12];

    rc = pcre_exec(H->re_three, NULL, &names[3*35], strlen(&names[3*35]), 0, 0, ovec, 12);
    /* if we have a match, then cousub name is not usefull
       and we will copy the county name over it. */
    if (rc > 0) {
        strcpy(&names[3*35], &names[2*35]);
        return;
    }

    rc = pcre_exec(H->re_comma, NULL, &names[3*35], strlen(&names[3*35]), 0, 0, ovec, 12);
    /* if we have a match, then cousub starts with "^\d+, "
       and we will remove this */
    if (rc > 1)
        strcpy(&names[3*35], &names[3*35]+ovec[1]);

}


int saveRecord(struct handle_struct *H, long tlid, struct streets_struct *c)
{
    /*
     *  placenames right and left
     *  0 - countrycode or country
     *  1 - state/province abbrv or name
     *  2 - county/region name
     *  3 - county sub-division name
     *  4 - place name || concty
     *  5 - village || concty || sub-division name
     *  6 - USPS postal city
     *  7 - ZCTA5 number
     *  8 - USPS postal state
     *  9 - ZCTA5 city
     * 10 - ZCTA5 state
     */

    static char lname[NPLACE][35];
#ifndef SINGLE_SIDED
    static char rname[NPLACE][35];
#endif
    char state[35];
    char nfmt[17];
    char sfmt[17];
    char *cfmt;
    long tfid;
    int fnum;
    int tnum;
    int mtfcc;
    int i;
    int code;
    int ret = 0;
    SHPObject *s;

    memset(lname, 0, NPLACE*35);
#ifndef SINGLE_SIDED
    memset(rname, 0, NPLACE*35);
#endif

    state[0] = '\0';
#ifndef SINGLE_SIDED        /* do DOUBLE sided output */
    /* if we have right side stuff do it */
    strcpy(rname[0], "US");
    getCountyStateName(H, c->sfips, c->cfips, rname[2], rname[1]);
    getCousubName(H, c->tfidr, rname[3]);
    getConctyName(H, c->tfidr, rname[4]);
    getPlaceName(H, c->tfidr, rname[5]);
    getZCTA5(H, c->tfidr, rname[7]);
    checkCousub(H, (char *)rname);

    if (c->rzip && rname[7] && strlen(rname[7]) && strcmp(c->rzip, rname[7]))
        fprintf(stderr, "TLID=%d: ZIPR=%s, ZCTA=%s\n", tlid, c->rzip, rname[7]);
    
    if (c->sides & 1) {
        getPostalName(H, c->rzip, rname[7], rname[6], state);
        if (strlen(state) && strcmp(state, rname[1]))
            fprintf(stderr, "TLID=%d: ST=%s, ZIP=%s, USPS_ST=%s\n", tlid, rname[1], c->rzip, state);
    }

    /* if we have left side stuff do it */
    strcpy(lname[0], "US");
    getCountyStateName(H, c->sfips, c->cfips, lname[2], lname[1]);
    getCousubName(H, c->tfidl, lname[3]);
    getConctyName(H, c->tfidl, lname[4]);
    getPlaceName(H, c->tfidl, lname[5]);
    getZCTA5(H, c->tfidl, lname[7]);
    checkCousub(H, (char *)lname);

    if (c->lzip && lname[7] && strlen(lname[7]) && strcmp(c->lzip, lname[7]))
        fprintf(stderr, "TLID=%d: ZIPL=%s, ZCTA=%s\n", tlid, c->lzip, lname[7]);
    
    if (c->sides & 2) {
        getPostalName(H, c->lzip, lname[7], lname[6], state);
        if (strlen(state) && strcmp(state, lname[1]))
            fprintf(stderr, "TLID=%d: ST=%s, ZIP=%s, USPS_ST=%s\n", tlid, lname[1], c->lzip, state);
    }
#else       /* do SINGLE sided output */

    /* if we have either side stuff do it */
    strcpy(lname[0], "US");
    getCountyStateName(H, c->statefp, c->countyfp, lname[2], lname[1]);
    if (c->side[0] == 'R') {
        getCousubName(H, c->tfidr, lname[3]);
        getConctyName(H, c->tfidr, lname[4]);
        getPlaceName(H, c->tfidr, lname[5]);
        getZCTA5(H, c->tfidr, lname[7]);
        checkCousub(H, (char *)lname);
        tfid = c->tfidr;
    }
    else if (c->side[0] == 'L') {
        getCousubName(H, c->tfidl, lname[3]);
        getConctyName(H, c->tfidl, lname[4]);
        getPlaceName(H, c->tfidl, lname[5]);
        getZCTA5(H, c->tfidl, lname[7]);
        checkCousub(H, (char *)lname);
        tfid = c->tfidl;
    }
    else if (strlen(c->name) == 0 && KEEP_NO_HN) {
        /* NOP  so continue */
    }
    else {
        return 0;
    }

    if (c->zip && strlen(lname[7]) && strcmp(c->zip, lname[7]))
        fprintf(stderr, "TLID=%ld: SIDE: %s, ZIP=%s, ZCTA=%s\n", tlid, c->side, c->zip, lname[7]);
    
    getPostalName(H, c->zip, lname[7], lname[6], state);

    if (strlen(state) && strcmp(state, lname[1]))
        fprintf(stderr, "TLID=%ld: ST=%s, ZIP=%s, USPS_ST=%s\n", tlid, lname[1], c->zip, state);
#endif

#ifdef SINGLE_SIDED

    code = analyze_cases(c->zip, lname[2], lname[8], lname[7], lname[10]);
    ret = output_cases(code, H, tlid, tfid, c, (char *)lname);

    return ret;
}


int write_rec(struct handle_struct *H, long tlid, long tfid, struct streets_struct *c, char *city, char *st, char *zip, char *ocity, char *ac4, char *ac3, char *ac1)
{
    SHPObject *s;
    char nfmt[17];
    char sfmt[17];
    char *cfmt;
    int fnum;
    int tnum;


#else
    char *ocity = coalesce(lname[6], lname[9], NULL);
    char *city   = coalesce(lname[5], lname[4], lname[3]);
    char *st     = lname[2];
    char *ucity  = lname[6];
    char *zcity  = lname[9];
    char *zip    = c->zip;
    char *zcta   = lname[7];
#endif

    /* write denormalized record to shapefile */

    s = SHPReadObject(H->iSHP, c->recno);
    H->cnt = SHPWriteObject(H->oSHP, -1, s);
    SHPDestroyObject(s);
    
    DBFWriteIntegerAttribute(H->oDBF, H->cnt,  OINDX(F_TLID), tlid);
    DBFWriteStringAttribute(H->oDBF,  H->cnt,  OINDX(F_NAME), c->name);
#ifdef PARSED_NAME
    DBFWriteStringAttribute(H->oDBF,  H->cnt,  OINDX(F_PREDIRABRV), c->predirabrv);
    DBFWriteStringAttribute(H->oDBF,  H->cnt,  OINDX(F_PRETYPABRV), c->pretypabrv);
    DBFWriteStringAttribute(H->oDBF,  H->cnt,  OINDX(F_SUFDIRABRV), c->sufdirabrv);
    DBFWriteStringAttribute(H->oDBF,  H->cnt,  OINDX(F_SUFTYPABRV), c->suftypabrv);
#ifdef FOLD_PREQUAL_2_SUFQUAL
    if (strlen(c->sufqualabr) == 0 && strlen(c->prequalabr)) {
        strcpy(c->sufqualabr, c->prequalabr);
        c->prequalabr[0] = '\0';
    }
#endif
    DBFWriteStringAttribute(H->oDBF,  H->cnt,  OINDX(F_PREQUALABR), c->prequalabr);
    DBFWriteStringAttribute(H->oDBF,  H->cnt,  OINDX(F_SUFQUALABR), c->sufqualabr);
#endif

#ifdef SINGLE_SIDED
    cfmt = scan_nt_nums(c->fromhn, c->tohn, &fnum, &tnum);
    if (strlen(cfmt) == 0 && (strlen(c->fromhn) || strlen(c->tohn)))
        printf("scan_nt_nums Failed %ld ('%s', '%s')\n", tlid, c->fromhn, c->tohn);
    mkSqlFmt(cfmt, sfmt, nfmt);
    DBFWriteIntegerAttribute(H->oDBF, H->cnt, OINDX(F_L_REFADDR),  fnum);
    DBFWriteIntegerAttribute(H->oDBF, H->cnt, OINDX(F_L_NREFADDR), tnum);
    DBFWriteStringAttribute(H->oDBF,  H->cnt, OINDX(F_L_SQLNUMF),  nfmt);
    DBFWriteStringAttribute(H->oDBF,  H->cnt, OINDX(F_L_SQLFMT),   sfmt);
    DBFWriteStringAttribute(H->oDBF,  H->cnt, OINDX(F_L_CFORMAT),  cfmt);

    DBFWriteIntegerAttribute(H->oDBF,  H->cnt, OINDX(F_SIDE), (c->side[0] == 'R')?1:2);
    DBFWriteIntegerAttribute(H->oDBF,  H->cnt, OINDX(F_L_TFID),  tfid);
#else
    cfmt = scan_nt_nums(c->lfromhn, c->ltohn, &fnum, &tnum);
    if (strlen(cfmt) == 0 && (strlen(c->lfromhn) || strlen(c->ltohn)))
        printf("scan_nt_nums Failed %ld ('%s', '%s')\n", tlid, c->lfromhn, c->ltohn);
    mkSqlFmt(cfmt, sfmt, nfmt);
    DBFWriteIntegerAttribute(H->oDBF, H->cnt, OINDX(F_L_REFADDR),  fnum);
    DBFWriteIntegerAttribute(H->oDBF, H->cnt, OINDX(F_L_NREFADDR), tnum);
    DBFWriteStringAttribute(H->oDBF,  H->cnt, OINDX(F_L_SQLNUMF),  nfmt);
    DBFWriteStringAttribute(H->oDBF,  H->cnt, OINDX(F_L_SQLFMT),   sfmt);
    DBFWriteStringAttribute(H->oDBF,  H->cnt, OINDX(F_L_CFORMAT),  cfmt);

    cfmt = scan_nt_nums(c->rfromhn, c->rtohn, &fnum, &tnum);
    if (strlen(cfmt) == 0 && (strlen(c->rfromhn) || strlen(c->rtohn)))
        printf("scan_nt_nums Failed %ld ('%s', '%s')\n", tlid, c->rfromhn, c->rtohn);
    mkSqlFmt(cfmt, sfmt, nfmt);
    DBFWriteIntegerAttribute(H->oDBF, H->cnt, OINDX(F_R_REFADDR),  fnum);
    DBFWriteIntegerAttribute(H->oDBF, H->cnt, OINDX(F_R_NREFADDR), tnum);
    DBFWriteStringAttribute(H->oDBF,  H->cnt, OINDX(F_R_SQLNUMF),  nfmt);
    DBFWriteStringAttribute(H->oDBF,  H->cnt, OINDX(F_R_SQLFNT),   sfmt);
    DBFWriteStringAttribute(H->oDBF,  H->cnt, OINDX(F_R_CFORMAT),  cfmt);
#endif

    DBFWriteStringAttribute(H->oDBF,  H->cnt, OINDX(F_L_USPS), ocity);
    DBFWriteStringAttribute(H->oDBF,  H->cnt, OINDX(F_L_AC5), city);
    if (!city || strlen(city) == 0)
        printf("city missing TLID=%ld, SIDE=%s\n", tlid, c->side);
    DBFWriteStringAttribute(H->oDBF,  H->cnt, OINDX(F_L_AC4), ac4);
    DBFWriteStringAttribute(H->oDBF,  H->cnt, OINDX(F_L_AC3), ac3);
    DBFWriteStringAttribute(H->oDBF,  H->cnt, OINDX(F_L_AC2), st);
    DBFWriteStringAttribute(H->oDBF,  H->cnt, OINDX(F_L_AC1), ac1);
#ifndef SINGLE_SIDED
    DBFWriteStringAttribute(H->oDBF,  H->cnt, OINDX(F_R_USPS), rname[6]);
    DBFWriteStringAttribute(H->oDBF,  H->cnt, OINDX(F_R_AC5),
        coalesce(rname[5], rname[4], rname[3]));
    if (strlen(coalesce(rname[5], rname[4], rname[3])) == 0)
        printf("city missing TLID=%ld, SIDE=%s\n", tlid, c->side);
    DBFWriteStringAttribute(H->oDBF,  H->cnt, OINDX(F_R_AC4), rname[3]);
    DBFWriteStringAttribute(H->oDBF,  H->cnt, OINDX(F_R_AC3), rname[2]);
    DBFWriteStringAttribute(H->oDBF,  H->cnt, OINDX(F_R_AC2), rname[1]);
    DBFWriteStringAttribute(H->oDBF,  H->cnt, OINDX(F_R_AC1), rname[0]);
    DBFWriteStringAttribute(H->oDBF,  H->cnt, OINDX(F_R_POSTCODE), c->rzip);
    DBFWriteStringAttribute(H->oDBF,  H->cnt, OINDX(F_L_POSTCODE), c->lzip);
#else
    DBFWriteStringAttribute(H->oDBF,  H->cnt, OINDX(F_L_POSTCODE), zip);
#endif
#ifdef MTFCC
    mtfcc = strtol(c->mtfcc+1, NULL, 10);
    DBFWriteIntegerAttribute(H->oDBF,  H->cnt, OINDX(F_MTFCC), mtfcc);
    DBFWriteStringAttribute(H->oDBF,   H->cnt, OINDX(F_PASSFLG), c->passflg);
    DBFWriteStringAttribute(H->oDBF,   H->cnt, OINDX(F_DIVROAD), c->divroad);
#endif
    return 1;
}


void processRecords(sqlite3 *db, struct handle_struct *H, int numsplit)
{
    int i, rc, cnt, nrows, seq;
    long tlid;
    static struct streets_struct this;
    char *sql;
    sqlite3_stmt *stmt;

    if (numsplit) {
        sql = "select count(*) from edges where ROADFLG='Y';";
        rc = sqlite3_prepare(db, sql, strlen(sql), &stmt, NULL);
        if (rc != SQLITE_OK) {
            fprintf(stderr, "processRecords: SQL Error: %s\n", sqlite3_errmsg(db));
            return;
        }
        if ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
            nrows = sqlite3_column_int(stmt,   0);
            nrows = nrows/numsplit+1;
            nrows = nrows?nrows:1;
            sqlite3_finalize(stmt);
        }
        printf("Streets-NNN will each have up to %d entities.\n", nrows);
    }

    sql = "select a.tlid, "
#ifdef PARSED_NAME
                " coalesce(b.name,''), "
#else
                " coalesce(a.fullname,'') as name, "
#endif
                " coalesce(b.paflag,''), "
                " a.mtfcc, "
                " a.passflg, "
#if YEAR2017
                " '' as divroad, "
#else
                " a.divroad, "
#endif
                " coalesce(c.fromhn,''), "
                " coalesce(c.tohn,''), "
                " coalesce(c.side,''), "
                " coalesce(c.zip,''), "
                " coalesce(c.plus4,''), "
                " a.statefp, "
                " a.countyfp, "
                " a.tfidl, "
                " a.tfidr, "
#ifdef PARSED_NAME
                " coalesce(b.predirabrv,''), "
                " coalesce(b.pretypabrv,''), "
                " coalesce(b.prequalabr,''), "
                " coalesce(b.sufdirabrv,''), "
                " coalesce(b.suftypabrv,''), "
                " coalesce(b.sufqualabr,''), "
#endif
                " a.recno "
           " from edges as a left outer join addr as c on (a.tlid=c.tlid) "
                " left outer join featnames as b on (a.tlid=b.tlid) "
                " left outer join addrfn as d on "
                "     (b.linearid=d.linearid and c.arid=d.arid) "
         "  where a.roadflg='Y'  and coalesce(a.fullname,'') != '' "
#ifdef PARSED_NAME
                " and coalesce(b.name,'') != '' "
#endif
          " order by a.tlid, a.fullname, c.side;";
    
    
    rc = sqlite3_prepare(db, sql, strlen(sql), &stmt, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "processRecords: SQL Error: %s\n", sqlite3_errmsg(db));
        return;
    }
    
    cnt = 0;
    seq = 2; /* 1 has already been opened and is in H now */
    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {

        tlid = this.tlid =       sqlite3_column_int(stmt,   N_tlid);
        strcpy(this.name,        sqlite3_column_text(stmt,  N_name));
        strcpy(this.paflag,      sqlite3_column_text(stmt,  N_paflag));
        strcpy(this.mtfcc,       sqlite3_column_text(stmt,  N_mtfcc));
        strcpy(this.passflg,     sqlite3_column_text(stmt,  N_passflg));
        strcpy(this.divroad,     sqlite3_column_text(stmt,  N_divroad));
        strcpy(this.fromhn,      sqlite3_column_text(stmt,  N_fromhn));
        strcpy(this.tohn,        sqlite3_column_text(stmt,  N_tohn));
        strcpy(this.side,        sqlite3_column_text(stmt,  N_side));
        strcpy(this.zip,         sqlite3_column_text(stmt,  N_zip));
        strcpy(this.plus4,       sqlite3_column_text(stmt,  N_plus4));
        strcpy(this.statefp,     sqlite3_column_text(stmt,  N_statefp));
        strcpy(this.countyfp,    sqlite3_column_text(stmt,  N_countyfp));
        this.tfidl =             sqlite3_column_int(stmt,   N_tfidl);
        this.tfidr =             sqlite3_column_int(stmt,   N_tfidr);
#ifdef PARSED_NAME
        strcpy(this.predirabrv,  sqlite3_column_text(stmt,  N_predirabrv));
        strcpy(this.pretypabrv,  sqlite3_column_text(stmt,  N_pretypabrv));
        strcpy(this.prequalabr,  sqlite3_column_text(stmt,  N_prequalabr));
        strcpy(this.sufdirabrv,  sqlite3_column_text(stmt,  N_sufdirabrv));
        strcpy(this.suftypabrv,  sqlite3_column_text(stmt,  N_suftypabrv));
        strcpy(this.sufqualabr,  sqlite3_column_text(stmt,  N_sufqualabr));
#endif
        this.recno =             sqlite3_column_int(stmt,   N_recno);
        
        if (saveRecord(H, tlid, &this)) cnt++;
        if (numsplit && cnt >= nrows) {
            if (getNextShape(H, seq++))
                exit(1);
            cnt = 0;
        }
    }
    sqlite3_finalize(stmt);
}

//#define RESTART

#ifdef WITH_PLACENAMES
#define ARGOFF 1
#else
#define ARGOFF 0
#endif

int main( int argc, char **argv ) {
    char    *fAddr;
    char    *fAddrfn;
#ifndef WITH_PLACENAMES
    char    *fFaces;
    char    *fCousub;
    char    *fConcty;
    char    *fPlace;
#endif
    char    *fFeatnames;
    char    *fiStreets;
    char    *foStreets;
    int     i;
    int     ret, t_ret;
    struct handle_struct *H;
    sqlite3 *db;
    sqlite3 *pc;
    sqlite3 *pl = NULL;
    char    *zErrMsg = 0;
    int     rc;
    time_t  s0, s1;
    int     numsplit = 0;
    int     loaded = 0;

    if (argc < 4 + ARGOFF)
        Usage();

    if (argc == 5 + ARGOFF) {
        numsplit = (int) strtol(argv[4 + ARGOFF], NULL, 10);
        if (numsplit < 2) numsplit = 0;
        printf("Streets output file will be split into %d parts\n", numsplit);
    }

    s0 = time(NULL);

    fiStreets    = mkFName(argv[1], "edges.dbf",     0, 0);
    foStreets    = mkFName(argv[2], "Streets.dbf",   1, 0);
    fAddr        = mkFName(argv[1], "addr.dbf",      0, 0);
    fAddrfn      = mkFName(argv[1], "addrfn.dbf",    0, 0);
    fFeatnames   = mkFName(argv[1], "featnames.dbf", 0, 0);
#ifndef WITH_PLACENAMES
    fFaces       = mkFName(argv[1], "faces.dbf",     0, 0);
#if YEAR2008
    fCousub      = mkFName(argv[1], "cousub.dbf",    0, 0);
    fPlace       = mkFName(argv[1], "place.dbf",     0, 1);
    fConcty      = mkFName(argv[1], "concity.dbf",   0, 1);
#elif YEAR2010
    fCousub      = mkFName(argv[1], "cousub10.dbf",  0, 0);
    fPlace       = mkFName(argv[1], "place10.dbf",   0, 1);
    fConcty      = mkFName(argv[1], "concity10.dbf", 0, 1);
#elif YEAR2011 || YEAR2017
    fCousub      = mkFName(argv[1], "cousub.dbf",    0, 1);
    fPlace       = mkFName(argv[1], "place.dbf",     0, 1);
    fConcty      = mkFName(argv[1], "concity.dbf",   0, 1);
#else
    printf("ABORT: COMPILED with INVALID selection for YEAR!\n");
    exit(1);
#endif
#endif

    /* open the usps actual zipcode to city, state dabase */
    rc = sqlite3_open(argv[3 + ARGOFF], &pc);
    if( rc ){
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(pc));
        sqlite3_close(pc);
        exit(1);
    }
    rc = sqlite3_exec(pc, "PRAGMA page_size = 8192;PRAGMA cache_size = 8000;",
            NULL, NULL, &zErrMsg);
    if ( rc != SQLITE_OK ) {
        if (zErrMsg) {
            fprintf(stderr, "Setting PRAGMAs Failed: %s\n", zErrMsg);
            sqlite3_free(zErrMsg);
        }
        sqlite3_close(pc);
        exit(1);
    }

#ifdef WITH_PLACENAMES
    /* open the places names database */
    rc = sqlite3_open(argv[3], &pl);
    if( rc ){
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(pl));
        sqlite3_close(pl);
        sqlite3_close(pc);
        exit(1);
    }
    rc = sqlite3_exec(pl, "PRAGMA page_size = 8192;PRAGMA cache_size = 8000;",
            NULL, NULL, &zErrMsg);
    if ( rc != SQLITE_OK ) {
        if (zErrMsg) {
            fprintf(stderr, "Setting PRAGMAs Failed: %s\n", zErrMsg);
            sqlite3_free(zErrMsg);
        }
        sqlite3_close(pl);
        sqlite3_close(pc);
        exit(1);
    }
#endif

    /* open our working database all the county data gets loaded here */
#ifndef RESTART
    unlink(DATABASE);
#endif
    
    rc = sqlite3_open(DATABASE, &db);
    if( rc ){
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
#ifdef WITH_PLACENAMES
        sqlite3_close(pl);
#endif
        sqlite3_close(pc);
        exit(1);
    }
    rc = sqlite3_exec(db, "PRAGMA page_size = 8192;PRAGMA cache_size = 8000;",
            NULL, NULL, &zErrMsg);
    if ( rc != SQLITE_OK ) {
        if (zErrMsg) {
            fprintf(stderr, "Setting PRAGMAs Failed: %s\n", zErrMsg);
            sqlite3_free(zErrMsg);
        }
        sqlite3_close(db);
#ifdef WITH_PLACENAMES
        sqlite3_close(pl);
#endif
        sqlite3_close(pc);
        exit(1);
    }
        

    /* load *_addr */
#ifndef RESTART
    s1 = time(NULL);
    ret = loadAddr(db, fAddr);
    if (ret == 0) loaded |= L_ADDR;
    printf("loadAddr: time=%d\n", (int)(time(NULL)-s1));
#endif

    /* load *_addrfn */
#ifndef RESTART
    s1 = time(NULL);
    ret = loadAddrfn(db, fAddrfn);
    if (ret == 0) loaded |= L_ADDRFN;
    printf("loadAddrfn: time=%d\n", (int)(time(NULL)-s1));
#endif

#ifndef WITH_PLACENAMES

    /* load *_cousub */
#ifndef RESTART
    s1 = time(NULL);
    ret = loadCousub(db, fCousub);
    if (ret == 0) loaded |= L_COUSUB;
    printf("loadCousub: time=%d\n", (int)(time(NULL)-s1));
#endif

    /* load *_concity */
#ifndef RESTART
    s1 = time(NULL);
    ret = loadConcity(db, fConcty);
    if (ret == 0) loaded |= L_CONCTY;
    printf("loadConcity: time=%d\n", (int)(time(NULL)-s1));
#endif

    /* load *_faces */
#ifndef RESTART
    s1 = time(NULL);
    ret = loadFaces(db, fFaces);
    if (ret == 0) loaded |= L_FACES;
    printf("loadFaces: time=%d\n", (int)(time(NULL)-s1));
#endif

    /* load *_place */
#ifndef RESTART
    s1 = time(NULL);
    ret = loadPlace(db, fPlace);
    if (ret == 0) loaded |= L_PLACE;
    printf("loadPlace: time=%d\n", (int)(time(NULL)-s1));
    ret = 0;
#endif

#endif

    /* load *_featnames */
#ifndef RESTART
    s1 = time(NULL);
    ret = loadFeatnames(db, fFeatnames);
    if (ret == 0) loaded |= L_FEATNAMES;
    printf("loadFeatnames: time=%d\n", (int)(time(NULL)-s1));
#endif

    /* load *_edges */
#ifndef RESTART
    s1 = time(NULL);
    ret = loadEdges(db, fiStreets);
    /* sorry nothing we can do it edges is missing */
    if (ret) goto err;
    printf("loadEdges: time=%d\n", (int)(time(NULL)-s1));
#endif

    printf("  calling createHandle\n");
    H = createHandle(db, pc, pl, fiStreets, foStreets, numsplit?1:0, loaded);
    
    printf("  calling processRecords\n");
    s1 = time(NULL);
    processRecords(db, H, numsplit);
    s1 = time(NULL)-s1;
    printf("processRecords: time=%d, nrec=%d, rec/sec=%.2f\n",
            (int)s1, H->cnt, (float)H->cnt/(float)(s1?s1:1));

    printf("  calling closeHandle\n");
    closeHandle(H);
    
    /* clean up and exit */
err:
    printf("  closing DB and freeing\n");

    sqlite3_close(db);
    sqlite3_close(pc);
#ifdef WITH_PLACENAMES
    sqlite3_close(pl);
#endif
    
    free(fiStreets);
    free(foStreets);
    free(fAddr);
    free(fAddrfn);
    free(fFeatnames);

    printf("Total time: %d sec.\n", (int)(time(NULL)-s0));
    
    exit(ret);
}
