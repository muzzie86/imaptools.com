

//#define _GNU_SOURCE
#include <assert.h>
#include <math.h>
#include <projects.h>
#include <shapefil.h>

#define UV projUV

#define LASTVERT(v,n)  ((v) == 0 ? n-2 : v-1)
#define NEXTVERT(v,n)  ((v) == n-2 ? 0 : v+1)

/* This is prj2prj, and used for converting ESRI Shapefile from one
project to another.  You need to describe the input/output projections
accordingly.   Please refer to the PROJ4 documentation about syntax  */

typedef struct {
  double x;
  double y;
} pointObj;


int msProjectPoint();


int main( int argc, char ** argv )

{

    static char **InprojArgs = NULL;
/*
    static char *InprojArgs[] = {
        "init=epsg:4326"

// don't use this def, instead set InprojArgs=NULL
//"proj=latlong",
//"datum=NAD83"

    //    "proj=merc",
    //    "ellps=clrk66",
    //    "lon_0=-96"
    };
 */

    static char *OutprojArgs[]= {
        "init=epsg:900913"
    };


    PJ *in;
    PJ *out;
    UV pdata;
    pointObj pt;

    SHPHandle    hSHP, oSHP;
    SHPObject  *sobjA;
    SHPObject  *sobjB;
    int        nShapeType, nEntities, nVertices, nParts, *panParts, i, iPart, nPartVertices;
    int        *panPartStart;
    double    *padVertices, adBounds[4];
    double    pBounds[4];
    double    padfMinBound[4], padfMaxBound[4];
    double    *padfX, *padfY;
    const char     *pszPlus;
    double      *points;
    char    newfile[256];

/* -------------------------------------------------------------------- */
/*      Display a usage message.                                        */
/* -------------------------------------------------------------------- */
    if( argc != 3 ) {
        printf( "prj4326to900913 shp_in shp_out\n" );
        exit( 1 );
    }

/* -------------------------------------------------------------------- */
/*      Open the passed shapefile.                                      */
/* -------------------------------------------------------------------- */
    hSHP = SHPOpen( argv[1], "rb" );

    if( hSHP == NULL ) {
        printf( "Unable to open:%s\n", argv[1] );
        exit( 1 );
    }
    SHPGetInfo( hSHP, &nEntities, &nShapeType, padfMinBound, padfMaxBound );


/* ------------------------------------------------------------------- */
/*      Create new Shapefile                                           */
/* ------------------------------------------------------------------- */
    oSHP = SHPCreate( argv[2], nShapeType );

    if( oSHP == NULL ) {
        printf( "Unable to create:%s\n", argv[2] );
        exit( 3 );
    }

   

/* -------------------------------------------------------------------- */
/*  Initialize Projections                                              */
/* -------------------------------------------------------------------- */
    if (InprojArgs) {
        if (!(in=pj_init(sizeof(InprojArgs)/sizeof(char *),InprojArgs))) {
            fprintf(stderr,"Input Projection initialization failed\n");
            exit(1);
        }
    }
    else
        in = NULL;

    if (!(out=pj_init(sizeof(OutprojArgs)/sizeof(char *),OutprojArgs))) {
        fprintf(stderr,"Output Projection initialization failed\n");
        exit(1);
    }

/* -------------------------------------------------------------------- */
/*      Print out the file bounds.                                      */
/* -------------------------------------------------------------------- */

#define DELTA 0.00001
   
    adBounds[0] = -180.0;
    adBounds[1] =  -90.0;
    adBounds[2] =  180.0;
    adBounds[3] =   90.0;

    while (1) {
        pt.x = adBounds[0];
        pt.y = adBounds[1];
        if (msProjectPoint(in,out,&pt) == 0) {
            pBounds[0] = pt.x;
            pBounds[1] = pt.y;
            break;
        }
        else {
            adBounds[0] += DELTA;
            adBounds[1] += DELTA;
        }
    }
    while (1) {
        pt.x = adBounds[2];
        pt.y = adBounds[3];
        if (msProjectPoint(in,out,&pt) == 0) {
            pBounds[2] = pt.x;
            pBounds[3] = pt.y;
            break;
        }
        else {
            adBounds[2] -= DELTA;
            adBounds[3] -= DELTA;
        }
    }

    printf( "Projection Bounds: (%lg,%lg) - (%lg,%lg)\n",
        adBounds[0], adBounds[1], adBounds[2], adBounds[3] );
    

    printf( "Projected: (%lg,%lg) - (%lg,%lg)\n",pBounds[0],pBounds[1],pBounds[2],pBounds[3] );
    

/* -------------------------------------------------------------------- */
/*    Skim over the list of shapes, printing all the vertices.    */
/* -------------------------------------------------------------------- */
    for( i = 0; i < nEntities; i++ ) {
        int        j;

        sobjA = SHPReadObject( hSHP, i );
        nVertices = sobjA->nVertices;
        nParts    = sobjA->nParts;
        panParts  = sobjA->panPartStart;

/*
        adBounds[0] = sobjA->dfXMin;
        adBounds[1] = sobjA->dfYMin;
        adBounds[2] = sobjA->dfXMax;
        adBounds[3] = sobjA->dfYMax;
*/

        padfX = malloc(nVertices * sizeof(double));
        assert(padfX);
        padfY = malloc(nVertices * sizeof(double));
        assert(padfY);

        if (nParts) {
            panPartStart = calloc(nParts, sizeof(int));
            assert(panPartStart);
        }
        else
            panPartStart = NULL;

/*
    printf( "\nShape:%d  Bounds:(%lg,%lg) - (%lg,%lg) has %d parts and %d verts total\n", i, adBounds[0], adBounds[1], adBounds[2], adBounds[3], nParts , nVertices);
*/

        for( j = 0, iPart = 1; j < nVertices; j++ ) {
            if( iPart < nParts && panParts[iPart] == j+1 ) {
                iPart++;
                pszPlus = "+";
            }
            else
                pszPlus = "";


            pt.x = sobjA->padfX[j];
            pt.y = sobjA->padfY[j];
            if (pt.x < adBounds[0])
                pt.x = adBounds[0];
            else if (pt.x > adBounds[2])
                pt.x =  adBounds[2];
            if (pt.y < adBounds[1])
                pt.y =  adBounds[1];
            else if (pt.y > adBounds[3])
                pt.y = adBounds[3];
            if (msProjectPoint(in,out,&pt) == 1) {
                printf("Ent: %d, Vert: %d, (%1g,%1g)\n", i, j, pt.x, pt.y);
            }
            padfX[j] = pt.x;
            padfY[j] = pt.y;

/*
            printf("(%lg,%lg) %s \n", padfX[j], padfY[j], pszPlus );
*/
        }

        sobjB = SHPCreateObject( sobjA->nSHPType, i, nParts, panParts,
            NULL, nVertices, padfX, padfY, NULL, NULL);

        SHPWriteObject(oSHP, -1, sobjB);

    }

    SHPClose( hSHP );
    SHPClose( oSHP );
}


