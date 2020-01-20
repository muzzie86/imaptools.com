/*
 * $Id: doNtStreets.c,v 1.6 2011/02/21 21:01:23 woodbri Exp $
 *
 * doNtStreets style infile outfile
 * 
 * This utility program processes Navteq Arcview data
 * need for iMaptools.com Mapping For Navteq data.
 * It handles the following layers:
 *
 * Streets and AltStreets
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

#define DEBUG

#ifdef DEBUG
#define INFO(a,b) printf(a,b)
#else
#define INFO(a,b)
#endif

#define DBFRSA DBFReadStringAttribute
#define DBFWSA DBFWriteStringAttribute
#define DBFRIA DBFReadIntegerAttribute
#define DBFWIA DBFWriteIntegerAttribute

typedef struct {
    int link_id;
    int rtype;
    int fclass;
    int explicatbl;
    int isTCH;
    int isExit;
    int gotName;
    int nrte;
    char rte_p[3][FWID_RTE_P+1];
    char rte_n[3][FWID_RTE_NUM+1];
    int rte_t[3];
    char name[81];
    int fspdlim;
    int tspdlim;
    char spdcat[2];
    char lfaddr[11];
    char ltaddr[11];
    char rfaddr[11];
    char rtaddr[11];
} REC;

/* regex pattern for checking the type argument */
static char *types = "^(US|CA|EURO|U|C|E)";

/* input field names */
static char *inAName[] = {"LINK_ID", "ST_NAME", "ROUTE_TYPE", "EXPLICATBL",
    "NAMEONRDSN", "EXITNAME",
    "L_REFADDR", "L_NREFADDR", "R_REFADDR", "R_NREFADDR", NULL};

#define aLINK_ID     aFldNums[0]
#define aST_NAME     aFldNums[1]
#define aROUTE_TYPE  aFldNums[2]
#define aEXPLICATBL  aFldNums[3]
#define aNAMEONRDSN  aFldNums[4]
#define aEXITNAME    aFldNums[5]
#define aL_REFADDR   aFldNums[6]
#define aL_NREFADDR  aFldNums[7]
#define aR_REFADDR   aFldNums[8]
#define aR_NREFADDR  aFldNums[9]

static char *inFName[] = {"LINK_ID", "NUM_STNMES", "ST_NAME", "FUNC_CLASS",
    "ROUTE_TYPE", "DIR_TRAVEL", "RAMP", "PAVED", "TUNNEL", "FERRY_TYPE",
    "EXPLICATBL", "TOLLWAY", "EXITNAME",
    /* the following are optional based on -x option */
    "FR_SPD_LIM", "TO_SPD_LIM", "SPEED_CAT",
    "L_REFADDR", "L_NREFADDR", "R_REFADDR", "R_NREFADDR",
    NULL};

#define fLINK_ID     fldNums[0]
#define fNUM_STNMES  fldNums[1]
#define fST_NAME     fldNums[2]
#define fFUNC_CLASS  fldNums[3]
#define fROUTE_TYPE  fldNums[4]
#define fDIR_TRAVEL  fldNums[5]
#define fRAMP        fldNums[6]
#define fPAVED       fldNums[7]
#define fTUNNEL      fldNums[8]
#define fFERRY_TYPE  fldNums[9]
#define fEXPLICATBL  fldNums[10]
#define fTOLLWAY     fldNums[11]
#define fEXITNAME    fldNums[12]
#define X_INFLD_IDX  13
#define fFR_SPD_LIM  fldNums[13]
#define fTO_SPD_LIM  fldNums[14]
#define fSPEED_CAT   fldNums[15]
#define fL_REFADDR   fldNums[16]
#define fL_NREFADDR  fldNums[17]
#define fR_REFADDR   fldNums[18]
#define fR_NREFADDR  fldNums[19]

    
/* output field definitions */
static char *fldName[] = {"LINK_ID", "NAME", "RTYPE",
    "RTE1_P", "RTE1_N", "RTE1_T",
    "RTE2_P", "RTE2_N", "RTE2_T",
    "RTE3_P", "RTE3_N", "RTE3_T",
    "FCLASS", "DIRECTION", "STYLE", "EXTRA",
    /* the following are optional based on -x option */
    "SPD_LABEL", "FR_SPD_LIM", "TO_SPD_LIM", "SPEED_CAT",
    "L_REFADDR", "L_NREFADDR", "R_REFADDR", "R_NREFADDR",
    NULL};
