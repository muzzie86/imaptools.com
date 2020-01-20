/*********************************************************************
*
*  scan-nums
*  Copyright 2013, Stephen Woodbridge, All rights Reserved
*  Redistribition without written permission strictly prohibited
*
**********************************************************************
*/
#include <assert.h>
#include <errno.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <shapefil.h>

#define MAX(a,b) ((a)>(b)?(a):(b))

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


void Usage() {
    printf("Usage: scan-numbers in_base out_base\n");
    exit(1);
}

int main ( int argc, char **argv ) {
    DBFHandle     dIN, dOUT;
    SHPHandle     sIN, sOUT;
    SHPObject    *s;
    DBFFieldType  fType;
    char          fldname[12];
    int           nEnt, sType, fw, fd;
    int           i, j, rec, n;
    int           fnum, tnum;
    char         *fin;
    char         *fout;
    char         *cfnum, *ctnum;
    char          nfmt[17];
    char          sfmt[17];
    char         *cfmt;
    double        sMin[4];
    double        sMax[4];

    char siflds[4][15] = { "L_F_ADD", "L_T_ADD", "R_F_ADD", "R_T_ADD" };
    int  iflds[4] = { -1, -1, -1, -1 };
    char soflds[10][15] = { "L_REFADDR", "L_NREFADDR", "L_SQLNUMF",
                            "L_SQLFMT", "L_CFORMAT", "R_REFADDR",
                            "R_NREFADDR", "R_SQLNUMF", "R_SQLFMT",
                            "R_CFORMAT" };
    int  ofldss[10] = { 11,11,12,16,16,11,11,12,16,16 };
    DBFFieldType oftyp[10] = { FTInteger, FTInteger, FTString, FTString, FTString, FTInteger, FTInteger, FTString, FTString, FTString };

    if (argc != 3) Usage();

    fin  = argv[1];
    fout = argv[2];

    if (!(dIN  = DBFOpen(fin, "rb"))) {
        printf("Failed to open dbf %s for read\n", fin);
        exit(1);
    }
    for (i=0; i<4; i++) {
        iflds[i] = DBFGetFieldIndex(dIN, siflds[i]);
        if (iflds[i] < 0) {
            printf("ERROR: DBF field '%s' not found!\n", siflds[i]);
            exit(1);
        }
    }

    if (!(sIN  = SHPOpen(fin, "rb"))) {
        printf("Failed to open shp %s for read\n", fin);
        exit(1);
    }

    SHPGetInfo(sIN, &nEnt, &sType, sMin, sMax);

    if (!(dOUT = DBFCreate(fout))) {
        printf("Failed to create dbf %s\n", fout);
        exit(1);
    }

    if (!(sOUT = SHPCreate(fout, sType))) {
        printf("Failed to create shp %s\n", fout);
        exit(1);
    }

    for (i=0; i<DBFGetFieldCount(dIN); i++) {
        fType = DBFGetFieldInfo(dIN, i, fldname, &fw, &fd);
        DBFAddField(dOUT, fldname, fType, fw, fd);
    }
    for (i=0; i<10; i++) {
        DBFAddField(dOUT, soflds[i], oftyp[i], ofldss[i], 0);
    }

    for (i=0; i<nEnt; i++) {
        rec = i;
        s = SHPReadObject(sIN, rec);
        n = SHPWriteObject(sOUT, -1, s);
        SHPDestroyObject(s);

        for (j=0; j<DBFGetFieldCount(dIN); j++) {
            fType = DBFGetFieldInfo(dIN, j, NULL, NULL, NULL);
            switch (fType) {
                case FTString:
                    DBFWriteStringAttribute( dOUT, n, j, DBFReadStringAttribute( dIN, rec, j));
                    break;
                case FTInteger:
                    DBFWriteIntegerAttribute(dOUT, n, j, DBFReadIntegerAttribute(dIN, rec, j));
                    break;
                case FTDouble:
                    DBFWriteDoubleAttribute( dOUT, n, j, DBFReadDoubleAttribute( dIN, rec, j));
                    break;
            }
        }

        cfnum = strdup(DBFReadStringAttribute( dIN, rec, iflds[0]));
        ctnum = strdup(DBFReadStringAttribute( dIN, rec, iflds[1]));
        cfmt = scan_nt_nums(cfnum, ctnum, &fnum, &tnum);
        mkSqlFmt(cfmt, sfmt, nfmt);
        DBFWriteIntegerAttribute(dOUT, n, j++, fnum);
        DBFWriteIntegerAttribute(dOUT, n, j++, tnum);
        DBFWriteStringAttribute(dOUT,  n, j++, nfmt);
        DBFWriteStringAttribute(dOUT,  n, j++, sfmt);
        DBFWriteStringAttribute(dOUT,  n, j++, cfmt);
        free(cfnum);
        free(ctnum);

        cfnum = strdup(DBFReadStringAttribute( dIN, rec, iflds[2]));
        ctnum = strdup(DBFReadStringAttribute( dIN, rec, iflds[3]));
        cfmt = scan_nt_nums(cfnum, ctnum, &fnum, &tnum);
        mkSqlFmt(cfmt, sfmt, nfmt);
        DBFWriteIntegerAttribute(dOUT, n, j++, fnum);
        DBFWriteIntegerAttribute(dOUT, n, j++, tnum);
        DBFWriteStringAttribute(dOUT,  n, j++, nfmt);
        DBFWriteStringAttribute(dOUT,  n, j++, sfmt);
        DBFWriteStringAttribute(dOUT,  n, j++, cfmt);
        free(cfnum);
        free(ctnum);
    }

    DBFClose(dIN);
    DBFClose(dOUT);
    SHPClose(sIN);
    SHPClose(sOUT);
}


