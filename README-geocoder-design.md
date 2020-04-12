#Design and Construction of a Geocoder

* Author: Stephen Woodbridge
* Date: 2014, updated 2020.
* stephenwoodbridge37 (at) gmail (dot) com

## Introduction

A geocoder is software thati takes an address and converts it to a location like a latitude and longitude. It is one of many tools in a geospatial tool box that this article is focused on. Since Google has popularized and to some extent commoditized mapping and geospatial tools lots of people have built application based on these tools. Unfortunately, these are black box proprietary tools and not ideal if you want to host your solutions on your servers your control. There are other options that we will discuss briefly but the primary goal here it to discuss how to build your own geocoder. While the bulk of this article will discuss generic issues around geocoding and how to solve them, our discussion will be focused on a geocoder that is a PostgreSQL/PostGIS extension. This article will still be useful for reads interested in building a geocoder for another environment.

A little bit about the author. I have been writing and maintaining geocoders for the last 20+ years. My first geocoder was written in 1999 based on Tiger shapefiles that I created for mapping.  This worked well but I had to implement the entire search and indexing based on reading data from 3300 county shapefiles for of disk. When the US Census finally released data in shapefiles, these were too different from what my code expected and it died and I have since then written a new SQL based geocoder described here, which is OpenSource. I have also written various geocoders using Navteq and TeleAtlas data, Canadian StatCan data and Canadian Geobase. So, when I designed my next geocder my goals were to write one generic geocoder that I could use for any of these problems. This article hopefully will convey and educate you on the things I learned and help you avoid some of the learning pains I have had.

## How does a geocoder work?

As I mention, a geocoder is software that takes an address and converts it to a location like a latitude and longitude. The idea is very simple; we start with a reference set of data that represents the universe of addresses that we want to search that has spatial information associated with it. We then get a query address and search our reference set and extract the associated spatial data and have our answer. What could go wrong?

To start with addresses are natural language and as such are subject to all vagueness of natural language like abbreviations, local names, typographical errors, data entry errors, and the list goes on and on.  Another related issue is parsing an address into components like the house number, street name, city, state, country, postal code, etc. and dealing with things like apartment, building, and other extraneous components. An address might really define the intersection of two streets.

Ultimately, a geocoder is a matching engine. It must match a query address against a set of reference addresses and find the best result. Because of the issues of natural language and the issues identified above it is very easy to end up matching ``apples`` against ``oranges`` and to get poor results. The solution is to standardize the reference address and to standardize the query address the same way, and then we are matching the query against the reference using a common standardization. A lot of our content will be focused on the specific issues and how to overcome them.

## Existing Geocoders

So before we design yet another geocoder, it might be prudent to survey some of the existing geocoders. I make no value judgments on these. I looked at a few of these briefly and decided that they were not suitable for my needs for whatever reason.

*   PAGC – Postal Address Geo Coder  - http://pagc.sourceforge.net/
*   PostGIS Tiger Geocoder - http://postgis.net/docs/Extras.html
*   Geocoder.us  - https://metacpan.org/pod/Geo::Coder::US
*   OpenStreetMap Nominatim - http://www.nominatim.org/
*   Febrl (Freely Extensible Biomedical Record Linkage) - http://sourceforge.net/projects/febrl/
*   Oracle Spatial Geocoder
*   Other random geocoders for reference - http://en.wikipedia.org/wiki/List_of_geocoding_systems http://geoservices.tamu.edu/Services/Geocode/OtherGeocoders/

This list is by no means comprehensive and I sure there are many other geocoders that are not listed here.

## PAGC – Postal Address Geo Coder

This project has some very interesting and excellent technology built into it. It was designed around working with small county wide data sets and did not scale well to national data sets. One of the key technology pieces of this project is the address standardizer. I extracted that code and create a simple Postgresql extension that made the address standardizer accessible through SQL queries in the data. This source code for this extension has been forked into the PostGIS source tree and we will be using it as the address standardizer and it will be discussed in detail below.

## PostGIS Tiger Geocoder

