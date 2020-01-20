/*********************************************************************
*
*  $Id: nt2pagc-sqlite.c,v 1.2 2013-11-01 15:07:33 woodbri Exp $
*
*  nt2pagc
*  Copyright 2008, Stephen Woodbridge, All rights Reserved
*  Redistribition without written permission strictly prohibited
*
**********************************************************************
*
*  nt2pagc.c
*
*  $Log: nt2pagc-sqlite.c,v $
*  Revision 1.2  2013-11-01 15:07:33  woodbri
*  Changes for Tiger 2012/2013 shapefile changes.
*
*  Revision 1.1  2011/01/29 16:26:52  woodbri
*  Adding files to support PAGC data generation for Tiger and Navteq.
*  Adding options to domaps to support carring, speed, and address ranges.
*
*
*  2011-01-14 sew; Initial version cloned from nt2rgeo-sqlite.c
*
**********************************************************************
*
* This utility requires the following Navteq files:
*   Streets
*   AltStreets
*   MtdArea
*   MtdZoneRec
*   Zones
* It will read this files into a sqlite3 database and then create
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

#define DATABASE "nt2pagc-sqlite.db"
#define POSTALCODES "usps-actual.db"
#define MAX_NAME 36
#define MAX_NUMW 12

#define RESTART 0
#define SINGLE_SIDED
#undef PARSED_NAME
#define FOLD_PREQUAL_2_SUFQUAL

#define MAX(a,b) ((a)>(b)?(a):(b))

/*
    KEEP_NO_HN 1 - to keep streets with no house number ranges
                   This is useful for geocoding intersections

    KEEP_NO_HN 0 - to skip streets with no house number ranges
*/
#define KEEP_NO_HN 0

#define ERR_LOAD_DBF -1
#define ERR_LOAD_INDEX -2

int DEBUG = 0;
int GotAltStreets = 1;
int GOT_USPS = 0;

struct place_struct {
    int lvl;
    char zname[MAX_NAME];
    long zareaid;
    char name[7][MAX_NAME];
    int v;
};

struct ac_key_struct {
    int lvl;
    int ac[7];
};

struct handle_struct {
    char    *fiStreets;
    DBFHandle   iDBF;
    SHPHandle   iSHP;
    char    *foStreets;
    DBFHandle   oDBF;
    SHPHandle   oSHP;
    int     cnt;
    int     nEnt;
    sqlite3 *db;
    sqlite3_stmt *stmt_getpz;
    sqlite3_stmt *stmt_getpn1;
    sqlite3_stmt *stmt_getpn2;
    sqlite3 *pc;
    sqlite3_stmt *stmt_getpc;

};

struct streets_struct {
    int     recno;
    char    st_name[81];
    int     num_stnmes;
    char    addr_type[2];
    char    l_refaddr[11];
    char    l_nrefaddr[11];
    char    l_addrsch[2];
    char    l_addrform[2];
    char    r_refaddr[11];
    char    r_nrefaddr[11];
    char    r_addrsch[2];
    char    r_addrform[2];
    long    ref_in_id;
    long    nref_in_id;
    long    l_area_id;
    long    r_area_id;
    char    l_postcode[12];
    char    r_postcode[12];
    int     l_numzones;
    int     r_numzones;
    int     num_ad_rng;
#ifdef PARSED_NAME
    char    st_nm_pref[3];
    char    st_typ_bef[31];
    char    st_nm_base[36];
    char    st_nm_suff[3];
    char    st_typ_aft[31];
    char    st_typ_att[2];
#endif
};


void Usage()
{
    printf("Usage: nt2pagc srcdir destdir [-u usps-actual.db] [num_split]\n");
    exit(1);
}


int loadMtdArea(sqlite3 *db, char *fin)
{
    int         rc;
    char        *zErrMsg;
    char        *sql;
    const char  *cols[] = {"ADMIN_LVL", "AREA_ID", "AREACODE_1", "AREACODE_2",
        "AREACODE_3", "AREACODE_4", "AREACODE_5", "AREACODE_6", "AREACODE_7",
        "AREA_NAME", "AREA_TYPE", "LANG_CODE"};
    
    /* create the tables */

    rc = loadDBF(db, fin, "mtdarea", 12, cols, NULL, 1);
    if (rc) {
        fprintf(stderr, "Failed to load DBF file (%s)\n", fin);
        return ERR_LOAD_DBF;
    }

    /* Create indexes that we will need */

    sql = "create index mtdarea_ac_idx on mtdarea (ADMIN_LVL,AREACODE_1,AREACODE_2,AREACODE_3,AREACODE_4,AREACODE_5,AREACODE_6,AREACODE_7);\n";
    rc = sqlite3_exec(db, sql, NULL, NULL, &zErrMsg);
    if( rc!=SQLITE_OK ){
    	fprintf(stderr, "SQL: %s", sql);
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        return ERR_LOAD_INDEX;
    }
    
    sql = "create index mtdarea_aid_idx on mtdarea (AREA_ID);\n";
    rc = sqlite3_exec(db, sql, NULL, NULL, &zErrMsg);
    if( rc!=SQLITE_OK ){
    	fprintf(stderr, "SQL: %s", sql);
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        return ERR_LOAD_INDEX;
    }

    return 0;
}


