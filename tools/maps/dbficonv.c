/*********************************************************************
*
*  $Id: dbficonv.c,v 1.3 2008/04/21 18:23:17 woodbri Exp $
*
*  dbfxlate
*  Copyright 2005, Stephen Woodbridge, All rights Reserved
*  Redistribition without written permission strictly prohibited
*
**********************************************************************
*
*  dbfxlate.c
*
*  $Log: dbficonv.c,v $
*  Revision 1.3  2008/04/21 18:23:17  woodbri
*  Updating cvs after 4/19/08 std run.
*  Also checking other random scripts not under control.
*
*  Revision 1.2  2008/01/29 22:39:24  woodbri
*   fixed bug in dbfstd.c
*
*  Revision 1.1  2008/01/28 04:33:26  woodbri
*  Adding new files for 20080125 freeze.
*
*  Revision 1.1  2008/01/20 20:40:19  woodbri
*  Initial check in of leaddog standardization tools.
*
*  Revision 1.1  2005/05/11 20:48:51  woodbri
*  Add new utility the scans a dbf file and changes the case of words in any
*  field ot fields. Allows UPPER, lower or Word Case.
*
*  2007-11-13 woodbri
*  Added additional functionality, to support global search and replace
*
*  2005-03-24 sew, created.
*
**********************************************************************
*
*  Utility to change the case of characters in dbf fields
*
**********************************************************************/

#include <getopt.h>
#include <assert.h>
#include <errno.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <getopt.h>
#include <shapefil.h>
#include <iconv.h>

#include "rgxtools.h"

#define UC(a) (((a) > '\140' && (a) < '\173')?((a)-'\40'):\
               ((a) > '\337' && (a) < '\376')?((a)-'\40'):(a))
#define LC(a) (((a) > '\100' && (a) < '\133')?((a)+'\40'):\
               ((a) > '\277' && (a) < '\336')?((a)+'\40'):(a))
#define MIN(a,b) ((a)<(b)?(a):(b))

#ifdef DMALLOC
#include "dmalloc.h"
#endif

// Debug print out
#define DBGOUT(s) (s)
//#define DBGOUT(s)

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

char  *optarg;
int   optind, opterr, optopt;

extern int DEBUG;
int DEBUG  = 0;

#define MAX_STRINGS 100
int numstrings = 0;
char *strin[MAX_STRINGS] = {NULL};
char *strout[MAX_STRINGS] = {NULL};
RXcache *rg[MAX_STRINGS] = {NULL};

void Usage()
{
  printf("Usage: dbficonv [-d|-h] [-f <enc>] [-t <enc>] <dbf file> [<field name(s)>]\n");
  printf("       -h  display this help message\n");
  printf("       -d  display field names and record count info\n");
  /*
  printf("       -u  force fields to uppercase\n");
  printf("       -l  force fields to lowercase\n");
  printf("       -w  force fields to word case\n");
  printf("       -i  ignore case while doing search and replace\n");
  printf("       -s  file with string replace list.\n");
  printf("           lines like: phrase_from<tab>phrase_to\n\n");
  printf("       -r  file with regex and replace strings.\n");
  printf("           lines like: regex_patt<tab>replace_patt\n\n");
  */
  printf("       -f <enc> | --ficonv=<enc> Encoding from. See iconv --list\n");
  printf("       -t <enc> | --ticonv=<enc> Encoding to.\n");
  printf("       WARNING: this command changes your dbf file,\n");
  printf("       WARNING: make sure you have backups!\n");
  exit(1);
}

/* assumes that fin is a .dbf file name */
char *readcpg(char *fin)
{
    FILE *CPG;
    char *f;
    char buf[1024];

    if (!fin) return NULL;

    f = strdup(fin);
    strcpy(f+strlen(f)-3, "cpg");
    CPG = fopen(f, "rb");
    free(f);
    if (!CPG) return NULL;
    f = fgets(buf, 1024, CPG);
    fclose(CPG);
    if (!f) return NULL;
    f = buf + strspn(buf, " \t\r\n");
    return strdup(f);
}


