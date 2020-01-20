/*
 * $Id: doNtHwys.c,v 1.7 2012-02-25 20:47:22 woodbri Exp $
 *
 * doNtHwys style infile outfile
 * 
 * This utility program processes Navteq Arcview data
 * need for iMaptools.com Mapping For Navteq data.
 * It handles the following layers:
 *
 * MajHwys, SecHwys
 *
 * This utility checks for multiple records associated with a LINK_ID
 * and merges them preserving the highest class highway. It sets the 
 * rendering style flags and drops attributes we do not need.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <assert.h>

#include "shapefil.h"
#include "doNtUtils.h"
#include "rgxtools.h"

/* regex pattern for checking the type argument */
static char *types = "^(US|CA|EURO|U|C|E)";

/* input field names */
static char *inFName[] = {"LINK_ID", "HIGHWAY_NM", "FUNC_CLASS", "ROUTE_TYPE",
    "FERRY_TYPE", NULL};

#define fLINK_ID     fldNums[0]
#define fHIGHWAY_NM  fldNums[1]
#define fFUNC_CLASS  fldNums[2]
#define fROUTE_TYPE  fldNums[3]
#define fFERRY_TYPE  fldNums[4]

/* output field definitions */
static char *fldName[] = {"LINK_ID", "NAME", "RTE_P", "RTE_N", "RTE_T",
    "FCLASS", "FERRY", "STYLE", "SHIELD", NULL};
static DBFFieldType fldType[] = {FTInteger, FTString, FTString, FTString,
    FTString, FTString, FTString, FTString, FTString};
static fldSize[] = {10, 80, 2, FWID_RTE_NUM, 1, 1, 1, 2, 2};

#define gLINK_ID    0
#define gNAME       1
#define gRTE_P      2
#define gRTE_N      3
#define gRTE_T      4
#define gFCLASS     5
#define gFERRY      6
#define gSTYLE      7
#define gSHIELD     8

void Usage() {
    printf("Usage: doNtHwys [-u] style infile outfile\n");
    printf("       -u    - indicates that the attribute data is UTF8\n");
    printf("       style - US|CA|EURO\n");
    exit(1);
}