int loadMtdZoneRec(sqlite3 *db, char *fin)
{
    int         rc;
    char        *zErrMsg;
    char        *sql;
    const char  *cols[] = {"ZONE_ID", "ZONE_NAME", "LANG_CODE", "Z_NMTYPE",
        "ZONE_TYPE", "AREA_ID"};

    /* create the tables */

    rc = loadDBF(db, fin, "mtdzonerec", 6, cols, NULL, 1);
    if (rc) {
        fprintf(stderr, "Failed to load DBF file (%s)\n", fin);
        return ERR_LOAD_DBF;
    }

    /* Create indexes that we will need */

    sql = "create index mtdzonerec_zoneid_idx on mtdzonerec (ZONE_ID);\n";
    rc = sqlite3_exec(db, sql, NULL, NULL, &zErrMsg);
    if( rc!=SQLITE_OK ){
    	fprintf(stderr, "SQL: %s", sql);
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        return ERR_LOAD_INDEX;
    }
    
    return 0;
}


int loadZones(sqlite3 *db, char *fin)
{
    int     rc;
    char        *zErrMsg;
    char        *sql;
    const char  *cols[] = {"LINK_ID", "ZONE_ID", "SIDE"};
    
    /* create the tables */

    rc = loadDBF(db, fin, "zones", 3, cols, NULL, 1);
    if (rc) {
        fprintf(stderr, "Failed to load DBF file (%s)\n", fin);
        return ERR_LOAD_DBF;
    }

    /* Create indexes that we will need */

    sql = "create index zones_linkid_idx on zones (LINK_ID);\n";
    rc = sqlite3_exec(db, sql, NULL, NULL, &zErrMsg);
    if( rc!=SQLITE_OK ){
    	fprintf(stderr, "SQL: %s", sql);
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        return ERR_LOAD_INDEX;
    }

    sql = "create index zones_zoneid_idx on zones (ZONE_ID);\n";
    rc = sqlite3_exec(db, sql, NULL, NULL, &zErrMsg);
    if( rc!=SQLITE_OK ){
    	fprintf(stderr, "SQL: %s", sql);
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        return ERR_LOAD_INDEX;
    }

    return 0;
}


int loadStreets(sqlite3 *db, char *fin)
{
    int         rc, ncols;
    char        *zErrMsg;
    char        *sql;
    const char  *cols[] = {"LINK_ID", "ST_NAME", "NUM_STNMES", "ADDR_TYPE",
        "L_REFADDR", "L_NREFADDR", "L_ADDRSCH", "L_ADDRFORM",
        "R_REFADDR", "R_NREFADDR", "R_ADDRSCH", "R_ADDRFORM",
        "REF_IN_ID", "NREF_IN_ID", "L_AREA_ID", "R_AREA_ID", 
        "L_POSTCODE", "R_POSTCODE", "L_NUMZONES", "R_NUMZONES", 
        "NUM_AD_RNG"
#ifdef PARSED_NAME
            , "ST_NM_PREF", "ST_TYP_BEF", "ST_NM_BASE", "ST_NM_SUFF"
            , "ST_TYP_AFT", "ST_TYP_ATT"
#endif
            , 0};

    ncols = 0;
    while (cols[ncols]) ncols++;
    
    /* create the tables */

    rc = loadDBF(db, fin, "streets", ncols, cols, "RECNO", 1);
    if (rc) {
        fprintf(stderr, "Failed to load DBF file (%s)\n", fin);
        return ERR_LOAD_DBF;
    }

    /* Create indexes that we will need */

    sql = "create index streets_linkid_idx on streets (LINK_ID);\n";
    rc = sqlite3_exec(db, sql, NULL, NULL, &zErrMsg);
    if( rc!=SQLITE_OK ){
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        return ERR_LOAD_INDEX;
    }

    return 0;
}

/*
 * AltStreets.dbf
Num     Name            Type    Len     Decimal
1.      LINK_ID         N       10      0
2.      ST_NAME         C       80      0
3.      FEAT_ID         N       10      0
4.      ST_LANGCD       C       3       0
5.      ST_NM_PREF      C       2       0
6.      ST_TYP_BEF      C       30      0
7.      ST_NM_BASE      C       35      0
8.      ST_NM_SUFF      C       2       0
9.      ST_TYP_AFT      C       30      0
10.     ST_TYP_ATT      C       1       0
11.     ADDR_TYPE       C       1       0
12.     L_REFADDR       C       10      0
13.     L_NREFADDR      C       10      0
14.     L_ADDRSCH       C       1       0
15.     L_ADDRFORM      C       1       0
16.     R_REFADDR       C       10      0
17.     R_NREFADDR      C       10      0
18.     R_ADDRSCH       C       1       0
19.     R_ADDRFORM      C       1       0
20.     NUM_AD_RNG      N       2       0
21.     ROUTE_TYPE      C       1       0
22.     DIRONSIGN       C       1       0
23.     EXPLICATBL      C       1       0
24.     NAMEONRDSN      C       1       0
25.     POSTALNAME      C       1       0
26.     STALENAME       C       1       0
27.     VANITYNAME      C       1       0
28.     JUNCTIONNM      C       1       0
29.     EXITNAME        C       1       0
30.     SCENIC_NM       C       1       0
*/

int loadAltStreets(sqlite3 *db, char *fin)
{
    int     rc;
    int     ncols;
    char        *zErrMsg;
    char        *sql;
    const char  *cols[] = {"LINK_ID", "ST_NAME", "ADDR_TYPE",
        "L_REFADDR", "L_NREFADDR", "L_ADDRSCH", "L_ADDRFORM",
        "R_REFADDR", "R_NREFADDR", "R_ADDRSCH", "R_ADDRFORM",
        "NUM_AD_RNG"
#ifdef PARSED_NAME
            , "ST_NM_PREF", "ST_TYP_BEF", "ST_NM_BASE", "ST_NM_SUFF"
            , "ST_TYP_AFT", "ST_TYP_ATT"
#endif
            , 0};
    
    ncols = 0;
    while (cols[ncols]) ncols++;
    
    /* create the tables */

    rc = loadDBF(db, fin, "altstreets", ncols, cols, NULL, 1);
    if (rc) {
        fprintf(stderr, "Failed to load DBF file (%s)\n", fin);
        return ERR_LOAD_DBF;
    }

    /* Create indexes that we will need */

    sql = "create index altstreets_linkid_idx on altstreets (LINK_ID);\n";
    rc = sqlite3_exec(db, sql, NULL, NULL, &zErrMsg);
    if( rc!=SQLITE_OK ){
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        return ERR_LOAD_INDEX;
    }

    return 0;
}