static DBFFieldType fldType[] = {FTInteger, FTString, FTString,
    FTString, FTString, FTString,
    FTString, FTString, FTString,
    FTString, FTString, FTString,
    FTString, FTString, FTString, FTString,
    FTString, FTInteger, FTInteger, FTString,
    FTString, FTString, FTString, FTString};
static int fldSize[] = {10, 80, 2,
    FWID_RTE_P, FWID_RTE_NUM, 2,
    FWID_RTE_P, FWID_RTE_NUM, 2,
    FWID_RTE_P, FWID_RTE_NUM, 2,
    1, 1, 2, 1,
    /* the following are for the -x option */
    5, 5, 3, 1,
    10, 10, 10, 10};

#define gLINK_ID     0
#define gNAME        1
#define gRTYPE       2
#define gRTE1_P      3
#define gRTE1_N      4
#define gRTE1_T      5
#define gRTE2_P      6
#define gRTE2_N      7
#define gRTE2_T      8
#define gRTE3_P      9
#define gRTE3_N      10
#define gRTE3_T      11
#define gFCLASS      12
#define gDIRECTION   13
#define gSTYLE       14
#define gEXTRA       15
/* the following are for the -x option */
#define gSPD_LABEL   16
#define X_FLD_IDX    gSPD_LABEL
#define gFR_SPD_LIM  17
#define gTO_SPD_LIM  18
#define gSPEED_CAT   19
#define gL_REFADDR   20
#define gL_NREFADDR  21
#define gR_REFADDR   22
#define gR_NREFADDR  23

void Usage() {
    printf("Usage: doNtStreets [-u] [-x] style infile outfile\n");
    printf("       -u    - indicate the attribute data is UTF8\n");
    printf("       -x    - add segment speed and house number ranges\n");
    printf("       style - US|CA|EURO\n");
    exit(1);
}

int maxKphFromCat(const char *cat) {
    /*  1 - Greater than 131 kph / 80 mph
        2 - 101-130 kph / 65-80 mph
        3 - 91-100 kph / 55-64 mph
        4 - 71-90 kph / 41-54 mph
        5 - 51-70 kph / 31-40 mph
        6 - 31-50 kph / 21-30 mph
        7 - 11-30 kph / 6-20 mph
        8 - less than 11 kph / 6 mph
        ''- bot applicable
    */
    switch (cat[0]) {
        case '1': return 131; break;
        case '2': return 130; break;
        case '3': return 100; break;
        case '4': return  90; break;
        case '5': return  70; break;
        case '6': return  50; break;
        case '7': return  30; break;
        case '8': return  10; break;
    }
    return 0;
}


const char *spdLabel(int fslim, int tslim, const char *cat) {
    static char label[10];
    int speed ;

    if (fslim == 998) fslim = 0;
    if (tslim == 998) tslim = 0;
    speed = fslim>tslim?fslim:tslim;
    if (speed == 0)
        speed = maxKphFromCat(cat);
    label[0] = '\0';
    if (speed > 0)
        sprintf(label, "%d", speed);

    return (const char *) label;
}


char *mkAltName(const char *in) {
    char *out = NULL;
    if (strstr(in, "Streets.")) {
        out = (char *)malloc(strlen(in)+3);
        strcpy(out, in);
        strcpy(strstr(out, "Streets."), "AltStreets.dbf");
    }
    return out;
}


int getAltStreet(int *iA, int link_id, sortStruct* alt, int rcnt) {
    if (*iA < rcnt) {
        while (alt[*iA].number < link_id && *iA < rcnt) (*iA)++;
        if (alt[*iA].number == link_id && *iA < rcnt)
            return 1;
    }
    return 0;
}

