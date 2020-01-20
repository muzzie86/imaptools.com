/*********************************************************************
*
*  $Id: nt2rgeo.c,v 1.5 2008/04/10 14:54:25 woodbri Exp $
*
*  nt2rgeo
*  Copyright 2008, Stephen Woodbridge, All rights Reserved
*  Redistribition without written permission strictly prohibited
*
**********************************************************************
*
*  nt2rgeo.c
*
*  $Log: nt2rgeo.c,v $
*  Revision 1.5  2008/04/10 14:54:25  woodbri
*  Check in last edits.
*  This program is deprecated! and it is questionable as to is correctness.
*  It has been abandoned for nt2rgeo-sqlite.
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
#include <shapefil.h>
#include <db.h>

#define DATABASE "nt2rgeo-DB.db"
#define DB_CACHESIZE 256*1024
//#define DB_CACHESIZE 131072
//#define DB_CACHESIZE  65536
#define MAX_NAME 36

//#define DEBUG 1

struct order_struct {
    long areaid;
    long linkid;
};

struct place_struct {
    int lvl;
    char zname[MAX_NAME];
    long zareaid;
    char name[7][MAX_NAME];
};

struct ac_key_struct {
    int lvl;
    int ac[7];
};

struct ac_data_struct {
    long areaid;
    struct ac_key_struct ac;
    char name[MAX_NAME];
    char lang[4];
    char type[2];
};

struct zoneRec_struct {
    char name[MAX_NAME];
    char lang[4];
    char nmtyp[2];
    char type[3];
    long areaid;
};

struct zones_struct {
    long zoneid;
    char side[2];
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
};

struct handle_struct {
    DB      *DBAreaCode;
    DB      *DBAreaID;
    DB      *DBZoneID;
    DB      *DBZones;
    DB      *DBStreets;
    char    *fiStreets;
    DBFHandle   iDBF;
    SHPHandle   iSHP;
    char    *foStreets;
    DBFHandle   oDBF;
    SHPHandle   oSHP;
    int     cnt;
    int     nEnt;
};

void pkv_AreaCode(DBT *key, DBT *data);
void pkv_AreaID(DBT *key, DBT *data);

void Usage()
{
    printf("Usage: nt2rgeo srcdir destdir\n");
    exit(1);
}


int loadMtdArea(char *fin, DB **pdb, DB **pdb2)
{
    DB      *db;
    DB      *db2;
    int     ret;
    int     i, j, k;
    int     ac, nl;
    int     nEnt;
    int     nLevel;
    int     nAC[7];
    int     nAreaId;
    int     nAreaName;
    int     nAreaType;
    int     nAreaLang;
    long    areaid;
    
    DBFHandle     dIN;
    DBT key, data;
    
    struct ac_key_struct  *ackey;
    struct ac_data_struct *aid;

    if ((ret = db_create(&db, NULL, 0)) != 0) {
        fprintf(stderr, "loadMtdArea: db_create: %s\n", db_strerror(ret));
        return ret;
    }
    *pdb = db;
    
    if ((ret = db->set_cachesize(db, 0, DB_CACHESIZE, 1)) != 0) {
        db->err(db, ret, "loadMtdArea: %s - %s Attempt to set_cachesize failed.", DATABASE, "DBAreaCode");
        return ret;
    }

    if ((ret = db->set_flags(db, DB_DUPSORT)) != 0) {
        db->err(db, ret, "loadMtdArea: %s - %s Attempt to set DUPSORT flag failed.", DATABASE, "DBAreaCode");
        return ret;
    }

    if ((ret = db->open(db, NULL,
            DATABASE, "DBAreaCode", DB_BTREE, DB_CREATE, 0600)) != 0) {
        db->err(db, ret, "loadMtdArea: %s - %s", DATABASE, "DBAreaCode");
        return ret;
    }

    if ((ret = db_create(&db2, NULL, 0)) != 0) {
        fprintf(stderr, "loadMtdArea: db_create: %s\n", db_strerror(ret));
        return ret;
    }
    *pdb2 = db2;

    if ((ret = db2->set_cachesize(db2, 0, DB_CACHESIZE, 1)) != 0) {
        db2->err(db2, ret, "loadMtdArea: %s - %s Attempt to set_cachesize failed.", DATABASE, "DBAreaID");
        return ret;
    }

    if ((ret = db2->set_flags(db2, DB_DUPSORT)) != 0) {
        db2->err(db2, ret, "loadMtdArea: %s - %s Attempt to set DUPSORT flag failed.", DATABASE, "DBAreaID");
        return ret;
    }

    if ((ret = db->open(db2, NULL,
            DATABASE, "DBAreaID", DB_BTREE, DB_CREATE, 0600)) != 0) {
        db2->err(db2, ret, "loadMtdArea: %s - %s", DATABASE, "DBAreaID");
        return ret;
    }

    if (!(dIN  = DBFOpen(fin, "rb"))) {
        fprintf(stderr, "Failed to open dbf %s for read\n", fin);
        return errno;
    }

    assert((nLevel    = DBFGetFieldIndex(dIN, "ADMIN_LVL" )) > -1);
    assert((nAreaId   = DBFGetFieldIndex(dIN, "AREA_ID"   )) > -1);
    assert((nAC[0]    = DBFGetFieldIndex(dIN, "AREACODE_1")) > -1);
    assert((nAC[1]    = DBFGetFieldIndex(dIN, "AREACODE_2")) > -1);
    assert((nAC[2]    = DBFGetFieldIndex(dIN, "AREACODE_3")) > -1);
    assert((nAC[3]    = DBFGetFieldIndex(dIN, "AREACODE_4")) > -1);
    assert((nAC[4]    = DBFGetFieldIndex(dIN, "AREACODE_5")) > -1);
    assert((nAC[5]    = DBFGetFieldIndex(dIN, "AREACODE_6")) > -1);
    assert((nAC[6]    = DBFGetFieldIndex(dIN, "AREACODE_7")) > -1);
    assert((nAreaName = DBFGetFieldIndex(dIN, "AREA_NAME" )) > -1);
    assert((nAreaType = DBFGetFieldIndex(dIN, "AREA_TYPE" )) > -1);
    assert((nAreaLang = DBFGetFieldIndex(dIN, "LANG_CODE" )) > -1);

    // assert size of AREA_NAME
                                                    
    nEnt = DBFGetRecordCount(dIN);

    printf("  loadMtdArea Pass 1: %d records\n", nEnt);
    for (i=0; i<nEnt; i++) {
        ackey = (struct ac_key_struct *) calloc(1, sizeof(struct ac_key_struct));
        assert(ackey);

        aid   = (struct ac_data_struct *) calloc(1, sizeof(struct ac_data_struct));
        assert(aid);

        ackey->lvl = DBFReadIntegerAttribute( dIN, i, nLevel);
        for (k=0; k<7; k++)
            ackey->ac[k] = DBFReadIntegerAttribute( dIN, i, nAC[k] );

        areaid = aid->areaid = DBFReadIntegerAttribute( dIN, i, nAreaId);
        strcpy(aid->name, DBFReadStringAttribute( dIN, i, nAreaName ));
        strcpy(aid->lang, DBFReadStringAttribute( dIN, i, nAreaLang ));
        strcpy(aid->type, DBFReadStringAttribute( dIN, i, nAreaType ));
        memcpy(&(aid->ac), ackey, sizeof(struct ac_key_struct));

        memset(&key,  0, sizeof(key));
        memset(&data, 0, sizeof(data));
        key.data  = ackey;
        key.size  = sizeof(struct ac_key_struct);
        data.data = &areaid;
        data.size = sizeof(areaid);

#ifdef DEBUG
        if (1) {
            printf("Rec:%d:", i);
            pkv_AreaCode(&key, &data);
        }
#endif

        if ((ret = db->put(db, NULL, &key, &data, 0)) != 0 &&
                ret != DB_KEYEXIST) {
            db->err(db, ret, "loadMtdArea: %s: %s: db-put",
                    DATABASE, "DBAreaCode");
            return ret;
        }
        free(ackey);
        free(aid);
    }

    printf("  loadMtdArea Pass 2: %d records\n", nEnt);
    for (i=0; i<nEnt; i++) {
        ackey = (struct ac_key_struct *) calloc(1, sizeof(struct ac_key_struct));
        assert(ackey);

        aid   = (struct ac_data_struct *) calloc(1, sizeof(struct ac_data_struct));
        assert(aid);

        ackey->lvl = DBFReadIntegerAttribute( dIN, i, nLevel);
        for (k=0; k<7; k++)
            ackey->ac[k] = DBFReadIntegerAttribute( dIN, i, nAC[k] );

        areaid = aid->areaid = DBFReadIntegerAttribute( dIN, i, nAreaId);
        strcpy(aid->name, DBFReadStringAttribute( dIN, i, nAreaName ));
        strcpy(aid->lang, DBFReadStringAttribute( dIN, i, nAreaLang ));
        strcpy(aid->type, DBFReadStringAttribute( dIN, i, nAreaType ));
        memcpy(&(aid->ac), ackey, sizeof(struct ac_key_struct));

        memset(&key,  0, sizeof(key));
        memset(&data, 0, sizeof(data));
        key.data  = &areaid;
        key.size  = sizeof(areaid);
        data.data = aid;
        data.size = sizeof(struct ac_data_struct);

#ifdef DEBUG
        if (1) {
            printf("Rec:%d:", i);
            pkv_AreaID(&key, &data);
        }
#endif
        
        if ((ret = db2->put(db2, NULL, &key, &data, 0)) != 0 &&
                ret != DB_KEYEXIST) {
            db2->err(db2, ret, "loadMtdArea: %s: %s: db-put",
                    DATABASE, "DBAreaID");
            return ret;
        }
        free(ackey);
        free(aid);
    }
    
    DBFClose(dIN);

    return 0;
}


int loadMtdZoneRec(char *fin, DB **pdb)
{
    DB      *db;
    int     ret;
    int     i;
    int     nEnt;
    int     nZoneID;
    int     nZoneName;
    int     nLangCode;
    int     nZoneNmTyp;
    int     nZoneType;
    int     nAreaID;
    long     zoneid;

    DBFHandle     dIN;
    DBT key, data;

    struct zoneRec_struct *zr;
    
    if ((ret = db_create(&db, NULL, 0)) != 0) {
        fprintf(stderr, "loadMtdZoneRec: db_create: %s\n", db_strerror(ret));
        return ret;
    }
    *pdb = db;

    if ((ret = db->set_cachesize(db, 0, DB_CACHESIZE, 1)) != 0) {
        db->err(db, ret, "loadMtdZoneRec: %s - %s Attempt to set_cachesize failed.", DATABASE, "DBMtdZoneRec");
        return ret;
    }

    if ((ret = db->set_flags(db, DB_DUPSORT)) != 0) {
        db->err(db, ret, "loadMtdZoneRec: %s - %s Attempt to set DUPSORT flag failed.", DATABASE, "DBMtdZoneRec");
        return ret;
    }

    if ((ret = db->open(db, NULL,
            DATABASE, "DBMtdZoneRec", DB_BTREE, DB_CREATE, 0600)) != 0) {
        db->err(db, ret, "loadMtdZoneRec: %s - %s", DATABASE, "DBMtdZoneRec");
        return ret;
    }

    if (!(dIN  = DBFOpen(fin, "rb"))) {
        fprintf(stderr, "Failed to open dbf %s for read\n", fin);
        return errno;
    }

    assert((nZoneID    = DBFGetFieldIndex(dIN, "ZONE_ID"    )) > -1);
    assert((nZoneName  = DBFGetFieldIndex(dIN, "ZONE_NAME"  )) > -1);
    assert((nLangCode  = DBFGetFieldIndex(dIN, "LANG_CODE"  )) > -1);
    assert((nZoneNmTyp = DBFGetFieldIndex(dIN, "Z_NMTYPE"   )) > -1);
    assert((nZoneType  = DBFGetFieldIndex(dIN, "ZONE_TYPE"  )) > -1);
    assert((nAreaID    = DBFGetFieldIndex(dIN, "AREA_ID"    )) > -1);

    nEnt = DBFGetRecordCount(dIN);

    printf("  loadMtdZoneRec: %d records\n", nEnt);
    for (i=0; i<nEnt; i++) {
        zr = (struct zoneRec_struct *) calloc(1, sizeof(struct zoneRec_struct));
        assert(zr);

        strcpy(zr->name,  DBFReadStringAttribute( dIN, i, nZoneName ));
        strcpy(zr->lang,  DBFReadStringAttribute( dIN, i, nLangCode ));
        strcpy(zr->nmtyp, DBFReadStringAttribute( dIN, i, nZoneNmTyp ));
        strcpy(zr->type,  DBFReadStringAttribute( dIN, i, nZoneType ));
        zr->areaid = DBFReadIntegerAttribute( dIN, i, nAreaID );
        zoneid     = DBFReadIntegerAttribute( dIN, i, nZoneID );

        memset(&key,  0, sizeof(key));
        memset(&data, 0, sizeof(data));
        key.data  = &zoneid;
        key.size  = sizeof(zoneid);
        data.data = zr;
        data.size = sizeof(struct zoneRec_struct);

        if ((ret = db->put(db, NULL, &key, &data, 0)) != 0 &&
                ret != DB_KEYEXIST) {
            db->err(db, ret, "loadMtdZoneRec: %s: %s: db-put",
                    DATABASE, "DBMtdZoneRec");
            return ret;
        }
        free(zr);
    }

    DBFClose(dIN);

    return 0;
}


int loadZones(char *fin, DB **pdb)
{
    DB      *db;
    int     ret;
    int     i;
    int     nEnt;
    int     nLinkID;
    int     nZoneID;
    int     nSide;
    long     linkid;

    DBFHandle     dIN;
    DBT key, data;

    struct zones_struct *zone;
    
    if ((ret = db_create(&db, NULL, 0)) != 0) {
        fprintf(stderr, "loadZones: db_create: %s\n", db_strerror(ret));
        return ret;
    }
    *pdb = db;

    if ((ret = db->set_cachesize(db, 0, DB_CACHESIZE*256, 1)) != 0) {
        db->err(db, ret, "loadZones: %s - %s Attempt to set_cachesize failed.", DATABASE, "DBZones");
        return ret;
    }

    if ((ret = db->set_flags(db, DB_DUPSORT)) != 0) {
        db->err(db, ret, "loadZones: %s - %s Attempt to set DUPSORT flag failed.", DATABASE, "DBZones");
        return ret;
    }

    if ((ret = db->open(db, NULL,
            DATABASE, "DBZones", DB_BTREE, DB_CREATE, 0600)) != 0) {
        db->err(db, ret, "loadZones: %s - %s", DATABASE, "DBZones");
        return ret;
    }

    if (!(dIN  = DBFOpen(fin, "rb"))) {
        fprintf(stderr, "Failed to open dbf %s for read\n", fin);
        return errno;
    }

    assert((nLinkID   = DBFGetFieldIndex(dIN, "LINK_ID" )) > -1);
    assert((nZoneID   = DBFGetFieldIndex(dIN, "ZONE_ID" )) > -1);
    assert((nSide     = DBFGetFieldIndex(dIN, "SIDE"    )) > -1);

    nEnt = DBFGetRecordCount(dIN);

    printf("  loadZones: %d records\n", nEnt);
    for (i=0; i<nEnt; i++) {
        zone = (struct zones_struct *) calloc(1, sizeof(struct zones_struct));
        assert(zone);

        linkid =       DBFReadIntegerAttribute( dIN, i, nLinkID );
        zone->zoneid = DBFReadIntegerAttribute( dIN, i, nZoneID );
        strcpy(zone->side, DBFReadStringAttribute( dIN, i, nSide ));
        
        memset(&key,  0, sizeof(key));
        memset(&data, 0, sizeof(data));
        key.data  = &linkid;
        key.size  = sizeof(linkid);
        data.data = zone;
        data.size = sizeof(struct zones_struct);

        if ((ret = db->put(db, NULL, &key, &data, 0)) != 0 &&
                ret != DB_KEYEXIST) {
            db->err(db, ret, "loadZones: %s: %s: db-put",
                    DATABASE, "DBZones");
            return ret;
        }
        free(zone);
    }

    DBFClose(dIN);

    return 0;
}


int order_compare(const void *e1, const void *e2) {
    struct order_struct *a = (struct order_struct *) e1;
    struct order_struct *b = (struct order_struct *) e2;

    if (a->areaid == b->areaid)
        return a->linkid - b->linkid;
    else
        return a->areaid - b->areaid;
}


int loadStreets(char *fin, DB **pdb, struct order_struct **O)
{
    DB      *db;
    int     ret;
    int     i;
    int     nEnt;
    int     nLinkID;
    int     nStName;
    int     nNumStNmes;
    int     nAddrType;
    int     nLRefAddr;
    int     nLNRefAddr;
    int     nLAddrSch;
    int     nLAddrForm;
    int     nRRefAddr;
    int     nRNRefAddr;
    int     nRAddrSch;
    int     nRAddrForm;
    int     nRefInId;
    int     nNRefInId;
    int     nLAreaId;
    int     nRAreaId;
    int     nLPostCode;
    int     nRPostCode;
    int     nLNumZones;
    int     nRNumZones;
    int     nNumAdRng;
    int     nRouteType;
    int     nPostalName;
    int     nExplicatbl;
    long    linkid;

    DBFHandle     dIN;
    DBT key, data;

    struct streets_struct *s;

    if ((ret = db_create(&db, NULL, 0)) != 0) {
        fprintf(stderr, "loadStreets: db_create: %s\n", db_strerror(ret));
        return ret;
    }
    *pdb = db;

    if ((ret = db->set_cachesize(db, 0, DB_CACHESIZE*1024*2, 1)) != 0) {
        db->err(db, ret, "loadStreets: %s - %s Attempt to set_cachesize failed.", DATABASE, "DBStreets");
        return ret;
    }

    if ((ret = db->set_flags(db, DB_DUPSORT)) != 0) {
        db->err(db, ret, "loadStreets: %s - %s Attempt to set DUPSORT flag failed.", DATABASE, "DBStreets");
        return ret;
    }

    if ((ret = db->open(db, NULL,
            DATABASE, "DBStreets", DB_BTREE, DB_CREATE, 0600)) != 0) {
        db->err(db, ret, "loadStreets: %s - %s", DATABASE, "DBStreets");
        return ret;
    }

    if (!(dIN  = DBFOpen(fin, "rb"))) {
        fprintf(stderr, "Failed to open dbf %s for read\n", fin);
        return errno;
    }

    assert((nLinkID     = DBFGetFieldIndex(dIN, "LINK_ID"    )) > -1);
    assert((nStName     = DBFGetFieldIndex(dIN, "ST_NAME"    )) > -1);
    assert((nNumStNmes  = DBFGetFieldIndex(dIN, "NUM_STNMES" )) > -1);
    assert((nAddrType   = DBFGetFieldIndex(dIN, "ADDR_TYPE"  )) > -1);
    assert((nLRefAddr   = DBFGetFieldIndex(dIN, "L_REFADDR"  )) > -1);
    assert((nLNRefAddr  = DBFGetFieldIndex(dIN, "L_NREFADDR" )) > -1);
    assert((nLAddrSch   = DBFGetFieldIndex(dIN, "L_ADDRSCH"  )) > -1);
    assert((nLAddrForm  = DBFGetFieldIndex(dIN, "L_ADDRFORM" )) > -1);
    assert((nRRefAddr   = DBFGetFieldIndex(dIN, "R_REFADDR"  )) > -1);
    assert((nRNRefAddr  = DBFGetFieldIndex(dIN, "R_NREFADDR" )) > -1);
    assert((nRAddrSch   = DBFGetFieldIndex(dIN, "R_ADDRSCH"  )) > -1);
    assert((nRAddrForm  = DBFGetFieldIndex(dIN, "R_ADDRFORM" )) > -1);
    assert((nRefInId    = DBFGetFieldIndex(dIN, "REF_IN_ID"  )) > -1);
    assert((nNRefInId   = DBFGetFieldIndex(dIN, "NREF_IN_ID" )) > -1);
    assert((nLAreaId    = DBFGetFieldIndex(dIN, "L_AREA_ID"  )) > -1);
    assert((nRAreaId    = DBFGetFieldIndex(dIN, "R_AREA_ID"  )) > -1);
    assert((nLPostCode  = DBFGetFieldIndex(dIN, "L_POSTCODE" )) > -1);
    assert((nRPostCode  = DBFGetFieldIndex(dIN, "R_POSTCODE" )) > -1);
    assert((nLNumZones  = DBFGetFieldIndex(dIN, "L_NUMZONES" )) > -1);
    assert((nRNumZones  = DBFGetFieldIndex(dIN, "R_NUMZONES" )) > -1);
    assert((nNumAdRng   = DBFGetFieldIndex(dIN, "NUM_AD_RNG" )) > -1);
    assert((nRouteType  = DBFGetFieldIndex(dIN, "ROUTE_TYPE" )) > -1);
    assert((nPostalName = DBFGetFieldIndex(dIN, "POSTALNAME" )) > -1);
    assert((nExplicatbl = DBFGetFieldIndex(dIN, "EXPLICATBL" )) > -1);

    nEnt = DBFGetRecordCount(dIN);

    *O = (struct order_struct *) calloc(nEnt, sizeof(struct order_struct));
    assert(*O);
    
    printf("  loadStreets: %d records\n", nEnt);
    for (i=0; i<nEnt; i++) {
        s = (struct streets_struct *) calloc(1, sizeof(struct streets_struct));
        assert(s);

        s->recno = i;
        linkid =              DBFReadIntegerAttribute( dIN, i, nLinkID );
        strcpy(s->st_name,    DBFReadStringAttribute(  dIN, i, nStName ));
        s->num_stnmes =       DBFReadIntegerAttribute( dIN, i, nNumStNmes );
        strcpy(s->addr_type,  DBFReadStringAttribute(  dIN, i, nAddrType ));
        strcpy(s->l_refaddr,  DBFReadStringAttribute(  dIN, i, nLRefAddr ));
        strcpy(s->l_nrefaddr, DBFReadStringAttribute(  dIN, i, nLNRefAddr ));
        strcpy(s->l_addrsch,  DBFReadStringAttribute(  dIN, i, nLAddrSch ));
        strcpy(s->l_addrform, DBFReadStringAttribute(  dIN, i, nLAddrForm ));
        strcpy(s->r_refaddr,  DBFReadStringAttribute(  dIN, i, nRRefAddr ));
        strcpy(s->r_nrefaddr, DBFReadStringAttribute(  dIN, i, nRNRefAddr ));
        strcpy(s->r_addrsch,  DBFReadStringAttribute(  dIN, i, nRAddrSch ));
        strcpy(s->r_addrform, DBFReadStringAttribute(  dIN, i, nRAddrForm ));
        s->ref_in_id =        DBFReadIntegerAttribute( dIN, i, nRefInId );
        s->nref_in_id =       DBFReadIntegerAttribute( dIN, i, nNRefInId );
        s->l_area_id =        DBFReadIntegerAttribute( dIN, i, nLAreaId );
        s->r_area_id =        DBFReadIntegerAttribute( dIN, i, nRAreaId );
        strcpy(s->l_postcode, DBFReadStringAttribute(  dIN, i, nLPostCode ));
        strcpy(s->r_postcode, DBFReadStringAttribute(  dIN, i, nRPostCode ));
        s->l_numzones =       DBFReadIntegerAttribute( dIN, i, nLNumZones );
        s->r_numzones =       DBFReadIntegerAttribute( dIN, i, nRNumZones );
        s->num_ad_rng =       DBFReadIntegerAttribute( dIN, i, nNumAdRng );
        strcpy(s->route_type, DBFReadStringAttribute(  dIN, i, nRouteType ));
        strcpy(s->postalname, DBFReadStringAttribute(  dIN, i, nPostalName ));
        strcpy(s->explicatbl, DBFReadStringAttribute(  dIN, i, nExplicatbl ));

        (*O)[i].areaid = s->l_area_id?s->l_area_id:s->r_area_id;
        (*O)[i].linkid = linkid;
        
        memset(&key,  0, sizeof(key));
        memset(&data, 0, sizeof(data));
        key.data  = &linkid;
        key.size  = sizeof(linkid);
        data.data = s;
        data.size = sizeof(struct streets_struct);

        if ((ret = db->put(db, NULL, &key, &data, 0)) != 0 &&
                ret != DB_KEYEXIST) {
            db->err(db, ret, "loadStreets: %s: %s: db-put",
                    DATABASE, "DBStreets");
            return ret;
        }
        free(s);
    }

    qsort(*O, nEnt, sizeof(struct order_struct), order_compare);

    DBFClose(dIN);

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


void dumpDB(DB *db, void (*pkv)(DBT *key, DBT *data))
{
    DBC *c = NULL;
    DBT key, data;
    int ret;

    if ((ret = db->cursor(db, NULL, &c, 0)) != 0) {
        db->err(db, ret, "dumpDB");
        return;
    }

    memset(&key,  0, sizeof(key));
    memset(&data, 0, sizeof(data));
    key.flags  = DB_DBT_MALLOC;
    data.flags = DB_DBT_MALLOC;

    pkv(NULL, NULL);
    while((ret = c->c_get(c, &key, &data, DB_NEXT)) == 0) {
        pkv(&key, &data);
        free(key.data);
        free(data.data);
    }
    
    c->c_close(c);
}


void pkv_AreaCode(DBT *key, DBT *data)
{
    struct ac_key_struct *a;
    
    if (!key) {
        printf("ADMIN_LVL|AREACODE1:AREACODE2:AREACODE3:AREACODE4:AREACODE5:AREACODE6:AREACODE7=>AREA_ID\n");
    }
    else {
        a = key->data;
        printf("%d|%d:%d:%d:%d:%d:%d:%d=>%ld\n", a->lvl,
                a->ac[0], a->ac[1], a->ac[2], a->ac[3], a->ac[4],
                a->ac[5], a->ac[6], *(long *) data->data);
    }
}


void pkv_AreaID(DBT *key, DBT *data)
{
    struct ac_key_struct  *a;
    struct ac_data_struct *b;

    if (!key) {
        printf("AREA_ID=>AREA_ID|ADMIN_LVL:AREACODE1:AREACODE2:AREACODE3:AREACODE4:AREACODE5:AREACODE6:AREACODE7|NAME:LANG:TYPE\n");
    }
    else {
        b = data->data;
        a = &(b->ac);
        printf("%ld=>%ld|%d|%d:%d:%d:%d:%d:%d:%d|%s:%s:%s\n",
                *(long *) key->data, b->areaid, a->lvl,
                a->ac[0], a->ac[1], a->ac[2], a->ac[3], a->ac[4],
                a->ac[5], a->ac[6], b->name, b->lang, b->type);
    }
}


void pkv_Streets(DBT *key, DBT *data)
{
    struct streets_struct *s;

    if (!key) {
        printf("LINK_ID=>RECNO:ST_NAME:NUM_STNMES:ADDR_TYPE:L_REFADDR:L_NREF_ADDR:L_ADDRSCH:L_ADDRFORM:R_REFADDR:R_NREFADDR:R_ADDRSCH:R_ADDRFORM:REF_IN_ID:NREF_IN_ID:L_AREA_ID:R_AREA_ID:L_POSTCODE:R_POSTCODE:L_NUMZONES:R_NUMZONES:NUM_AD_RNG:ROUTE_TYPE:POSTALNAME:EXPLICATBL\n");
    }
    else {
        s = data->data;
        printf("%ld=>%d:%s:%d:%s:%s:%s:%s:%s:%s:%s:%s:%s:%ld:%ld:%ld:%ld:%s:%s:%d:%d:%d:%s:%s:%s\n",
                *(long *) key->data, s->recno, s->st_name, s->num_stnmes, s->addr_type,
                s->l_refaddr, s->l_nrefaddr, s->l_addrsch, s->l_addrform,
                s->r_refaddr, s->r_nrefaddr, s->r_addrsch, s->r_addrform,
                s->ref_in_id, s->nref_in_id, s->l_area_id, s->r_area_id,
                s->l_postcode, s->r_postcode, s->l_numzones, s->r_numzones,
                s->num_ad_rng, s->route_type, s->postalname, s->explicatbl);
    }
}


void pkv_ZoneID(DBT *key, DBT *data)
{
    struct zoneRec_struct *z;

    if (!key) {
        printf("ZONE_ID=>ZONE_NAME:LANG_CODE:Z_NMTYPE:ZONE_TYPE:AREA_ID\n");
    }
    else {
        z = data->data;
        printf("%ld=>%s:%s:%s:%s:%ld\n", *(long *) key->data,
                z->name, z->lang, z->nmtyp, z->type, z->areaid);
    }
}


void pkv_Zones(DBT *key, DBT *data)
{
    struct zones_struct *z;

    if (!key) {
        printf("LINK_ID=>ZONE_ID:SIDE\n");
    }
    else {
        z = data->data;
        printf("%ld=>%ld:%s\n", *(long *) key->data,
                z->zoneid, z->side);
    }
}


struct handle_struct *createHandle(char *fiStreets, char *foStreets, DB *DBAreaCode, DB *DBAreaID, DB *DBZoneID, DB *DBZones, DB *DBStreets)
{
    char *sin;
    char *sout;
    struct handle_struct *H;
    H = (struct handle_struct *) calloc(1, sizeof(struct handle_struct));
    assert(H);

    H->fiStreets  = fiStreets;
    H->foStreets  = foStreets;
    H->DBAreaCode = DBAreaCode;
    H->DBAreaID   = DBAreaID;
    H->DBZoneID   = DBZoneID;
    H->DBZones    = DBZones;
    H->DBStreets  = DBStreets;
    H->cnt        = 0;
    
    /* open shapefiles */
    
    sin = strdup(H->fiStreets);
    strcpy(sin+strlen(sin)-3, "shp");
    if (!(H->iSHP  = SHPOpen(sin, "rb"))) {
        fprintf(stderr, "Failed to open shp %s for read\n", sin);
        exit(1);
    }
    free(sin);
    SHPGetInfo(H->iSHP, &H->nEnt, NULL, NULL, NULL);
    
    if (!(H->oDBF  = DBFCreate(H->foStreets))) {
        fprintf(stderr, "Failed to create dbf %s\n", foStreets);
        exit(1);
    }

    sout = strdup(H->foStreets);
    strcpy(sout+strlen(sout)-3, "shp");
    if (!(H->oSHP  = SHPCreate(sout, SHPT_ARC))) {
        fprintf(stderr, "Failed to create shp %s\n", sout);
        exit(1);
    }
    free(sout);

    /* create output structure */
    assert(DBFAddField(H->oDBF, "LINK_ID",    FTInteger, 10, 0) != -1);
    assert(DBFAddField(H->oDBF, "NAME",       FTString,  80, 0) != -1);
    assert(DBFAddField(H->oDBF, "ADDR_TYPE",  FTString,   1, 0) != -1);
    assert(DBFAddField(H->oDBF, "L_REFADDR",  FTString,  10, 0) != -1);
    assert(DBFAddField(H->oDBF, "L_NREFADDR", FTString,  10, 0) != -1);
    assert(DBFAddField(H->oDBF, "L_ADDRSCH",  FTString,   1, 0) != -1);
    assert(DBFAddField(H->oDBF, "L_ADDRFORM", FTString,   1, 0) != -1);
    assert(DBFAddField(H->oDBF, "R_REFADDR",  FTString,  10, 0) != -1);
    assert(DBFAddField(H->oDBF, "R_NREFADDR", FTString,  10, 0) != -1);
    assert(DBFAddField(H->oDBF, "R_ADDRSCH",  FTString,   1, 0) != -1);
    assert(DBFAddField(H->oDBF, "R_ADDRFORM", FTString,   1, 0) != -1);
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
    
    return H;
}


