/*********************************************************************
*
*  $Id: tgr2rgeo-sqlite.c,v 1.5 2012-06-03 13:40:59 woodbri Exp $
*
*  tgr2rgeo
*  Copyright 2008, Stephen Woodbridge, All rights Reserved
*  Redistribition without written permission strictly prohibited
*
**********************************************************************
*
*  tgr2rgeo-sqlite.c
*
*  $Log: tgr2rgeo-sqlite.c,v $
*  Revision 1.5  2012-06-03 13:40:59  woodbri
*  Updated to support changes in Tiger 2011 attribute names.
*
*  Revision 1.4  2011/01/29 16:26:52  woodbri
*  Adding files to support PAGC data generation for Tiger and Navteq.
*  Adding options to domaps to support carring, speed, and address ranges.
*
*  Revision 1.3  2010/10/18 02:51:13  woodbri
*  Changes to nt2rgeo-sqlite.c to make Zones optional.
*  Changes to tgr2rgeo-sqlite.c to optionally include mtcfcc in the data.
*  Changes to doNtPolygons.c to include DISP_CLASS to water polygons.
*  Changes to Makefile to support the above.
*
*  Revision 1.2  2009/01/09 03:08:02  woodbri
*  No longer load COUNTYNS column which was not used and is not include in
*  tiger2008 shapefiles.
*
*  Revision 1.1  2008/09/30 18:30:20  woodbri
*  Adding geo-sqlite.c and tgr2rgeo-sqlite.c, updates to Makefile for same.
*  Filter out tileindexes when loading data to pg.
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
* denormalized and suitable for the iMaptools Reverse Geocoder.
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

#define YEAR2017
#define YEAR "2019"

#define DATABASE "tgr2rgeo-sqlite.db"
#define POSTALCODES "usps-actual.db"
#define MAX_NAME 36
#define MAX_NUMW 12

#define MAX(a,b) ((a)>(b)?(a):(b))
#define MIN(a,b) ((a)<(b)?(a):(b))

#define ERR_LOAD_DBF -1
#define ERR_LOAD_INDEX -2

int DEBUG = 0;
int MTFCC = 0;

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
    sqlite3_stmt *stmt_getpc;
    sqlite3_stmt *stmt_getfips;
    sqlite3_stmt *stmt_getcousub;
    sqlite3_stmt *stmt_getplace;
};

#define N_tlid          0
#define N_fullname      1
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
#define N_name          15
#define N_predirabrv    16
#define N_pretypabrv    17
#define N_prequalabr    18
#define N_sufdirabrv    19
#define N_suftypabrv    20
#define N_sugqualabr    21
#define N_recno         22
#else
#define N_recno         15
#endif

#define L_ADDR           1
#define L_ADDRFN         2
#define L_FEATNAMES      4
#define L_FACES          8
#define L_COUSUB        16
#define L_PLACE         32
#define L_STATE         64
#define L_COUNTY       128

struct streets_struct {
    int     recno;
    int     tlid;
    int     tfidl;
    int     tfidr;
    char    fullname[101];
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
    char    name[101];
    char    predirabrv[16];
    char    pretypabrv[51];
    char    prequalabr[16];
    char    sufdirabrv[16];
    char    suftypabrv[51];
    char    sugqualabr[16];
#endif
};

struct current_struct {
    int     recno;
    int     tlid;
    int     tfidl;
    int     tfidr;
    int     sides;
    char    fullname[101];
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
};


void Usage()
{
    printf("Usage: tgr2rgeo [-mtfcc] src/edges.dbf dest_st_dir path/usps-actual.db [num_split]\n");
#ifdef YEAR2010
    printf("       Built for YEAR2010 Tiger file formats.\n");
#elif defined(YEAR2011)
    printf("       Built for YEAR2011 Tiger file formats.\n");
#elif defined(YEAR2017)
    printf("       Built for YEAR2017 Tiger file formats.\n");
#else
    printf("       Built for pre-YEAR2010 Tiger file formats.\n");
#endif
    printf("       Assuming YEAR: %s\n", YEAR);
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


int loadState(sqlite3 *db, char *fin)
{
    int         rc;
    char        *zErrMsg;
    char        *sql;
    const char  *cols[] = {"STATEFP","STUSPS","NAME"};
    
    /* create the tables */

    rc = loadDBF(db, fin, "state", 3, cols, NULL, 1);
    if (rc) {
        fprintf(stderr, "Failed to load DBF file (%s)\n", fin);
        rc = createTable(db, "state", 3, cols, NULL);
        return ERR_LOAD_DBF;
    }

    /* Create indexes that we will need */

    sql = "create index state_fp_idx on state (STATEFP);\n";
    rc = sqlite3_exec(db, sql, NULL, NULL, &zErrMsg);
    if( rc!=SQLITE_OK ){
    	fprintf(stderr, "SQL: %s", sql);
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        return ERR_LOAD_INDEX;
    }

    return 0;
}