void mergeRecords(int doextra, int utf8, int region, REC *A, REC *B) {
    int isRte, i, ok;
    const char *str;
    
    A->isTCH |= (!strcasecmp("TRANS CANADA HWY", B->name))?1:0;

    /* isExit is set based on EXITNAME column in Streets */
    if (B->isExit) {
        strcpy(A->rte_n[A->nrte], B->name);
        A->rte_t[A->nrte] = B->rtype + 10;
        A->nrte++;
    }
    /* rtype = 1-6 or 0 is it is not a route name */
    else if (B->rtype) {
        isRte = extractRouteNum(region, B->rte_n[0], B->rte_p[0], B->name);
        if (isRte && A->nrte < 3) {
            for (i=0, ok=1; i<A->nrte; i++)
                if (!strcmp(A->rte_n[i], B->rte_n[0])) {
                    ok = 0;
                    break;
                }
            if (ok) {
                A->rte_t[A->nrte] = B->rtype;
                strcpy(A->rte_n[A->nrte], B->rte_n[0]);
                strcpy(A->rte_p[A->nrte], B->rte_p[0]);
                /* FIXME: this should be 7 as 1-6 are already used */
                if (!strcmp(A->rte_p[A->nrte], "E"))
                    A->rte_t[A->nrte] = 7;
                    
                A->nrte++;
            }
        }
    }

    /* if this is a name, check if we need it  and set it */
    if (!A->gotName && !B->rtype && !B->isExit) {
        if (utf8)
            str = u_wordCase(B->name);
        else
            str = wordCase(B->name);
        strcpy(A->name, str);
        A->gotName = 1;
    }

    /* check the status of the rtype and set it if better */
    if (!A->rtype || B->rtype < A->rtype) A->rtype = B->rtype;
    if (B->fclass < A->fclass) A->fclass = B->fclass;

    /* merge speed limits, speed cat, and house numbers */
    if (doextra) {
        if (A->fspdlim == 0) A->fspdlim = B->fspdlim;
        if (A->tspdlim == 0) A->tspdlim = B->tspdlim;
        if (strlen(B->spdcat) && !strlen(A->spdcat))
            strcpy(A->spdcat, B->spdcat);
        if (strlen(B->lfaddr) && strlen(B->ltaddr) &&
            !strlen(A->lfaddr) && !strlen(A->ltaddr)) {
            strcpy(A->lfaddr, B->lfaddr);
            strcpy(A->ltaddr, B->ltaddr);
        }
        if (strlen(B->rfaddr) && strlen(B->rtaddr) &&
            !strlen(A->rfaddr) && !strlen(A->rtaddr)) {
            strcpy(A->rfaddr, B->rfaddr);
            strcpy(A->rtaddr, B->rtaddr);
        }
    }
}