int polyDirection(double *p, int n)
{
  double mx, my, area;
  int i, v=0, lv, nv;

  /* first find lowest, rightmost vertex of polygon */
  mx = p[0];
  my = p[1];

  for(i=0; i<n-1; i++) {
    if((p[i*2+1] < my) || ((p[i*2+1] == my) && (p[i*2] > mx))) {
      v = i;
      mx = p[i*2];
      my = p[i*2+1];
    }
  }

  lv = LASTVERT(v,n);
  nv = NEXTVERT(v,n);

  area = p[lv*2]*p[v*2+1] - p[lv*2+1]*p[v*2] + p[lv*2+1]*p[nv*2] - p[lv*2]*p[nv*2+1]
+ p[v*2]*p[nv*2+1] - p[nv*2]*p[v*2+1];

  if(area > 0)
    return(1); /* counter clockwise orientation */
  else
    if(area < 0) /* clockwise orientation */
      return(-1);
    else
      return(0); /* shouldn't happen unless the polygon has duplicate arcs */

}

int msProjectPoint(PJ *in, PJ *out, pointObj *point)
{
  UV p;

  p.u = point->x;
  p.v = point->y;

  if(in==NULL) { /* input coordinates are lat/lon */
    p.u *= DEG_TO_RAD; /* convert to radians */
    p.v *= DEG_TO_RAD;
    p = pj_fwd(p, out);
  } else {
    if(out==NULL) { /* output coordinates are lat/lon */
      p = pj_inv(p, in);
      p.u *= RAD_TO_DEG; /* convert to decimal degrees */
      p.v *= RAD_TO_DEG;
    } else { /* need to go from one projection to another */
      p = pj_inv(p, in);
      p = pj_fwd(p, out);
    }
  }
  if (p.u == HUGE_VAL || p.v == HUGE_VAL)
    return 1;

  point->x = p.u;
  point->y = p.v;
  return 0;
}