/* assumes that fout is a .dbf file name */
int writecpg(char *fout, char *enc)
{
    FILE *CPG;
    char *f;

    if (!fout || !enc) return -1;

    f = strdup(fout);
    strcpy(f+strlen(f)-3, "cpg");
    CPG = fopen(f, "wb");
    if (!CPG) {
        fprintf(stderr, "Failed to open (%s) for create\n", f);
        free(f);
        return -1;
    }
    free(f);
    fprintf(CPG, "%s", enc);
    fclose(CPG);
    return 0;
}


void loadStrings(const char *in) {
    static char buf[1024];
    FILE *inf;
    char *p;

    inf = fopen(in, "rb");
    if (inf == NULL) {
        fprintf(stderr, "Failed to open %s : %s\n", in, strerror(errno));
        exit(1);
    }

    while (!feof(inf) && numstrings<MAX_STRINGS) {
        if (fgets(buf, 1024, inf)) {
            if (buf[strlen(buf)-1] == '\n')
                buf[strlen(buf)-1] = '\0';
            if (p = strchr(buf, '\t')) {
                *p = '\0';
                strin[numstrings] = strdup(buf);
                strout[numstrings] = strdup(p+1);
                numstrings++;
            }
            else {
                fprintf(stderr, "SKIPPING(%d): %s\n", numstrings+1, buf);
            }
        }
        else if (!feof(inf)) {
            fprintf(stderr, "Error occured reading %s : %s\n",
                    in, strerror(errno));
            exit(1);
        }
    }
    if (numstrings == MAX_STRINGS && !feof(inf)) {
        fprintf(stderr, "More the %d strings were attempted to be loaded!\n", MAX_STRINGS);
        exit(1);
    }
    fclose(inf);
}


void loadRegex(const char *in){
    static char buf[1024];
    FILE *inf;
    char *p;

    inf = fopen(in, "rb");
    if (inf == NULL) {
        fprintf(stderr, "Failed to open %s : %s\n", in, strerror(errno));
        exit(1);
    }

    while (!feof(inf) && numstrings<MAX_STRINGS) {
        if (fgets(buf, 1024, inf)) {
            if (buf[strlen(buf)-1] == '\n')
                buf[strlen(buf)-1] = '\0';
            if (p = strchr(buf, '\t')) {
                *p = '\0';
                strin[numstrings] = strdup(buf);
                strout[numstrings] = strdup(p+1);
                rg[numstrings] = buildRXcache(buf, p+1, 1);
                numstrings++;
            }
            else {
                fprintf(stderr, "SKIPPING no tab (%d): %s\n", numstrings+1, buf);
            }
        }
        else if (!feof(inf)) {
            fprintf(stderr, "Error occured reading %s : %s\n",
                    in, strerror(errno));
            exit(1);
        }
    }
    if (numstrings == MAX_STRINGS && !feof(inf)) {
        fprintf(stderr, "More the %d regexs were attempted to be loaded!\n", MAX_STRINGS);
        exit(1);
    }
    fclose(inf);
}


void freeStrings() {
    int i;

    for(i=0; i<numstrings; i++) {
        free(strin[i]);
        free(strout[i]);
    }
}


const char *xlate(const char *in, int icase, int partial) {
    static char out[256];
    int i;

    if (partial) {
        fprintf(stderr, "Sorry, the partial option is not implemented yet!\n");
        exit(1);
    }
    else {
        for (i=0; i<numstrings && (toupper(in[0]) >= toupper(strin[i][0])); i++)
            if (!strcasecmp(in, strin[i]))
                return strout[i];
        return in;
    }
}


const char *chgCase(int iopt, const char *in) {
    static char out[256];
    int i;
        
    for(i=0; i<MIN(255,strlen(in)); i++)
        switch (iopt) {
            case 1: // upper
                out[i] = UC(in[i]);
                break;
            case 2: // lower
                out[i] = LC(in[i]);
                break;
            case 3: // word
                if (!i || strchr( " -/()0123456789", in[i-1]) )
                    out[i] = UC(in[i]);
                else
                    out[i] = LC(in[i]);
                break;
            default:  // no change
                out[i] = in[i];
                break;
        }
    out[i] = '\0';
    return out;
}


