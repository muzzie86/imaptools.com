/*
 * $Id: doTgrRoads.c,v 1.2 2012-06-03 13:39:33 woodbri Exp $
 * doTgrRoads infile outfile
 *
 * This utility program process Tiger 2011 shapefiles to prepare
 * it for iMaptools.com Mapping. It handles the following layers:
 *
 * PRIMARYROADS, PRISECROADS, ROADS
 *
 * This utilitylooks for I,U,S,C class highways and extracts
 * the route number into a separate column for shields.
 *
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pcre.h>
#include <assert.h>

#include "shapefil.h"

static char *cpat = "^(?:(E|N|NE|NW|S|SE|SW|W) )?Co (?:(Aid|Trunk) )?\\K([^,\\(]{1,10})$";
static char *ipat = "^(?:(E|N|NE|NW|S|SE|SW|W) )?I- -?\\K(\\d+)\\s?\\w?$";
static char *spat = "^(?:(E|N|NE|NW|S|SE|SW|W) )?(?:State|DC) (:?Hwy|Rte) \\K(\\w+([- ][^NSEW])?)$";
static char *upat = "^(?:(E|N|NE|NW|S|SE|SW|W) )?US Hwy (?:(Hghwy|Hghy|Hgihway|Highway|Higfhway|Hghwy Rt|Hiway|Hw|Hwt|Hwy|Route|Rt|US Hwy) )?\\K(\\d+\\w?)$";

static char *inFName[] = {"STATEFP", "COUNTYFP", "LINEARID",
                          "FULLNAME", "RTTYP", "MTFCC", NULL};

void Usage() {
    printf("Usage: doTgrRoads [-u] infile outfile\n");
    printf("       -u  - attribute data is UTF8\n");
    exit(1);
}

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

#define OVECCOUNT 12

int main(int argc, char *argv[]) {

    static char buf[1024];

    SHPHandle inSHP, outSHP = NULL;
    DBFHandle inDBF, outDBF = NULL;
    SHPObject *shape;

    int fldNums[6] = {-1, -1, -1, -1, -1, -1};
    DBFFieldType fldType[6];
    int fldSize[6];
    int rteFNum;

    char *rtenum;
    char *name;
    char *rttyp;

    pcre *cre = NULL;
    pcre *ire = NULL;
    pcre *sre = NULL;
    pcre *ure = NULL;

    int erroff;
    int ovec[OVECCOUNT];
    int rc;
    const char *errstr;

    int nRecords;
    int shapeType;
    int numFields;
    int i, j, k, n;
    int utf8 = 0; /* set flag to indicate data is NOT UTF8 */
    int ierr;
    int ret = 1;

    /* quick check of args */
    if (argc > 1 && argv[1][0] == '-' && argv[1][1] == 'u') {
        utf8 = 1;
        argv++;
        argc--;
    }
    if (argc != 3) Usage();

    /* open shapefile */
    mkFileName(argv[1], ".shp", buf);
    inSHP = SHPOpen(buf, "rb");
    if ( !inSHP ) {
        fprintf(stderr, "Unable to open shp file (%s)\n", buf);
        exit(1);
    }
    SHPGetInfo(inSHP, &nRecords, &shapeType, NULL, NULL);

    /* open dbf file */
    mkFileName(argv[1], ".dbf", buf);
    inDBF = DBFOpen(buf, "rb");
    if ( !inDBF ) {
        fprintf(stderr, "Unable to open dbf file (%s)\n", buf);
        exit(1);
    }

    for (i=0, ierr=0; inFName[i]; i++) {
        fldNums[i] = DBFGetFieldIndex(inDBF, inFName[i]);
        if ( i>1 && fldNums[i] < 0) {
            fprintf(stderr, "Feild (%s) missing from DBF file.\n", inFName[i]);
            ierr = 1;
        }
        else {
            fldType[i] = DBFGetFieldInfo(inDBF, fldNums[i], NULL, &(fldSize[i]), NULL);
        }
    }

    if ( ierr ) {
        fprintf(stderr, "Required DBF fields not found!\n");
        exit(1);
    }

    /* Create output files */

    mkFileName(argv[2], ".shp", buf);
    outSHP = SHPCreate(buf, shapeType);
    if ( !outSHP ) {
        fprintf(stderr, "Unable to create shp file (%s)\n", buf);
        exit(1);
    }

    mkFileName(argv[2], ".dbf", buf);
    outDBF = DBFCreate(buf);
    if ( !outDBF ) {
        fprintf(stderr, "Unable to create dbf file (%s)\n", buf);
        exit(1);
    }

    /* add the fields to the output dbf file */
    for (i=0; i<6; i++) {
        if (fldNums[i] < 0) continue;
        DBFAddField(outDBF, inFName[i], fldType[i], fldSize[i], 0);
    }
    rteFNum = DBFAddField(outDBF, "RTENUM", FTString, 10, 0);

    /*
       Loop the th records and copy the shapefiles
       and modify the DBF as needed
    */

    /* compile our regexs */
    cre = pcre_compile(cpat, 0, &errstr, &erroff, NULL);
    if (cre == NULL) {
        fprintf(stderr, "PCRE compilation failed for cpat at offset %d: %s\n",
            erroff, errstr);
        goto ERRORS;
    }

    ire = pcre_compile(ipat, 0, &errstr, &erroff, NULL);
    if (ire == NULL) {
        fprintf(stderr, "PCRE compilation failed for ipat at offset %d: %s\n",
            erroff, errstr);
        goto ERRORS;
    }

    sre = pcre_compile(spat, 0, &errstr, &erroff, NULL);
    if (cre == NULL) {
        fprintf(stderr, "PCRE compilation failed for spat at offset %d: %s\n",
            erroff, errstr);
        goto ERRORS;
    }

    ure = pcre_compile(upat, 0, &errstr, &erroff, NULL);
    if (cre == NULL) {
        fprintf(stderr, "PCRE compilation failed for upat at offset %d: %s\n",
            erroff, errstr);
        goto ERRORS;
    }

    for (j=0; j<nRecords; j++) {

        shape = SHPReadObject(inSHP, j);
        n = SHPWriteObject(outSHP, -1, shape);
        SHPDestroyObject(shape);

        for (i=0; i<6; i++) {
            if (fldNums[i] < 0) continue;
            DBFWriteStringAttribute(outDBF, j, fldNums[i],
                DBFReadStringAttribute(inDBF, j, fldNums[i]));
        }

        rttyp = strdup(DBFReadStringAttribute(inDBF, j, fldNums[4]));
        name  = strdup(DBFReadStringAttribute(inDBF, j, fldNums[3]));

        switch (rttyp[0]) {
            case 'I':
                rc = pcre_exec(ire, NULL, name, strlen(name), 0, 0, ovec, OVECCOUNT);
                break;
            case 'S':
                rc = pcre_exec(sre, NULL, name, strlen(name), 0, 0, ovec, OVECCOUNT);
                break;
            case 'U':
                rc = pcre_exec(ure, NULL, name, strlen(name), 0, 0, ovec, OVECCOUNT);
                break;
            case 'C':
                rc = pcre_exec(cre, NULL, name, strlen(name), 0, 0, ovec, OVECCOUNT);
                break;
            default:
                rc = 0;
                break;
        }

        buf[0] = '\0';;
        if (rc > 0) {
            buf[0] = '\0';
            strncpy(buf, name + ovec[0], ovec[1]-ovec[0]);
            buf[ovec[1]-ovec[0]] = '\0';
        }

        DBFWriteStringAttribute(outDBF, j, rteFNum, buf);

        free(name);
        free(rttyp);
    }

    ret = 0;

ERRORS:

    if (cre) pcre_free(cre);
    if (ire) pcre_free(ire);
    if (sre) pcre_free(sre);
    if (ure) pcre_free(ure);

    if (outSHP) SHPClose(outSHP);
    if (outDBF) DBFClose(outDBF);
    if (inSHP) SHPClose(inSHP);
    if (inDBF) DBFClose(inDBF);

    return(ret);
}
