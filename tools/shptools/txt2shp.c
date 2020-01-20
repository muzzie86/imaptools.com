/*********************************************************************
*
*  $Id: txt2shp.c,v 1.1 2006/12/15 03:05:36 woodbri Exp $
*
*  txt2shp
*  Copyright 2006, Stephen Woodbridge, All rights Reserved
*  Redistribition without written permission strictly prohibited
*
**********************************************************************
*
*  txt2shp.c
*
*  $Log: txt2shp.c,v $
*  Revision 1.1  2006/12/15 03:05:36  woodbri
*  Added txt2shp and test file.
*
*
*  2006-12-14 sew, created
*
**********************************************************************
*
*  Create a shapefile from a tab delimited txt file
*
**********************************************************************/

#include <getopt.h>
#include <assert.h>
#include <errno.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <shapefil.h>

static char buf[2048];

void Usage() {
    printf("Usage: txt2shp basename\n");
    printf("  reads basename.txt, write basename.(shp,shx,dbf)\n");
    printf("  basename.txt must be formated as:\n");
    printf("# comments\n");
    printf("column_name1<tab>type<tab>width<tab>decimals<nl>\n");
    printf("column_name2<tab>type<tab>width<tab>decimals<nl>\n");
    printf("column_nameN<tab>type<tab>width<tab>decimals<nl>\n");
    printf("__END__\n");
    printf("latitude<tab>longitude<tab>col1<tab>col2<tab>colN<nl>\n");
    printf("latitude<tab>longitude<tab>col1<tab>col2<tab>colN<nl>\n");
    printf("latitude<tab>longitude<tab>col1<tab>col2<tab>colN<nl>\n");
    printf("\n   where:\n");
    printf("     type : S = String, N = Number, D = Double\n");
    printf("    width : maximum field width for this column\n");
    printf(" decimals : number of decimal for Double or 0\n");
    exit(0);
}

#define MAXFIELDS 100

int main( int argc, char ** argv )
{
    FILE        *in;
    DBFHandle    dH;
    SHPHandle    sH;
    SHPObject   *sobj;

    int          ftypes[MAXFIELDS] = {0};

    char        *file;
    char        *name;
    char        *type;
    char        *width;
    char        *deci;
    char        *p;
    char        *q;
    double       X, Y;
    double       dd;
    int          ii;
    int          num;
    int          fnum;
    int          nfile;
    int          fcnt;
    int          ftype;
    int          fwidth;
    int          fdeci;

    if (argc != 2)
        Usage();

    file = (char *) calloc(strlen(argv[1])+5, sizeof(char));
    strcpy(file, argv[1]);
    nfile = strlen(file);
    if (strstr(file, ".txt") != file + nfile - 4)
        strcat(file, ".txt");
  
    in = fopen(file, "rb");
    if (!in) {
        printf("Falied to open %s : %s\n", file, strerror(errno));
        exit(1);
    }

    sH = SHPCreate( file, SHPT_POINT );
    if (!sH) {
        printf("Failed to create shape file for %s!\n", file);
        exit(1);
    }

    dH = DBFCreate( file );
    if (!dH) {
        printf("Failed to create dbf file for %s!\n", file);
        exit(1);
    }

    /***********  Read Schema Information **************/

    fcnt = 0;
    while (1) {
        if (feof(in) || !fgets(buf, 2048, in)) exit(1);
        if (fcnt >= MAXFIELDS) abort();

        if (*buf == '#')
            continue;
        
        if (strstr(buf, "__END__"))
            break;

        name  = buf;
        type  = strstr(name,  "\t") + 1;
        *(type - 1) = '\0';
        width = strstr(type,  "\t") + 1;
        *(width - 1) = '\0';
        deci  = strstr(width, "\t") + 1;

        switch (*type) {
            case 's':
            case 'S':
                ftypes[fcnt] = ftype  = FTString;
                fwidth = strtol(width, NULL, 10);
                fdeci  = 0;
                break;
            case 'i':
            case 'I':
                ftypes[fcnt] = ftype  = FTInteger;
                fwidth = strtol(width, NULL, 10);
                fdeci  = 0;
                break;
            case 'd':
            case 'D':
                ftypes[fcnt] = ftype  = FTDouble;
                fwidth = strtol(width, NULL, 10);
                fdeci  = strtol(deci, NULL, 10);
                break;
            default:
                printf("Invalid type (%s)\n", type);
                exit(1);
                break;
        }
      
        DBFAddField( dH, name, ftype, fwidth, fdeci );
        
        fcnt++;

    }

    /**********  read in the data **********************/

    while (1) {
        if (feof(in) || !fgets(buf, 2048, in))
            break;

        if (*buf == '#')
            continue;
        
        if (strstr(buf, "__END__"))
            break;

        p = buf;
        Y = strtod(p, NULL);
        p = strstr(p, "\t") + 1;
        X = strtod(p, NULL);
        p = strstr(p, "\t") + 1;

        sobj = SHPCreateSimpleObject(SHPT_POINT, 1, &X, &Y, NULL);
        num  = SHPWriteObject(sH, -1, sobj);
        SHPDestroyObject(sobj);

        fnum = 0;
        while (fnum < fcnt) {
            q = strpbrk(p, "\t\n");
            if (q) {
                *q = '\0';
                switch (ftypes[fnum]) {
                    case FTString:
                        DBFWriteStringAttribute(dH, num, fnum, p);
                        break;
                    case FTInteger:
                        ii = strtol(p, NULL, 10);
                        DBFWriteIntegerAttribute(dH, num, fnum, ii);
                        break;
                    case FTDouble:
                        dd = strtod(p, NULL);
                        DBFWriteDoubleAttribute(dH, num, fnum, dd);
                        break;
                    default:
                        abort();
                        break;
                }
                p = q + 1;
            }
            else
                break;

            fnum++;
        }

    }

    DBFClose(dH);
    SHPClose(sH);

    free(file);
    return 0;
}
