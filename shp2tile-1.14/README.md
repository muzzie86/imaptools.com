```
*********************************************************************
*
*  $Id: README,v 1.2 20021/03/18 13:46:14 woodbri Exp woodbri $
*
*  shp2tile
*  Copyright 2002, Stephen Woodbridge
*
*  tools@swoodbridge.com
*  http://swoodbridge.com/tools/
*
*  Development sponsored by:
*  Luca Pescatore <info@pescatoreluca.com>
*
*   This program is free software; you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation; either version 2 of the License, or
*   (at your option) any later version.
*
*   This program is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with this program; if not, write to the Free Software
*   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*
**********************************************************************

Installation

  tar xzvf shp2tile-1.1.tgz
  make
  make install

You probably will need to adjust the paths in the make file.

NOTE: Different linux distributions install shapefil.h in different 
locations. If you get an error compiling check the location of this
file and change the #include int shp2tile.c, it will typically require
one of the following.

#include <shapefil.h>
#include <libshp/shapefil.h>


Usage: shp2tile <options> <shapefile> <dest name>
  where <options> are:
   [-s|--no-stats]  do not print stats
   [-w|--no-write]  do not write output files
   [-r|--row] <n>   generate <n> rows in Y (default =2)
   [-c|--col] <m>   generate <m> cols in X (default =2)
   [-e|--square-ext]   adjust extent to produce square tiles
   [-q|--quadtree] <n> quadtree type of index (auto split tiles > n objects)
   [-m|--maxdepth] <n> maximum depth for --quadtree option
  <shapefile>       name of file to read and split into tiles (required)
  <dest name>       output file names: <dest name>-<num>.<ext>
                    and num is the cell index. Empty cells are
                    are not written as files.

Use the --no-write option to get a report of what shp2tile will do to your file.
You can use this report to adjust the number of tiles for your needs.

Note: tile boundaries are fuzzy. If an object crosses a tile boundary
then we try to fit it in the first tile we can after growing the
boundaries by +-5% in dx or dy. If it still doesn't fit then we throw it
into the super tile. We do NOT split object.

Version 1.1

Daniel Morissette has contributed the --square-ext and --quadtree options to shp2tile.
These are two very nice options that work as follows:

--square-ext

This options asks shp2tile to adjust the source extents to result in
square tiles.  This way no need to worry about feeding it with a number
of rows/columns that matches the width/height of the dataset when you want
square tiles.

--quadtree <max_obj>

This is a cool one!  With this option, you specify a maximum number of
objects per tile, and any tile that ends up with more objects than the
threshold will be reprocessed (recursively).  When used with a 2x2 grid
this results in a kind of quadtree type tile organization.

What this all means is that there is no more need to worry about
pre-calculating the number of tiles for a given dataset, and it's much
easier to automate the tiling process, you just need to decide on an
acceptable number of objects per tile for your dataset and then use:

  shp2tile --square-ext --quadtree 8000 srcfile.shp output_dir/dst

There is no set rule on the number of objects. You want the number as 
big as possible so that when you open the file for any given map draw
most all of the objects in the file are draw. Obviously if you make
the number small this will be the case, but you will create a large
number of files and many of them will need to be opened to draw the map
and this is not good either. I use a number around 8000 for most
applications of street data. Do some testing and see what works
best for you.

--maxdepth <depth>

This is an option to the --quadtree that will allow you to control
the maximum depth that the sub tiling can go at the cost of larger files.

NOTE: --no-write option may not be specified with the --quadtree option.

Examples:

Here is an example of a 4 x 4 tiling:

[woodbri@linus test]$ shp2tile -r 4 -c 4 minroads tt
Shapefile Type: Arc   # of Shapes: 66002

File Bounds: (  -71.898465,   42.156704)
         to  (  -71.020895,   42.734089)

Divided into 4 rows X 4 columns (dx, dy) = (    0.219393,    0.144346)

  Cell     Count      XMin         YMin         YMax         YMax
[  0,  0]      0   -71.898465    42.156704   -71.679073    42.301050
[  0,  1]   1325   -71.679073    42.156704   -71.459680    42.301050
[  0,  2]   3284   -71.459680    42.156704   -71.240287    42.301050
[  0,  3]    222   -71.240287    42.156704   -71.020895    42.301050
[  1,  0]      0   -71.898465    42.301050   -71.679073    42.445397
[  1,  1]   3034   -71.679073    42.301050   -71.459680    42.445397
[  1,  2]   6441   -71.459680    42.301050   -71.240287    42.445397
[  1,  3]  19252   -71.240287    42.301050   -71.020895    42.445397
[  2,  0]      0   -71.898465    42.445397   -71.679073    42.589743
[  2,  1]   2565   -71.679073    42.445397   -71.459680    42.589743
[  2,  2]   5605   -71.459680    42.445397   -71.240287    42.589743
[  2,  3]  12419   -71.240287    42.445397   -71.020895    42.589743
[  3,  0]    673   -71.898465    42.589743   -71.679073    42.734089
[  3,  1]   1865   -71.679073    42.589743   -71.459680    42.734089
[  3,  2]   8524   -71.459680    42.589743   -71.240287    42.734089
[  3,  3]    793   -71.240287    42.589743   -71.020895    42.734089
[ global]      0   -71.898465    42.156704   -71.020895    42.734089
        --------
           66002

[woodbri@linus test]$ ls
minroads.dbf  tt-10.dbf  tt-12.dbf  tt-14.dbf  tt-2.dbf  tt-5.dbf  tt-7.dbf
minroads.shp  tt-10.shp  tt-12.shp  tt-14.shp  tt-2.shp  tt-5.shp  tt-7.shp
minroads.shx  tt-10.shx  tt-12.shx  tt-14.shx  tt-2.shx  tt-5.shx  tt-7.shx
tt-1.dbf      tt-11.dbf  tt-13.dbf  tt-15.dbf  tt-3.dbf  tt-6.dbf  tt-9.dbf
tt-1.shp      tt-11.shp  tt-13.shp  tt-15.shp  tt-3.shp  tt-6.shp  tt-9.shp
tt-1.shx      tt-11.shx  tt-13.shx  tt-15.shx  tt-3.shx  tt-6.shx  tt-9.shx

Notice three tiles and the super tile (global) are all empty. Also this
file has a pretty good distribution of the 66K objects across 13
remaining tiles, but here is another test using 4 x 1 tiles:

[woodbri@linus test]$ shp2tile --no-write -r 4 -c 1 minroads tt                                      
Shapefile Type: Arc   # of Shapes: 66002

File Bounds: (  -71.898465,   42.156704)
         to  (  -71.020895,   42.734089)

Divided into 4 rows X 1 columns (dx, dy) = (    0.877570,    0.144346)

  Cell     Count      XMin         YMin         YMax         YMax
[  0,  0]   4829   -71.898465    42.156704   -71.020895    42.301050
[  1,  0]  28723   -71.898465    42.301050   -71.020895    42.445397
[  2,  0]  20590   -71.898465    42.445397   -71.020895    42.589743
[  3,  0]  11860   -71.898465    42.589743   -71.020895    42.734089
[ global]      0   -71.898465    42.156704   -71.020895    42.734089
        --------
           66002


Here is an example of a file that you would NOT want to tile:

[woodbri@linus mdata]$ shp2tile -w -r 4 -c 4 majrdnet 
Shapefile Type: Polygon   # of Shapes: 32

File Bounds: (  -97.207419,   24.547821)
         to  (  -67.177048,   49.000645)

Divided into 4 rows X 4 columns (dx, dy) = (    7.507593,    6.113206)

  Cell     Count      XMin         YMin         YMax         YMax
[  0,  0]      0   -97.207419    24.547821   -89.699826    30.661027
[  0,  1]      0   -89.699826    24.547821   -82.192234    30.661027
[  0,  2]      0   -82.192234    24.547821   -74.684641    30.661027
[  0,  3]      0   -74.684641    24.547821   -67.177048    30.661027
[  1,  0]      1   -97.207419    30.661027   -89.699826    36.774233
[  1,  1]      1   -89.699826    30.661027   -82.192234    36.774233
[  1,  2]      0   -82.192234    30.661027   -74.684641    36.774233
[  1,  3]      0   -74.684641    30.661027   -67.177048    36.774233
[  2,  0]      0   -97.207419    36.774233   -89.699826    42.887439
[  2,  1]      2   -89.699826    36.774233   -82.192234    42.887439
[  2,  2]      4   -82.192234    36.774233   -74.684641    42.887439
[  2,  3]      3   -74.684641    36.774233   -67.177048    42.887439
[  3,  0]      1   -97.207419    42.887439   -89.699826    49.000645
[  3,  1]      0   -89.699826    42.887439   -82.192234    49.000645
[  3,  2]      0   -82.192234    42.887439   -74.684641    49.000645
[  3,  3]      3   -74.684641    42.887439   -67.177048    49.000645
[ global]     17   -97.207419    24.547821   -67.177048    49.000645
        --------
              32

Notice that 53% of the objects landed in the super tile! This is because
this data set has long stringy roads that run from top to bottom and/or
side to side so very few of them will fit into smaller tiles. This is
not a huge data set but illustrates a potential problem. You could also
try using less row x col to make bigger tiles:

[woodbri@linus mdata]$ shp2tile -w -r 2 -c 2 majrdtile                      
Shapefile Type: Polygon   # of Shapes: 32

File Bounds: (  -97.207419,   24.547821)
         to  (  -67.177048,   49.000645)

Divided into 2 rows X 2 columns (dx, dy) = (   15.015186,   12.226412)

  Cell     Count      XMin         YMin         YMax         YMax
[  0,  0]      5   -97.207419    24.547821   -82.192234    36.774233
[  0,  1]      0   -82.192234    24.547821   -67.177048    36.774233
[  1,  0]      7   -97.207419    36.774233   -82.192234    49.000645
[  1,  1]     13   -82.192234    36.774233   -67.177048    49.000645
[ global]      7   -97.207419    24.547821   -67.177048    49.000645
        --------
              32

This is a little better and would generate 4 output tiles, IE: 3 quarter
tiles and the super tile.

Example with Polygons:

[woodbri@linus 25017]$ shp2tile -w -r 3 -c 3 water                          
Shapefile Type: Polygon   # of Shapes: 570

File Bounds: (  -71.875211,   42.169240)
         to  (  -71.035596,   42.721890)

Divided into 3 rows X 3 columns (dx, dy) = (    0.279872,    0.184217)

  Cell     Count      XMin         YMin         YMax         YMax
[  0,  0]      2   -71.875211    42.169240   -71.595339    42.353457
[  0,  1]     98   -71.595339    42.169240   -71.315468    42.353457
[  0,  2]     27   -71.315468    42.169240   -71.035596    42.353457
[  1,  0]      1   -71.875211    42.353457   -71.595339    42.537673
[  1,  1]    113   -71.595339    42.353457   -71.315468    42.537673
[  1,  2]    158   -71.315468    42.353457   -71.035596    42.537673
[  2,  0]     43   -71.875211    42.537673   -71.595339    42.721890
[  2,  1]     81   -71.595339    42.537673   -71.315468    42.721890
[  2,  2]     46   -71.315468    42.537673   -71.035596    42.721890
[ global]      1   -71.875211    42.169240   -71.035596    42.721890
        --------
             570

Example with points:

[woodbri@linus us]$ shp2tile -w -r 3 -c 3 us_pop2k                          
Shapefile Type: Point   # of Shapes: 26907

File Bounds: ( -176.695584,   17.970810)
         to  (  -65.305670,   71.268260)

Divided into 3 rows X 3 columns (dx, dy) = (   37.129971,   17.765817)

  Cell     Count      XMin         YMin         YMax         YMax
[  0,  0]    122  -176.695584    17.970810  -139.565613    35.736627
[  0,  1]    779  -139.565613    17.970810  -102.435641    35.736627
[  0,  2]   6154  -102.435641    17.970810   -65.305670    35.736627
[  1,  0]      4  -176.695584    35.736627  -139.565613    53.502443
[  1,  1]   2705  -139.565613    35.736627  -102.435641    53.502443
[  1,  2]  16851  -102.435641    35.736627   -65.305670    53.502443
[  2,  0]    251  -176.695584    53.502443  -139.565613    71.268260
[  2,  1]     40  -139.565613    53.502443  -102.435641    71.268260
[  2,  2]      0  -102.435641    53.502443   -65.305670    71.268260
[ global]      1  -176.695584    17.970810   -65.305670    71.268260
        --------
           26907

Example showing that DBF info is copied into the new files:

[woodbri@linus test]$ dbfdump --info minroads 
Filename:       minroads.dbf
Version:        0x03 (ver. 3)
Num of records: 66002
Header length:  449
Record length:  127
Last change:    1995/7/26
Num fields:     13
Field info:
Num     Name            Type    Len     Decimal
1.      TLID            N       10      0
2.      NAME            C       40      0
3.      CFCC            C       3       0
4.      FRADDL          N       11      0
5.      TOADDL          N       11      0
6.      FRADDR          N       11      0
7.      TOADDR          N       11      0
8.      ZIPL            C       5       0
9.      ZIPR            C       5       0
10.     CLASS           C       1       0
11.     RTE_NUM         C       6       0
12.     ALTNAME         N       6       0
13.     ALTADDR         N       6       0
[woodbri@linus test]$ dbfdump --info tt-1    
Filename:       tt-1.dbf
Version:        0x03 (ver. 3)
Num of records: 1325
Header length:  449
Record length:  127
Last change:    1995/7/26
Num fields:     13
Field info:
Num     Name            Type    Len     Decimal
1.      TLID            N       10      0
2.      NAME            C       40      0
3.      CFCC            C       3       0
4.      FRADDL          N       11      0
5.      TOADDL          N       11      0
6.      FRADDR          N       11      0
7.      TOADDR          N       11      0
8.      ZIPL            C       5       0
9.      ZIPR            C       5       0
10.     CLASS           C       1       0
11.     RTE_NUM         C       6       0
12.     ALTNAME         N       6       0
13.     ALTADDR         N       6       0
[woodbri@linus test]$
```
