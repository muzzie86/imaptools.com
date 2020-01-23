/*
 * dbflike.c
 *
 * Routine to copy a DBF file and and force it to be like a ref
 *
 * Usage: dbflike reffile infile outfile
 *
 * Author: Stephen Woodbridge
 * Date:   2004-01-03
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#ifdef DEBIAN_LIBSHP
#include <shapefil.h>
#else
#include <libshp/shapefil.h>
#endif

void Usage()
{
  printf("Usage: dbflike reffile infile outfile\n");
  exit(1);
}

int main( int argc, char **argv )
{

  DBFHandle  hREF;
  DBFHandle  hIN;
  DBFHandle  hOUT;

  DBFFieldType type;

  char       *ref;
  char       *fin;
  char       *fout;
  char       **flds;
  char       fname[12];
  char       stmp[256];
  double     dtmp;
  long int   itmp;
  int        nfld;
  int        numFields;
  int        numFieldsI;
  long int   numRec;
  long int   k;
  int        w, d;
  int        i, ii, j, err;

  if ( argc < 4 ) Usage();

  ref  = argv[1];
  fin  = argv[2];
  fout = argv[3];

  // Open the files we need to work with

  hREF = DBFOpen(ref, "rb");
  if (hREF == NULL) {
    printf("Failed to open '%s' for read!\n", ref);
    exit(1);
  }

  hIN = DBFOpen(fin, "rb");
  if (hIN == NULL) {
    printf("Failed to open '%s' for read!\n", fin);
    exit(1);
  }

  hOUT = DBFCreate(fout);
  if (hOUT == NULL) {
    printf("Failed to create '%s'\n", fout);
    exit(1);
  }

  // Copy the field definitions to the new DBF
  // dropping those on the command line. No validation
  // that all or any of the command line args actually
  // match the DBF is attempted

  numFields  = DBFGetFieldCount(hREF);
  numFieldsI = DBFGetFieldCount(hIN);

  /*
  if (numFields != numFieldsI) {
      printf("Ref file has %d fields, In file has %d fields\n",
              numFields, numFieldsI);
      exit(1);
  }
  */
  
  i = 0;
  while (i<numFields) {
    type = DBFGetFieldInfo(hREF, i, fname, &w, &d);
    err = DBFAddField(hOUT, fname, type, w, d);
    if (err == -1) {
      printf("DBFAddField failed.\n");
      exit(1);
    }
    i++;
  }
  DBFClose(hREF);

  // Copy each record field by field skipping any fields
  // that match the command line args

  numRec = DBFGetRecordCount(hIN);
  
  k = 0;
  while (k<numRec) {
    i  = 0;
    while (i<numFields) {
      type  = DBFGetFieldInfo(hOUT, i, fname, &w, &d);
      ii = DBFGetFieldIndex(hIN, fname);
      if (ii > -1)
      switch (type) {
        case FTString:
          strcpy(stmp, DBFReadStringAttribute(hIN, k, ii));
          DBFWriteStringAttribute(hOUT, k, i, stmp);
          break;
        case FTInteger:
          itmp = DBFReadIntegerAttribute(hIN, k, ii);
          DBFWriteIntegerAttribute(hOUT, k, i, itmp);
          break;
        case FTDouble:
          dtmp = DBFReadDoubleAttribute(hIN, k, ii);
          DBFWriteDoubleAttribute(hOUT, k, i, dtmp);
          break;
        case FTLogical:
          strcpy(stmp, DBFReadStringAttribute(hIN, k, ii));
          DBFWriteStringAttribute(hOUT, k, i, stmp);
          break;
        case FTInvalid:
        default:
          printf("Error: got an invalid field type\n");
          abort();
      }  
      i++;
    }
  k++;
}
      
  DBFClose(hOUT);
  DBFClose(hIN);
  
  exit(0);
}
