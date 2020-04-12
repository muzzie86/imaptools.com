# iMaptools SQL Geocoder

The iMaptools SQL Geocoder for Tiger or Navteq data provides SQL callable functions for the PostgreSQL database. This software is provided under an MIT license.

## System Requirements

* Install PostgreSQL, PostGIS, address_standardier, and fuzzystrmatch extensions
* Disk space at least 80GB free space to install a pg_dump
* Disk space about 150GB to download and prepare the data

I strongly reccomend that you at least double those numbers because running out of disk causes a lot of other problems and disk is in general very cheap.

## Installation

This requires the following steps:

* Install PostgreSQl and PostGIS
* Compile the tools in this repository
* Download shapefile data from Census or HERE
* Prepare and load the shapefile data into the database
* Prepare the loaded data in the database
* Make a pg_dump of the geocoder database for backup or to move to a production server.

Instructions for most of these steps can be found in this repository.

## Overview

Many geocoders that you might have used in the past are probably black boxes, where you stuff and address into and get back a result. One of the problems with this is that you don``t have much control of what is happening or the results you get. Many of these will fail to find an address and instead return zipcode or city centroid. While this might be acceptable for some use cases, it is not always acceptable. This geocoder has functions for geocoding addresses, intersections, postal code centroids or city centroids, but the function for addresses will give you an address of fail it will never return a result that is not an address. Likewise, the other functions will only return the result that is requested or fail also.

What this means is that you have to take a little more responsibility for example if you have and address and the address query fails you have to decide if postal code centroid is ok or not and perform an additional query. Or maybe the address request is not a house but an intersection, and then you have to call the intersection finder function. I will provide examples below of the various cases. At some point we might add the black box like function for those that do not care.
Geocoding is a matching problem. We have a reference data set in this case the Navteq data. We standardize every record in the reference data set, and then later when we get a geocode request we standardize the request input and match that against the reference data set. This will hopefully give us some candidate records from the reference data set, and we then score them and return potential candidates in best to worse scored order. You will see references to standardizer below and that it uses the ``lex``, ``gaz``, and ``rules`` tables to do the standardization. It is important that you do not change these tables without re-standardizing the reference data set. If you change these tables then say the references data set standardize something to ``A`` but now the changed tables might standardize the same thing now to ``B``, so when we try to match ``A`` in the reference to ``B`` in the input data it will fail. We have done a lot to make sure that the standardizer works correctly for the data we provide. If you feel that there is some need to make changes it would be best to contact us and discuss the changes before you break the geocoder.

## Geocoder Public Functions

If you browse the database with something like pgAdmin3 you will notice that there are other functions that are not documented here. Some of these are internal utility functions that the geocoder is built on and some are for preparing the data in the database and others are for batch geocoding of a table. This later case has too many assumptions about column names to be made generic, but it might be helpful as a model if you need to do this. I do not recommend running any of the undocumented functions as some of them drop tables and might otherwise damage the working database.

I``m also going to document the address_standardizer functions here because I wrote these also and they are used in the geocoder and you can use them to prepare your data for geocoding.
tiger2019_geo=# \df data.
                                                                                      List of functions
 Schema |             Name             |    Result data type    |                                                Argument data types                                                 |  Type
--------+------------------------------+------------------------+--------------------------------------------------------------------------------------------------------------------+--------
 data   | imt_compute_xy               | record                 | id integer, num text, aoff double precision, OUT x double precision, OUT y double precision                        | normal
 data   | imt_geo_add_where_clauses    | text                   | inp stdaddr, plan integer                                                                                          | normal
 data   | imt_geo_planner              | SETOF text             | inp stdaddr                                                                                                        | normal
 data   | imt_geo_planner2             | text                   | inp stdaddr, opt integer                                                                                           | normal
 data   | imt_geo_score                | SETOF geo_result       | sql text, sin stdaddr, aoff double precision                                                                       | normal
 data   | imt_geo_score2               | SETOF geo_result       | sql text, sin stdaddr, aoff double precision                                                                       | normal
 data   | imt_geo_test                 | SETOF text             | myid integer                                                                                                       | normal
 data   | imt_geo_test2                | SETOF text             | myid integer                                                                                                       | normal
 data   | imt_geocode_intersection     | SETOF geo_intersection | micro1_in text, micro2_in text, macro_in text, fuzzy integer, method integer                                       | normal
 data   | imt_geocoder                 | SETOF geo_result2      | micro_in text, macro_in text, fuzzy integer, aoffset double precision, method integer                              | normal
 data   | imt_geocoder                 | SETOF geo_result2      | sfld stdaddr, fuzzy integer, aoffset double precision, method integer                                              | normal
 data   | imt_geocoder_best            | SETOF geo_result2      | sql text, fuzzy integer, aoffset double precision, method integer                                                  | normal
 data   | imt_intersection_point_to_id | integer                | point geometry, tolerance double precision                                                                         | normal
 data   | imt_make_intersections       | character varying      | geom_table character varying, tolerance double precision, geo_cname character varying, gid_cname character varying | normal
 data   | max                          | numeric                | a numeric, b numeric                                                                                               | normal