char *mkFName(char *path, char *file)
{
    char *f;
    f = calloc(strlen(path)+strlen(file)+2, sizeof(char));
    assert(f);

    strcpy(f, path);
    if (f[strlen(f)-1] != '/')
        strcat(f, "/");
    strcat(f, file);

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
#ifdef PARSED_NAME
    assert(DBFAddField(H->oDBF, "ST_NM_PREF", FTString,   2, 0) != -1);
    assert(DBFAddField(H->oDBF, "ST_TYP_BEF", FTString,  30, 0) != -1);
    assert(DBFAddField(H->oDBF, "ST_NM_BASE", FTString,  35, 0) != -1);
    assert(DBFAddField(H->oDBF, "ST_NM_SUFF", FTString,   2, 0) != -1);
    assert(DBFAddField(H->oDBF, "ST_TYP_AFT", FTString,  30, 0) != -1);
    assert(DBFAddField(H->oDBF, "ST_TYP_ATT", FTString,   1, 0) != -1);
#endif
#ifdef SINGLE_SIDED
    assert(DBFAddField(H->oDBF, "REFADDR",    FTInteger, 10, 0) != -1);
    assert(DBFAddField(H->oDBF, "NREFADDR",   FTInteger, 10, 0) != -1);
    assert(DBFAddField(H->oDBF, "OREFADDR",   FTString,  10, 0) != -1);
    assert(DBFAddField(H->oDBF, "ONREFADDR",  FTString,  10, 0) != -1);
    assert(DBFAddField(H->oDBF, "ADDR_TYPE",  FTString,   1, 0) != -1);
    assert(DBFAddField(H->oDBF, "ADDRSCH",    FTString,   1, 0) != -1);
    assert(DBFAddField(H->oDBF, "ADDRFORM",   FTString,   2, 0) != -1);
    assert(DBFAddField(H->oDBF, "SIDE",       FTString,   1, 0) != -1);
    assert(DBFAddField(H->oDBF, "AC5",        FTString,  35, 0) != -1);
    assert(DBFAddField(H->oDBF, "AC4",        FTString,  35, 0) != -1);
    assert(DBFAddField(H->oDBF, "AC3",        FTString,  35, 0) != -1);
    assert(DBFAddField(H->oDBF, "AC2",        FTString,  35, 0) != -1);
    assert(DBFAddField(H->oDBF, "AC1",        FTString,  35, 0) != -1);
    assert(DBFAddField(H->oDBF, "POSTCODE",   FTString,  11, 0) != -1);
#else
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
#endif
    return 0;
}
    