const char *striconv(iconv_t *cd, char *s) {
    static char out[256];
    long icnt;
    long ocnt = 256;
    char *u = out;
    int ibytes;

    memset(out, 0, 256);
    out[0] = '\0';
    icnt = strlen(s);
    ibytes = iconv(cd, &s, &icnt, &u, &ocnt);
    if (ibytes == -1) {
        iconv(cd, 0, 0, 0, 0);
        out[0] = '\0';
    }

    return (const char *)out;
}


const char *xlateRegex(const char *in, int iopt) {
    int i;
    char *s;
    char *r;
    const char *out;

    s = strdup(in);
    for(i=0; i<MAX_STRINGS && rg[i]; i++) {
        /* repeat the pattern as long as it matches or 100 times */
        int j = 0;
        while ((r = creplace(rg[i], s)) && j<100) {
            free(s);
            s = r;
            j++;
        }
    }

    out = chgCase(iopt, (const char *)s);
    free(s);
//    printf("%-35s\t%s\n", in, out);
    return out;
}


void print_fields(DBFHandle dIN) {
    int           i, fw, fd;
    char          fldname[12];
    DBFFieldType  fType;
    
    fprintf(stderr, "DBF Records = %d\nNum Fieldname    Typ Wid Dec\n",
             DBFGetRecordCount(dIN));
    for (i=0; i<DBFGetFieldCount(dIN); i++) {
        fType = DBFGetFieldInfo(dIN, i, fldname, &fw, &fd);
        fprintf(stderr, " %2d %-12s  %c  %3d %3d\n", i+1, fldname, 
                DBFGetNativeFieldType(dIN, i), fw, fd);
    }
}