void closeHandle(struct handle_struct *H)
{
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
    struct zones_struct *zones;
    struct zoneRec_struct *zr;
    static struct zoneRec_struct cur;
    DB  *db = H->DBZones;
    DB  *dbzr = H->DBZoneID;
    DBC *c = NULL;
    DBC *zc = NULL;
    static DBT key, data;
    static DBT zkey, zdata;
    int ret, zret;
    long zoneid;

    if ((ret = db->cursor(db, NULL, &c, 0)) != 0) {
        db->err(db, ret, "getPostalZone: DBZones");
        return;
    }

    if ((ret = dbzr->cursor(dbzr, NULL, &zc, 0)) != 0) {
        dbzr->err(dbzr, ret, "getPostalZone: DBZoneID");
        return;
    }

    memset(&key,  0, sizeof(key));
    memset(&data, 0, sizeof(data));
    data.flags = DB_DBT_MALLOC;

    key.data = &linkid;
    key.size = sizeof(linkid);

    ret = c->c_get(c, &key, &data, DB_SET);
    while(ret == 0) {
        zones = data.data;
        zoneid = zones->zoneid;

        /* zoneid==0 if there is not associated record */
        if (zoneid) {
            memset(&zkey,  0, sizeof(zkey));
            memset(&zdata, 0, sizeof(zdata));
            zdata.flags = DB_DBT_MALLOC;
            
            memset(&cur, 0, sizeof(struct zoneRec_struct));

            zkey.data = &zoneid;
            zkey.size = sizeof(zoneid);

            zret = zc->c_get(zc, &zkey, &zdata, DB_SET);
            while(zret == 0) {
                zr = zdata.data;

                if (!strlen(cur.name))
                    memcpy(&cur, zr, sizeof(struct zoneRec_struct));
                else if (!strcmp(zr->type, "PA"))
                    memcpy(&cur, zr, sizeof(struct zoneRec_struct));
                else if (zr->nmtyp[0] == 'B')
                    memcpy(&cur, zr, sizeof(struct zoneRec_struct));

                if (zdata.data) {
                    free(zdata.data);
                    zdata.data = NULL;
                }
                zret = zc->c_get(zc, &zkey, &zdata, DB_NEXT_DUP);
            }
            
            if (strlen(cur.name)) {
                switch (zones->side[0]) {
                    case 'L':
                        strcpy(PL->zname, cur.name);
                        PL->zareaid = cur.areaid;
                        break;
                    case 'R':
                        strcpy(PR->zname, cur.name);
                        PR->zareaid = cur.areaid;
                        break;
                    case 'B':
                    default:
                        strcpy(PL->zname, cur.name);
                        PL->zareaid = cur.areaid;
                        strcpy(PR->zname, cur.name);
                        PR->zareaid = cur.areaid;
                        break;
                }
            }
            
            if (zdata.data) {
                free(zdata.data);
                zdata.data = NULL;
            }
        }
        
        if (data.data) {
            free(data.data);
            data.data = NULL;
        }
        ret = c->c_get(c, &key, &data, DB_NEXT_DUP);
    }
    if (data.data) {
        free(data.data);
        data.data = NULL;
    }
    
    c->c_close(c);
    zc->c_close(zc);
}