struct handle_struct *createHandle(sqlite3 *db, sqlite3 *pc, char *fiStreets, char *foStreets, int seq, int gotZones)
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

    /* prepare some sqlite queries that we need to reuse */

    H->db = db;
    H->pc = pc;
    
    if (pc && GOT_USPS) {
        sql = "select city, st from zipcode where zip=?;";
        rc = sqlite3_prepare(pc, sql, strlen(sql), &H->stmt_getpc, NULL);
        if (rc != SQLITE_OK) {
            fprintf(stderr, "SQL: %s\n", sql);
            fprintf(stderr, "getPostalCity: SQL Error: %s\n", sqlite3_errmsg(pc));
            exit(1);
        }
    }

    if (gotZones) {
        sql = "select a.SIDE, b.* from zones a, mtdzonerec b where a.LINK_ID=? and a.ZONE_ID=b.ZONE_ID;";
        rc = sqlite3_prepare(db, sql, strlen(sql), &H->stmt_getpz, NULL);
        if (rc != SQLITE_OK) {
            fprintf(stderr, "SQL: %s\n", sql);
            fprintf(stderr, "getPostalZone: SQL Error: %s\n", sqlite3_errmsg(db));
            exit(1);
        }
    }

    sql = "select * from mtdarea where AREA_ID=? order by AREA_TYPE;";
    rc = sqlite3_prepare(db, sql, strlen(sql), &H->stmt_getpn1, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL: %s\n", sql);
        fprintf(stderr, "getName: SQL Error: %s\n", sqlite3_errmsg(db));
        exit(1);
    }

    sql = "select AREA_ID from mtdarea where ADMIN_LVL=? and AREACODE_1=? and AREACODE_2=? and AREACODE_3=? and AREACODE_4=? and AREACODE_5=? and AREACODE_6=? and AREACODE_7=?;";
    rc = sqlite3_prepare(db, sql, strlen(sql), &H->stmt_getpn2, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL: %s\n", sql);
        fprintf(stderr, "getName: SQL Error: %s\n", sqlite3_errmsg(db));
        exit(1);
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

    if (H->stmt_getpz)
        sqlite3_finalize(H->stmt_getpz);
    sqlite3_finalize(H->stmt_getpn1);
    sqlite3_finalize(H->stmt_getpn2);
    if (H->stmt_getpc) sqlite3_finalize(H->stmt_getpc);
    
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

    stmt = H->stmt_getpc;

    if (!zip || !stmt ||!GOT_USPS)
        return ret;

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

// TODO : change the following to return an array of place names
//        then create multiple output records one for each.

void getPostalZone(struct handle_struct *H, long linkid, struct place_struct *PL, struct place_struct *PR)
{
    sqlite3_stmt *stmt;
    char side[2];
    char zname[MAX_NAME];
    char nmtyp[2];
    char ztype[3];
    char *sql;
    int rc;
    int v;
    int areaid;
    
/*
    sql = "select a.SIDE, b.* from zones a, mtdzonerec b where a.LINK_ID=? and a.ZONE_ID=b.ZONE_ID;";
    rc = sqlite3_prepare(db, sql, strlen(sql), &stmt, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL: %s\n", sql);
        fprintf(stderr, "getPostalZone: SQL Error: %s\n", sqlite3_errmsg(db));
        return;
    }
*/
    stmt = H->stmt_getpz;
    if (!stmt) return;

    rc = sqlite3_bind_int(stmt, 1, linkid);
    if ( rc!=SQLITE_OK ) {
        fprintf(stderr, "getPostalZone: SQL bind_int error\n");
        exit(1);
    }

    rc = sqlite3_step(stmt);
    while(rc == SQLITE_ROW) {
        strcpy(side,  sqlite3_column_text(stmt, 0));
        strcpy(zname, sqlite3_column_text(stmt, 2));
        strcpy(nmtyp, sqlite3_column_text(stmt, 4));
        strcpy(ztype, sqlite3_column_text(stmt, 5));
        areaid =      sqlite3_column_int(stmt,  6);

/*
    TODO - Add in these when selecting multiple names
           Remember that names will get standardized
           in the geocoder so abbreviations may not be
           kept as unique names.

    ztype = KA - Known As - Replaces Admin
            KD - Known As - Does Not Replace Admin
            PA - Postal Area
            GC - Greater City  -- Don't care about this
            TA - TMC Area      -- Don't care about this

    nmtyp - A - Abbreviation
            B - Base
            E - Exonym         -- Don't care about this
            S - Synonym
*/

        if (!strcmp(ztype, "PA"))
            v = 3;
        else if (nmtyp[0] == 'A')
            v = 2;
        else if (nmtyp[0] == 'B')
            v = 1;
        else
            v = 0;

        switch (side[0]) {
            case 'L':
                if (!PL->v || v > PL->v) {
                    strcpy(PL->zname, zname);
                    PL->zareaid = areaid;
                    PL->v = v;
                }
                break;
            case 'R':
                if (!PR->v || v > PR->v) {
                    strcpy(PR->zname, zname);
                    PR->zareaid = areaid;
                    PR->v = v;
                }
                break;
            case 'B':
            default:
                if (!PL->v || v > PL->v) {
                    strcpy(PL->zname, zname);
                    PL->zareaid = areaid;
                    PL->v = v;
                }
                if (!PR->v || v > PR->v) {
                    strcpy(PR->zname, zname);
                    PR->zareaid = areaid;
                    PR->v = v;
                }
                break;
        }
        rc = sqlite3_step(stmt);
    }
    sqlite3_reset(stmt);
}


long areaCode2AreaID(sqlite3_stmt *stmt, struct ac_key_struct *ac)
{
    struct ac_key_struct *a;
    int areaid = 0;
    int i, rc;

    rc = sqlite3_bind_int(stmt, 1, ac->lvl);
    if ( rc!=SQLITE_OK ) {
        fprintf(stderr, "getName: SQL bind_int error\n");
        exit(1);
    }
    for (i=0; i<7; i++) {
        rc = sqlite3_bind_int(stmt, i+2, ac->ac[i]);
        if ( rc!=SQLITE_OK ) {
            fprintf(stderr, "getName: SQL bind_int error\n");
            exit(1);
        }
    }

    rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW)
        areaid = sqlite3_column_int(stmt, 0);

    rc = sqlite3_reset(stmt);
    if ( rc!=SQLITE_OK )
        fprintf(stderr, "areaCode2AreaID: SQL reset error\n");
    
    return areaid;
}


int getName(sqlite3_stmt *stmt, long areaid, char *name, struct ac_key_struct *parent)
{
    struct ac_data_struct *ac = NULL;
    char *sql;
    char type[2];
    int lvl;
    int rc;

    name[0] = '\0';

    rc = sqlite3_bind_int(stmt, 1, areaid);
    if ( rc!=SQLITE_OK ) {
        fprintf(stderr, "getName: SQL bind_int error\n");
        exit(1);
    }

    rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
        while (rc == SQLITE_ROW) {
            strcpy(name, sqlite3_column_text(stmt,  9));
            lvl = parent->lvl = sqlite3_column_int(stmt, 0);
            parent->ac[0] = sqlite3_column_int(stmt, 2);
            parent->ac[1] = sqlite3_column_int(stmt, 3);
            parent->ac[2] = sqlite3_column_int(stmt, 4);
            parent->ac[3] = sqlite3_column_int(stmt, 5);
            parent->ac[4] = sqlite3_column_int(stmt, 6);
            parent->ac[5] = sqlite3_column_int(stmt, 7);
            parent->ac[6] = sqlite3_column_int(stmt, 8);
            parent->lvl--;
            parent->ac[parent->lvl] = 0;
            strcpy(type, sqlite3_column_text(stmt, 10));
            if (type[0] == 'A' ||  type[0] == 'B') break;
            rc = sqlite3_step(stmt);
        }
    }
    else
        lvl = 0;

    rc = sqlite3_reset(stmt);
    if ( rc!=SQLITE_OK ) {
        fprintf(stderr, "getName: SQL reset error\n");
    }
    return lvl;
}

