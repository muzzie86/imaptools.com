/*
 * $Id: doNtUtils.c,v 1.5 2011/02/21 21:01:23 woodbri Exp $
 *
 * doNtUtils.h
 *   - Utility to sort DBF file based on an integer attribute column
 *   - Utilities to parse streetnames and extract route numbers
 *   - Utilities to assign style and hwy shield numbers
 *
 * Author: Stephen Woodbridge
 * Date:   2009-02-28
 *
 * $Log: doNtUtils.c,v $
 * Revision 1.5  2011/02/21 21:01:23  woodbri
 * Changed the way we detect highway labels in both the streets and highway
 * layers for EURO styling.
 *
 * Revision 1.4  2011/02/18 04:00:59  woodbri
 * Changed the way we deal with names and route numbers.
 * NAME:RTE1:RTE2:RTE3 now allows use to do multiple labels.
 *
 * Revision 1.3  2011/02/17 03:14:44  woodbri
 * Redid code to extract name and hwy numbers.
 *
 * Revision 1.2  2011/01/29 16:26:52  woodbri
 * Adding files to support PAGC data generation for Tiger and Navteq.
 * Adding options to domaps to support carring, speed, and address ranges.
 *
 * Revision 1.1  2009/10/18 00:53:12  woodbri
 * Adding domaps to cvs.
 *
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <string.h>

#ifdef UNICODE
#define U_CHARSET_IS_UTF8 1
#include "unicode/utypes.h"
#include "unicode/uchar.h"
#include "unicode/ustring.h"
#include "unicode/ucasemap.h"
#define LENGTHOF(array) (int32_t)strlen(array)
#endif


#include "shapefil.h"
#include "doNtUtils.h"
#include "rgxtools.h"

char *st = "AK,AL,AR,AZ,CA,CO,CT,DC,DE,FL,GA,HI,IA,ID,IL,IN,KS,KY,"
           "LA,MA,MD,ME,MI,MN,MO,MS,MT,NC,ND,NE,NH,NJ,NM,NV,NY,"
           "OH,OK,OR,PA,PR,RI,SC,SD,TN,TX,UT,VA,VT,WA,WI,WV,WY,"
           "E,SR,RT,M,FM,RM,RR";

void mkFileName(char *base, char *ext, char *buf) {
    int i;
    
    strcpy(buf, base);
    for (i=strlen(buf)-1; i>=0; i--)
        if (buf[i] == '.') {
            buf[i] = '\0';
            break;
        }
        else if (buf[i] == '/') {
            break;
        }
    strcat(buf, ext);
}

const char* wordCase(const char *in) {
    static char out[256];
    int i;

    for (i=0; i<MIN(255,strlen(in)); i++)
        if (!i || strchr(" -/()0123456789", in[i-1]))
            out[i] = UC(in[i]);
        else
            out[i] = LC(in[i]);
    out[i] = '\0';
    return out;
}

#ifdef UNICODE
const char* u_wordCase(const char *in) {
    static char out[256];
    UErrorCode errorCode;
    UCaseMap *csm;
    int32_t length;

    errorCode = U_ZERO_ERROR;
    csm = ucasemap_open("", 0, &errorCode);
    if (U_FAILURE(errorCode)) {
        printf("ucasemap_open(\"\") failed - %s\n", u_errorName(errorCode));
        exit(1);
    }

    length = ucasemap_utf8ToTitle(csm, out, (int32_t)sizeof(out), in, LENGTHOF(in), &errorCode);
    if (U_FAILURE(errorCode) || length != LENGTHOF(in) || out[length] != 0) {
        printf("ucasemap_utf8ToTitle() failed - length=%ld - %s\n",
            u_errorName(errorCode));
        exit(1);
    }

    return out;
}
#else
const char* u_wordCase(const char *in) {
    return in;
}
#endif

int cmp_num_desc(const void *i, const void *j) {
    int a = ((sortStruct *)i)->number;
    int b = ((sortStruct *)j)->number;
    if(a > b) return(-1);
    if(a < b) return(1);
    return(0);
}


int cmp_num_asc(const void *i, const void *j) {
    int a = ((sortStruct *)i)->number;
    int b = ((sortStruct *)j)->number;
    if(a > b) return(1);
    if(a < b) return(-1);
    return(0);
}


sortStruct* sortDBF(DBFHandle dbfH, char *fldname, int (*compar)(const void *e1, const void *e2)) {
    sortStruct   *d;
    DBFFieldType dbfField;
    int          fType;
    int          fieldNumber;
    int          i,j;
    int          num_records;
    
    num_records = DBFGetRecordCount(dbfH);
    fieldNumber = DBFGetFieldIndex(dbfH, fldname);

    if (fieldNumber < 0) {
        fprintf(stderr,"Item %s doesn't exist!\n", fldname);
        return NULL;
    }

    /* ASSUMPTION: that LINK_ID is numeric */
    fType = DBFGetFieldInfo(dbfH, fieldNumber, NULL, NULL, NULL);
    if (fType != FTInteger && fType != FTDouble) {
        fprintf(stderr,"Item %s is not an numeric field!\n", fldname);
        return NULL;
    }

    d = (sortStruct *) malloc(sizeof(sortStruct) * num_records);
    if (!d) {
        fprintf(stderr, "Unable to allocate sort array.\n");
        return NULL;
    }

    for (i=0; i<num_records; i++) {
        d[i].number = DBFReadDoubleAttribute(dbfH, i, fieldNumber);
        d[i].index  = i;
    }

    qsort(d, num_records, sizeof(sortStruct), compar);

    return d;
}