long areaCode2AreaID(struct handle_struct *H, struct ac_key_struct *ac)
{
    DB  *db   = H->DBAreaCode;
    DBC *c = NULL;
    DBT key, data;
    struct ac_key_struct *a;
    int areaid = 0;
    int ret;

    if ((ret = db->cursor(db, NULL, &c, 0)) != 0) {
        db->err(db, ret, "areaCode2AreaID: DBAreaCode");
        return 0;
    }
    
    memset(&key,  0, sizeof(key));
    memset(&data, 0, sizeof(data));
    data.flags = DB_DBT_MALLOC;

    key.data = ac;
    key.size = sizeof(struct ac_key_struct);

    if (c->c_get(c, &key, &data, DB_SET) == 0) {
        areaid = *(long *)data.data;
        free(data.data);
    }

    c->c_close(c);
    return areaid;
}


int getName(DBC *c, long areaid, char *name, struct ac_key_struct *parent)
{
    static DBT key, data;
    struct ac_data_struct *ac = NULL;
    int lvl;
    int ret;

    name[0] = '\0';

    memset(&key,  0, sizeof(key));
    memset(&data, 0, sizeof(data));
    data.flags = DB_DBT_MALLOC;

    key.data = &areaid;
    key.size = sizeof(areaid);

    ret = c->c_get(c, &key, &data, DB_SET);
    if (ret == 0) {
        while (ret == 0) {
            ac = data.data;
            if (ac->type[0] == 'B') break;
            if (data.data) {
                free(data.data);
                data.data = NULL;
            }
            ret = c->c_get(c, &key, &data, DB_NEXT_DUP);
        }
        lvl = ac->ac.lvl;
        strcpy(name, ac->name);
        memcpy(parent, &(ac->ac), sizeof(struct ac_key_struct));
        parent->lvl--;
        parent->ac[parent->lvl] = 0;
        if (data.data) {
            free(data.data);
            data.data = NULL;
        }
    }
    else
        lvl = 0;
    
    return lvl;
}