The PostGIS Tiger Geocoder is written as stored procedures in a PostgreSQL/PostGIS database and works with Tiger data. This would get replaced with the geocoder that this article discusses. The current implementation is very Tiger specific and does not support other data sets. The address standardizer is written in plpgsql and is not easily extensible and is quirky with various issues.

## Geocoder.us  

This is a Perl module that was built to provide geocoding based on Tiger data. The web site has been setup as a service. The Perl module for the geocoder is available CPAN.org.

## OpenStreetMap Nominatim

This geocoder is based on OSM (Open Street Map) data. It requires OSM data and a lot of compute resources to load the data for the planet. My limited use of it indicates that the results are highly variable but I have not used it extensively so I will not comment further.

## Febrl - Freely Extensible Biomedical Record Linkage

This software package was developed to do data standardization and cleaning and probabilistic record linkage (``fuzzy`` matching). Since geocoding is basically matching in input record, ie. Address, against a reference data set of address records it can be used to do geocoding. This is interesting because it uses some very high tech statistical methods that are not found in many other geocoders.

## Oracle Spatial Geocoder

While not Open Source, I want to include this geocoder because it works very well with Navteq data with good coverage in North America and it Europe.  I would like emulate the quality and performance of this geocoder in any geocoder I design.

## Design Goals

The design goals for this project are:
*   One code base that can be used for multiple data sets
*   Support for at least North America and Europe
*   Portability between Linux and Windows
*   Fast (25-35 ms per request on average) when used with national data sets with 40-80 million records.
*   Geocoder will run as a stored procedure in a PostgreSQL database.

These goals were targeted to overcome the short comings of previous geocoders that I wrote and to build on the lessons and research that I``ve done. The follow sections cover a lot of the issues and solutions. Each section deals with a critical component and discusses them in detail.

The last goal to run as a stored procedure in a PostgreSQL database is an implementation issue and there is nothing stopping you from doing a different implementation. The design concepts are mostly generic and implementation independent. The database provides a lot of facilities like data storage, disk caching, indexing, and searching built into it and by using these we can avoid a lot of additional implementation effort. The database also provides use with portability between Linux and Windows.

## Reference Data

Good geocoding requires having good data to match against. This can be in the form of parcel data or street segments with address ranges. Data can be sourced from a variety of potential sources from commercial vendors like Navteq or TeleAtlas or from public or governmental sources like StatCan or GeoBase for Canada, Tiger from the US Census, or from other local country sources. What we need are the basic fields of:
*   House number or house number ranges along a street segment
*   Street name
*   City name
*   State and/or Country
*   Spatial location

Basically we need the reference data to have the fields that we want to match against along with the spatial location of the parcel or street segment.

It is also possible to geocoder against landmarks, points of interest, place names, postal code centroids, city centroids, etc., if you have that data by adapting your queries to these.

When you get data from some source you will find that it might be already parsed neatly into detailed fields like:
*   Right_address_from
*   Right_address_to
*   Left_address_from
*   Left_address_to
*   Qualifier_prefix
*   Direction_prefix
*   Prefix_type
*   Street_name
*   Suffix_type
*   Direction_suffix
*   Qualifier_suffix

For example, the US Census Tiger data has does this. It is tempting to use this parsed data to save the effort of parsing it ourselves, but this is a bad idea. The fact is that we will still need to parse our incoming query address so we will need to have a parser regardless and if the arbitrary way that the data vendor parsed the fields just happens to be slightly different than the way our parser parses the same address then we will not match the records. This is discussed in detail in the ``Address Standardization`` section next.

Other issues aside, your geocoder can be no better than the reference data you are using. Nothing will replace bad or missing data. 

## US Census Tiger Data

Tiger data is a mixed bag because for the coverage area and the cost of downloading it, it is hard to beat. On the other hand, the US Census has to comply with Title 13 that basically says the Tiger data cannot uniquely identify any single residence to specific census demographics data.  To accomplish this, the address ranges on street segments have been fuzzed. An example of this is that most short streets that have less than 100 houses on them will have address ranges of 1-99 an 2-100 regardless of what the actual house number ranges are. The Tiger quality has been greatly improved in the years leading up to the 2010 Census. The US Census spend something like $200M to update the national road map by digitizing high resolution satellite imagery and aligning the existing road network with these images. They also renewed their partnerships with the local stakeholders to update their data. Tiger updates are now back to annual updates to the network. Census continues to make year updates to the data around the August timeframe each year.

