/*********************************************************************
*
*  $Id: nt2rgeo-sqlite.c,v 1.12 2012-06-03 13:53:42 woodbri Exp $
*
*  nt2rgeo
*  Copyright 2008, Stephen Woodbridge, All rights Reserved
*  Redistribition without written permission strictly prohibited
*
**********************************************************************
*
*  nt2rgeo.c
*
*  $Log: nt2rgeo-sqlite.c,v $
*  Revision 1.12  2012-06-03 13:53:42  woodbri
*  Added support to carry FUNC_CLASS to output files.
*
*  Revision 1.11  2011/02/09 04:49:37  woodbri
*  Added ALT_NAMES to rgeo and updated Makefile help target to reflect
*  this.
*
*  Revision 1.10  2011/02/08 20:23:04  woodbri
*  Update type in README
*  do-nt2rgeo added numsplit
*  load-nt2pg added encoding, move tables to schema "data" instead of
*  "rgeo"
*  nt2rgeo-sqlite.c improved error messages and added AltStreet support
*  domaps/* added various additional support requested by client
*  Added various automation scripts.
*
*  Revision 1.9  2010/10/18 02:51:13  woodbri
*  Changes to nt2rgeo-sqlite.c to make Zones optional.
*  Changes to tgr2rgeo-sqlite.c to optionally include mtcfcc in the data.
*  Changes to doNtPolygons.c to include DISP_CLASS to water polygons.
*  Changes to Makefile to support the above.
*
*  Revision 1.8  2008/10/17 18:24:59  woodbri
*  Added SPEED_CAT to the output data.
*
*  Revision 1.7  2008/09/30 20:14:30  woodbri
*  Added an option to include the FR_SPD_LIM and TO_SPD_LIM from the Streets.deb
*  to the output files.
*
*  Revision 1.6  2008/06/26 20:59:33  woodbri
*  Fixed a bug when seq was < 0.
*
*  Revision 1.5  2008/05/15 02:49:47  woodbri
*  Arrgh, left RESTART=1, clearing it.
*
*  Revision 1.4  2008/05/13 18:40:06  woodbri
*  Added optional parameter nt2rgeo-sqlite to allow you to split the output file
*  because the dbf files were getting over 2GB. Added dbname as required argument
*  to rgeo2pg.
*
*  Revision 1.3  2008/05/12 20:00:55  woodbri
*  Had to wide the dbf fields because we were loosing data.
*
*  Revision 1.2  2008/04/11 13:29:15  woodbri
*  Fixes to build all the indexes that we actually need so this runs fast.
*
*  Revision 1.1  2008/04/10 14:56:54  woodbri
*  Adding new files based on sqlite as underlying data store.
*
*  Revision 1.4  2008/03/27 15:04:54  woodbri
*  Changed the code to work around what appears to be a bug in libdb. I add
*  two passes over the MtdArea file to load the two separate DB tables. It was
*  throwing errors on Navteq DCA6 when I read the file and added one record
*  to each table for each record read.
*  Also adding a script to automate the processing and a sample mapfile.
*
*  Revision 1.3  2008/03/25 18:43:09  woodbri
*  Updated Makefile to add libshp to the include search path
*  Updated nt2rgeo.c to be for libdb4.2 API
*
*  Revision 1.2  2008/02/04 04:09:01  woodbri
*  Adding copyright and header.
*
*
**********************************************************************
*
* This utility requires the following Navteq files:
*   Streets
*   MtdArea
*   MtdZoneRec
*   Zones
* It will read this files into a Berkeley DB file and then create
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

#define DATABASE "nt2rgeo-sqlite.db"
#define MAX_NAME 36

#define MAX(a,b) ((a)>(b)?(a):(b))

#define ERR_LOAD_DBF -1
#define ERR_LOAD_INDEX -2

int DEBUG = 0;

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
    sqlite3_stmt *stmt_getalt;
    sqlite3_stmt *stmt_altnames;
    sqlite3_stmt *stmt_getpz;
    sqlite3_stmt *stmt_getpn1;
    sqlite3_stmt *stmt_getpn2;
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
    char    route_type[2];
    char    postalname[2];
    char    explicatbl[2];
    char    alt_names[255];
#ifdef SPEED_LIMIT
    int     fr_spd_lim;
    int     to_spd_lim;
    int     speed_cat;
    int     func_class;
#endif
};

void Usage()
{
    printf("Usage: nt2rgeo srcdir destdir [num_split]\n");
#ifdef ALTNAMES
    printf("       Build with support for reporting alternate street names.\n");
#endif
#ifdef SPEED_LIMIT
    printf("       Build with support for reporting speed limits and road classes.\n");
#endif
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
        fprintf(stderr, "This is OK outside the US and Canada.\n");
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
        fprintf(stderr, "This is OK outside the US and Canada.\n");
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
        "NUM_AD_RNG", "ROUTE_TYPE", "POSTALNAME", "EXPLICATBL"
#ifdef SPEED_LIMIT
            , "FR_SPD_LIM", "TO_SPD_LIM", "SPEED_CAT", "FUNC_CLASS"
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
    char        *zErrMsg;
    char        *sql;
    const char  *cols[] = {"LINK_ID", "ST_NAME", "ADDR_TYPE",
        "L_REFADDR", "L_NREFADDR", "L_ADDRSCH", "L_ADDRFORM",
        "R_REFADDR", "R_NREFADDR", "R_ADDRSCH", "R_ADDRFORM",
        "NUM_AD_RNG", "ROUTE_TYPE", "POSTALNAME", "EXPLICATBL"};
    
    /* create the tables */

    rc = loadDBF(db, fin, "altstreets", 15, cols, NULL, 1);
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
#ifdef ALTNAMES
    assert(DBFAddField(H->oDBF, "ALT_NAME",   FTString, 254, 0) != -1);