void getPlaceNames(struct handle_struct *H, long areaid, struct place_struct *P)
{
    DB  *db   = H->DBAreaID;
    DBC *c = NULL;
    int lvl;
    int ret;
    static char name[MAX_NAME];
    static struct ac_key_struct parent;

    if ((ret = db->cursor(db, NULL, &c, 0)) != 0) {
        db->err(db, ret, "getName: DBAreaID");
        return;
    }

    lvl = getName(c, areaid, name, &parent);
    P->lvl = lvl;
    strcpy(P->name[lvl-1], name);
    while (lvl > 0) {
        areaid = areaCode2AreaID(H, &parent);
        lvl = getName(c, areaid, name, &parent);
        strcpy(P->name[lvl-1], name);
    }

    c->c_close(c);
}


void saveRecord(struct handle_struct *H, long linkid, struct streets_struct *c)
{
    static DBT key, data;
    static struct place_struct PL, PR;
    long LAreaID = -1;
    long RAreaID = -1;
    SHPObject *s;

    /* debug look at the records we have selected */
    if (0) {
        memset(&key,  0, sizeof(key));
        memset(&data, 0, sizeof(data));

        key.data = &linkid;
        key.size = sizeof(linkid);
        data.data = c;
        data.size = sizeof(struct streets_struct);
        pkv_Streets(&key, &data);
    }

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

    /* write denormalized record to shapefile */

    s = SHPReadObject(H->iSHP, c->recno);
    H->cnt = SHPWriteObject(H->oSHP, -1, s);
    SHPDestroyObject(s);
    
    DBFWriteIntegerAttribute(H->oDBF, H->cnt,  0, linkid);
    DBFWriteStringAttribute(H->oDBF,  H->cnt,  1, c->st_name);
    DBFWriteStringAttribute(H->oDBF,  H->cnt,  2, c->addr_type);
    DBFWriteStringAttribute(H->oDBF,  H->cnt,  3, c->l_refaddr);
    DBFWriteStringAttribute(H->oDBF,  H->cnt,  4, c->l_nrefaddr);
    DBFWriteStringAttribute(H->oDBF,  H->cnt,  5, c->l_addrsch);
    DBFWriteStringAttribute(H->oDBF,  H->cnt,  6, c->l_addrform);
    DBFWriteStringAttribute(H->oDBF,  H->cnt,  7, c->r_refaddr);
    DBFWriteStringAttribute(H->oDBF,  H->cnt,  8, c->r_nrefaddr);
    DBFWriteStringAttribute(H->oDBF,  H->cnt,  9, c->r_addrsch);
    DBFWriteStringAttribute(H->oDBF,  H->cnt, 10, c->r_addrform);
    DBFWriteStringAttribute(H->oDBF,  H->cnt, 11, PL.name[4]);
    DBFWriteStringAttribute(H->oDBF,  H->cnt, 12, PL.name[3]);
    DBFWriteStringAttribute(H->oDBF,  H->cnt, 13, PL.name[2]);
    DBFWriteStringAttribute(H->oDBF,  H->cnt, 14, PL.name[1]);
    DBFWriteStringAttribute(H->oDBF,  H->cnt, 15, PL.name[0]);
    DBFWriteStringAttribute(H->oDBF,  H->cnt, 16, PR.name[4]);
    DBFWriteStringAttribute(H->oDBF,  H->cnt, 17, PR.name[3]);
    DBFWriteStringAttribute(H->oDBF,  H->cnt, 18, PR.name[2]);
    DBFWriteStringAttribute(H->oDBF,  H->cnt, 19, PR.name[1]);
    DBFWriteStringAttribute(H->oDBF,  H->cnt, 20, PR.name[0]);
    DBFWriteStringAttribute(H->oDBF,  H->cnt, 21, c->l_postcode);
    DBFWriteStringAttribute(H->oDBF,  H->cnt, 22, c->r_postcode);
}