/*

-- This SQL will get all the city|county|state|country names for a zone_id
-- or an area_id, excluding Exonyms
-- It seems at first glance, that area_id==zone_id 

select distinct a.zone_id, a.area_id, a.zone_type, a.area_type, a.area_name as city, (select area_name as county from mtdarea where admin_lvl=3 and areacode_1=a.areacode_1 and areacode_2=a.areacode_2 and areacode_3=a.areacode_3 and lang_code='ENG' and area_type='B' ) as county, (select area_name as state from mtdarea where admin_lvl=2 and areacode_1=a.areacode_1 and areacode_2=a.areacode_2 and lang_code='ENG' and area_type='B' ) as state, (select area_name as state from mtdarea where admin_lvl=1 and areacode_1=a.areacode_1 and lang_code='ENG' and area_type='A' ) as country from (select * from mtdarea b, mtdzonerec c where b.area_id=c.area_id and area_type != 'E' ) as a;

*/

void getPlaceNames(struct handle_struct *H, long areaid, struct place_struct *P)
{
    sqlite3_stmt *stmt, *stmt2;
    static char name[MAX_NAME];
    static struct ac_key_struct parent;
    int lvl;

    /*
    char *sql;
    int rc;

    sql = "select * from mtdarea where AREA_ID=?;";
    rc = sqlite3_prepare(db, sql, strlen(sql), &stmt, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL: %s\n", sql);
        fprintf(stderr, "getName: SQL Error: %s\n", sqlite3_errmsg(db));
        exit(1);
    }

    sql = "select AREA_ID from mtdarea where ADMIN_LVL=? and AREACODE_1=? and AREACODE_2=? and AREACODE_3=? and AREACODE_4=? and AREACODE_5=? and AREACODE_6=? and AREACODE_7=?;";
    rc = sqlite3_prepare(db, sql, strlen(sql), &stmt2, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL: %s\n", sql);
        fprintf(stderr, "getName: SQL Error: %s\n", sqlite3_errmsg(db));
        exit(1);
    }
*/
    stmt  = H->stmt_getpn1;
    stmt2 = H->stmt_getpn2;
    
    lvl = getName(stmt, areaid, name, &parent);
    P->lvl = lvl;
    strcpy(P->name[lvl-1], name);
    while (lvl > 0) {
        areaid = areaCode2AreaID(stmt2, &parent);
        lvl = getName(stmt, areaid, name, &parent);
        strcpy(P->name[lvl-1], name);
    }
}


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
    if (strlen(f) == 0 && strlen(t) == 0)
        return bad;
    
    for (i=0; i<10; i++) {
        memset(ftok[i], 0,  MAX_NUMW);
        memset(ttok[i], 0,  MAX_NUMW);
    }
    memset(ftyp, 0,  MAX_NUMW);
    memset(ttyp, 0,  MAX_NUMW);
    memset(fmt,  0,  2*MAX_NUMW);

    ntf = tokenize_num(f, ftok, ftyp);
    ntt = tokenize_num(t, ttok, ttyp);

    if (ntf != ntt || ntf == 0)
        return bad;

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