#endif
#ifdef SPEED_LIMIT
    assert(DBFAddField(H->oDBF, "FR_SPD_LIM", FTInteger,  5, 0) != -1);
    assert(DBFAddField(H->oDBF, "TO_SPD_LIM", FTInteger,  5, 0) != -1);
    assert(DBFAddField(H->oDBF, "SPEED_CAT",  FTInteger,  1, 0) != -1);
    assert(DBFAddField(H->oDBF, "FUNC_CLASS", FTInteger,  1, 0) != -1);
    assert(DBFAddField(H->oDBF, "ROUTE_TYPE", FTInteger,  1, 0) != -1);
#endif

    return 0;
}
    

struct handle_struct *createHandle(sqlite3 *db, char *fiStreets, char *foStreets, int seq, int gotZones, int gotAltStreets)
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

    if (gotAltStreets) {
        sql = "select * from altstreets where LINK_ID=?;";
        rc = sqlite3_prepare(db, sql, strlen(sql), &H->stmt_getalt, NULL);
        if (rc != SQLITE_OK) {
            fprintf(stderr, "SQL: %s\n", sql);
            fprintf(stderr, "getAltStreet: SQL Error: %s\n", sqlite3_errmsg(db));
            exit(1);
        }
#ifdef ALTNAMES
        sql = "select substr(group_concat(DISTINCT ST_NAME),1,254) from altstreets where LINK_ID=?;";
        rc = sqlite3_prepare(db, sql, strlen(sql), &H->stmt_altnames, NULL);
        if (rc != SQLITE_OK) {
            fprintf(stderr, "SQL: %s\n", sql);
            fprintf(stderr, "getAltStreet: SQL Error: %s\n", sqlite3_errmsg(db));
            exit(1);
        }
#endif
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

    sql = "select * from mtdarea where AREA_ID=?;";
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

    if (H->stmt_getalt)
        sqlite3_finalize(H->stmt_getalt);
    if (H->stmt_getpz)
        sqlite3_finalize(H->stmt_getpz);
    sqlite3_finalize(H->stmt_getpn1);
    sqlite3_finalize(H->stmt_getpn2);
    
    /* close shapefiles */
    if (H->iDBF) DBFClose(H->iDBF);
    if (H->oDBF) DBFClose(H->oDBF);
    if (H->iSHP) SHPClose(H->iSHP);
    if (H->oSHP) SHPClose(H->oSHP);
    
    /* free memory */
    memset(H, 0, sizeof(struct handle_struct));
    free(H);
}


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

        if (!strcmp(ztype, "PA"))
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
            if (type[0] == 'B') break;
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