int loadCounty(sqlite3 *db, char *fin)
{
    int         rc;
    char        *zErrMsg;
    char        *sql;
    const char  *cols[] = {"STATEFP","COUNTYFP","GEOID","NAME","NAMELSAD"};
    
    /* create the tables */

    rc = loadDBF(db, fin, "county", 5, cols, NULL, 1);
    if (rc) {
        fprintf(stderr, "Failed to load DBF file (%s)\n", fin);
        rc = createTable(db, "county", 5, cols, NULL);
        return ERR_LOAD_DBF;
    }

    /* Create indexes that we will need */

    sql = "create index county_geoid_idx on county (STATEFP,COUNTYFP);\n";
    rc = sqlite3_exec(db, sql, NULL, NULL, &zErrMsg);
    if( rc!=SQLITE_OK ){
    	fprintf(stderr, "SQL: %s", sql);
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        return ERR_LOAD_INDEX;
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


int loadCousub(sqlite3 *db, char *fin)
{
    int     rc;
    char        *zErrMsg;
    char        *sql;
    const char  *cols11[] = {"GEOID","NAME"};
    const char  *cols10[] = {"GEOID10","NAME10"};
    const char  *cols09[] = {"COSBIDFP","NAME"};

    /* create the tables */

#ifdef YEAR2010
    rc = loadDBFrename(db, fin, "cousub", 2, cols10, cols09, NULL, 1);
#elif defined(YEAR2011) || defined(YEAR2017)
    rc = loadDBFrename(db, fin, "cousub", 2, cols11, cols09, NULL, 1);
#else
    rc = loadDBF(db, fin, "cousub", 2, cols09, NULL, 1);
#endif
    if (rc) {
        fprintf(stderr, "Failed to load DBF file (%s)\n", fin);
        return ERR_LOAD_DBF;
    }

    /* Create indexes that we will need */

    sql = "create unique index cousub_cosidfp_idx on cousub (COSBIDFP);\n";
    rc = sqlite3_exec(db, sql, NULL, NULL, &zErrMsg);
    if( rc!=SQLITE_OK ){
    	fprintf(stderr, "SQL: %s", sql);
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        return ERR_LOAD_INDEX;
    }

    return 0;
}


int loadEdges(sqlite3 *db, char *fin)
{
    int     rc;
    char        *zErrMsg;
    char        *sql;
    const char  *cols[] = {"STATEFP","COUNTYFP","TLID","TFIDL",
        "TFIDR","MTFCC","FULLNAME","SMID","LFROMADD","LTOADD","RFROMADD",
        "RTOADD","ZIPL","ZIPR","FEATCAT","HYDROFLG","RAILFLG","ROADFLG",
#ifdef YEAR2017
        /* GCSEFLG is just a dummy field to keep the count correct
           since DIVROAD no longer exists */
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
    	fprintf(stderr, "SQL: %s", sql);
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        return ERR_LOAD_INDEX;
    }

    return 0;
}


int loadFaces(sqlite3 *db, char *fin)
{
    int     rc;
    char        *zErrMsg;
    char        *sql;
    const char  *cols10[] = {"TFID","STATEFP10","COUNTYFP10","COUSUBFP10",
        "PLACEFP10","CONCTYFP10","LWFLAG"};
    const char  *cols09[] = {"TFID","STATEFP","COUNTYFP","COUSUBFP",
        "PLACEFP","CONCTYFP","LWFLAG"};

    /* create the tables */

#ifdef YEAR2010
    rc = loadDBFrename(db, fin, "faces", 7, cols10, cols09, NULL, 1);
#elif defined(YEAR2011) || defined(YEAR2017)
    rc = loadDBF(db, fin, "faces", 7, cols09, NULL, 1);
#else
    rc = loadDBF(db, fin, "faces", 7, cols09, NULL, 1);
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


int loadPlace(sqlite3 *db, char *fin)
{
    int     rc;
    char        *zErrMsg;
    char        *sql;
    const char  *cols11[] = {"GEOID","NAME"};
    const char  *cols10[] = {"GEOID10","NAME10"};
    const char  *cols09[] = {"PLCIDFP","NAME"};
    
    /* create the tables */

#ifdef YEAR2010
    rc = loadDBFrename(db, fin, "place", 2, cols10, cols09, NULL, 1);
#elif defined(YEAR2011) || defined(YEAR2017)
    rc = loadDBFrename(db, fin, "place", 2, cols11, cols09, NULL, 1);
#else
    rc = loadDBF(db, fin, "place", 2, cols09, NULL, 1);
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
    assert(DBFAddField(H->oDBF, "LINK_ID",    FTInteger, 10, 0) != -1);
    assert(DBFAddField(H->oDBF, "NAME",       FTString,  80, 0) != -1);
    assert(DBFAddField(H->oDBF, "L_REFADDR",  FTInteger, 10, 0) != -1);
    assert(DBFAddField(H->oDBF, "L_NREFADDR", FTInteger, 10, 0) != -1);
    assert(DBFAddField(H->oDBF, "L_SQLNUMF",  FTString,  12, 0) != -1);
    assert(DBFAddField(H->oDBF, "L_SQLFMT",   FTString,  16, 0) != -1);
    assert(DBFAddField(H->oDBF, "L_CFORMAT",  FTString,  16, 0) != -1);
    assert(DBFAddField(H->oDBF, "R_REFADDR",  FTInteger, 10, 0) != -1);
    assert(DBFAddField(H->oDBF, "R_NREFADDR", FTInteger, 10, 0) != -1);
    assert(DBFAddField(H->oDBF, "R_SQLNUMF",  FTString,  12, 0) != -1);
    assert(DBFAddField(H->oDBF, "R_SQLFMT",   FTString,  16, 0) != -1);
    assert(DBFAddField(H->oDBF, "R_CFORMAT",  FTString,  16, 0) != -1);
    assert(DBFAddField(H->oDBF, "L_AC5",      FTString,  35, 0) != -1);
    assert(DBFAddField(H->oDBF, "L_AC4",      FTString,  35, 0) != -1);
    assert(DBFAddField(H->oDBF, "L_AC3",      FTString,  35, 0) != -1);
    assert(DBFAddField(H->oDBF, "L_AC2",      FTString,  35, 0) != -1);
    assert(DBFAddField(H->oDBF, "L_AC1",      FTString,  35, 0) != -1);
    assert(DBFAddField(H->oDBF, "R_AC5",      FTString,  35, 0) != -1);
    assert(DBFAddField(H->oDBF, "R_AC4",      FTString,  35, 0) != -1);
    assert(DBFAddField(H->oDBF, "R_AC3",      FTString,  35, 0) != -1);
    assert(DBFAddField(H->oDBF, "R_AC2",      FTString,  35, 0) != -1);
    assert(DBFAddField(H->oDBF, "R_AC1",      FTString,  35, 0) != -1);
    assert(DBFAddField(H->oDBF, "L_POSTCODE", FTString,  11, 0) != -1);
    assert(DBFAddField(H->oDBF, "R_POSTCODE", FTString,  11, 0) != -1);
    if (MTFCC) {
    assert(DBFAddField(H->oDBF, "MTFCC",      FTInteger,  5, 0) != -1);
    assert(DBFAddField(H->oDBF, "PASSFLG",    FTString,   1, 0) != -1);
    assert(DBFAddField(H->oDBF, "DIVROAD",    FTString,   1, 0) != -1);
    }

    return 0;
}
    

struct handle_struct *createHandle(sqlite3 *db, sqlite3 *pc, char *fiStreets, char *foStreets, int seq, int loaded)
{
    char *sin;
    char *sout;
    char *sql;
    int  rc;
    
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
    
    sql = "select city, st from zipcode where zip=?;";
    rc = sqlite3_prepare(pc, sql, strlen(sql), &H->stmt_getpc, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL: %s\n", sql);
        fprintf(stderr, "getPostalCity: SQL Error: %s\n", sqlite3_errmsg(pc));
        exit(1);
    }

    sql = "select c.NAME as county, s.STUSPS as st from state s left join county c on s.STATEFP=c.STATEFP where s.STATEFP=? and c.COUNTYFP=?;";
    rc = sqlite3_prepare(db, sql, strlen(sql), &H->stmt_getfips, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL: %s\n", sql);
        fprintf(stderr, "getStateCounty: SQL Error: %s\n", sqlite3_errmsg(pc));
        exit(1);
    }

/*
    sql = "select county, st from fipscodes where fips=? and fipc=?;";
    rc = sqlite3_prepare(pc, sql, strlen(sql), &H->stmt_getfips, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL: %s\n", sql);
        fprintf(stderr, "getStateCounty: SQL Error: %s\n", sqlite3_errmsg(pc));
        exit(1);
    }
*/

    if (loaded & L_FACES && loaded & L_COUSUB) {
        sql = "select b.name from faces a, cousub b where TFID=? and b.COSBIDFP=a.STATEFP||a.COUNTYFP||a.COUSUBFP;";
        rc = sqlite3_prepare(db, sql, strlen(sql), &H->stmt_getcousub, NULL);
        if (rc != SQLITE_OK) {
            fprintf(stderr, "SQL: %s\n", sql);
            fprintf(stderr, "getFacesInfo: SQL Error: %s\n", sqlite3_errmsg(db));
            exit(1);
        }
    }

    if (loaded & L_FACES && loaded & L_PLACE) {
        sql = "select b.name from faces a, place b where tfid=? and b.PLCIDFP=a.STATEFP||a.PLACEFP;";
        rc = sqlite3_prepare(db, sql, strlen(sql), &H->stmt_getplace, NULL);
        if (rc != SQLITE_OK) {
            fprintf(stderr, "SQL: %s\n", sql);
            fprintf(stderr, "getFacesInfo: SQL Error: %s\n", sqlite3_errmsg(db));
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
    
    /* free memory */
    memset(H, 0, sizeof(struct handle_struct));
    free(H);
}


int getPostalName(struct handle_struct *H, char *zip, char *city, char *st)
{
    sqlite3_stmt *stmt;
    int rc;
    int ret = 0;

    if (!zip)
        return ret;
    
    stmt = H->stmt_getpc;

    rc = sqlite3_bind_text(stmt, 1, zip, strlen(zip), SQLITE_STATIC);
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
    sqlite3_reset(stmt);
    return ret;
}


int getFacesInfo(struct handle_struct *H, int tfid, int indx, char *name)
{
    sqlite3_stmt *stmt;
    char *sql;
    int rc;
    int ret = 0;

    if ( !tfid || ((H->loaded & indx) == 0) )
        return ret;

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
#define getPlaceName(a,b,c)  getFacesInfo(a,b,L_PLACE,c)

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
                        MAX(strlen(ftok[i]), strlen(ttok[i])));
                *nf = strtol(ftok[i], NULL, 10);
                *nt = strtol(ttok[i], NULL, 10);
            }
            else if (ftyp[i] == 'N') {
                sprintf(fmt+strlen(fmt), "%%%dd", 
                        MAX(strlen(ftok[i]), strlen(ttok[i])));
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
        printf("merge_hn: scan_nt_nums: tlid: %d a('%s', '%S')\n",
                tlid, af, at);
    if (!strlen(bfmt) && (strlen(bf) || strlen(bt)))
        printf("merge_hn: scan_nt_nums: tlid: %d b('%s', '%S')\n",
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


void saveRecord(struct handle_struct *H, long tlid, struct current_struct *c)
{
    /*
     *  placenames right and left
     *  0 - countrycode or country
     *  1 - state/province abbrv or name
     *  2 - county/region name
     *  3 - county sub-division name
     *  4 - postal city or place name
     *  5 - village or sub-division name
     *  6 - TBD
     */
    static char lname[7][35];
    static char rname[7][35];
    char nfmt[17];
    char sfmt[17];
    char *cfmt;
    int fnum;
    int tnum;
    int mtfcc;
    int i;
    SHPObject *s;

    memset(lname, 0, 7*35);
    memset(rname, 0, 7*35);

    /* if we have right side stuff do it */
    strcpy(rname[0], "US");
    getCountyStateName(H, c->sfips, c->cfips, rname[2], rname[1]);
    getCousubName(H, c->tfidr, rname[3]);
    if (!strlen(rname[4])) getPlaceName(H, c->tfidr, rname[4]);
    if (c->sides & 1) {
        getPostalName(H, c->rzip, rname[4], rname[1]);
    }

    /* if we have left side stuff do it */
    strcpy(lname[0], "US");
    getCountyStateName(H, c->sfips, c->cfips, lname[2], lname[1]);
    getCousubName(H, c->tfidl, lname[3]);
    if (!strlen(lname[4])) getPlaceName(H, c->tfidl, lname[4]);
    if (c->sides & 2) {
        getPostalName(H, c->lzip, lname[4], lname[1]);
    }

    if (0) {
        printf(" L(%ld)=%s:%s:%s:%s:%s:%s:%s\n",
                c->tlid, lname[0], lname[1], lname[2], lname[3],
                lname[4], lname[5], lname[6]);
        printf(" R(%ld)=%s:%s:%s:%s:%s:%s:%s\n",
                c->tlid, rname[0], rname[1], rname[2], rname[3],
                rname[4], rname[5], rname[6]);
    }

    /* write denormalized record to shapefile */

    s = SHPReadObject(H->iSHP, c->recno);
    H->cnt = SHPWriteObject(H->oSHP, -1, s);
    SHPDestroyObject(s);
    
    DBFWriteIntegerAttribute(H->oDBF, H->cnt,  0, tlid);
    DBFWriteStringAttribute(H->oDBF,  H->cnt,  1, c->fullname);

    cfmt = scan_nt_nums(c->lfromhn, c->ltohn, &fnum, &tnum);
    if (strlen(cfmt) == 0 && (strlen(c->lfromhn) || strlen(c->ltohn)))
        printf("scan_nt_nums Failed %d ('%s', '%s')\n", tlid, c->lfromhn, c->ltohn);
    mkSqlFmt(cfmt, sfmt, nfmt);
    DBFWriteIntegerAttribute(H->oDBF, H->cnt,  2, fnum);
    DBFWriteIntegerAttribute(H->oDBF, H->cnt,  3, tnum);
    DBFWriteStringAttribute(H->oDBF,  H->cnt,  4, nfmt);
    DBFWriteStringAttribute(H->oDBF,  H->cnt,  5, sfmt);
    DBFWriteStringAttribute(H->oDBF,  H->cnt,  6, cfmt);

    cfmt = scan_nt_nums(c->rfromhn, c->rtohn, &fnum, &tnum);
    if (strlen(cfmt) == 0 && (strlen(c->rfromhn) || strlen(c->rtohn)))
        printf("scan_nt_nums Failed %d ('%s', '%s')\n", tlid, c->rfromhn, c->rtohn);
    mkSqlFmt(cfmt, sfmt, nfmt);
    DBFWriteIntegerAttribute(H->oDBF, H->cnt,  7, fnum);
    DBFWriteIntegerAttribute(H->oDBF, H->cnt,  8, tnum);
    DBFWriteStringAttribute(H->oDBF,  H->cnt,  9, nfmt);
    DBFWriteStringAttribute(H->oDBF,  H->cnt, 10, sfmt);
    DBFWriteStringAttribute(H->oDBF,  H->cnt, 11, cfmt);

    DBFWriteStringAttribute(H->oDBF,  H->cnt, 12, lname[4]);
    DBFWriteStringAttribute(H->oDBF,  H->cnt, 13, lname[3]);
    DBFWriteStringAttribute(H->oDBF,  H->cnt, 14, lname[2]);
    DBFWriteStringAttribute(H->oDBF,  H->cnt, 15, lname[1]);
    DBFWriteStringAttribute(H->oDBF,  H->cnt, 16, lname[0]);
    DBFWriteStringAttribute(H->oDBF,  H->cnt, 17, rname[4]);
    DBFWriteStringAttribute(H->oDBF,  H->cnt, 18, rname[3]);
    DBFWriteStringAttribute(H->oDBF,  H->cnt, 19, rname[2]);
    DBFWriteStringAttribute(H->oDBF,  H->cnt, 20, rname[1]);
    DBFWriteStringAttribute(H->oDBF,  H->cnt, 21, rname[0]);
    DBFWriteStringAttribute(H->oDBF,  H->cnt, 22, c->lzip);
    DBFWriteStringAttribute(H->oDBF,  H->cnt, 23, c->rzip);
    if (MTFCC) {
        mtfcc = strtol(c->mtfcc+1, NULL, 10);
        DBFWriteIntegerAttribute(H->oDBF,  H->cnt, 24, mtfcc);
        DBFWriteStringAttribute(H->oDBF,   H->cnt, 25, c->passflg);
#ifdef YEAR2017
        DBFWriteStringAttribute(H->oDBF,   H->cnt, 26, "");
#else
        DBFWriteStringAttribute(H->oDBF,   H->cnt, 26, c->divroad);
#endif
    }
}


void processRecords(sqlite3 *db, struct handle_struct *H, int numsplit)
{
    int i, rc, cnt, nrows, seq;
    long tlid;
    static struct current_struct cur;
    static struct streets_struct this;
    char *sql;
    sqlite3_stmt *stmt;

    if (numsplit) {
        sql = "select count(distinct tlid) from edges where ROADFLG='Y';";
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
                " coalesce(a.fullname,''), "
                " coalesce(b.paflag,''), "
                " a.mtfcc, "
                " a.passflg, "
#ifdef YEAR2017
                " a.gcseflg as divroad, "
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
                " coalesce(c.name,''), "
                " coalesce(c.predirabrv,''), "
                " coalesce(c.pretypabrv,''), "
                " coalesce(c.prequalabr,''), "
                " coalesce(c.sufdirabrv,''), "
                " coalesce(c.suftypabrv,''), "
                " coalesce(c.sufqualabr,''), "
#endif
                " a.recno "
           " from edges as a left outer join addr as c on (a.tlid=c.tlid) "
                " left outer join featnames as b on (a.tlid=b.tlid) "
                " left outer join addrfn as d on "
                "     (b.linearid=d.linearid and c.arid=d.arid) "
         "  where a.roadflg='Y' "
          " order by a.tlid, a.fullname, c.side;";
    
    
    rc = sqlite3_prepare(db, sql, strlen(sql), &stmt, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "processRecords: SQL Error: %s\n", sqlite3_errmsg(db));
        return;
    }
    
    long last_tlid = -1;
    cnt = 0;
    seq = 2; /* 1 has already been opened and is in H now */
    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {

        tlid = this.tlid =       sqlite3_column_int(stmt,   N_tlid);
        strcpy(this.fullname,    sqlite3_column_text(stmt,  N_fullname));
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
        this.tfidl =             sqlite3_column_int(stmt,  N_tfidl);
        this.tfidr =             sqlite3_column_int(stmt,  N_tfidr);
#ifdef PARSED_NAME
        strcpy(this.name,        sqlite3_column_text(stmt,  N_name));
        strcpy(this.predirabrv,  sqlite3_column_text(stmt,  N_predirabrv));
        strcpy(this.pretypabrv,  sqlite3_column_text(stmt,  N_pretypabrv));
        strcpy(this.prequalabr,  sqlite3_column_text(stmt,  N_prequalabr));
        strcpy(this.sufdirabrv,  sqlite3_column_text(stmt,  N_sufdirabrv));
        strcpy(this.suftypabrv,  sqlite3_column_text(stmt,  N_suftypabrv));
        strcpy(this.sufqualabr,  sqlite3_column_text(stmt,  N_sufqualabr));
#endif
        this.recno =             sqlite3_column_int(stmt,   N_recno);
        
        if (last_tlid != tlid && last_tlid != -1) {
            saveRecord(H, last_tlid, &cur);
            memset(&cur, 0, sizeof(struct current_struct));
            last_tlid = -1;
            cnt++;
            if (numsplit && cnt >= nrows) {
                if (getNextShape(H, seq++))
                    exit(1);
                cnt = 0;
            }
        }
        
        /* rules for selecting which of multiple records we use */

        /* if it is the first records take it */
        if (last_tlid == -1) {
            last_tlid = tlid;
            cur.tlid  = this.tlid;
            cur.tfidl = this.tfidl;
            cur.tfidr = this.tfidr;
            cur.recno = this.recno;
            strcpy(cur.mtfcc, this.mtfcc);
            strcpy(cur.passflg, this.passflg);
            strcpy(cur.divroad, this.divroad);
            strcpy(cur.sfips, this.statefp);
            strcpy(cur.cfips, this.countyfp);
        }
        if (this.paflag[0] == 'P' || this.paflag[0] == '\0') {
            if (strlen(cur.fullname) && strcmp(cur.fullname, this.fullname)) 
                printf("WARN: Dup paflag: P, tlid: %ld, '%s', '%s'\n",
                        tlid, cur.fullname, this.fullname);
            strcpy(cur.fullname, this.fullname);
        }
        else {
            if (strlen(cur.altname) && strcmp(cur.altname, this.fullname))
                printf("WARN: Dup paflag: A, tlid: %ld, '%s', '%s'\n",
                        tlid, cur.altname, this.fullname);
            strcpy(cur.altname, this.fullname);
        }
        merge_RL(&cur, &this);
    }
    if (last_tlid != -1)
        saveRecord(H, last_tlid, &cur);
    sqlite3_finalize(stmt);
}

#define RESTART 0

int main( int argc, char **argv ) {
    char    fState[35];
    char    fCounty[35];
    char    *fAddr;
    char    *fAddrfn;
    char    *fFaces;
    char    *fCousub;
    char    *fPlace;
    char    *fFeatnames;
    char    *fiStreets;
    char    *foStreets;
    int     i;
    int     ret, t_ret;
    struct handle_struct *H;
    sqlite3 *db;
    sqlite3 *pc;
    char    *zErrMsg = 0;
    int     rc;
    time_t  s0, s1;
    int     numsplit = 0;
    int     loaded = 0;

    if (argc >= 4 && !strcmp(argv[1], "-mtfcc")) {
        MTFCC = 1;
        argv++;
        argc--;
    }

    if (argc < 4)
        Usage();

    if (argc == 5) {
        numsplit = (int) strtol(argv[4], NULL, 10);
        if (numsplit < 2) numsplit = 0;
        printf("Streets output file will be split into %d parts\n", numsplit);
    }

    s0 = time(NULL);

    fiStreets    = mkFName(argv[1], "edges.dbf",     0, 0);
    foStreets    = mkFName(argv[2], "Streets.dbf",   1, 0);
    fAddr        = mkFName(argv[1], "addr.dbf",      0, 0);
    fAddrfn      = mkFName(argv[1], "addrfn.dbf",    0, 0);
    fFeatnames   = mkFName(argv[1], "featnames.dbf", 0, 0);
    fFaces       = mkFName(argv[1], "faces.dbf",      0, 0);
#ifdef YEAR2010
    fCousub      = mkFName(argv[1], "cousub10.dbf",  0, 0);
    fPlace       = mkFName(argv[1], "place10.dbf",   0, 1);
#elif defined(YEAR2011) || defined(YEAR2017)
    fCousub      = mkFName(argv[1], "cousub.dbf",    0, 1);
    fPlace       = mkFName(argv[1], "place.dbf",     0, 1);
#else
    fCousub      = mkFName(argv[1], "cousub.dbf",    0, 0);
    fPlace       = mkFName(argv[1], "place.dbf",     0, 1);
#endif
    sprintf(fState, "SAVE/tl_%s_us_state.dbf", YEAR);
    sprintf(fCounty, "SAVE/tl_%s_us_county.dbf", YEAR);
    
    /* open the usps actual zipcode to city, state dabase */
    rc = sqlite3_open(argv[3], &pc);
    if( rc ){
        fprintf(stderr, "Can't open database %s: %s\n", argv[3], sqlite3_errmsg(pc));
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
        exit(1);
    }

    /* open our working database all the county data gets loaded here */
    if (!RESTART)
        unlink(DATABASE);
    
    rc = sqlite3_open(DATABASE, &db);
    if( rc ){
        fprintf(stderr, "Can't open database %s: %s\n", DATABASE, sqlite3_errmsg(db));
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


    /* load *_state */
    if (!RESTART) {
        s1 = time(NULL);
        ret = loadState(db, fState);
        if (ret == 0) loaded |= L_STATE;
        printf("loadAddr: time=%d\n", time(NULL)-s1);
    }

    /* load *_county */
    if (!RESTART) {
        s1 = time(NULL);
        ret = loadCounty(db, fCounty);
        if (ret == 0) loaded |= L_COUNTY;
        printf("loadAddr: time=%d\n", time(NULL)-s1);
    }

    /* load *_addr */
    if (!RESTART) {
        s1 = time(NULL);
        ret = loadAddr(db, fAddr);
        if (ret == 0) loaded |= L_ADDR;
        printf("loadAddr: time=%d\n", time(NULL)-s1);
    }

    /* load *_addrfn */
    if (!RESTART) {
        s1 = time(NULL);
        ret = loadAddrfn(db, fAddrfn);
        if (ret == 0) loaded |= L_ADDRFN;
        printf("loadAddrfn: time=%d\n", time(NULL)-s1);
    }

    /* load *_cousub */
    if (!RESTART) {
        s1 = time(NULL);
        ret = loadCousub(db, fCousub);
        if (ret == 0) loaded |= L_COUSUB;
        printf("loadCousub: time=%d\n", time(NULL)-s1);
    }

    /* load *_faces */
    if (!RESTART) {
        s1 = time(NULL);
        ret = loadFaces(db, fFaces);
        if (ret == 0) loaded |= L_FACES;
        printf("loadFaces: time=%d\n", time(NULL)-s1);
    }

    /* load *_featnames */
    if (!RESTART) {
        s1 = time(NULL);
        ret = loadFeatnames(db, fFeatnames);
        if (ret == 0) loaded |= L_FEATNAMES;
        printf("loadFeatnames: time=%d\n", time(NULL)-s1);
    }

    /* load *_edges */
    if (!RESTART) {
        s1 = time(NULL);
        ret = loadEdges(db, fiStreets);
        /* sorry nothing we can do it edges is missing */
        if (ret) goto err;
        printf("loadEdges: time=%d\n", time(NULL)-s1);
    }

    /* load *_place */
    if (!RESTART) {
        s1 = time(NULL);
        ret = loadPlace(db, fPlace);
        if (ret == 0) loaded |= L_PLACE;
        printf("loadPlace: time=%d\n", time(NULL)-s1);
    }

    printf("  calling createHandle\n");
    H = createHandle(db, pc, fiStreets, foStreets, numsplit?1:0, loaded);
    
    printf("  calling processRecords\n");
    s1 = time(NULL);
    processRecords(db, H, numsplit);
    s1 = time(NULL)-s1;
    printf("processRecords: time=%d, nrec=%d, rec/sec=%.2f\n",
            s1, H->cnt, (float)H->cnt/(float)(s1?s1:1));

    printf("  calling closeHandle\n");
    closeHandle(H);
    
    /* clean up and exit */
err:
    printf("  closing DB and freeing\n");

    sqlite3_close(db);
    sqlite3_close(pc);
    
    free(fiStreets);
    free(foStreets);
    free(fAddr);
    free(fAddrfn);
    free(fFeatnames);

    printf("Total time: %d sec.\n", time(NULL)-s0);
    
    exit(ret);
}