void saveRecord(struct handle_struct *H, long linkid, struct streets_struct *c)
{
    static struct place_struct PL, PR;
    char nfmt[17];
    char sfmt[17];
    char *cfmt;
    int fnum;
    int tnum;
    int off = 0;
    long LAreaID = -1;
    long RAreaID = -1;
    SHPObject *s;

    memset(&PL, 0, sizeof(struct place_struct));
    memset(&PR, 0, sizeof(struct place_struct));

    // TODO collect and array of placenames associated with teh record
    //      and output all of them

    /* retrieve the zones and find the postal name */
    if (c->l_numzones || c->r_numzones)
        getPostalZone(H, linkid, &PL, &PR);

    LAreaID = PL.zareaid?PL.zareaid:c->l_area_id;
    RAreaID = PR.zareaid?PR.zareaid:c->r_area_id;
    
    /* retrieve the parent hierarchy of the zones */
    getPlaceNames(H, LAreaID, &PL);
    if (LAreaID == RAreaID)
        PR = PL;
    else
        getPlaceNames(H, RAreaID, &PR);

    if (GOT_USPS) {
        if (strlen(c->l_postcode))
            getPostalName(H, c->l_postcode, PL.name[4], PL.name[1]);
        if (strlen(c->r_postcode))
            getPostalName(H, c->r_postcode, PR.name[4], PR.name[1]);
    }

    if (0) {
        printf(" L(%d)=%s:%s:%s:%s:%s:%s:%s\n",
                PL.lvl, PL.name[0], PL.name[1], PL.name[2], PL.name[3],
                PL.name[4], PL.name[5], PL.name[6]);
        printf(" R(%d)=%s:%s:%s:%s:%s:%s:%s\n",
                PR.lvl, PR.name[0], PR.name[1], PR.name[2], PR.name[3],
                PR.name[4], PR.name[5], PR.name[6]);
    }

    /* write denormalized record to shapefile */

    s = SHPReadObject(H->iSHP, c->recno);
#ifdef SINGLE_SIDED
    /* write out the left side */
    cfmt = scan_nt_nums(c->l_refaddr, c->l_nrefaddr, &fnum, &tnum);
    if (fnum || tnum) {
        H->cnt = SHPWriteObject(H->oSHP, -1, s);
        
        DBFWriteIntegerAttribute(H->oDBF, H->cnt,  0, linkid);
        DBFWriteStringAttribute(H->oDBF,  H->cnt,  1, c->st_name);
#ifdef PARSED_NAME
        off = 6;
        DBFWriteStringAttribute(H->oDBF,  H->cnt,  2, c->st_nm_pref);
        DBFWriteStringAttribute(H->oDBF,  H->cnt,  3, c->st_typ_bef);
        DBFWriteStringAttribute(H->oDBF,  H->cnt,  4, c->st_nm_base);
        DBFWriteStringAttribute(H->oDBF,  H->cnt,  5, c->st_nm_suff);
        DBFWriteStringAttribute(H->oDBF,  H->cnt,  6, c->st_typ_aft);
        DBFWriteStringAttribute(H->oDBF,  H->cnt,  7, c->st_typ_att);
#endif
        DBFWriteIntegerAttribute(H->oDBF, H->cnt,  2+off, fnum);
        DBFWriteIntegerAttribute(H->oDBF, H->cnt,  3+off, tnum);
        DBFWriteStringAttribute(H->oDBF,  H->cnt,  4+off, c->l_refaddr);
        DBFWriteStringAttribute(H->oDBF,  H->cnt,  5+off, c->l_nrefaddr);
        DBFWriteStringAttribute(H->oDBF,  H->cnt,  6+off, c->addr_type);
        DBFWriteStringAttribute(H->oDBF,  H->cnt,  7+off, c->l_addrsch);
        DBFWriteStringAttribute(H->oDBF,  H->cnt,  8+off, c->l_addrform);

        DBFWriteStringAttribute(H->oDBF,  H->cnt,  9+off, "L");

        DBFWriteStringAttribute(H->oDBF,  H->cnt, 10+off, PL.name[4]);
        DBFWriteStringAttribute(H->oDBF,  H->cnt, 11+off, PL.name[3]);
        DBFWriteStringAttribute(H->oDBF,  H->cnt, 12+off, PL.name[2]);
        DBFWriteStringAttribute(H->oDBF,  H->cnt, 13+off, PL.name[1]);
        DBFWriteStringAttribute(H->oDBF,  H->cnt, 14+off, PL.name[0]);
        DBFWriteStringAttribute(H->oDBF,  H->cnt, 15+off, c->l_postcode);
    }

    /* write out the right side */
    cfmt = scan_nt_nums(c->r_refaddr, c->r_nrefaddr, &fnum, &tnum);
    if (fnum || tnum) {
        H->cnt = SHPWriteObject(H->oSHP, -1, s);

        DBFWriteIntegerAttribute(H->oDBF, H->cnt,  0, linkid);
        DBFWriteStringAttribute(H->oDBF,  H->cnt,  1, c->st_name);
#ifdef PARSED_NAME
        off = 6;
        DBFWriteStringAttribute(H->oDBF,  H->cnt,  2, c->st_nm_pref);
        DBFWriteStringAttribute(H->oDBF,  H->cnt,  3, c->st_typ_bef);
        DBFWriteStringAttribute(H->oDBF,  H->cnt,  4, c->st_nm_base);
        DBFWriteStringAttribute(H->oDBF,  H->cnt,  5, c->st_nm_suff);
        DBFWriteStringAttribute(H->oDBF,  H->cnt,  6, c->st_typ_aft);
        DBFWriteStringAttribute(H->oDBF,  H->cnt,  7, c->st_typ_att);
#endif
        DBFWriteIntegerAttribute(H->oDBF, H->cnt,  2+off, fnum);
        DBFWriteIntegerAttribute(H->oDBF, H->cnt,  3+off, tnum);
        DBFWriteStringAttribute(H->oDBF,  H->cnt,  4+off, c->r_refaddr);
        DBFWriteStringAttribute(H->oDBF,  H->cnt,  5+off, c->r_nrefaddr);
        DBFWriteStringAttribute(H->oDBF,  H->cnt,  6+off, c->addr_type);
        DBFWriteStringAttribute(H->oDBF,  H->cnt,  7+off, c->r_addrsch);
        DBFWriteStringAttribute(H->oDBF,  H->cnt,  8+off, c->r_addrform);

        DBFWriteStringAttribute(H->oDBF,  H->cnt,  9+off, "R");

        DBFWriteStringAttribute(H->oDBF,  H->cnt, 10+off, PR.name[4]);
        DBFWriteStringAttribute(H->oDBF,  H->cnt, 11+off, PR.name[3]);
        DBFWriteStringAttribute(H->oDBF,  H->cnt, 12+off, PR.name[2]);
        DBFWriteStringAttribute(H->oDBF,  H->cnt, 13+off, PR.name[1]);
        DBFWriteStringAttribute(H->oDBF,  H->cnt, 14+off, PR.name[0]);
        DBFWriteStringAttribute(H->oDBF,  H->cnt, 15+off, c->r_postcode);
    }
    SHPDestroyObject(s);
#else
    H->cnt = SHPWriteObject(H->oSHP, -1, s);
    SHPDestroyObject(s);
    
    DBFWriteIntegerAttribute(H->oDBF, H->cnt,  0, linkid);
    DBFWriteStringAttribute(H->oDBF,  H->cnt,  1, c->st_name);

    cfmt = scan_nt_nums(c->l_refaddr, c->l_nrefaddr, &fnum, &tnum);
    mkSqlFmt(cfmt, sfmt, nfmt);
    DBFWriteIntegerAttribute(H->oDBF, H->cnt,  2, fnum);
    DBFWriteIntegerAttribute(H->oDBF, H->cnt,  3, tnum);
    DBFWriteStringAttribute(H->oDBF,  H->cnt,  4, nfmt);
    DBFWriteStringAttribute(H->oDBF,  H->cnt,  5, sfmt);
    DBFWriteStringAttribute(H->oDBF,  H->cnt,  6, cfmt);

    cfmt = scan_nt_nums(c->r_refaddr, c->r_nrefaddr, &fnum, &tnum);
    mkSqlFmt(cfmt, sfmt, nfmt);
    DBFWriteIntegerAttribute(H->oDBF, H->cnt,  7, fnum);
    DBFWriteIntegerAttribute(H->oDBF, H->cnt,  8, tnum);
    DBFWriteStringAttribute(H->oDBF,  H->cnt,  9, nfmt);
    DBFWriteStringAttribute(H->oDBF,  H->cnt, 10, sfmt);
    DBFWriteStringAttribute(H->oDBF,  H->cnt, 11, cfmt);

    DBFWriteStringAttribute(H->oDBF,  H->cnt, 12, PL.name[4]);
    DBFWriteStringAttribute(H->oDBF,  H->cnt, 13, PL.name[3]);
    DBFWriteStringAttribute(H->oDBF,  H->cnt, 14, PL.name[2]);
    DBFWriteStringAttribute(H->oDBF,  H->cnt, 15, PL.name[1]);
    DBFWriteStringAttribute(H->oDBF,  H->cnt, 16, PL.name[0]);
    DBFWriteStringAttribute(H->oDBF,  H->cnt, 17, PR.name[4]);
    DBFWriteStringAttribute(H->oDBF,  H->cnt, 18, PR.name[3]);
    DBFWriteStringAttribute(H->oDBF,  H->cnt, 19, PR.name[2]);
    DBFWriteStringAttribute(H->oDBF,  H->cnt, 20, PR.name[1]);
    DBFWriteStringAttribute(H->oDBF,  H->cnt, 21, PR.name[0]);
    DBFWriteStringAttribute(H->oDBF,  H->cnt, 22, c->l_postcode);
    DBFWriteStringAttribute(H->oDBF,  H->cnt, 23, c->r_postcode);
#endif
}


