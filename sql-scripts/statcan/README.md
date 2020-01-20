# Canada National Road Network

You can use the GIS data from Statistics Canada from geocoding, reverse geocoding, mapping, and/or routing. Download the data from https://www12.statcan.gc.ca/census-recensement/2011/geo/RNF-FRR/index-eng.cfm or google "statcan canada NRN data"

Download the data and unzip the files into a directory ``shape`` and then modify the ``load-statcan-data.sh`` to load it into a postgresql database.

There are sql files in ``rgeo`` and ``geocoder`` that can adapted to prepare the data for for one or the other.

```
$ ls /mnt/u/srcdata/statcan/
docs                       NRN_RRN_MB_6_0_SHAPE.zip   NRN_RRN_ON_12_0_SHAPE.zip
Documentation.pdf.zip      NRN_RRN_NB_8_0_SHAPE.zip   NRN_RRN_PE_17_0_SHAPE.zip
load-statcan-data.sh       NRN_RRN_NL_7_0_SHAPE.zip   NRN_RRN_QC_9_0_SHAPE.zip
NRN_RRN_AB_14_0_SHAPE.zip  NRN_RRN_NS_14_1_SHAPE.zip  NRN_RRN_SK_10_0_SHP.zip
nrn_rrn_ab_shp_en.zip      NRN_RRN_NT_10_0_SHAPE.zip  NRN_RRN_YT_14_0_SHAPE.zip
NRN_RRN_BC_14_0_SHAPE.zip  NRN_RRN_NU_7_0_SHAPE.zip   shape
```

