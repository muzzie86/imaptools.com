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

#include <string.h>
#ifdef DEBIAN_LIBSHP
#include <shapefil.h>
#else
#include <libshp/shapefil.h>
#endif

void Usage()
{
  printf("Usage: dbf_drop_col infile outfile col_name ...\n");
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
  int        i, ii, j, err;

  if ( argc < 4 ) Usage();

  fin  = argv[1];
  fout = argv[2];
  flds = &argv[3];
  nfld = argc - 3;

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
  
  i = 0;
  while (i<numFields) {
    type = DBFGetFieldInfo(hIN, i, fname, &w, &d);
    for (j=0; j<nfld; j++)
      if (!strcasecmp(fname, flds[j])) goto NEXT;
    err = DBFAddField(hOUT, fname, type, w, d);
    if (err == -1) {
      printf("DBFAddField failed.\n");
      exit(1);
    }
NEXT:
    i++;
  }

  // Copy each record field by field skipping any fields
  // that match the command line args

  numRec = DBFGetRecordCount(hIN);
  
  k = 0;
  while (k<numRec) {
    i  = 0;
    ii = 0;
    while (i<numFields) {
      type = DBFGetFieldInfo(hIN, i, fname, &w, &d);
      for (j=0; j<nfld; j++)
        if (!strcasecmp(fname, flds[j])) goto NEXT2;
      switch (type) {
        case FTString:
          strcpy(stmp, DBFReadStringAttribute(hIN, k, i));
          DBFWriteStringAttribute(hOUT, k, ii, stmp);
	  break;
        case FTInteger:
          itmp = DBFReadIntegerAttribute(hIN, k, i);
          DBFWriteIntegerAttribute(hOUT, k, ii, itmp);
	  break;
	case FTDouble:
	  dtmp = DBFReadDoubleAttribute(hIN, k, i);
	  DBFWriteDoubleAttribute(hOUT, k, ii, dtmp);
	  break;
	case FTLogical:
          strcpy(stmp, DBFReadStringAttribute(hIN, k, i));
          DBFWriteStringAttribute(hOUT, k, ii, stmp);
	  break;
	case FTInvalid:
	default:
	  printf("Error: got an invalid field type\n");
	  abort();
      }  
      ii++;
NEXT2:
      i++;
    }
  k++;
}
      
  DBFClose(hOUT);
  DBFClose(hIN);
  
  exit(0);
}