## Google Data

Google used to use Navteq data and other commercial data vendors for their maps and geocoding, but now they have their own data collection and GIS system that is used to collect StreetView imagery and to keep their GIS data up to date. This data is not available to the public except via the services they provide through the GMaps API. I mention it here because everyone compares geocoder results to what Google does and it is good to understand what you will be compared against.

## Navteq, TeleAtlas et al.

These commercial vendors have been providing high quality data for mapping, geocoding and driving direction applications for years. They drive the roads to collect data and have a process for integrating changes to the road network. The data from these vendors goes directly into GPS, vehicle navigation systems and various geospatial web sites. I have worked extensively with both Navteq and TeleAtlas data. One of the goals it to be able to have the geocoder work with data from either of these vendors.

## Other Data Sources

There are other data sources, but you need to evaluate the data before you invest in something that might not be suitable. Some of the things that should be on your evaluation check list are:
*   Road name coverage, ie: number of roads that have actual and valid road names
*   House number coverage
*   Coverage of city, state, postcode on the records
*   Is the data delivered in a format that is easy for you to work with?

Get a sample for data from an urban area and from a rural area. The rural area data is often a lot less useful than data for urban areas.

## Data Loading

Source data comes in a large variety of formats and schemes. Each vendor has their own standards. To be able to make a geocoder that works across multiple vendor data sets, we will target a database table schema that is somewhat vendor agnostic. This means that data sets will need some kind of loading process that converts vendor data into our vendor agnostic table structures. Over time we should collect these processes for common used vendor data sets.

## Address Standardization

The address standardizer is probably the most critical and complex part of any geocoder. It breaks down an address into tokens, classifies the tokens, and then based on some rules outputs the tokens grouped into standardized fields. There are a lot of ways to do this and our focus is not to design and build an address standardizer, but rather to integrate an existing address standardizer into our geocoder as part of the data processing. As mentioned above, the Address Standardizer has been fork out of the PAGC Geocoder project and been wrapped into a PostgreSQL extension. If someone wanted to it could be wrapped into other languages like Perl, Python, or called directly from C/C++ code.

We will go into some detail on how the address standardizer works, how to install it, how to configure it, and how to invoke it.