int main(int argc, char *argv[]) {

    static char buf[1024];
    static sortStruct* data;
    int nRecords;

    SHPHandle inSHP, outSHP;
    DBFHandle inDBF, outDBF;
    SHPObject *shape;
    int fldNums[5] = {-1, -1, -1, -1, -1};
    int shapeType;
    int numFields;
    int i, j, k, n;
    int link_id;
    int ierr = 0;
    int numf = 9;
    int region = 0;
    int isTCH;
    int isRte;
    int isName;
    int gotName;
    int rtype;
    int lrtype;
    int fclass;
    int lfclass;
    int utf8 = 0;  /* set flag to indicate data is NOT UTF* */
    char rte_p[3];
    char rte_n[FWID_RTE_NUM+1];
    char rte_t[2];
    char ferry[2];
    char style[3];
    char shield[3];
    const char *name;
    const char *str;

    /* quick check of args */
    if (argc > 1 && argv[1][0] == '-' && argv[1][1] == 'u') {
        utf8 = 1;
        argv++;
        argc--;
    }
    if (argc != 4) Usage();
    if (!match(argv[1], types, 1)) Usage();

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

    switch (LC(argv[1][0])) {
        case 'u':
            region = 0;
            break;
        case 'c':
            region = 1;
            break;
        case 'e':
            region = 2;
            break;
        default:
            fprintf(stderr, "Unknown type ($s)!\n", argv[1]);
            exit(1);
    }

    for (i=0; inFName[i]; i++) {
        fldNums[i] = DBFGetFieldIndex(inDBF, inFName[i]);
        if (fldNums[i] < 0) {
            fprintf(stderr, "Field (%s) Missing from Input!\n", inFName[i]);
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
    for (i=0; fldName[i]; i++)
        DBFAddField(outDBF, fldName[i], fldType[i], fldSize[i], 0);

    data = sortDBF(inDBF, inFName[0], cmp_num_asc);
    if (!data) {
        fprintf(stderr, "Failed to allocate and sort input DBF! Aborting.\n");
        exit(1);
    }
    /*
     * OUT_FLD OUT IN   IN_FIELD
     * ---------------------------
     * LINK_ID  0   0   LINK_ID
     * NAME     1   1   HIGHWAY_NM
     * RTE_P    2
     * RTE_N    3
     * RTE_T    4   3   ROUTE_TYPE
     * FCLASS   5   2   FUNC_CLASS
     * FERRY    6   4   FERRY_TYPE
     * STYLE    7
     * SHIELD   8
     *
     *
     * ROUTE_TYPE and FUNC_CLASS
     * ======================
     * (space) NOT APPLICABLE
     * 1 LEVEL 1 ROAD   Major Hwys
     * 2 LEVEL 2 ROAD   Major Hwys
     * 3 LEVEL 3 ROAD   Secondary Hwys
     * 4 LEVEL 4 ROAD   Secondary Hwys
     * 5 LEVEL 5 ROAD
     * 6 LEVEL 6 ROAD
     */
    
    /* loop through the records, modify them if needed and write them out */
    for (i=0; i<nRecords; i++) {

        j = data[i].index;
        link_id = data[i].number;

        shape = SHPReadObject(inSHP, j);
        n = SHPWriteObject(outSHP, -1, shape);
        SHPDestroyObject(shape);

        memset(rte_t, 0, 2);
        memset(rte_p, 0, 3);
        memset(rte_n, 0, FWID_RTE_NUM+1);
        memset(ferry, 0, 2);

        DBFWriteIntegerAttribute(outDBF, n, 0,  /* LINK_ID */
                DBFReadIntegerAttribute(inDBF, j, fLINK_ID));
        DBFWriteStringAttribute(outDBF, n, 5,   /* FCLASS */
                DBFReadStringAttribute(inDBF, j, fFUNC_CLASS));
        DBFWriteStringAttribute(outDBF, n, 4,   /* RTE_T */
                DBFReadStringAttribute(inDBF, j, fROUTE_TYPE));
        ferry[0] = (!strcasecmp(
                    DBFReadStringAttribute(inDBF, j, fFERRY_TYPE),
                    "B")
                )?'F':'\0';
        DBFWriteStringAttribute(outDBF, n, 6, ferry);

        /*
         * We might have multiple records for a given LINK_ID because
         * the road segment might have multiple names like:
         *  i-95, RT-128, RT-3, Technology Highway
         * So we loop throught the records and collect the information
         * then write out the results and continue on to the next
         *
         * Numbered routes take priority by Interstate, US Highway,
         * and State Highway respectively in the US
         * 
         * Trans Canadian Highway, others ...
         * 
         * In Europe, highway priority is smallest numbered is higher
         * priority, and M<num>, E<num>, etc
         */

        isTCH = 0;
        for (k=1, gotName=0, isName=0, lfclass=100, lrtype=100; k;) {
            assert( link_id == DBFReadIntegerAttribute(inDBF, j, fLINK_ID));
            
            rtype  = strtol(DBFReadStringAttribute(inDBF, j, fROUTE_TYPE), NULL, 10);
            fclass = strtol(DBFReadStringAttribute(inDBF, j, fFUNC_CLASS), NULL, 10);
            name = DBFReadStringAttribute(inDBF, j, fHIGHWAY_NM);
            if (!strcasecmp("TRANS CANADA HWY", name)) isTCH |= 1;

            if (!rtype && strlen(name) <= FWID_RTE_NUM && isdigit(name[0])) {
                isRte = extractRouteNum(region, rte_n, rte_p, name);
                if (isRte && lrtype > 10) {
                    strcpy(rte_t, "10");
                    rtype = lrtype = 10;
                }
            }
            else if (rtype) {
                isRte = extractRouteNum(region, rte_n, rte_p, name);
                if (isRte && lrtype != 7 && rtype < lrtype) {
                    if (!strcmp(rte_p, "E")) rtype = 7;
                    sprintf(rte_t, "%d", rtype);
                    DBFWriteStringAttribute(outDBF, n, gRTE_T, rte_t);
                    DBFWriteStringAttribute(outDBF, n, gRTE_N, rte_n);
                    DBFWriteStringAttribute(outDBF, n, gRTE_P, rte_p);
                    lrtype = rtype;
                }
            }

            /* 
             * TODO: need to adjust rtype if this is an exit.
             * so scan the name anf if it is an exit rtype += 10;
             */

            /* if this is a name chack if we need it  and set it */
            if (!rtype && !gotName) {
                if (utf8)
                    str = u_wordCase(name);
                else
                    str = wordCase(name);
                DBFWriteStringAttribute(outDBF, n, gNAME, str);
                gotName = 1;
            }

            /* check the status of the fclass and set it if better */
            if (fclass < lfclass) {
                lfclass = fclass;
                DBFWriteStringAttribute(outDBF, n, gFCLASS,
                        DBFReadStringAttribute(inDBF, j, fFUNC_CLASS));
            }

            if (i+1<nRecords && link_id == data[i+1].number) {
                i++;
                j = data[i].index;
            }
            else {
                k = 0; /* exit the for k */
            }

        } /* end for k */

        strcpy(style, getStyle(region, fclass, 0, ferry, rte_p));
        rtype  = strtol(rte_t, NULL, 10);
        strcpy(shield, getShield(region, rtype, fclass, rte_p, isTCH));

        DBFWriteStringAttribute(outDBF, n, gSTYLE, style);
        DBFWriteStringAttribute(outDBF, n, gSHIELD, shield);
    } /* end for i */


    free(data);

    SHPClose(outSHP);
    DBFClose(outDBF);
    SHPClose(inSHP);
    DBFClose(inDBF);

    return(0);
}
