/*********************************************************************
*
*  $Id: dbfcase.c,v 1.1 2005/05/11 20:48:51 woodbri Exp $
*
*  dbfcase
*  Copyright 2005, Stephen Woodbridge, All rights Reserved
*  Redistribition without written permission strictly prohibited
*
**********************************************************************
*
*  dbfcase.c
*
*  $Log: dbfcase.c,v $
*  Revision 1.1  2005/05/11 20:48:51  woodbri
*  Add new utility the scans a dbf file and changes the case of words in any
*  field ot fields. Allows UPPER, lower or Word Case.
*
*
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
#include <string.h>
#include <shapefil.h>

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

extern int DEBUG;
int DEBUG  = 0;


void Usage()
{
  printf("Usage: dbfcase [-i|-h] {-u|-l|-w} <dbf file> [<list of fields>]\n");
  printf("       -h  display this help message\n");
  printf("       -i  display field names and record count info\n");
  printf("       -u  force fields to uppercase\n");
  printf("       -l  force fields to lowercase\n");
  printf("       -w  force fields to word case\n\n");
  printf("       WARNING: this command changes your dbf file,\n");
  printf("       WARNING: make sure you have backups!\n");
  exit(1);
}


const char* chgCase(int iopt, const char *in) {
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
        }
    out[i] = '\0';
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
    int           nEnt, nFld, iopt;
    int           i, j;
    int          *fldnums = NULL;
    int          fw, fd;
    int          err;
    char         *s;
    char         *opt;

    if (argc < 3) Usage();

    opt  = argv[1];
    fin  = argv[2];
    argc -= 3;
    argv += 3;

    switch (opt[1]) {
        case 'u':
            iopt = 1;
            break;
        case 'l':
            iopt = 2;
            break;
        case 'w':
            iopt = 3;
            break;
        case 'h':
            Usage();
        case 'i':
            if ((dIN  = DBFOpen(fin, "rb+"))) {
                print_fields(dIN);
                DBFClose(dIN);
            } else 
                fprintf(stderr, "Failed to open dbf %s for read/write\n", fin);
            exit(1);
        default:
            fprintf(stderr, "%s is not a valid option\n", opt);
            Usage();
    }
    
    if (argc) {
        fldnums = malloc(argc * sizeof(int));
        if (!fldnums) {
            fprintf(stderr, "Failed to allocate %d ints.\n", argc);
            exit(1);
        }
        // initialize them to undefined
        for (i=0; i<argc; i++) fldnums[i] = -1;
    }
  
    if (!(dIN  = DBFOpen(fin, "rb+"))) {
        fprintf(stderr, "Failed to open dbf %s for read/write\n", fin);
        exit(1);
    }

    if (fldnums) {
        for (i=0; i<DBFGetFieldCount(dIN); i++) {
            fType = DBFGetFieldInfo(dIN, i, fldname, &fw, &fd);
            for (j=0; j<argc; j++)
                if (!strcmp(argv[j], fldname))
                    fldnums[j] = i;
        }
        // make sure we found them all
        err = 0;
        for (i=0; i<argc; i++)
            if (fldnums[i] == -1) {
                err = 1;
                fprintf(stderr, "ERROR: Field (%s) not found\n", argv[i]);
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
            for (j=0; j<argc; j++) {
                s = (char *) DBFReadStringAttribute(dIN, i, fldnums[j]);
                DBFWriteStringAttribute(dIN, i, fldnums[j], chgCase(iopt, s));
            }
        } else {
            for (j=0; j<nFld; j++) {
                s = (char *) DBFReadStringAttribute(dIN, i, j);
                DBFWriteStringAttribute(dIN, i, j, chgCase(iopt, s));
            }

        }
    }

    DBFClose(dIN);
}
