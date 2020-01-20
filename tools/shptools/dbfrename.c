/*********************************************************************
*
*  $Id: dbfrename.c,v 1.1 2005/10/23 15:57:33 woodbri Exp $
*
*  dbfrename
*  Copyright 2005, Stephen Woodbridge, All rights Reserved
*  Redistribition without written permission strictly prohibited
*
**********************************************************************
*
*  dbfrename.c
*
*  $Log: dbfrename.c,v $
*  Revision 1.1  2005/10/23 15:57:33  woodbri
*  Adding dbfrename.c - a utility for coping a dbf and renaming the attribute
*  column names.
*
*
*  2005-10-08 sew, created
*
**********************************************************************
*
*  Utility to copy dbf file and rename columns
*
**********************************************************************/

#include <getopt.h>
#include <assert.h>
#include <errno.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <shapefil.h>

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
  printf("dbfrename <in file> <out file> <old col> <new col> ...\n");
  exit(0);
}

int main( int argc, char ** argv )
{
  DBFHandle     dIN, dOUT;
  DBFFieldType  fType;
  char         *fin;
  char         *fout;
  char          fldname[12];
  char          fldout[12];
  int           nEnt, sType, fw, fd;
  int           i, j, rec, n;

  if (argc < 5) Usage();

  fin  = argv[1];
  fout = argv[2];
  argc -= 3;
  argv += 3;

  if (!(dIN  = DBFOpen(fin, "rb"))) {
    printf("Failed to open dbf %s for read\n", fin);
    exit(1);
  }

  if (!(dOUT = DBFCreate(fout))) {
    printf("Failed to create dbf %s\n", fout);
    exit(1);
  }

  for (i=0; i<DBFGetFieldCount(dIN); i++) {
    fType = DBFGetFieldInfo(dIN, i, fldname, &fw, &fd);
    strcpy(fldout, fldname);
    for (j=0; j<argc; j += 2)
        if (!strcmp(argv[j], fldname))
            strcpy(fldout, argv[j+1]);
    DBFAddField(dOUT, fldout, fType, fw, fd);
  }

  nEnt = DBFGetRecordCount(dIN);

  for (i=0; i<nEnt; i++) {
    for (j=0; j<DBFGetFieldCount(dIN); j++) {
      fType = DBFGetFieldInfo(dIN, j, NULL, NULL, NULL);
      switch (fType) {
        case FTString:
          DBFWriteStringAttribute( dOUT, i, j, DBFReadStringAttribute( dIN, i, j));
          break;
        case FTInteger:
          DBFWriteIntegerAttribute(dOUT, i, j, DBFReadIntegerAttribute(dIN, i, j));
          break;
        case FTDouble:
          DBFWriteDoubleAttribute( dOUT, i, j, DBFReadDoubleAttribute( dIN, i, j));
          break;
      }
    }
  }

  DBFClose(dIN);
  DBFClose(dOUT);
}


