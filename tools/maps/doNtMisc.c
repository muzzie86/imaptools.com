/*
 * $Id: doNtMisc.c,v 1.4 2011/02/17 03:15:51 woodbri Exp $
 *
 * doNtMisc type infile outfile
 * 
 * This utility program processes Navteq Arcview data
 * need for iMaptools.com Mapping For Navteq data.
 * It handles the following layers:
 *
 * WaterSeg, RailRds, NamedPlc
 *
 * For these layers, we mostly just copy the shapefiles
 * and remove most attributes. For the NamedPls layer we also
 * classify the records by population.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include "shapefil.h"
#include "doNtUtils.h"
#include "rgxtools.h"

/* regex pattern for checking the type argument */
static char *types = "^(NamedPlc|Hamlet|WaterSeg|RailRds|N|W|R)";

/* input field names */
static char *fldN[] = {"POI_ID", "POI_NAME", "POPULATION", "CAPITAL", "POI_NMTYPE", NULL};
static char *fldH[] = {"POI_ID", "POI_NAME", "POI_NMTYPE", NULL};
static char *fldW[] = {"LINK_ID", "POLYGON_NM", NULL};
static char *fldR[] = {"LINK_ID", "RAILWAY_NM", NULL};

/* output field definitions */
static char *fldName[] = {"ID", "NAME", "POP", "CAPITAL", NULL};
static DBFFieldType fldType[] = {FTInteger, FTString, FTInteger, FTString};
static fldSize[] = {10, 80, 10, 1};

void Usage() {
    printf("Usage: doNtMisc [-u] type infile outfile\n");
    printf("       -u    - indicates attribute data is UTF8\n");
    printf("       -c    - use alternate city categories\n");
    printf("       type  - NamedPlc, WaterSeg, RailRds\n");
    exit(1);
}

int getPop(int pop, int cap) {
    if (cap == 1) return 1;
    if (pop <= 1000) return 6;
    if (pop <= 10000) return 5;
    if (pop <= 30000) return 4;
    if (pop <= 400000) return 3;
    return 2;
}


int main(int argc, char *argv[]) {

    static char buf[1024];

    RXcache *rxcBad = NULL;
    
    SHPHandle inSHP, outSHP;
    DBFHandle inDBF, outDBF;
    SHPObject *shape;
    char **names;
    const char *str;
    int fldNums[5] = {-1, -1, -1, -1, -1};
    int shapeType;
    int nRecords;
    int numFields;
    int i, n;
    int ierr = 0;
    int iPID, iNAME, iFCODE;
    int numf = 2;
    int namedplc;
    int hamlet;
    int pop;
    int cap;
    int pstyle;
    int utf8 = 0;
    int city = 0;

    /* quick check of args */
    if (argc > 1 && argv[1][0] == '-' && argv[1][1] == 'u') {
        utf8 = 1;
        argv++;
        argc--;
    }
    if (argc > 1 && argv[1][0] == '-' && argv[1][1] == 'c') {
        city = 1;
        argv++;
        argc--;
    }
    if (argc != 4) Usage();
    if (!match(argv[1], types, 1)) Usage();
    namedplc = match(argv[1], "^namedplc", 1);
    hamlet   = match(argv[1], "^hamlet", 1);
    if (namedplc)
        numf = 4;

    /* open the shapefile */
    mkFileName(argv[2], ".shp", buf);
    inSHP = SHPOpen(buf, "rb");
    if ( !inSHP ) {
        fprintf(stderr, "Unable to open shp file (%s)\n", buf);
        exit(1);
    }
    SHPGetInfo(inSHP, &nRecords, &shapeType, NULL, NULL);

    /* open the dbf dile */
    mkFileName(argv[2], ".dbf", buf);
    inDBF = DBFOpen(buf, "rb");
    if ( !inDBF ) {
        fprintf(stderr, "Unable to open dbf file (%s)\n", buf);
        exit(1);
    }

    switch (argv[1][0]) {
        case 'N':
        case 'n':
            names = fldN;
            break;
        case 'H':
        case 'h':
            names = fldH;
            break;
        case 'W':
        case 'w':
            names = fldW;
            break;
        case 'R':
        case 'r':
            names = fldR;
            break;
    }

    for (i=0; names[i]; i++) {
        fldNums[i] = DBFGetFieldIndex(inDBF, names[i]);
        if (fldNums[i] < 0) {
            fprintf(stderr, "Field (%s) Missing from Input!\n", names[i]);
            ierr = 1;
        }
    }
    
    if ( ierr ) {
        fprintf(stderr, "Required DBF fields not found!\n");
        exit(1);
    }

    /* create output files */
    mkFileName(argv[3], ".shp", buf);
    outSHP = SHPCreate(buf, shapeType);
    if ( !outSHP ) {
        fprintf(stderr, "Unable to create shp file (%s)\n", buf);
        exit(1);
    }
    
    mkFileName(argv[3], ".dbf", buf);
    outDBF = DBFCreate(buf);
    if ( !outDBF ) {
        fprintf(stderr, "Unable to create dbf file (%s)\n", buf);
        exit(1);
    }

    /* add the fields to the output dbf file */
    for (i=0; i<numf; i++)
        DBFAddField(outDBF, fldName[i], fldType[i], fldSize[i], 0);

    /* loop through the records, modify them if needed and write them out */
    for (i=0; i<nRecords; i++) {

        /* check for bad records we want to skip */
        if (rxcBad && cmatch(rxcBad,
                    (char *)DBFReadStringAttribute(inDBF, i, fldNums[1])))
            continue;

        if (namedplc) {
            if (!strcmp("E",
                        (char *)DBFReadStringAttribute(inDBF, i, fldNums[4])))
                continue;
            pop = DBFReadIntegerAttribute(inDBF, i, fldNums[2]);
            if (pop < 1) /* skip non populated places */
                continue;
        }
        else if (hamlet) {
            if (!strcmp("E",
                        (char *)DBFReadStringAttribute(inDBF, i, fldNums[2])))
                continue;
        }

        shape = SHPReadObject(inSHP, i);
        n = SHPWriteObject(outSHP, -1, shape);
        SHPDestroyObject(shape);
        DBFWriteIntegerAttribute(outDBF, n, 0,
                DBFReadIntegerAttribute(inDBF, i, fldNums[0]));
        if (utf8)
            str = u_wordCase(DBFReadStringAttribute(inDBF, i, fldNums[1]));
        else
            str = wordCase(DBFReadStringAttribute(inDBF, i, fldNums[1]));
        DBFWriteStringAttribute(outDBF, n, 1, str);
        if (namedplc) {
            if (city) {
                cap = strtol(DBFReadStringAttribute(inDBF, i, fldNums[3]), NULL, 10);
                pstyle = getPop(pop, cap);
            } else {
                pstyle = (int) log10(pop);
            }
            DBFWriteIntegerAttribute(outDBF, n, 2, pstyle);
            DBFWriteStringAttribute(outDBF, n, 3,
                    DBFReadStringAttribute(inDBF, i, fldNums[3]));
        }
    }

    SHPClose(outSHP);
    DBFClose(outDBF);
    SHPClose(inSHP);
    DBFClose(inDBF);

    return(0);
}