*UPDATE:* Since I originally wrote this and built the SQL Geocoder, I have written an new Address Standardize in C++ that is much more user friendly, supports UTF-8, and multiple countries. It is largely compatible with the PostGIS address_standardizer extension and can be found here (https://github.com/woodbri/address-standardizer).

## How does it work?

An address is made up of the following components: [house number | street name | city | state | postal code| country]. There may be additional tokens like: [building | box number |unit designator | junk]. And ``street name`` can even be classified into finer grain tokens like: [prefix qualifier | prefix directional | prefix type | street | suffix type | suffix directional | suffix qualifier]. We use three tables of data, a lexicon, a gazetteer, and a rules table that allow us to configure the address standardizer to work with whatever data we are trying to parse. There is a sample set that works well most North American data sets, but should probably be customized for optimal performance. 

The gazetteer contains words that are related to recognizing place names. This can be used to expand abbreviations, provide for mapping alias names to standard names like mapping ``Manhattan`` to ``New York`` or ``NYC`` to ``New York``. In addition to the mapping functionality it also allows these tokens to be classified, which we will go into in more detail later.

The lexicon is identical to the gazetteer in structure but it has a broader focus in identifying all the other tokens that we might come across when parsing and address. For example it might detect the word ``BOX`` and tag it as start of a PO Box string. But because the word might also be found in other circumstances like ``Box Car Alley`` we might also want to tag is as being a plain ``WORD``. It also provides a mapping function like in the gazetteer for providing a standardized string for the token like mapping ``ST`` to ``Street`` or ``ST`` to ``Saint``. So it provides a mechanism where the user can define tokens that might be found, provide a standardized string for the token, and classify the token.

And finally the rules table that contains lists of input tokens and a mapping to output tokens and a probability that is used to decide when conflicts are encountered. For example, take these addresses:
```
123 north main st
123 n main street
234 south oak ave
```

They might all be parsed into these input tokens:
```
Number  Direction Word Type
```

And then would get mapped by the rules to these output tokens:
```
House Predir Street Suftyp
```

This should give you an overview of how the address standardizer works and we will go into more detail on how to work with these tables to customize them to a specific project's needs.

## How do I use the address standardizer?

There are currently two function calls that can be used to access the address standardizer.
```
select * from standardize_address('lex', 'gaz', 'rules', '123 main st boston ma 02001');
select * from standardize_address('lex', 'gaz', 'rules', '123 main st',  'boston ma 02001');
```

The first form takes the address as a single string and splits it into street part and the location part and then calls the standardizer. The second form assumes you already have the address and location parts separated. The address is processed as we described above and the tokens are processed, standardized, classified and out put into the following fields.
```
 building | house_num | predir | qual | pretype | name | suftype | sufdir | ruralroute | extra |  city  |     state     | country | postcode | box | unit
----------+-----------+--------+------+---------+------+---------+--------+------------+-------+--------+---------------+---------+----------+-----+------
          | 123       |        |      |         | MAIN | STREET  |        |            |       | BOSTON | MASSACHUSETTS |         | 02001    |     |
```

You might notice that ``st`` has been standardized to ``STREET`` and ``ma`` has been standardized to ``MASSACHUSETTS``. This was done based on the mapping in lex and gaz tables that were passed to ``standardize_address``.

## The Gazetteer Table
TBD

## The Lexicon Table
TBD

## The Rules Table
TBD

## Customizing the Address Standardizer
TBD

## Single Line Address Parsing

We just saw an example of the single line parsing when we passed the full address as a single string to ``standardize_address`` in the example above. There is function that allows us to access this directly.
```
select * from parse_address('123 main st boston ma 02001');
```

That returns a record as follows:
```
 num | street  | street2 |  address1   |  city  | state |  zip  | zipplus | country
-----+---------+---------+-------------+--------+-------+-------+---------+---------
 123 | main st |         | 123 main st | boston | MA    | 02001 |         | US
```

This can also handle intersections when ``@`` is used as the separator between the two streets of the intersection. For example:
```
select * from parse_address('middle st @  main st boston ma 02001');
```

Returns:
```
 num |  street   | street2 | address1 |  city  | state |  zip  | zipplus | country
-----+-----------+---------+----------+--------+-------+-------+---------+---------
     | middle st | main st |          | boston | MA    | 02001 |         | US
```

The single line parsing function is not perfect because there are a lot of ambiguous cases and it is currently focused on parsing addresses in the United States so it may not work in other locals as expected. Future work will be on redesigning this so work with the same mechanisms that are used for the address standardizer. Field ``address1`` is just a convenience field and is a concatenation of the ``num`` and ``street``.

## Fuzzy Searching

Fuzzy searching is the ability to search for records that are ``near`` matches. This might help you find results when you have a misspelling or maybe you entered an address based on hearing something in a noisy environment and you have something close to what was said but not exactly. There are a lot of techniques and tools for doing this. You can create a trie index or use soundex and metaphone or double metaphone. I experimented with a bunch of these and I find for my purposes a modified double metaphone works pretty well. Before explaining that, let take a quick review of some of the other methods. 

*Trie's* are created by taking all possible 3 character sequences of your word(s). For example, for the word ``middle`` we would create ``mid``, ``idd``, ``ddl``, and ``dle``. Then when searching we would look for words that contain these tries, and the more of the tries that they contain the high the ranking. You can also penalize the score for tries that one has and the other does not. I find this to be too complex and difficult to manage, although postgresql does have an extension to make it easy to create and search based on this technique. My experience when testing it is that it generated too many false positives that were not good choices.

*Soundex* has been around for ages and it takes a word and generates a code based on the first letter and the following consonants. The problem with this is that if you get the first letter wrong then it totally fails. Think of words like ``fish``, ``phish`` or ``pride`` and ``bride``.

*Metaphone* and its successor double metaphone generate a phonic key for the word. It analyzes the word and based on how the word sounds taking into account the some letters can be hard or soft depending on what letters are around them. Double metaphone fixed some problems in the original metaphone key generation and added the ability to create an alternate key for some words.

For out purposes we will use double metaphone but only use the primary key that generates. To implement fuzzy searching, we will create an indexed column using double metaphone primary key based on the base name for the street and if we fail to generate a key, then we will use the original word. This later constraint allows us to store route numbers which will fail to generate a metaphone key as our fuzzy index. When we do a fuzzy search, we create a double metaphone key for the query street name and search based on the fuzzy index to select candidate records. Later we will discuss how to score these records to find the best candidates for our results.

## Overall Design

Now that we have looked at some of the tools that we have to work with, our next step will be to look at how we assemble these tools to work with our data. In this section we will look at how we structure our data so it can be efficiently processed and queried. At a high level our geocoder will look something like this:
```
[standardized streets] <- [intermediate agnostic table] <- [raw data]
```

There may be additional tables to support searching for intersections, or other queries that we might want to support, but the above is the core to the geocoder.

The standardized streets table will look very much like an extension of the standardize_address() functions output. This means that to geocode a query address, we can pass the address to standardize_address() and then the output will align with our standardized streets table and a query is reasonable straight forward.

Here is the schema for the standardized streets table:
```
CREATE TABLE stdstreets
(
  id integer NOT NULL,
  building character varying,
  house_num character varying,
  predir character varying,
  qual character varying,
  pretype character varying,
  name character varying,
  suftype character varying,
  sufdir character varying,
  ruralroute character varying,
  extra character varying,
  city character varying,
  state character varying,
  country character varying,
  postcode character varying,
  box character varying,
  unit character varying,
  name_dmeta character varying,
  CONSTRAINT stdstreets_pkey PRIMARY KEY (id)
)
WITH (
  OIDS=FALSE
);
```

You can see that this table is identical to the output for standardize_address with the addition of an ``id`` column on the front for linking the record to the intermediate table and the ``name_dmeta`` append to the end for doing fuzzy searching.

The intermediate agnostic table format is designed to be somewhere between the standardized street table and the raw vendor data. It is also the place that we want to store the spatial data because we do not want it in the standardized street table for performance reasons and we only need to use it in the final step of generating a location for the address. The primary goals in designing this table are

1.  It has a one-to-one record relationship with the standardized streets table
2.  The structure is simple but robust enough that we can easily transform most raw data sets into it
3.  We will need access to this table for the final step of geocoding to score the results and to compute the location.

There can be some flexibility in the design of this table in the area of how house number ranges or parcel address values are handle because the code that works with these fields is localized and only comes into play after the initial search of candidate records occurs. One table schema for the intermediate table is:
```
CREATE TABLE streets
(
  gid serial NOT NULL,  -- unique table reference and link to stdstreets table
  link_id numeric(10,0),  -- id of original record that generated this record
  refaddr numeric(10,0),  -- house number range from
  nrefaddr numeric(10,0),  -- house number range to
  name character varying(80),  -- street base name
  predirabrv character varying(15),
  pretypabrv character varying(50),
  prequalabr character varying(15),
  sufdirabrv character varying(15),
  suftypabrv character varying(50),
  sufqualabr character varying(15),
  side smallint,  -- which side of the street are we dealing with
  tfid numeric(10,0),  -- reference to the topological polygon that this side of the street is associated with
  usps character varying(35),  -- USPS preferred cisty name
  ac5 character varying(35),  -- village|hamlet
  ac4 character varying(35),  -- city
  ac3 character varying(35),  -- county
  ac2 character varying(35),  -- state
  ac1 character varying(35),  -- country
  postcode character varying(11),
  the_geom geometry(MultiLineString, 4326),
  CONSTRAINT streets_pkey PRIMARY KEY (gid)
)
WITH (
  OIDS=FALSE
);
```

As mentioned earlier, this table might have additional column that might be useful.

**Note: need to identify the minimum list of required fields we expect to be here.**

Raw data is what we get from a vendor source. It will be different for all vendors. It is nearly impossible to build a geocoder that can work with this raw data directly and still be portable across vendors and data sets. One of the first steps for loading data is to transform it into some intermediate standard table format. We will not go into detail of how to do this because it will be different for each dataset, but if we define the intermediate standard table then this should give you a clear target definition for that translation.

Raw vendor data often has a few aspects associated with it that are not compatible with our standardized streets table. One is the location data (city, state, postal code) may be referenced via ids that require a table join to resolve. In fact, Navteq data has a complex reference system that cannot be resolved via a simple join. Also there may be multiple location names associated with a give street or side of a street. And finally, street data is often presented with both right side and left side address ranges and location references because for streets that follow administrative boundaries the right and left side may not be in the same locality. Furthermore, when we are looking at postal delivery addresses there are address that must be address to an adjacent locality because that it the post office that serves them.

## Query Planning and Processing

In this section we look into the details of what our queries will look like and how to dynamically construct them based on the input we have. Earlier we suggested that if we just standardized our query address that we could then just query the standardized streets table based on that. While this idea if correct there are still a lot of details to work out. Our strategy needs to be based on what data we actually have for the query address. For example, what if the query address is missing the postal code but the reference data has a postal code. If we do a straight query then this field will not match and we fail to find a desired record. This could be true for multiple fields. The way to think about geocoding is in two phases:

1.  Cast a wide net that selects a super set of the records we are interested in
2.  Then filter that set and score each record based on it match the request

To cast a wide net, we can eliminate fields that are not critical to the search. For the street component all we really need is the street name or the metaphone key for the street name. For the location portion of the address we have some potential redundancy in that we can search based on city,state or postal code or city, state, postal code. In fact we might want to generate multiple queries because if we fail on the city component, we might succeed on the postal code component.

We might generate series of queries like the following:

0.  query for exact terms input, ignoring building, extra, box, and unit
1.  try 0 with postcode only or state only, if no postcode
2.  try 0 with city, state only
3.  then loosen up on house_num, predir, sufdir, qual, pretype, suftype
4.  try 3 with postcode only or state only, if no postcode
5.  try 3 with city, state only
•   only if the request wants fuzzy searching then add
6.  then try searching on metaphone name
7.  try 6 with postcode only or state only, if no postcode
8.  try 6 with city state only

In this sequence of queries we go from exact to general as we cast a wider net. Since every potential geocode request can have different input terms it makes sense to dynamically generate the queries above based on the specific data needed. We could generate all the queries above and execute them one at a time and stop when we have a result that we think is good enough with the idea that this would speed things up. In fact because of query caching and page caching, we probably will not lose much time of we just UNION all the queries together and filter out duplicate record and this saves us from having to decide what is ``good enough``. We still have a lot of records and need some way to prioritize them so we can present the best ones first. We do that by scoring the results as we discuss in the next section.

## Scoring Results

Geocoders often present you with a score that indicates if they found an address or postalcode or a city-state. Some also give you a quality score that indicates how well the result matched the query address. Since we are only searching for address matches we will always return and address or fail. We are not coding a fallback to postal code centroid or city-state centroid, although this could be built and our address geocoder could be called from that.

When we do a geocode search as we have describe above, we get a list of candidate records and we need to rank them from good to bad. To do this we develop a scoring mechanism that will assign a score based on how well a given reference candidate matches our query address. We want to compare the input query terms against the candidate records and we can use a Levenshtein distance to compute and error score. Because some terms are more valuable in determining the score, we probably want to have different weights for the different terms. For example, how close we match the street name would have more value the say matching or not on the directional or the street type.

* For predir, sufdir, pretype, suftype and qual error we multiple by weight ``w1``.
* For name errors we multiply by weight ``w2``.
* For city, state, postcode, and country errors we multiply by weight ``w3``.

We sum the weighted errors for each term and then subtract that from 1.0 to generate a score for each candidate record and sort the resulting records by this score so the best record is returned first.

## Loading Tiger Data

See:

* [README-geocoder.md](./README-geocoder.md)
* [tools/README.md](./tools/README.md)