int processQuery(sqlite3 *db, struct handle_struct *H, int nrows, char *sql)
{
    int i, rc, seq, nrec;
    long linkid;
    static struct streets_struct this;
    sqlite3_stmt *stmt;

    rc = sqlite3_prepare(db, sql, strlen(sql), &stmt, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "processQuery: SQL Error: %s\n", sqlite3_errmsg(db));
        return 1;
    }
    
    seq = 2; /* 1 has already been opened and is in H now */
    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {

        linkid =                 sqlite3_column_int(stmt,   0);
        strcpy(this.st_name,     sqlite3_column_text(stmt,  1));
        this.num_stnmes =        sqlite3_column_int(stmt,   2);
        strcpy(this.addr_type,   sqlite3_column_text(stmt,  3));
        strcpy(this.l_refaddr,   sqlite3_column_text(stmt,  4));
        strcpy(this.l_nrefaddr,  sqlite3_column_text(stmt,  5));
        strcpy(this.l_addrsch,   sqlite3_column_text(stmt,  6));
        strcpy(this.l_addrform,  sqlite3_column_text(stmt,  7));
        strcpy(this.r_refaddr,   sqlite3_column_text(stmt,  8));
        strcpy(this.r_nrefaddr,  sqlite3_column_text(stmt,  9));
        strcpy(this.r_addrsch,   sqlite3_column_text(stmt, 10));
        strcpy(this.r_addrform,  sqlite3_column_text(stmt, 11));
        this.ref_in_id =         sqlite3_column_int(stmt,  12);
        this.nref_in_id =        sqlite3_column_int(stmt,  13);
        this.l_area_id =         sqlite3_column_int(stmt,  14);
        this.r_area_id =         sqlite3_column_int(stmt,  15);
        strcpy(this.l_postcode,  sqlite3_column_text(stmt, 16));
        strcpy(this.r_postcode,  sqlite3_column_text(stmt, 17));
        this.l_numzones =        sqlite3_column_int(stmt,  18);
        this.r_numzones =        sqlite3_column_int(stmt,  19);
        this.num_ad_rng =        sqlite3_column_int(stmt,  20);
        this.recno =             sqlite3_column_int(stmt,  21);
#ifdef PARSED_NAME
        strcpy(this.st_nm_pref,  sqlite3_column_text(stmt, 22);
        strcpy(this.st_typ_bef,  sqlite3_column_text(stmt, 23);
        strcpy(this.st_nm_base,  sqlite3_column_text(stmt, 24);
        strcpy(this.st_nm_suff,  sqlite3_column_text(stmt, 25);
        strcpy(this.st_typ_aft,  sqlite3_column_text(stmt, 26);
        strcpy(this.st_typ_att,  sqlite3_column_text(stmt, 27);
#endif
        
        saveRecord(H, linkid, &this);
        if (nrows && H->cnt >= nrows) {
            if (getNextShape(H, seq++))
                exit(1);
        }
        
    }
    sqlite3_finalize(stmt);
    return 0;
}


void processRecords(sqlite3 *db, struct handle_struct *H, int numsplit)
{
    int rc, nrec;
    int nrows = 0;
    char *sql;
    sqlite3_stmt *stmt;

#ifdef SINGLE_SIDED
    nrec = 2;
#else
    nrec = 1;
#endif

    if (numsplit) {
        sql = "select count(*) from streets";
        rc = sqlite3_prepare(db, sql, strlen(sql), &stmt, NULL);
        if (rc != SQLITE_OK) {
            fprintf(stderr, "processRecords: SQL Error: %s\n", sqlite3_errmsg(db));
            return;
        }
        if ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
            nrows = sqlite3_column_int(stmt,   0);
            sqlite3_finalize(stmt);
        }

        if (GotAltStreets) {
            sql = "select count(*) from altstreets a, streets b where a.LINK_ID=b.LINK_ID";
            rc = sqlite3_prepare(db, sql, strlen(sql), &stmt, NULL);
            if (rc != SQLITE_OK) {
                fprintf(stderr, "processRecords: SQL Error: %s\n", sqlite3_errmsg(db));
                return;
            }
            if ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
                nrows += sqlite3_column_int(stmt,   0);
                sqlite3_finalize(stmt);
            }
        }
        nrows = nrows*nrec/numsplit+1;
        nrows = nrows?nrows:1;
        printf("Streets-NNN will each have up to %d entities.\n", nrows);
    }

    sql = "select * from streets order by LINK_ID";
    if (processQuery(db, H, nrows, sql)) {
        printf("Error: Query '%s' failed!\n", sql);
        return;
    }

    if (GotAltStreets) {
        sql = "select a.LINK_ID, a.ST_NAME, 1 as NUM_STNMES, a.ADDR_TYPE, a.L_REFADDR, a.L_NREFADDR, a.L_ADDRSCH, a.L_ADDRFORM, a.R_REFADDR, a.R_NREFADDR, a.R_ADDRSCH, a.R_ADDRFORM, b.REF_IN_ID, b.NREF_IN_ID, b.L_AREA_ID, b.R_AREA_ID, b.L_POSTCODE, b.R_POSTCODE, b.L_NUMZONES, b.R_NUMZONES, a.NUM_AD_RNG, b.RECNO "
#ifdef PARSED_NAME
        "a.ST_NM_PREF, a.ST_TYP_BEF, a.ST_NM_BASE, a.ST_NM_SUFF, a.ST_TYP_AFT, a.ST_TYP_ATT "
#endif
        "from altstreets a, streets b where a.LINK_ID=b.LINK_ID order by a.LINK_ID";
        if (processQuery(db, H, nrows, sql)) {
            printf("Error: Query '%s' failed!\n", sql);
            return;
        }
    }

    return;
}


int main( int argc, char **argv ) {
    char    *fiAltStreets;
    char    *fiStreets;
    char    *foStreets;
    char    *fMtdArea;
    char    *fZones;
    char    *fMtdZoneRec;
    int     i;
    int     ret, t_ret;
    struct handle_struct *H;
    sqlite3 *db;
    sqlite3 *pc = NULL;
    char    *zErrMsg = 0;
    int     rc;
    time_t  s0, s1;
    int     numsplit = 0;
    int     gotZones = 0;

    if (argc < 3)
        Usage();

    if (argc == 6)
        numsplit = (int) strtol(argv[5], NULL, 10);
    else if (argc == 4 && strcmp(argv[3], "-u"))
        numsplit = (int) strtol(argv[3], NULL, 10);

    if (numsplit < 2) numsplit = 0;
    if (numsplit)
        printf("Streets output file will be split into about %d parts\n", numsplit);

    s0 = time(NULL);

    fiAltStreets = mkFName(argv[1], "AltStreets.dbf");
    fiStreets    = mkFName(argv[1], "Streets.dbf");
    foStreets    = mkFName(argv[2], "Streets.dbf");
    fMtdArea     = mkFName(argv[1], "MtdArea.dbf");
    fMtdZoneRec  = mkFName(argv[1], "MtdZoneRec.dbf");
    fZones       = mkFName(argv[1], "Zones.dbf");

    /* open the usps actual zipcode to city, state dabase */
    if (!strcmp(argv[3], "-u")) {
        rc = sqlite3_open(argv[4], &pc);
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
            exit(1);
        }
        GOT_USPS = 1;
    }

    if (!RESTART)
        unlink(DATABASE);
    
    rc = sqlite3_open(DATABASE, &db);
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
        

    /* load MtdArea */
    if (!RESTART) {
        s1 = time(NULL);
        ret = loadMtdArea(db, fMtdArea);
        if (ret) goto err;
        printf("loadMtdArea: time=%d\n", time(NULL)-s1);
    }

    /* load MtdZoneRec */
    if (!RESTART) {
        s1 = time(NULL);
        ret = loadMtdZoneRec(db, fMtdZoneRec);
        /* if (ret) goto err; */
        if (ret) gotZones = 0;
        printf("loadMtdZoneRec: time=%d\n", time(NULL)-s1);
    }

    /* load Zones */
    if (!RESTART) {
        s1 = time(NULL);
        ret = loadZones(db, fZones);
        /* if (ret) goto err; */
        if (ret) gotZones = 0;
        printf("loadZones: time=%d\n", time(NULL)-s1);
    }

    /* load AltStreets */
    /*
     *  TODO: on RESTART we don't know if AltStreets was loaded!!!!!
     */
    if (!RESTART) {
        s1 = time(NULL);
        ret = loadAltStreets(db, fiAltStreets);
        if (ret) {
            printf("AltStreets not found - ignoring!\n");
            GotAltStreets = 0;
        }
        printf("loadAltStreets: time=%d\n", time(NULL)-s1);
    }

    /* load Streets */
    if (!RESTART) {
        s1 = time(NULL);
        ret = loadStreets(db, fiStreets);
        if (ret) goto err;
        printf("loadStreets: time=%d\n", time(NULL)-s1);
    }

    printf("  calling createHandle\n");
    H = createHandle(db, pc, fiStreets, foStreets, numsplit?1:0, gotZones);
    
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
    
    free(fiAltStreets);
    free(fiStreets);
    free(foStreets);
    free(fMtdArea);
    free(fMtdZoneRec);
    free(fZones);

    printf("Total time: %d sec.\n", time(NULL)-s0);
    
    exit(ret);
}