void processRecords(struct handle_struct *H, struct order_struct *O)
{
    DB *DBStreets = H->DBStreets;
    DBC *c = NULL;
    static DBT key, data;
    int i, ret;
    long linkid;
    static struct streets_struct cur;
    struct streets_struct *this;

    if ((ret = DBStreets->cursor(DBStreets, NULL, &c, 0)) != 0) {
        DBStreets->err(DBStreets, ret,
                "processRecords: Failed to get DB cursor for DBStreets");
        return;
    }

    long last_linkid = -1;
    for (i=0; i<H->nEnt; i++) {
        /* if its the same linkid then ww can skip it */
        if (i && O[i].linkid == O[i-1].linkid)
            continue;
        
        memset(&key,  0, sizeof(key));
        memset(&data, 0, sizeof(data));
        data.flags = DB_DBT_MALLOC;

        /* build the key based on O[i].linkid */
        linkid = O[i].linkid;
        key.data = &linkid;
        key.size = sizeof(linkid);
        
        /* DB_SET the key on first, htne DB_NEXT_DUP */
        ret = c->c_get(c, &key, &data, DB_SET);
        if (ret != 0)
            continue;
        
        while (ret == 0) {
            this = data.data;
            if (last_linkid != linkid && last_linkid != -1) {
                saveRecord(H, last_linkid, &cur);
                memset(&cur, 0, sizeof(struct streets_struct));
                last_linkid = -1;
            }
            
            /* rules for selecting which of multiple records we use */

            /* if it is the first records take it */
            if (last_linkid == -1) {
                last_linkid = linkid;
                memcpy(&cur, this, sizeof(struct streets_struct));
            }
            /* if it is the postname record take it */
            else if (this->postalname[0] == 'Y') {
                memcpy(&cur, this, sizeof(struct streets_struct));
            }
            /* if cur is the postalname then we assume it is the best */
            else if (cur.postalname[0] != 'Y') {
                /* if cur has no addr ranges and this does, take it */
                if (this->num_ad_rng > 0 && cur.num_ad_rng == 0) {
                    memcpy(&cur, this, sizeof(struct streets_struct));
                }
                /* if this has right and left and cur is missing one, take it */
                else if ((strlen(this->l_refaddr) && strlen(this->r_refaddr)) &&
                        (!strlen(cur.l_refaddr) || !strlen(cur.r_refaddr))) {
                    memcpy(&cur, this, sizeof(struct streets_struct));
                }
                /* one is left only and one is right only, merge them */
                else if (0) {
                }
                /* otherwise give explicatbl a preference */
                else if (cur.explicatbl[0] == 'N' && this->explicatbl[0] == 'Y') {
                    memcpy(&cur, this, sizeof(struct streets_struct));
                }
            }
            if (data.data) free(data.data);
            data.data = NULL;
            ret = c->c_get(c, &key, &data, DB_NEXT_DUP);
        }
        if (data.data) free(data.data);
        data.data = NULL;
    }
    if (last_linkid != -1)
        saveRecord(H, last_linkid, &cur);
    
    c->c_close(c);
}