/*
 * ROUTE_TYPE
 * =====================
 * (space) Not Applicable
 * 1 U.S. Interstate or European Level 1 Road
 * 2 U.S. Federal or European Level 2 Road
 * 3 U.S. State or European Level 3 Road
 * 4 U.S. County or European Level 4 Road
 * 5 European Level 5 Road
 * 6 European Level 6 Road
 *
 * STATE ROUTE TYPES
 * ==================
 * State State Route
 * Alabama AL-
 * Alaska AK-
 * Arizona AZ-
 * Arkansas AR-
 * California CA-
 * Colorado CO-/E-
 * Connecticut CT-
 * Delaware SR-
 * Florida SR-
 * Georgia GA-
 * Hawaii HI-
 * Idaho ID-
 * Illinois IL-
 * Indiana IN-
 * Iowa IA-
 * Kansas KS-
 * Kentucky KY-
 * Louisiana LA-
 * Maine ME-
 * Maryland MD-
 * Massachusetts RT-
 * Michigan M-
 * Minnesota MN-
 * Mississippi MS-
 * Missouri MO-
 * Montana MT-
 * Nebraska NE-
 * Nevada NV-
 * New Hampshire RT-
 * New Jersey RT-
 * New Mexico NM-
 * New York RT-
 * North Carolina NC-
 * North Dakota ND-
 * Ohio OH-
 * Oklahoma OK-
 * Oregon OR-
 * Pennsylvania PA-
 * Rhode Island RI-
 * South Carolina SC-
 * South Dakota SD-
 * Tennessee TN-
 * Texas TX-, FM-, RM-, RR-
 * Utah UT-
 * Vermont VT-
 * Virginia VA-
 * Washington WA-
 * West Virginia WV-
 * Wisconsin WI-
 * Wyoming WY-
 *
 */

int extractRouteNum(int region, char *rte_n, char *rte_p, const char *name) {
    int   i, j, len;
    char *p = NULL;

    rte_n[0] = '\0';
    rte_p[0] = '\0';

    switch (region) {
        case 0: /* US */
        case 1: /* CA */
            /*
             * First test for weird names we want to ignore
             * Just add tests here and return 0
             */
            if ( strstr(name, "I-") && strstr(name, "-BL") ) return 0;
            
            if ( (p = strchr(name, '-')) && p-name < 4  ) {
                strncpy(rte_p, name, MIN(2,p-name));
                rte_p[MIN(2,p-name)] = '\0';
                p++;
                strncpy(rte_n, p, FWID_RTE_NUM);
                rte_n[FWID_RTE_NUM] = '\0';
                if( (p = strchr(rte_n, ' ')) ) *p = '\0';
                if( (p = strchr(rte_n, '-')) ) *p = '\0';
            }
            else
                return 0;
            break;
        case 2: /* euro */
            len = strlen(name);
            if (len || len <= FWID_RTE_NUM) {
                strcpy(rte_n, name);
                for (i=0; i<=FWID_RTE_P; i++) {
                    if (isalpha(name[i])) rte_p[i] = name[i];
                    else break;
                }
                rte_p[i] = '\0';
            }
            else
                return 0;
            break;
        default:
            return 0;
    }
    return 1;
}


char *getStyle(int region, int fc, int tolls, char *extra, char *pre) {
    static char class[3];

    class[0] = '\0';

    switch (region) {
        case 0: /* US */
        case 1: /* CA */
            /* The NA styles are 1-6, 11-16 */
            if      (fc > 4)                   ;
            else if (!strlen(pre))             ;
            else if (!strcmp(pre,  "I")) fc = 1;
            else if (!strcmp(pre, "US")) fc = 2;
            else if (strstr(st, pre))    fc = 3;
            else if (!strcmp(pre, "CR")) fc = 4;
            break;
        case 2: /* euro */
            /* The euro styles are 21-26, 31-36 */
            if (fc > 0 && fc <= 4) fc += 20;
            break;
    }

    if (tolls) fc = 9;
    if (*extra == 'R') fc = 99;
    else if (*extra != '\0') fc += 10;
    
    sprintf(class, "%d", fc%100);
    return class;
}


char *getShield(int region, int rt, int fc, char *pre, int tc) {
    static char class[3];
    int xit;

    class[0] = '\0';
    xit = rt/10;

    switch (region) {
        /*
         *  0 unknown
         *  1 interstate
         *  2 ushwy
         *  3 state hwy
         *  4 county hwy
         * 10 unknow exit
         * 11 interstate exit
         * 12 ushwy exit
         * 13 state hwy exit
         * 14 county hwy exit
        */
        case 0: /* US */
            if      (fc > 4)                   ;
            else if (!strlen(pre))             ;
            else if (!strcmp(pre,  "I")) fc = 1;
            else if (!strcmp(pre, "US")) fc = 2;
            else if (strstr(st, pre))    fc = 3;
            else if (!strcmp(pre, "CR")) fc = 4;
            if (xit) fc += 10;
            break;
        /*
         *  0 unknown
         *  5 trans canadian hwy (TCH)
         *  6 AU
         *  7 has prefix
         * 10 unknown exit
         * 15 TCH exit
         * 16 AU exit
         * 17 has prefix exit
        */
        case 1: /* CA */
            if (tc)                         fc = 5;
            else if (!strcmp(pre, "AU"))    fc = 6;
            else if (strlen(pre))           fc = 7;
            else                            fc = 0;
            if (xit) fc += 10;
            break;
        /*
         *  0 unknown
         * 21 rtype 1
         * 22 rtype 2
         * 23 rtype 3
         * 25 E-
         * 30 unknown exit
         * 31 rtpye 1 exit
         * 32 rtype 2 exit
         * 33 rtype 3 exit
         * 35 E- exit
        */
        case 2: /* euro */
            if (rt) fc = rt + 20;
            else fc = 0;
            break;
    }

    sprintf(class, "%d", fc%100);
    return class;
}
