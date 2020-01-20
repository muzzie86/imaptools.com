/*
 * $Id: doNtPolygons.c,v 1.6 2011/02/17 03:16:56 woodbri Exp $
 *
 * doNtPolygons type dcs infile outfile
 * 
 * This utility program processes Navteq Arcview ploygon data
 * need for iMaptools.com Mapping For Navteq data.
 * It handles the following layers:
 *
 * Adminbndy3, AdminBndy4, Islands, LandUseA, LandUseB, Oceans and
 * WaterPoly
 *
 * For all of these except AdminBnd4 and LandUse it just copies the
 * shapefile and drops the extra attribute columns. We always keep
 * the POLYGON_ID column as a back reference to the source data for
 * debugging if needed.
 *
 * For AdminBnd4, we scan the names and strip out certain patterns.
 * Patterns are stripped only for US DCAs.
 *
 * For LandUse, we classify the records based on type and add
 * a STYLE field.
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include "shapefil.h"
#include "doNtUtils.h"
#include "rgxtools.h"

/* regex pattern for checking the type argument */
static char *types = "^(Adminbndy3|AdminBndy4|Adminbndy5|Islands|LandUse[AB]|Landmark|Oceans|WaterPoly)";

/*
 * Navteq Feature codes and Feature types
 * LandUse FEAT_COD
    1   900103 PARK/MONUMENT (NATIONAL)
    2   900130 PARK (STATE)
    3   900150 PARK (CITY/COUNTY)
    4   900202 WOODLAND
    5   2000123 GOLF COURSE
    6   2000420 CEMETERY
    7   2000403 UNIVERSITY/COLLEGE
    8   900107 NATIVE AMERICAN RESERVATION
    9   900108 MILITARY BASE
    10  1700215 PARKING LOT
    11  1700216 PARKING GARAGE
    12  1900403 AIRPORT
    13  1907403 AIRCRAFT ROADS
    14  2000124 SHOPPING CENTRE
    15  2000200 INDUSTRIAL COMPLEX
    16  2000457 SPORTS COMPLEX
    17  2000408 HOSPITAL
 * Landmark FEAT_COD
    18  2009000 BUSINESS/COMMERCE
    19  2009001 CONVENTION/EXHIBITION
    20  2009002 CULTURAL
    21  2009003 EDUCATION
    22  2009005 GOVERNMENT
    23  2009006 HISTORICAL
    24  2009007 MEDICAL
    25  2009010 RETAIL
    26  2009011 SPORTS
    27  2009012 TOURIST
    28  2009013 TRANSPORTATION
 * LandUse FEAT_COD
    29  509998  BEACH
    30  900140  PARK IN WATER
 *
 * Landmark FEAT_CODE
    31  2005999 ENHANCED BUILDING/LANDMARK
 */

static int features[] = {900103,900130,900150,900202,2000123,2000420,2000403,900107,900108,1700215,1700216,1900403,1907403,2000124,2000200,2000457,2000408,2009000,2009001,2009002,2009003,2009005,2009006,2009007,2009010,2009011,2009012,2009013,509998,900140,2005999,0};


/* output DBF definition */
#define jPID  0
#define jNAME 1

/* METERS_PER_DEGREE = EARTH_CIRCUMFRENCE_IN_METERS / 360 */
#define METERS_PER_DEGREE (40075160/360)
#define KILOMETERS_PER_DEGREE (40075.16/360)

static char *fldName[] = {"PID", "NAME", NULL};
static DBFFieldType fldType[] = {FTInteger, FTString};
static fldSize[] = {10, 80};

void Usage() {
    printf("Usage: doNtPolygons [-u] [-a n] type ccode infile outfile\n");
    printf("       -u    - indicate the attribute data is UTF8\n");
    printf("       -a n  - 0  compute area of bbox\n");
    printf("               1  compute area as log(bbox)\n");
    printf("               2  compute area as area\n");
    printf("               3  compute area as log(area)\n");
    printf("       type  - Adminbndy3, AdminBndy4, Islands, LandUseA\n");
    printf("               LandUseB, Oceans, WaterPoly Landmark\n");
    printf("       ccode - ISO 2 character country code\n");
    printf("               which may be used to trigger country specific processing\n");
    exit(1);
}