int stat_print(DB *db, u_int32_t flags) {
    DB_BTREE_STAT *sp = NULL;
    int ret;

    //memset(&sp, 0, sizeof(DB_BTREE_STAT));

    ret = db->stat(db, (void *) &sp, flags);
    if (ret == 0 && sp) {
        printf("        bt_magic: %d\n", sp->bt_magic);
        printf("      bt_version: %d\n", sp->bt_version);
        printf("        bt_nkeys: %d\n", sp->bt_nkeys);
        printf("        bt_ndata: %d\n", sp->bt_ndata);
        //printf("      bt_pagecnt: %d\n", sp->bt_pagecnt);
        printf("     bt_pagesize: %d\n", sp->bt_pagesize);
        printf("       bt_maxkey: %d\n", sp->bt_maxkey);
        printf("       bt_minkey: %d\n", sp->bt_minkey);
        printf("       bt_re_len: %d\n", sp->bt_re_len);
        printf("       bt_re_pad: %d\n", sp->bt_re_pad);
        printf("       bt_levels: %d\n", sp->bt_levels);
        printf("       bt_int_pg: %d\n", sp->bt_int_pg);
        printf("      bt_leaf_pg: %d\n", sp->bt_leaf_pg);
        printf("       bt_dup_pg: %d\n", sp->bt_dup_pg);
        printf("      bt_over_pg: %d\n", sp->bt_over_pg);
        //printf("     bt_empty_pg: %d\n", sp->bt_empty_pg);
        printf("         bt_free: %d\n", sp->bt_free);
        printf("   bt_int_pgfree: %d\n", sp->bt_int_pgfree);
        printf("  bt_leaf_pgfree: %d\n", sp->bt_leaf_pgfree);
        printf("   bt_dup_pgfree: %d\n", sp->bt_dup_pgfree);
        printf("  bt_over_pgfree: %d\n", sp->bt_over_pgfree);
    }
    return ret;
}


