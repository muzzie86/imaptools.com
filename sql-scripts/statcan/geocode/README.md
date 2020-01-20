# STATCAN National Road Network - NRN

You can download the shapefiles from here:
* https://open.canada.ca/data/en/dataset?portal_type=dataset&q=nrn

## Steps to Load data

* Review, edit and run ``load-statcan-data.sh``
* Create a lexicon for the data with
  ```
  psql -d dbname -c "select mk_statcan_lex()" | sort -u > lex-statcan.txt
  ```
  * edit lex-statcan.txt to clean it up, move the last line to the first
  * combine duplicate lines with DET_SUF and DET_PRE by changing it to DETACH
  * add abbreviations as needed
  * look at the address standardizer sample lexicon for canada for other ideas
* copy nid, housenumber, stname, cleaned(placenam), datasetnam, 'canada' to address table
* parse address table using lexicon
* compute statistics on patterns
* build grammar to tanslate patterns

