/*********************************************************************
*
*  $Id: dbfxlate.c,v 1.3 2010/07/25 00:43:22 woodbri Exp $
*
*  dbfxlate
*  Copyright 2005, Stephen Woodbridge, All rights Reserved
*  Redistribition without written permission strictly prohibited
*
**********************************************************************
*
*  dbfxlate.c
*
*  $Log: dbfxlate.c,v $
*  Revision 1.3  2010/07/25 00:43:22  woodbri
*  Increased MAX_STRINGS from 5000 to 10000.
*
*  Revision 1.2  2008/04/21 18:23:17  woodbri
*  Updating cvs after 4/19/08 std run.
*  Also checking other random scripts not under control.
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

#define MAX_STRINGS 10000
int numstrings = 0;
char *strin[MAX_STRINGS] = {NULL};
char *strout[MAX_STRINGS] = {NULL};
RXcache *rg[MAX_STRINGS] = {NULL};

void Usage()
{
  printf("Usage: dbfxlate [-d|-h] [-u|-l|-w] [-p] [-i] [-s field] [-t translate.txt] [-r regex.txt] <dbf file> [field names]\n");
  printf("       -h  display this help message\n");
  printf("       -d  display field names and record count info\n");
  printf("       -u  force fields to uppercase\n");
  printf("       -l  force fields to lowercase\n");
  printf("       -w  force fields to word case\n");
  printf("       -p  partial strings replaced, otherwise only whole fields(NOT IMPLEMENTED)\n");
  printf("       -i  ignore case while doing search and replace\n");
  printf("       -s field  Optional field that is used for replacement strings\n");
  printf("          If missing then replacements are done in the original field\n");
  printf("       -t  file with translate list.\n");
  printf("           lines like: phrase_from<tab>phrase_to\n\n");
  printf("       -r  file with regex and replace strings.\n");
  printf("           lines like: regex_patt<tab>replace_patt\n\n");
  printf("       WARNING: this command changes your dbf file,\n");
  printf("       WARNING: make sure you have backups!\n");
  exit(1);
}

void loadStrings(const char *in) {
    static char buf[1024];
    FILE *inf;
    char *p, *q;

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
                q =p+1;
                /* we are only interested in the first 2 columns */
                if (p = strchr(q, '\t'))
                    *p = '\0';
                strin[numstrings] = strdup(buf);
                strout[numstrings] = strdup(q);
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
    char *p, *q;

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
                q =p+1;
                /* we are only interested in the first 2 columns */
                if (p = strchr(q, '\t'))
                    *p = '\0';
                strin[numstrings] = strdup(buf);
                strout[numstrings] = strdup(q);
                rg[numstrings] = buildRXcache(buf, q, 1);
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

const char *strMatch(const char *in, int icase) {
    int i;

    for(i=0; i<MAX_STRINGS && strin[i]; i++)
        if (icase && !strcasecmp(strin[i], in))
            return strout[i];
        else if (!icase && !strcmp(strin[i], in))
            return strout[i];
    return NULL;
}

const char *regexMatch(const char *in) {
    int i;

    for(i=0; i<MAX_STRINGS && rg[i]; i++)
        if (cmatch(rg[i], (char *)in))
            return strout[i];
    return NULL;
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
    char         *fin;
    char          fldname[12];
    int           nEnt, nFld, nSub;
    int           i, j;
    int          *fldnums = NULL;
    int          fw, fd;
    int          err;
    int          cnt = 0;
    char         *s;
    const char   *ss;
    char         *opt;
    static struct option long_options[] = {
        {"help",     no_argument,       0, 'h'},
        {"display",  no_argument,       0, 'd'},
        {"upper",    no_argument,       0, 'u'},
        {"lower",    no_argument,       0, 'l'},
        {"word",     no_argument,       0, 'w'},
        {"partial",  no_argument,       0, 'p'},
        {"icase",    no_argument,       0, 'i'},
        {"subcol",   required_argument, 0, 's'},
        {"trans",    required_argument, 0, 't'},
        {"regex",    required_argument, 0, 'r'},
        {0, 0, 0, 0}
    };
    int c;
    int display = 0;
    int iopt = 0;
    int partial = 0;
    int icase = 0;
    int trans = 0;
    char *tin = NULL;
    char *sub = NULL;

    while (1) {
        int option_index = 0;
        c = getopt_long( argc, argv, "hdulwpis:t:r:",
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
            case 's':
                sub = optarg;
                break;
            case 't':
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

    if (sub) {
        nSub = DBFGetFieldIndex(dIN, sub);
        if (nSub == -1) {
            fprintf(stderr, "ERROR: Field (%s) not found\n", sub);
            exit(1);
        }
    }
    else
        nSub = -1;

    nEnt = DBFGetRecordCount(dIN);
    nFld = DBFGetFieldCount(dIN);
    
    for (i=0; i<nEnt; i++) {
        if (nSub<0) {
            if (fldnums) {
                for (j=0; optind+j<argc; j++) {
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
                    s = (char *) DBFReadStringAttribute(dIN, i, j);
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
        else /* we scan the columns and substitute if we get a match */
        {
            if (fldnums) {
                for (j=0; optind+j<argc; j++) {
                    s = (char *) DBFReadStringAttribute(dIN, i, fldnums[j]);
                    if (trans == 1 && (ss = strMatch(s, icase))) {
                        DBFWriteStringAttribute(dIN, i, nSub, ss);
                        cnt++;
                        break;
                    }
                    else if (trans == 2 && (ss = regexMatch(s))) {
                        DBFWriteStringAttribute(dIN, i, nSub, ss);
                        cnt++;
                        break;
                    }
                }
            } else {
                for (j=0; j<nFld; j++) {
                    s = (char *) DBFReadStringAttribute(dIN, i, j);
                    if (trans == 1 && (ss = strMatch(s, icase))) {
                        DBFWriteStringAttribute(dIN, i, nSub, ss);
                        cnt++;
                        break;
                    }
                    else if (trans == 2 && (ss = regexMatch(s))) {
                        DBFWriteStringAttribute(dIN, i, nSub, ss);
                        cnt++;
                        break;
                    }
                }
            }
        }
    }

    if (nSub>-1)
        printf("%d Records updated\n", cnt);
    DBFClose(dIN);
    freeStrings();
    for(i=0; i<MAX_STRINGS && rg[i]; i++)
        freeRXcache(rg[i]);
}
