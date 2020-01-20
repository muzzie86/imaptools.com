/*
 * dbf_drop_col.c
 *
 * Routine to copy a DBF file and drop a list of columns
 *
 * Usage: dbf_drop_col infile outfile column_name ...
 *
 * Author: Stephen Woodbridge
 * Date:   2004-01-03
 *
 */

#include <stdlib.h>
#include <string.h>
#ifdef DEBIAN_LIBSHP
#include <shapefil.h>
#else
#include <libshp/shapefil.h>
#endif

void Usage()
{
  printf("Usage: dbf_add_col infile outfile col_name [CN] width prec ...\n");
  exit(1);
}

int main( int argc, char **argv )
{

  DBFHandle  hIN;
  DBFHandle  hOUT;

  DBFFieldType type;

  char       *fin;
  char       *fout;
  char       **flds;
  char       fname[12];
  char       stmp[256];
  double     dtmp;
  long int   itmp;
  int        nfld;
  int        numFields;
  long int   numRec;
  long int   k;
  int        w, d;
  int        i, j, err;

  if ( argc < 7 ) Usage();
  if ( (argc-3) % 4 != 0 ) Usage();

  fin  = argv[1];
  fout = argv[2];
  flds = &argv[3];
  nfld = (argc - 3) / 4;

  // Open the files we need to work with

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

  numFields = DBFGetFieldCount(hIN);
  
  for (i=0; i<numFields; i++) {
    type = DBFGetFieldInfo(hIN, i, fname, &w, &d);
    err = DBFAddField(hOUT, fname, type, w, d);
    if (err == -1) {
      printf("DBFAddField failed.\n");
      exit(1);
    }
  }

  for (i=0; i<nfld; i+=4) {
    switch (flds[i+1][0]) {
      case 'N':
      case 'n':
        err = DBFAddField(hOUT, flds[i], FTDouble, strtol(flds[i+2], NULL, 10), strtol(flds[i+3], NULL, 10));
        break;
      case 'C':
      case 'c':
      case 'S':
      case 's':
        err = DBFAddField(hOUT, flds[i], FTString, strtol(flds[i+2], NULL, 10), 0);
        break;
      default:
        printf("Invalid field def: '%s' '%s' '%s' '%s'\n", flds[i], flds[i+1], flds[i+2], flds[i+3]);
        exit(1);
    }
  }

  // Copy each record field by field skipping any fields
  // that match the command line args

  numRec = DBFGetRecordCount(hIN);
  
  for (k=0; k<numRec; k++) {
    for (i=0; i<numFields; i++) {
      type = DBFGetFieldInfo(hIN, i, fname, &w, &d);
      switch (type) {
        case FTString:
          strcpy(stmp, DBFReadStringAttribute(hIN, k, i));
          DBFWriteStringAttribute(hOUT, k, i, stmp);
          break;
        case FTInteger:
          itmp = DBFReadIntegerAttribute(hIN, k, i);
          DBFWriteIntegerAttribute(hOUT, k, i, itmp);
          break;
        case FTDouble:
          dtmp = DBFReadDoubleAttribute(hIN, k, i);
          DBFWriteDoubleAttribute(hOUT, k, i, dtmp);
          break;
        case FTLogical:
          strcpy(stmp, DBFReadStringAttribute(hIN, k, i));
          DBFWriteStringAttribute(hOUT, k, i, stmp);
          break;
        case FTInvalid:
        default:
          printf("Error: got an invalid field type\n");
          abort();
      }
    }
  }
      
  DBFClose(hOUT);
  DBFClose(hIN);
  
  exit(0);
}
