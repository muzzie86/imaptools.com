#!/usr/bin/perl

# I believe the orginal script was written by Steve Lime. -sw
# Modifications by Stephen Woodbridge

use mapscript;

sub Usage() {
  print "Usage: project.pl [from projection] [to projection] [in shapefile] [out shapefile]\n";
  exit 0;
}

&Usage() unless $#ARGV == 3;

#
# open the shapefiles
#
$in_shapefile = new mapscript::shapefileObj($ARGV[2], -1) or die "Unable to open input shapefile.";
$out_shapefile = new mapscript::shapefileObj($ARGV[3], $in_shapefile->{type}) or die "Unable to open output shapefile.";

#
# create a couple of projection objects
#
$from_projection = new mapscript::projectionObj($ARGV[0]) or die "Unable to initialize \"from\" projection.";
$to_projection = new mapscript::projectionObj($ARGV[1]) or die "Unable to initialize \"to\" projection.";

#
# make a copy of the dBase file
# 
system("cp ". $ARGV[2] .".dbf ". $ARGV[3] .".dbf");

#
# and finally loop through the shapefile
#

$shape = new mapscript::shapeObj($in_shapefile->{type});
for($i=0; $i<$in_shapefile->{numshapes}; $i++)
{  
  $status = $in_shapefile->get($i, $shape);
  die "Error reading shape $i." unless $status == $mapscript::MS_SUCCESS;
  $shape->project($from_projection, $to_projection);
  $out_shapefile->add($shape); 
}

undef $out_shapefile;
exit;

