


#include <assert.h>
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


void msProjectPoint();


int main( int argc, char ** argv )

{

/*
    static char *InprojArgs = NULL;
 */
    static char *InprojArgs[] = {
"proj=lcc",
"datum=NAD83",
"lon_0=-84d30",
"lat_1=30d45",
"lat_2=29d35",
"lat_0=29",
"x_0=600000",
"y_0=0",
"to_meter=0.3048006096",
"no_defs"

// don't use this def, instead set InprojArgs=NULL
//"proj=latlong",
//"datum=NAD83"

 	//	"proj=merc",
	//	"ellps=clrk66",
	//	"lon_0=-96"
	};

    static char *OutprojArgs[]= {
"proj=utm",
"ellps=GRS80",
"zone=16",
"north",
"no_defs"
   	//	"proj=aea",
	//	"ellps=clrk66",
	//	"lon_0=-96",
	//	"lat_1=29.5",
	//	"lat_2=45.5"
    };


    PJ *in;
    PJ *out;
    UV pdata;
	pointObj pt;

    SHPHandle	hSHP, oSHP;
    SHPObject  *sobjA;
    SHPObject  *sobjB;
    int		nShapeType, nEntities, nVertices, nParts, *panParts, i, iPart, nPartVertices;
    int		*panPartStart;
    double	*padVertices, adBounds[4];
    double	pBounds[4];
    double	padfMinBound[4], padfMaxBound[4];
    double	*padfX, *padfY;
    const char 	*pszPlus;
    double      *points;
    char	newfile[256];

/* -------------------------------------------------------------------- */
/*      Display a usage message.                                        */
/* -------------------------------------------------------------------- */
    if( argc != 2 )
    {
	printf( "shpproject shp_file\n" );
	exit( 1 );
    }

/* -------------------------------------------------------------------- */
/*      Open the passed shapefile.                                      */
/* -------------------------------------------------------------------- */
    hSHP = SHPOpen( argv[1], "rb" );

    if( hSHP == NULL )
    {
	printf( "Unable to open:%s\n", argv[1] );
	exit( 1 );
    }
    SHPGetInfo( hSHP, &nEntities, &nShapeType, padfMinBound, padfMaxBound );


/* ------------------------------------------------------------------- */
/*      Create new Shapefile                                           */
/* ------------------------------------------------------------------- */
    strcpy(newfile,"new_");
	strcat(newfile,argv[1]);

    oSHP = SHPCreate( newfile, nShapeType );

    if( oSHP == NULL )
    {
    printf( "Unable to create:%s\n", newfile );
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
    } else in = NULL;

    if (!(out=pj_init(sizeof(OutprojArgs)/sizeof(char *),OutprojArgs))) {
		fprintf(stderr,"Output Projection initialization failed\n");
	    exit(1);
	}

/* -------------------------------------------------------------------- */
/*      Print out the file bounds.                                      */
/* -------------------------------------------------------------------- */
    adBounds[0] = padfMinBound[0];
    adBounds[1] = padfMaxBound[0];
    adBounds[2] = padfMinBound[1];
    adBounds[3] = padfMaxBound[1];

/*
    printf( "File Bounds: (%lg,%lg) - (%lg,%lg)\n",
	    adBounds[0], adBounds[1], adBounds[2], adBounds[3] );
 */

    pt.x = adBounds[0];
    pt.y = adBounds[1];
    msProjectPoint(in,out,&pt);
    pBounds[0] = pt.x;
    pBounds[1] = pt.y;

    pt.x = adBounds[2];
    pt.y = adBounds[3];
    msProjectPoint(in,out,&pt);
    pBounds[2] = pt.x;
    pBounds[3] = pt.y;
    

/*
    printf( "Projected: (%lg,%lg) - (%lg,%lg)\n",pBounds[0],pBounds[1],pBounds[2],pBounds[3] );
    
*/

/* -------------------------------------------------------------------- */
/*	Skim over the list of shapes, printing all the vertices.	*/
/* -------------------------------------------------------------------- */
    for( i = 0; i < nEntities; i++ )
    {
	int		j;

        sobjA = SHPReadObject( hSHP, i );
        nVertices = sobjA->nVertices;
        nParts    = sobjA->nParts;
        panParts  = sobjA->panPartStart;

        adBounds[0] = sobjA->dfXMin;
        adBounds[1] = sobjA->dfYMin;
        adBounds[2] = sobjA->dfXMax;
        adBounds[3] = sobjA->dfYMax;

        padfX = malloc(nVertices * sizeof(double));
        assert(padfX);
        padfY = malloc(nVertices * sizeof(double));
        assert(padfY);

        if (nParts) {
            panPartStart = calloc(nParts, sizeof(int));
            assert(panPartStart);
        } else
            panPartStart = NULL;

/*
	printf( "\nShape:%d  Bounds:(%lg,%lg) - (%lg,%lg) has %d parts and %d verts total\n", i, adBounds[0], adBounds[1], adBounds[2], adBounds[3], nParts , nVertices);
*/

        for( j = 0, iPart = 1; j < nVertices; j++ )
        {
            if( iPart < nParts && panParts[iPart] == j+1 )
            {
            iPart++;
            pszPlus = "+";
            }
            else
                pszPlus = "";


            pt.x = sobjA->padfX[j];
            pt.y = sobjA->padfY[j];
            msProjectPoint(in,out,&pt);
            padfX[j] = pt.x;
            padfY[j] = pt.y;

/*
            printf("(%lg,%lg) %s \n", padfX[j], padfY[j], pszPlus );
*/
        }

        sobjB = SHPCreateObject( sobjA->nSHPType, i, nParts, panPartStart,
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

void msProjectPoint(PJ *in, PJ *out, pointObj *point)
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
  point->x = p.u;
  point->y = p.v;
  return;
}