/*
 * aStyle = 0  compute area of bbox
 *          1  compute area as log(bbox)
 *          2  compute area
 *          3  compute area as log(area)
*/
float computeArea(SHPObject *sobj, int aStyle) {
    int j, k;
    int partEnd;
    float area, dx, dy;

    if (aStyle < 2) {
        area = (sobj->dfXMax - sobj->dfXMin) * KILOMETERS_PER_DEGREE *
               (sobj->dfYMax - sobj->dfYMin) * KILOMETERS_PER_DEGREE;
    }
    else {
        area = 0.0;
        if (sobj->nParts) {
            for (k=0; k<sobj->nParts; k++) {
                partEnd = sobj->nVertices;
                if (k < sobj->nParts-1)
                    partEnd = sobj->panPartStart[k+1];
                for (j=sobj->panPartStart[k]; j<partEnd-1; j++) {
                    dx = sobj->padfX[j+1] - sobj->padfX[j];
                    dy = (sobj->padfY[j+1] + sobj->padfY[j]) / 2.0;
                    area += dx * KILOMETERS_PER_DEGREE *
                            dy * KILOMETERS_PER_DEGREE;
                }
            }
        }
        else {
            for (j=0; j<sobj->nVertices-1; j++) {
                dx = sobj->padfX[j+1] - sobj->padfX[j];
                dy = (sobj->padfY[j+1] + sobj->padfY[j]) / 2.0;
                area += dx * KILOMETERS_PER_DEGREE *
                        dy * KILOMETERS_PER_DEGREE;
            }
        }
    }

    area = fabs(area);
    if (aStyle % 2) area = floor(log10(area)+0.5);
    return area;
}


int simplifyFeatureCode(int fc, int *fclist) {
    int i = 1;
    while (*fclist) {
        if (fc == *fclist)
            return i;
        i++;
        fclist++;
    }
    return -1;
}