(15 rows)
```

The source for all of these function can be found in directory ``sql-scripts/geocoder/`` where you will find ``prep-tiger-geo-new.sql`` and ``prep-geo-nt-new.sql`` and various other related script files. 

## Function: parse_address(address text)

The parse_address() function is a utility function that takes an address as a single text string and attempts to break it into components. These components can then be reused to form input to other commands.

Example:
```
select * from parse_address('11 radcliff rd north chelmsford ma 01863-2232');
 num | street      | street2 | address1       | city             | state | zip   | zipplus | country
-----+-------------+---------+----------------+------------------+-------+-------+---------+---------
  11 | radcliff rd |         | 11 radcliff rd | north chelmsford | MA    | 01863 | 2232    | US
```

This command will recognize ``<street name 1> @ <street name 2> <location>`` as an intersection and put the respective names in fields ``street`` and ``street2``. The ``address1`` field is just the concatenation of ``num`` and ``street``. This function is not perfect because there can be a lot of ambiguities and an address like in the example above north could belong to either the city or as a street name directional.


## Function: standardize_address(lex text, gaz text, rules text, micro text, macro text)

This is the core address standardizer from the PAGC geocoder project that is used by the geocoder to standardize the reference data and the incoming geocode requests. It is table driven and uses the ``lex``, ``gaz``, and ``rules`` tables for processing the request. To make things simple we have called the tables ``lex``, ``gaz``, and ``rules``. The ``micro`` field is the house number and street name which is conveniently ``address1`` field from the parse_address() function. The ``macro`` field is the location information. The ``macro`` field can be ``city state``, ``city state zip``, or just ``zip`` and you can optionally include ``US`` or ``USA`` for the country.

Example:
```
select * from standardize_address('lex', 'gaz', 'rules', '11 radcliff rd', 'north chelmsford, ma us');
 building | house_num | predir | qual | pretype | name     | suftype | sufdir | ruralroute | extra | city             | state         | country | postcode | box | unit
----------+-----------+--------+------+---------+----------+---------+--------+------------+-------+------------------+---------------+---------+----------+-----+------
          | 11        |        |      |         | RADCLIFF | ROAD    |        |            |       | NORTH CHELMSFORD | MASSACHUSETTS | USA     |          |     |
```

You do not need to use this function for geocoding because it is called internally in the geocoder. I``m documenting it here because it might be a useful function to help you manage your addresses or address lists.

## Function: imt_geocoder(micro_in text, macro_in text, fuzzy integer, aoffset double precision, method integer)

The function is the address geocoder. It takes ``micro``, the house number and street name and ``macro`` the place name description. ``fuzzy`` can be set to ``1`` or ``0`` to turn on or off fuzzy searching. ``aoffset`` it the amount in meters to offset the location from the center line of the street to the right or left based on which side the address falls on the street. The ``method`` is not used and should be set to ``0`` in case it is used in a future version. This function will standardize the ``micro`` and ``macro`` fields and match it against the reference data set and score the candidate records and return them in best to worst order. Fuzzy matching means that we will widen the search based metaphone keys of the street name in an attempt to match to streets the sounds like the name in the request. This allows us to match records in spite of some simple typos in the request.

Example:
```
select * from imt_geocoder('11 radcliff rd', 'north chelmsford, ma us', 0, 0.0, 0);
 rid | id       | id1      | id2 | id3      | completeaddressnumber | predirectional | premodifier | postmodifier | pretype | streetname | posttype | postdirectional | placename        | placename_usps | statename     | countrycode | zipcode | building | unit | plan | parity | geocodematchcode | x         | y