int main(int argc, char *argv[]) {

    static char buf[1024];
    int iA = 0;

    SHPHandle inSHP, outSHP;
    DBFHandle inDBF, outDBF, inADBF;
    SHPObject *shape;
    REC A;
    REC B;
    static sortStruct* data;
    static sortStruct* dataA;
    int fldNums[21] =  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
                        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1};
    int aFldNums[11] = {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1};
    int shapeType;
    int nRecords;
    int nARecords;
    int i, j, k, m, mm, n;
    int link_id;
    int ierr = 0;
    int region = 0;
    int tollway;
    int utf8 = 0;
    int doextra = 0;
    char extra[2];
    char style[3];
    char shield[3];
    char *altName;

    /* quick check of args */
    if (argc > 1 && argv[1][0] == '-' && argv[1][1] == 'u') {
        utf8 = 1;
        argv++;
        argc--;
    }
    if (argc > 1 && argv[1][0] == '-' && argv[1][1] == 'x') {
        doextra = 1;
        argv++;
        argc--;
    }
    if (!doextra) {
        inFName[X_INFLD_IDX] = NULL;
        fldName[X_FLD_IDX] = NULL;
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
    INFO("Streets: num records = %d\n", nRecords);

    /* open the dbf dile */
    mkFileName(argv[2], ".dbf", buf);
    inDBF = DBFOpen(buf, "rb");
    if ( !inDBF ) {
        fprintf(stderr, "Unable to open dbf file (%s)\n", buf);
        exit(1);
    }

    altName = mkAltName(argv[2]);
    if (altName) {
        inADBF = DBFOpen(altName, "rb");
        if ( !inADBF ) {
            fprintf(stderr, "Unable to open dbf file (%s)\n", buf);
            /* exit(1); */
        }
        free(altName);
        nARecords = DBFGetRecordCount(inADBF);
        INFO("AltStreets: num records = %d\n", nARecords);
    }
    else {
        inADBF = NULL;
    }

    INFO("AltStreets: %s\n", inADBF?"FOUND":"NOT FOUND");

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

    /* only read the AltStreets if we opened it */
    if ( inADBF ) {
        for (i=0; inAName[i]; i++) {
            aFldNums[i] = DBFGetFieldIndex(inADBF, inAName[i]);
            if (aFldNums[i] < 0) {
                fprintf(stderr, "Field (%s) Missing from Input!\n", inAName[i]);
                ierr = 1;
            }
        }
        
        if ( ierr ) {
            fprintf(stderr, "Required DBF fields not found in Alt DBF!\n");
            exit(1);
        }
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

    /*
     * OUT_FLD    OUT IN AIN  IN_FIELD
     * ---------------------------------
     * LINK_ID     0   0   0   LINK_ID
     * NAME        1   2   1   ST_NAME
     * RTE_P       2
     * RTE_N       3
     * RTE_T       4   3   2   ROUTE_TYPE
     * DIRECTION   5   5       DIRECTION
     * FUNC_CLASS      3       FUNC_CLASS
     * EXTRA       6
     * STYLE       7
     * SHIELD      8
     * FR_SPD_LIM  9           FR_SPD_LIM  N5
     * TO_SPD_LIM 10           TO_SPD_LIM  N5
     * SPEED_CAT  11           SPEED_CAT   C1
     * L_REFADDR  12           L_REFADDR   C10
     * L_NREFADDR 13           L_NREFADDR  C10
     * R_REFADDR  14           R_REFADDR   C10
     * R_NREFADDR 15           R_NREFADDR  C10
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

    /* add the fields to the output dbf file */
    for (i=0; fldName[i]; i++)
        DBFAddField(outDBF, fldName[i], fldType[i], fldSize[i], 0);

    /* sort the Streets and the AltStreets based on LINK_ID */

    INFO("Start load and sort Streets\n", "");
    data = sortDBF(inDBF, inFName[0], cmp_num_asc);
    if (!data) {
        fprintf(stderr, "Failed to allocate and sort input DBF! Aborting.\n");
        exit(1);
    }
    INFO("Done load and sort Streets\n", "");

    INFO("Start load and sort AltStreets\n", "");
    if ( inADBF ) {
        dataA = sortDBF(inADBF, inAName[0], cmp_num_asc);
        if (!dataA) {
            fprintf(stderr, "Failed to allocate and sort input Alt DBF! Aborting.\n");
            exit(1);
        }
    }
    INFO("Done load and sort AltStreets\n", "");
    
    /* loop through the records, modify them if needed and write them out */
    for (i=0; i<nRecords; i++) {

        if (i % 100000 == 0) {
            INFO("Processed %7d records\n", i);
        }

        j = data[i].index;
        link_id = data[i].number;

        shape = SHPReadObject(inSHP, j);
        n = SHPWriteObject(outSHP, -1, shape);
        SHPDestroyObject(shape);

        memset(extra, 0, 2);

        DBFWIA(outDBF, n, gLINK_ID,   DBFRIA(inDBF, j, fLINK_ID));
        DBFWSA(outDBF, n, gFCLASS,    DBFRSA(inDBF, j, fFUNC_CLASS));
        DBFWSA(outDBF, n, gDIRECTION, DBFRSA(inDBF, j, fDIR_TRAVEL));
        tollway = (!strcasecmp(DBFRSA(inDBF, j, fTOLLWAY), "Y"))?1:0;
        
        if (!strcasecmp(DBFRSA(inDBF, j, fPAVED), "N"))      extra[0] = 'D';
        if (!strcasecmp(DBFRSA(inDBF, j, fFERRY_TYPE), "B")) extra[0] = 'F';
        if (!strcasecmp(DBFRSA(inDBF, j, fTUNNEL), "Y"))     extra[0] = 'T';
        if (!strcasecmp(DBFRSA(inDBF, j, fRAMP), "Y"))       extra[0] = 'R';
        if (!strcasecmp(DBFRSA(inDBF, j, fEXITNAME), "Y"))   extra[0] = 'E';
        DBFWSA(outDBF, n, gEXTRA, extra);

        memset(&A, 0, sizeof(A));
        A.link_id = link_id;
        A.rtype   = strtol(DBFRSA(inDBF, j, fROUTE_TYPE), NULL, 10);
        A.fclass  = strtol(DBFRSA(inDBF, j, fFUNC_CLASS), NULL, 10);
        A.explicatbl = (!strcasecmp(DBFRSA(inDBF, j, fEXPLICATBL), "Y"))?1:0;
        A.isExit = (!strcasecmp(DBFRSA(inDBF, j, fEXITNAME), "Y"))?1:0;
        strcpy(A.name, DBFRSA(inDBF, j, fST_NAME));
        if (doextra) {
            if (fFR_SPD_LIM > -1)
                A.fspdlim = strtol(DBFRSA(inDBF, j, fFR_SPD_LIM), NULL, 10);
            if (fTO_SPD_LIM > -1)
                A.tspdlim = strtol(DBFRSA(inDBF, j, fTO_SPD_LIM), NULL, 10);
            if (fSPEED_CAT > -1)
                strcpy(A.spdcat, DBFRSA(inDBF, j, fSPEED_CAT));
            if (fL_REFADDR > -1)
                strcpy(A.lfaddr, DBFRSA(inDBF, j, fL_REFADDR));
            if (fL_NREFADDR > -1)
                strcpy(A.ltaddr, DBFRSA(inDBF, j, fL_NREFADDR));
            if (fR_REFADDR > -1)
                strcpy(A.rfaddr, DBFRSA(inDBF, j, fR_REFADDR));
            if (fR_NREFADDR > -1)
                strcpy(A.rtaddr, DBFRSA(inDBF, j, fR_NREFADDR));
        }

        memcpy(&B, &A, sizeof(A));
        /* remove name if route or exit */
        if (A.rtype || A.isExit) A.name[0] = '\0';
        
        /* force the first record to be merged with itself
         * for the side effects
         */
        A.fclass = 100; 

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

        while (1) {
            assert( link_id == B.link_id);

            mergeRecords(doextra, utf8, region, &A, &B);
            
            /* Old format Navteq has alt names in Streets so look there */
            if (i+1<nRecords && link_id == data[i+1].number) {
                i++;
                j = data[i].index;
                B.link_id = data[i].number;
                B.rtype   = strtol(DBFRSA(inDBF, j, fROUTE_TYPE), NULL, 10);
                B.fclass  = strtol(DBFRSA(inDBF, j, fFUNC_CLASS), NULL, 10);
                B.explicatbl = (!strcasecmp(DBFRSA(inDBF, j, fEXPLICATBL), "Y"))?1:0;
                B.isExit = (!strcasecmp(DBFRSA(inDBF, j, fEXITNAME), "Y"))?1:0;
                strcpy(B.name, DBFRSA(inDBF, j, fST_NAME));
                if (doextra) {
                    if (fFR_SPD_LIM > -1)
                        B.fspdlim = strtol(DBFRSA(inDBF, j, fFR_SPD_LIM), NULL, 10);
                    if (fTO_SPD_LIM > -1)
                        B.tspdlim = strtol(DBFRSA(inDBF, j, fTO_SPD_LIM), NULL, 10);
                    if (fSPEED_CAT > -1)
                        strcpy(B.spdcat, DBFRSA(inDBF, j, fSPEED_CAT));
                    if (fL_REFADDR > -1)
                        strcpy(B.lfaddr, DBFRSA(inDBF, j, fL_REFADDR));
                    if (fL_NREFADDR > -1)
                        strcpy(B.ltaddr, DBFRSA(inDBF, j, fL_NREFADDR));
                    if (fR_REFADDR > -1)
                        strcpy(B.rfaddr, DBFRSA(inDBF, j, fR_REFADDR));
                    if (fR_NREFADDR > -1)
                        strcpy(B.rtaddr, DBFRSA(inDBF, j, fR_NREFADDR));
                }
            }
            /* new files formats use AltStreets so look there */
            else if (inADBF && (int) getAltStreet(&iA, link_id, dataA, nARecords)) {
                k = dataA[iA].index;
                B.link_id = dataA[iA].number;
                iA++;
                B.rtype   = strtol(DBFRSA(inADBF, k, aROUTE_TYPE), NULL, 10);
                B.fclass  = 100;
                B.explicatbl = (!strcasecmp(DBFRSA(inADBF, k, aEXPLICATBL), "Y"))?1:0;
                B.isExit = (!strcasecmp(DBFRSA(inADBF, k, aEXITNAME), "Y"))?1:0;
                strcpy(B.name, DBFRSA(inADBF, k, aST_NAME));
                if (doextra) {
                    if (aL_REFADDR > -1)
                        strcpy(B.lfaddr, DBFRSA(inADBF, k, aL_REFADDR));
                    if (aL_NREFADDR > -1)
                        strcpy(B.ltaddr, DBFRSA(inADBF, k, aL_NREFADDR));
                    if (aR_REFADDR > -1)
                        strcpy(B.rfaddr, DBFRSA(inADBF, k, aR_REFADDR));
                    if (aR_NREFADDR > -1)
                        strcpy(B.rtaddr, DBFRSA(inADBF, k, aR_NREFADDR));
                }
            }
            else {
                break;
            }

        } /* end while(1) */

        strcpy(shield, getShield(region, A.rte_t[0], A.fclass, A.rte_p[0], A.isTCH));
        DBFWSA(outDBF, n, gRTE1_T,  shield);
        DBFWSA(outDBF, n, gRTE1_N,  A.rte_n[0]);
        DBFWSA(outDBF, n, gRTE1_P,  A.rte_p[0]);
        strcpy(shield, getShield(region, A.rte_t[1], A.fclass, A.rte_p[1], A.isTCH));
        DBFWSA(outDBF, n, gRTE2_T,  shield);
        DBFWSA(outDBF, n, gRTE2_N,  A.rte_n[1]);
        DBFWSA(outDBF, n, gRTE2_P,  A.rte_p[1]);
        strcpy(shield, getShield(region, A.rte_t[2], A.fclass, A.rte_p[2], A.isTCH));
        DBFWSA(outDBF, n, gRTE3_T,  shield);
        DBFWSA(outDBF, n, gRTE3_N,  A.rte_n[2]);
        DBFWSA(outDBF, n, gRTE3_P,  A.rte_p[2]);
        DBFWSA(outDBF, n, gNAME,   A.name);
        sprintf(buf, "%d", A.fclass);
        DBFWSA(outDBF, n, gFCLASS, buf);

        mm = 0;
        for (m=0; m<A.nrte; m++)
            if (!A.rte_t[mm] || A.rte_t[m] < A.rte_t[mm]) mm = m;

        sprintf(buf, "%d", A.rte_t[mm]);
        DBFWSA(outDBF, n, gRTYPE, buf);

        strcpy(style, getStyle(region, A.fclass, tollway, extra, A.rte_p[mm]));
        DBFWSA(outDBF, n, gSTYLE, style);

        if (doextra) {
            DBFWSA(outDBF, n, gSPD_LABEL,  spdLabel(A.fspdlim, A.tspdlim, A.spdcat));
            DBFWIA(outDBF, n, gFR_SPD_LIM, A.fspdlim);
            DBFWIA(outDBF, n, gTO_SPD_LIM, A.tspdlim);
            DBFWSA(outDBF, n, gSPEED_CAT,  A.spdcat);
            DBFWSA(outDBF, n, gL_REFADDR,  A.lfaddr);
            DBFWSA(outDBF, n, gL_NREFADDR, A.ltaddr);
            DBFWSA(outDBF, n, gR_REFADDR,  A.rfaddr);
            DBFWSA(outDBF, n, gR_NREFADDR, A.rtaddr);
        }
    } /* end for i */

    INFO("Output %7d records\n", n);

    free(data);
    if (inADBF && dataA) free(dataA);

    SHPClose(outSHP);
    DBFClose(outDBF);
    SHPClose(inSHP);
    DBFClose(inDBF);
    if (inADBF) DBFClose(inADBF);

    return(0);
}