int main( int argc, char **argv ) {
    DB      *DBAreaCode = NULL;
    DB      *DBAreaID   = NULL;
    DB      *DBZoneID   = NULL;
    DB      *DBZones    = NULL;
    DB      *DBStreets  = NULL;
    char    *fiStreets;
    char    *foStreets;
    char    *fMtdArea;
    char    *fZones;
    char    *fMtdZoneRec;
    int     i;
    int     ret, t_ret;
    struct handle_struct *H;
    struct order_struct *order;

    if (argc < 3)
        Usage();

    fiStreets   = mkFName(argv[1], "Streets.dbf");
    foStreets   = mkFName(argv[2], "Streets.dbf");
    fMtdArea    = mkFName(argv[1], "MtdArea.dbf");
    fMtdZoneRec = mkFName(argv[1], "MtdZoneRec.dbf");
    fZones      = mkFName(argv[1], "Zones.dbf");

    unlink(DATABASE);

    /* load MtdArea */
    ret = loadMtdArea(fMtdArea, &DBAreaCode, &DBAreaID);
    if (ret) goto err;
    //dumpDB(DBAreaCode, pkv_AreaCode);
    //dumpDB(DBAreaID, pkv_AreaID);

    /* load MtdZoneRec */
    ret = loadMtdZoneRec(fMtdZoneRec, &DBZoneID);
    if (ret) goto err;
    //dumpDB(DBZoneID, pkv_ZoneID);

    /* load Zones */
    ret = loadZones(fZones, &DBZones);
    if (ret) goto err;
    //dumpDB(DBZones, pkv_Zones);

    ret = loadStreets(fiStreets, &DBStreets, &order);
    if (ret) goto err;
    //dumpDB(DBStreets, pkv_Streets);

    printf("  calling createHandle\n");
    H = createHandle(fiStreets, foStreets, DBAreaCode, DBAreaID,
            DBZoneID, DBZones, DBStreets);
    
    printf("  calling processRecords\n");
    processRecords(H, order);

    printf("  calling closeHandle\n");
    closeHandle(H);
    
    /* clean up and exit */
err:
    printf("  closing DBs\n");
    
    free(fiStreets);
    free(foStreets);
    free(fMtdArea);
    free(fMtdZoneRec);
    free(fZones);
    
#define WHICH_STATS 0
//#define WHICH_STATS DB_FAST_STAT
    
    if (argc > 3 && !strcmp("-s", argv[3])) {
        if (DBAreaCode) {
            printf("\nDB Stats for DBAreaCode:\n");
            stat_print(DBAreaCode, WHICH_STATS);
        }
        if (DBAreaID) {
            printf("\nDB Stats for DBAreaID:\n");
            stat_print(DBAreaID, WHICH_STATS);
        }
        if (DBZoneID) {
            printf("\nDB Stats for DBZoneID:\n");
            stat_print(DBZoneID, WHICH_STATS);
        }
        if (DBZones) {
            printf("\nDB Stats for DBZones:\n");
            stat_print(DBZones, WHICH_STATS);
        }
        if (DBStreets) {
            printf("\nDB Stats for DBStreets:\n");
            stat_print(DBStreets, WHICH_STATS);
        }
    }
    
    if (DBAreaCode) DBAreaCode->close(DBAreaCode, DB_NOSYNC);
    if (DBAreaID)   DBAreaID->close(DBAreaID, DB_NOSYNC);
    if (DBZoneID)   DBZoneID->close(DBZoneID, DB_NOSYNC);
    if (DBZones)    DBZones->close(DBZones, DB_NOSYNC);
    if (DBStreets)  DBStreets->close(DBStreets, DB_NOSYNC);
    
    exit(ret);
}