-----+----------+----------+-----+----------+-----------------------+----------------+-------------+--------------+---------+------------+----------+-----------------+------------------+----------------+---------------+-------------+---------+----------+------+------+--------+------------------+-----------+----------
   0 | 55964714 | 22037362 |   2 | 21014541 | 11                    |                |             |              |         | RADCLIFFE  | RD       |                 | NORTH CHELMSFORD |                | MASSACHUSETTS | USA         | 01863   |          |      | 0    | t      | 0.875            | -71.38932 | 42.61937
   0 | 55964718 | 22037362 |   2 | 21014541 | 11                    |                |             |              |         | RADCLIFFE  | RD       |                 | NORTH CHELMSFORD |                | MASSACHUSETTS | USA         | 01863   |          |      | 0    | t      | 0.875            | -71.38932 | 42.61937
   0 | 55964712 | 22037362 |   1 | 21014541 | 11                    |                |             |              |         | RADCLIFFE  | RD       |                 | NORTH CHELMSFORD |                | MASSACHUSETTS | USA         | 01863   |          |      | 0    | f      | 0.775            | -71.38932 | 42.61937
   0 | 55964716 | 22037362 |   1 | 21014541 | 11                    |                |             |              |         | RADCLIFFE  | RD       |                 | NORTH CHELMSFORD |                | MASSACHUSETTS | USA         | 01863   |          |      | 0    | f      | 0.775            | -71.38932 | 42.61937
```

There is a lot of information returned here most of which you can probably ignore depending on how you are using the data.
* rid - For single address geocodes this will always be 0 (zero).
* id – gid for the record in the streets and stdstreets tables
* id1 – Navteq link_id, this is a back reference to the original Navteq record matched
* id2 – 1 for right side, 2 for left side of the street
* id3 – Navteq area_id
* completeaddressnumber – standardized house number
* predirectional – streets.st_nm_pref value from Navteq
* premodifier – null in Navteq, only used with Tiger data
* postmodifier – null in Navteq, only used in Tiger data
* pretype – streets.st_typ_bef value from Navteq
* streetname – streets.st_nm_base value from Navteq
* posttype – streets.st_typ_aft value from Navteq
* postdirectional – streets.st_nm_suff value from Navteq
* placename – streets.city valuefrom Navteq
* placename_usps – null in Navteq, only used with Tiger augmented data
* statename – streets.state value from Navteq
* countrycode – streets.country value from Navteq
* zipcode – streets.postcode value from Navteq
* building – building designation from standardizer
* unit – within building unit designator from standardizer
* plan – multiple query plans are generated for each request and this is the plan number that returned this result. This is mostly useful for my debugging. If you are interested look at the output of imt_geo_planner2() to see how plans are constructed.
* parity – a Boolean flag to indicate if the address range that was match, matched the parity of the house number. For example if the address range was even and the house number was even then this would be true otherwise false.
* geocodematchcode – This value is an indication of how well the address matched the reference data record being reported. Remember that we are only matching address records here, if we cannot find address records to match then we fail. We will never return a zipcode or city centroid from this query. We compare each component of the request to the corresponding components of the reference record and compute a weighted average score. A score of 1.0 is a perfect match and a score of 0.0 is a perfect none match. The goal here is to rank the candidate records based on best match to worse match. There is no magic number that that says this is good or bad it is all relative ranking.
* x – the longitude of the address
* y – the latitude of the address

## Function: imt_geocode_intersection(micro1_in text, micro2_in text, macro_in text, fuzzy integer, method integer)

This function geocodes an intersection between two streets. ``micro1_in`` and ``micro2_in`` are the street names and ``macro_in`` is the location description. ``fuzzy`` can be set to ``1`` or ``0`` to turn on or off fuzzy searching respectively and will fuzzy match on the street names if on. Again, ``method`` is not used and should be set to ``0`` in case it is used in a future version. It will standardize the input and match it against the references data set scoring the results as in the address geocoder.

Example:
```
select * from imt_geocode_intersection('radcliff rd', 'crooked spring rd', 'north chelmsford, ma us', 0, 0);
 id1      | id2      | id1a     | id2a     | street1      | street2           | placename        | placename_usps | statename     | countrycode | postcode | plan | geocodematchcode  | x         | y
----------+----------+----------+----------+--------------+-------------------+------------------+----------------+---------------+-------------+----------+------+-------------------+-----------+---------
 55964712 | 55968194 | 22037362 | 22038272 | RADCLIFFE RD | CROOKED SPRING RD | NORTH CHELMSFORD |                | MASSACHUSETTS | USA         | 01863    | 6    | 0.861111111111111 | -71.38751 | 42.6202
```

## ISSUES AND BUGS

1. There are a small number of streets names that did not standardize in the ball park of 8000 out of 80,000,000 segments. The 8000 is down from 250,000 in the evaluation data last loaded. I have included a table called ``failures`` which you can look at to see what is in there.
2. As mentioned above, ``parse_address(addres text)`` included with the PostGIS extension is not perfect. The code is somewhat obtuse and the the ``lex``, ``gaz``, and ``rules`` tables are hard to work with. While it is not part of PostGIS, I have created a new [address standardizer project](https://github.com/woodbri/address-standardizer) written in C++ and much easier to work with. It is not included here because I didn't want to add another requirement and additional complexity for users that just want to quickly get a geocoder working.