int main(int argc, char *argv[]) {

    static char buf[1024];

    RXcache *rxcBad = NULL;
    
    SHPHandle inSHP, outSHP;
    DBFHandle inDBF, outDBF;
    SHPObject *shape;
    const char *str;
    int shapeType;
    int nRecords;
    int numFields;
    int i, n;
    int iPID, iNAME, iFCODE, iDISPC, iHGT;
    int landuse;
    int waterpoly;
    int doarea = -1;
    int nf;
    int jHgt;
    int utf8 = 0;
    float area;

    if (argc > 1 && argv[1][0] == '-' && argv[1][1] == 'u') {
        utf8 = 1;
        argv++;
        argc--;
    }
    if (argc > 2 && !strcmp(argv[1], "-a")) {
        doarea = strtol(argv[2], NULL, 10);
        argc--; argv++;
        argc--; argv++;
    }

    /* quick check of args */
    if (argc != 5) Usage();
    if (!match(argv[1], types, 1)) Usage();
    landuse = match(argv[1], "^(landuse|landmark)", 1);
    waterpoly = match(argv[1], "^(waterpoly)", 1);

    /* open the shapefile */
    mkFileName(argv[3], ".shp", buf);
    inSHP = SHPOpen(buf, "rb");
    if ( !inSHP ) {
        fprintf(stderr, "Unable to open shp file (%s)\n", buf);
        exit(1);
    }
    SHPGetInfo(inSHP, &nRecords, &shapeType, NULL, NULL);

    /* open the dbf dile */
    mkFileName(argv[3], ".dbf", buf);
    inDBF = DBFOpen(buf, "rb");
    if ( !inDBF ) {
        fprintf(stderr, "Unable to open dbf file (%s)\n", buf);
        exit(1);
    }

    numFields = DBFGetFieldCount(inDBF);
    iPID   = DBFGetFieldIndex(inDBF, "POLYGON_ID");
    iNAME  = DBFGetFieldIndex(inDBF, "POLYGON_NM");
    iFCODE = DBFGetFieldIndex(inDBF, "FEAT_COD");
    iDISPC = DBFGetFieldIndex(inDBF, "DISP_CLASS");
    iHGT   = DBFGetFieldIndex(inDBF, "HEIGHT");

    if ( iPID < 0 || iNAME < 0 || (iFCODE < 0 && landuse) || iDISPC < 0 && waterpoly) {
        fprintf(stderr, "Required DBF fields POLYGON_ID or POLYGON_NM not found!\n");
        exit(1);
    }

    /* create output files */
    mkFileName(argv[4], ".shp", buf);
    outSHP = SHPCreate(buf, shapeType);
    if ( !outSHP ) {
        fprintf(stderr, "Unable to create shp file (%s)\n", buf);
        exit(1);
    }
    
    mkFileName(argv[4], ".dbf", buf);
    outDBF = DBFCreate(buf);
    if ( !outDBF ) {
        fprintf(stderr, "Unable to create dbf file (%s)\n", buf);
        exit(1);
    }

    /* add the fields to the output dbf file */
    for (i=0; fldName[i]; i++)
        DBFAddField(outDBF, fldName[i], fldType[i], fldSize[i], 0);

    if (landuse) {
        DBFAddField(outDBF, "FCODE", FTInteger, 3, 0);
        if (iHGT > -1)
            jHgt = DBFAddField(outDBF, "HGT", FTInteger, 3, 0);
    }

    if (waterpoly)
        DBFAddField(outDBF, "DISP_CLASS", FTInteger, 1, 0);

    if (doarea > -1)
        nf = DBFAddField(outDBF, "AREA", FTDouble, 12, 3);

    /* check if we are doing the US adminbndy4 layer and fileter bad words */
    if (match(argv[1], "^adminbndy4", 1) && !strcasecmp(argv[2], "US")) {
        rxcBad = buildRXcache(", TOWN OF|(TOWN OF)| TWP| TOWNSHIP| LOCATION| DISTRICT| PLANTATION| GRANT GORE", NULL, 1);
        if ( !rxcBad ) {
            fprintf(stderr, "Failed to create regex cache object!\n");
            exit(1);
        }
    }
        
    /* loop through the records, modify them if needed and write them out */
    for (i=0; i<nRecords; i++) {

        /* check for bad records we want to skip */
        if (rxcBad && cmatch(rxcBad, (char *)DBFReadStringAttribute(inDBF, i, iNAME)))
            continue;

        shape = SHPReadObject(inSHP, i);
        n = SHPWriteObject(outSHP, -1, shape);
        if (doarea > -1)
            area = computeArea(shape, doarea);
        SHPDestroyObject(shape);
        DBFWriteIntegerAttribute(outDBF, n, jPID,
                DBFReadIntegerAttribute(inDBF, i, iPID));
        if (utf8)
            str = u_wordCase(DBFReadStringAttribute(inDBF, i, iNAME));
        else
            str = wordCase(DBFReadStringAttribute(inDBF, i, iNAME));
        DBFWriteStringAttribute(outDBF, n, jNAME, str);
        if (landuse) {
            DBFWriteIntegerAttribute(outDBF, n, 2, 
                simplifyFeatureCode(DBFReadIntegerAttribute(inDBF, i, iFCODE),
                    features));
            if (iHGT > -1)
                DBFWriteIntegerAttribute(outDBF, n, jHgt,
                    DBFReadIntegerAttribute(inDBF, i, iHGT));
        }
                
        if (waterpoly)
            DBFWriteIntegerAttribute(outDBF, n, 2,
                DBFReadIntegerAttribute(inDBF, i, iDISPC));
        if (doarea > -1)
            DBFWriteDoubleAttribute(outDBF, n, nf, area);
    }

    SHPClose(outSHP);
    DBFClose(outDBF);
    SHPClose(inSHP);
    DBFClose(inDBF);

    return(0);
}
