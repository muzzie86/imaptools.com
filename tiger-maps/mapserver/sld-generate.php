<?php

/*****************************************
* use this script to generate an SLD for your MapServer layer
******************************************/

// define variables
define( "MAPFILE", "D:/ms4w/apps/benchmarking/mapserver/spain-shapefiles.map" );

// open map
$oMap = ms_newMapObj( MAPFILE );

// get the parcel layer
$oLayer = $oMap->getLayerByName("point_geometry");

// generate the sld for that layer
$SLD = $oLayer->generateSLD();

// save sld to a file
$fp = fopen("sld/point-geom-sld.xml", "w");
fputs( $fp, $SLD );
fclose($fp);

?>