int main( int argc, char ** argv )
{
    DBFHandle     dIN;
    DBFFieldType  fType;
    iconv_t      *cd = NULL;
    char         *ficonv = NULL;
    char         *ticonv = NULL;
    char         *ienc   = NULL;
    char          out[1024];
    char         *u = out;
    char         *fin;
    char          fldname[12];
    int           nEnt, nFld;
    int           i, j;
    int          *fldnums = NULL;
    int           fw, fd;
    int           err;
    char         *s;
    char         *opt;
    static struct option long_options[] = {
        {"help",     no_argument,       0, 'h'},
        {"display",  no_argument,       0, 'd'},
/*
        {"upper",    no_argument,       0, 'u'},
        {"lower",    no_argument,       0, 'l'},
        {"word",     no_argument,       0, 'w'},
        {"partial",  no_argument,       0, 'p'},
        {"icase",    no_argument,       0, 'i'},
        {"strings",  required_argument, 0, 's'},
        {"regex",    required_argument, 0, 'r'},
*/
        {"ficonv",   required_argument, 0, 'f'},
        {"ticonv",   required_argument, 0, 't'},
        {0, 0, 0, 0}
    };
    int c;
    int display = 0;
    int iopt = 0;
    int partial = 0;
    int icase = 0;
    int trans = 0;
    char *tin = NULL;

    while (1) {
        int option_index = 0;
        c = getopt_long( argc, argv, "hdf:t:",
                long_options, &option_index);

        /* Detect the end of optione. */
        if (c == -1)
            break;

        switch (c) {
            case 0:
                break;
            case 'h':
                Usage();
                break;
            case 'd':
                display = 1;
                break;
            case 'u':
                iopt = 1;
                break;
            case 'l':
                iopt = 2;
                break;
            case 'w':
                iopt = 3;
                break;
            case 'p':
                partial = 1;
                break;
            case 'i':
                icase = 1;
                break;
            case 'f':
                ficonv = optarg;
                break;
            case 't':
                ticonv = optarg;
                break;
            case 's':
                tin = optarg;
                loadStrings(tin);
                trans = 1;
                break;
            case 'r':
                tin = optarg;
                loadRegex(tin);
                trans = 2;
                break;
            case '?':
                break;
            default:
                abort();
        }
    }
    
    if (argc-optind < 1) Usage();
    fin  = argv[optind++];

    if (display) {
        if ((dIN  = DBFOpen(fin, "rb+"))) {
            print_fields(dIN);
            DBFClose(dIN);
        } else 
            fprintf(stderr, "Failed to open dbf %s for read/write\n", fin);
        if (tin) {
            printf("\nTranslate table from %s (%d)\n", tin, numstrings);
            for(i=0; i<numstrings; i++)
                printf("%s  =>  %s\n", strin[i], strout[i]);
        }
        exit(1);
    }
    

    ienc = readcpg(fin);

    if (!ticonv || !(ienc || ficonv)) {
        printf("You need to specify BOTH --ficonv AND --ticonv values\n"
               "See 'iconv --list' for list of valid options\n");
        exit(1);
    }
    else if ((ficonv || ienc) && ticonv) {
        if (ienc)
            cd = iconv_open((const char *)ienc,   (const char *)ficonv);
        else
            cd = iconv_open((const char *)ticonv, (const char *)ficonv);
        if (cd == (iconv_t)(-1)) {
            printf("Failed to open iconv_open(%s, %s), see 'iconv --list' for list of options\n", ticonv, ficonv);
            printf("%s\n", strerror(errno));
            exit(1);
        }

        if (err = writecpg(fin, ticonv)) {
            printf("Failed to create .cpg file for %s (%d)\n", fin, err);
            exit(1);
        }
    }

    if (optind < argc) {
        fldnums = malloc((argc-optind) * sizeof(int));
        if (!fldnums) {
            fprintf(stderr, "Failed to allocate %d ints.\n", argc);
            exit(1);
        }
        // initialize them to undefined
        for (i=0; optind+i<argc; i++) fldnums[i] = -1;
    }
  
    if (!(dIN  = DBFOpen(fin, "rb+"))) {
        fprintf(stderr, "Failed to open dbf %s for read/write\n", fin);
        exit(1);
    }

    if (fldnums) {
        for (i=0; i<DBFGetFieldCount(dIN); i++) {
            fType = DBFGetFieldInfo(dIN, i, fldname, &fw, &fd);
            for (j=0; optind+j<argc; j++)
                if (!strcmp(argv[optind+j], fldname))
                    fldnums[j] = i;
        }
        // make sure we found them all
        err = 0;
        for (i=0; optind+i<argc; i++)
            if (fldnums[i] == -1) {
                err = 1;
                fprintf(stderr, "ERROR: Field (%s) not found\n", argv[optind+i]);
            }
        if (err) {
            print_fields(dIN);
            exit(1);
        }
    }

    nEnt = DBFGetRecordCount(dIN);
    nFld = DBFGetFieldCount(dIN);
    
    for (i=0; i<nEnt; i++) {
        if (fldnums) {
            for (j=0; optind+j<argc; j++) {
                if (cd)
                    s = (char *) striconv(cd, (char *)DBFReadStringAttribute(dIN, i, fldnums[j]));
                else
                    s = (char *) DBFReadStringAttribute(dIN, i, fldnums[j]);

                if (!strlen(s))
                    DBFWriteStringAttribute(dIN, i, fldnums[j], s);
                else if (trans == 1)
                    DBFWriteStringAttribute(dIN, i, fldnums[j],
                        chgCase(iopt, xlate(s, icase, partial)));
                else if (trans == 2)
                    DBFWriteStringAttribute(dIN, i, fldnums[j],
                        xlateRegex(s, iopt));
                else
                    DBFWriteStringAttribute(dIN, i, fldnums[j],
                        chgCase(iopt, s));
            }
        } else {
            for (j=0; j<nFld; j++) {
                if (cd)
                    s = (char *) striconv(cd, (char *)DBFReadStringAttribute(dIN, i, j));
                else
                    s = (char *) DBFReadStringAttribute(dIN, i, fldnums[j]);
                
                if (!strlen(s))
                    DBFWriteStringAttribute(dIN, i, j, s);
                else if (trans == 1)
                    DBFWriteStringAttribute(dIN, i, j,
                        chgCase(iopt, xlate(s, icase, partial)));
                else if (trans == 2)
                    DBFWriteStringAttribute(dIN, i, j,
                        xlateRegex(s, iopt));
                else
                    DBFWriteStringAttribute(dIN, i, j,
                        chgCase(iopt, s));
            }

        }
    }

    DBFClose(dIN);
    freeStrings();
    for(i=0; i<MAX_STRINGS && rg[i]; i++)
        freeRXcache(rg[i]);
}