void getAltStreets(struct handle_struct *H, long linkid, struct streets_struct *c)
{
    sqlite3_stmt *stmt;
    int rc;
    char *p;

    /* first get all alternate names for street */
    stmt = H->stmt_altnames;

    if (stmt) {
        rc = sqlite3_bind_int(stmt, 1, linkid);
        if ( rc!=SQLITE_OK ) {
            fprintf(stderr, "getAltStreets: SQL bind_int error\n");
            exit(1);
        }

        rc = sqlite3_step(stmt);
        if (rc == SQLITE_ROW) {
            p = (char *) sqlite3_column_text(stmt, 0);
            if (p)
                strcpy(c->alt_names, p);
            rc = sqlite3_reset(stmt);
            if ( rc!=SQLITE_OK ) {
                fprintf(stderr, "getAltStreets: SQL reset error\n");
            }
        }
    }

    /* next combine values into main record to fill gaps in the data */
    stmt = H->stmt_getalt;

    rc = sqlite3_bind_int(stmt, 1, linkid);
    if (stmt) {
        if ( rc!=SQLITE_OK ) {
            fprintf(stderr, "getAltStreets: SQL bind_int error\n");
            exit(1);
        }

        while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
            if (strlen(c->l_refaddr) == 0 && strlen(c->l_nrefaddr) == 0 &&
                    strlen(sqlite3_column_text(stmt, 3)) != 0 &&
                    strlen(sqlite3_column_text(stmt, 4)) != 0 ) {
                strcpy(c->l_refaddr, sqlite3_column_text(stmt, 3));
                strcpy(c->l_nrefaddr, sqlite3_column_text(stmt, 4));
                strcpy(c->l_addrsch, sqlite3_column_text(stmt, 5));
                strcpy(c->l_addrform, sqlite3_column_text(stmt, 6));
            }
            if (strlen(c->r_refaddr) == 0 && strlen(c->r_nrefaddr) == 0 &&
                    strlen(sqlite3_column_text(stmt, 7)) != 0 &&
                    strlen(sqlite3_column_text(stmt, 8)) != 0 ) {
                strcpy(c->r_refaddr, sqlite3_column_text(stmt, 7));
                strcpy(c->r_nrefaddr, sqlite3_column_text(stmt, 8));
                strcpy(c->r_addrsch, sqlite3_column_text(stmt, 9));
                strcpy(c->r_addrform, sqlite3_column_text(stmt, 10));
            }
            if (strlen(c->st_name) == 0) {
                strcpy(c->st_name, sqlite3_column_text(stmt, 1));
                strcpy(c->addr_type, sqlite3_column_text(stmt, 2));
            }
        }

        rc = sqlite3_reset(stmt);
        if ( rc!=SQLITE_OK ) {
            fprintf(stderr, "getAltStreets: SQL reset error\n");
        }
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

int tokenize_num(char *s, char tok[][10], char *typ) {
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
    char ftok[10][10];
    char ttok[10][10];
    char ftyp[10];
    char ttyp[10];
    int i, ntf, ntt;
    int w;

    *nf = 0;
    *nt = 0;
    if (strlen(f) == 0 && strlen(t) == 0)
        return bad;
    
    for (i=0; i<10; i++) {
        memset(ftok[i], 0,  10);
        memset(ttok[i], 0,  10);
    }
    memset(ftyp, 0,  10);
    memset(ttyp, 0,  10);
    memset(fmt,  0,  20);

    ntf = tokenize_num(f, ftok, ftyp);
    ntt = tokenize_num(t, ttok, ttyp);

    if (ntf != ntt || ntf == 0)
        return bad;

    for (i=0; i<10; i++) {
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
    int n;
    long LAreaID = -1;
    long RAreaID = -1;
    SHPObject *s;

    memset(&PL, 0, sizeof(struct place_struct));
    memset(&PR, 0, sizeof(struct place_struct));

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

    if (0) {
        printf(" L(%d)=%s:%s:%s:%s:%s:%s:%s\n",
                PL.lvl, PL.name[0], PL.name[1], PL.name[2], PL.name[3],
                PL.name[4], PL.name[5], PL.name[6]);
        printf(" R(%d)=%s:%s:%s:%s:%s:%s:%s\n",
                PR.lvl, PR.name[0], PR.name[1], PR.name[2], PR.name[3],
                PR.name[4], PR.name[5], PR.name[6]);
    }

    /* getAltStreet info if its available and we need it */
    if (H->stmt_getalt) {
        getAltStreets(H, linkid, c);
    }

    /* write denormalized record to shapefile */

    s = SHPReadObject(H->iSHP, c->recno);
    H->cnt = SHPWriteObject(H->oSHP, -1, s);
    SHPDestroyObject(s);
    
    n = 0;
    DBFWriteIntegerAttribute(H->oDBF, H->cnt, n++, linkid);
    DBFWriteStringAttribute(H->oDBF,  H->cnt, n++, c->st_name);

    cfmt = scan_nt_nums(c->l_refaddr, c->l_nrefaddr, &fnum, &tnum);
    mkSqlFmt(cfmt, sfmt, nfmt);
    DBFWriteIntegerAttribute(H->oDBF, H->cnt, n++, fnum);
    DBFWriteIntegerAttribute(H->oDBF, H->cnt, n++, tnum);
    DBFWriteStringAttribute(H->oDBF,  H->cnt, n++, nfmt);
    DBFWriteStringAttribute(H->oDBF,  H->cnt, n++, sfmt);
    DBFWriteStringAttribute(H->oDBF,  H->cnt, n++, cfmt);

    cfmt = scan_nt_nums(c->r_refaddr, c->r_nrefaddr, &fnum, &tnum);
    mkSqlFmt(cfmt, sfmt, nfmt);
    DBFWriteIntegerAttribute(H->oDBF, H->cnt, n++, fnum);
    DBFWriteIntegerAttribute(H->oDBF, H->cnt, n++, tnum);
    DBFWriteStringAttribute(H->oDBF,  H->cnt, n++, nfmt);
    DBFWriteStringAttribute(H->oDBF,  H->cnt, n++, sfmt);
    DBFWriteStringAttribute(H->oDBF,  H->cnt, n++, cfmt);

    DBFWriteStringAttribute(H->oDBF,  H->cnt, n++, PL.name[4]);
    DBFWriteStringAttribute(H->oDBF,  H->cnt, n++, PL.name[3]);
    DBFWriteStringAttribute(H->oDBF,  H->cnt, n++, PL.name[2]);
    DBFWriteStringAttribute(H->oDBF,  H->cnt, n++, PL.name[1]);
    DBFWriteStringAttribute(H->oDBF,  H->cnt, n++, PL.name[0]);
    DBFWriteStringAttribute(H->oDBF,  H->cnt, n++, PR.name[4]);
    DBFWriteStringAttribute(H->oDBF,  H->cnt, n++, PR.name[3]);
    DBFWriteStringAttribute(H->oDBF,  H->cnt, n++, PR.name[2]);
    DBFWriteStringAttribute(H->oDBF,  H->cnt, n++, PR.name[1]);
    DBFWriteStringAttribute(H->oDBF,  H->cnt, n++, PR.name[0]);
    DBFWriteStringAttribute(H->oDBF,  H->cnt, n++, c->l_postcode);
    DBFWriteStringAttribute(H->oDBF,  H->cnt, n++, c->r_postcode);
#ifdef ALTNAMES
    DBFWriteStringAttribute(H->oDBF,  H->cnt, n++, c->alt_names);
#endif
#ifdef SPEED_LIMIT
    DBFWriteIntegerAttribute(H->oDBF, H->cnt, n++, c->fr_spd_lim);
    DBFWriteIntegerAttribute(H->oDBF, H->cnt, n++, c->to_spd_lim);
    DBFWriteIntegerAttribute(H->oDBF, H->cnt, n++, c->speed_cat);
    DBFWriteIntegerAttribute(H->oDBF, H->cnt, n++, c->func_class);
    DBFWriteIntegerAttribute(H->oDBF, H->cnt, n++,
        c->route_type[0]=='\0'?0:c->route_type[0]-'0');
#endif
}


void processRecords(sqlite3 *db, struct handle_struct *H, int numsplit)
{
    int i, rc, cnt, nrows, seq;
    long linkid;
    static struct streets_struct cur;
    static struct streets_struct this;
    char *sql;
    char *p;
    sqlite3_stmt *stmt;

    if (numsplit) {
        sql = "select count(*) from (select distinct LINK_ID from streets) as foo;";
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

    sql = "select * from streets order by LINK_ID;";
    rc = sqlite3_prepare(db, sql, strlen(sql), &stmt, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "processRecords: SQL Error: %s\n", sqlite3_errmsg(db));
        return;
    }
    
    long last_linkid = -1;
    cnt = 0;
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
        strcpy(this.route_type,  sqlite3_column_text(stmt, 21));
        strcpy(this.postalname,  sqlite3_column_text(stmt, 22));
        strcpy(this.explicatbl,  sqlite3_column_text(stmt, 23));
#ifdef SPEED_LIMIT
        this.fr_spd_lim =        sqlite3_column_int(stmt,  24);
        this.to_spd_lim =        sqlite3_column_int(stmt,  25);
        this.speed_cat =         sqlite3_column_int(stmt,  26);
        p =                      sqlite3_column_text(stmt, 27);
        this.func_class =        p[0]=='\0'?0:p[0]-'0';
        this.recno =             sqlite3_column_int(stmt,  28);
#else
        this.recno =             sqlite3_column_int(stmt,  24);
#endif
        
        if (last_linkid != linkid && last_linkid != -1) {
            saveRecord(H, last_linkid, &cur);
            memset(&cur, 0, sizeof(struct streets_struct));
            last_linkid = -1;
            cnt++;
            if (numsplit && cnt >= nrows) {
                if (getNextShape(H, seq++))
                    exit(1);
                cnt = 0;
            }
        }
        
        /* rules for selecting which of multiple records we use */

        /* if it is the first records take it */
        if (last_linkid == -1) {
            last_linkid = linkid;
            memcpy(&cur, &this, sizeof(struct streets_struct));
        }
        /* if it is the postname record take it */
        else if (this.postalname[0] == 'Y') {
            memcpy(&cur, &this, sizeof(struct streets_struct));
        }
        /* if cur is the postalname then we assume it is the best */
        else if (cur.postalname[0] != 'Y') {
            /* if cur has no addr ranges and this does, take it */
            if (this.num_ad_rng > 0 && cur.num_ad_rng == 0) {
                memcpy(&cur, &this, sizeof(struct streets_struct));
            }
            /* if this has right and left and cur is missing one, take it */
            else if ((strlen(this.l_refaddr) && strlen(this.r_refaddr)) &&
                    (!strlen(cur.l_refaddr) || !strlen(cur.r_refaddr))) {
                memcpy(&cur, &this, sizeof(struct streets_struct));
            }
            /* one is left only and one is right only, merge them */
            else if (0) {
            }
            /* otherwise give explicatbl a preference */
            else if (cur.explicatbl[0] == 'N' && this.explicatbl[0] == 'Y') {
                memcpy(&cur, &this, sizeof(struct streets_struct));
            }
        }
    }
    if (last_linkid != -1)
        saveRecord(H, last_linkid, &cur);
    sqlite3_finalize(stmt);
}

#define RESTART 0

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
    char    *zErrMsg = 0;
    int     rc;
    time_t  s0, s1;
    int     numsplit = 0;
    int     gotZones = 1;
    int     gotAltStreets = 1;

    if (argc < 3)
        Usage();

    if (argc == 4) {
        numsplit = (int) strtol(argv[3], NULL, 10);
        if (numsplit < 2) numsplit = 0;
        printf("Streets output file will be split into %d parts\n", numsplit);
    }

    s0 = time(NULL);

    fiAltStreets = mkFName(argv[1], "AltStreets.dbf");
    fiStreets    = mkFName(argv[1], "Streets.dbf");
    foStreets    = mkFName(argv[2], "Streets.dbf");
    fMtdArea     = mkFName(argv[1], "MtdArea.dbf");
    fMtdZoneRec  = mkFName(argv[1], "MtdZoneRec.dbf");
    fZones       = mkFName(argv[1], "Zones.dbf");

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
    if (!RESTART) {
        s1 = time(NULL);
        ret = loadAltStreets(db, fiAltStreets);
        if (ret) gotAltStreets = 0;
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
    H = createHandle(db, fiStreets, foStreets, numsplit?1:0, gotZones, gotAltStreets);
    
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
    
    free(fiAltStreets);
    free(fiStreets);
    free(foStreets);
    free(fMtdArea);
    free(fMtdZoneRec);
    free(fZones);

    printf("Total time: %d sec.\n", time(NULL)-s0);
    
    exit(ret);
}
