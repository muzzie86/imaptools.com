create extension if not exists postgis with schema public;
create extension if not exists fuzzystrmatch with schema public;
create extension if not exists addressstandardizer2 with schema public;

select addgeometrycolumn('rawdata', 'address_default_geocode', 'geom', 4326, 'POINT', 2);

update rawdata.address_default_geocode set geom = st_transform(st_setsrid(st_makepoint(longitude, latitude), 4283), 4326);
-- Query returned successfully: 14247235 rows affected, 21:13 minutes execution time.

create index address_default_geocode_geom_gidx on rawdata.address_default_geocode using gist (geom);
-- Query returned successfully with no result in 08:56 minutes.

cluster rawdata.address_default_geocode using address_default_geocode_geom_gidx;
-Query returned successfully with no result in 24:38 minutes.

vacuum analyze rawdata.address_default_geocode;

-- export lexicon entries ----------------------------------------------------------------

select 'LEXENTRY:\t'||name||'\t'||name||'\tWORD,CITY\tDETACH' from (
    select distinct locality_name as name from rawdata.addresses order by name
) as foo;

select 'LEXENTRY:\t'||name||'\t'||name||'\tWORD,PROV\tDETACH' from (
    select state_name as name from rawdata.state
    union all
    select state_abbreviation as name from rawdata.state
) as foo;

select distinct 'LEXENTRY:\t'||word||'\t'||std||'\tWORD,TYPE\tDETACH' as lex from (
    select code as word, code as std from rawdata.street_type_aut
    union all
    select name as word, code as std from rawdata.street_type_aut
    union all
    select description as word, code as std from rawdata.street_type_aut
) as foo
order by lex;

select distinct street_name from rawdata.addresses where street_name like '_ %' or street_name like '__ %' order by 1;
/*
-- 214
*/
select distinct street_name from rawdata.addresses where street_name like 'ST %' order by 1;
/*
-- 214
*/
select distinct locality_name from rawdata.addresses where locality_name like 'ST %' order by 1;
/*
-- 40
*/
select distinct locality_name from rawdata.addresses where locality_name like '% ON %' or locality_name like '% OF %' order by 1;
/*
-- 40
*/
select distinct locality_name from rawdata.addresses where locality_name like '% VIEW' order by 1;
/*
"VALLEY HEIGHTS"
"VALLEY OF LAGOONS"
"VALLEY VIEW"
*/
select distinct locality_name from rawdata.addresses where locality_name like 'AVENUE %' order by 1;
select distinct locality_name from rawdata.addresses where locality_name like 'PARK %' order by 1;
/*
"PARK AVENUE"
"PARK GROVE"
"PARK HOLME"
"PARK ORCHARDS"
"PARK RIDGE"
"PARK RIDGE SOUTH"
*/
select distinct locality_name from rawdata.addresses where locality_name like 'THE %' order by 1;
/*
"THE ANGLE"
"THE BASIN"
"THE BIGHT"
"THE BLUFF"
"THE BRANCH"
"THE BROTHERS"
"THE CAVES"
"THE CHANNON"
"THE COMMON"
"THE COVE"
"THE DAWN"
"THE DIMONDS"
"THE ENTRANCE"
"THE ENTRANCE NORTH"
"THE FALLS"
"THE FRESHWATER"
"THE GAP"
"THE GARDENS"
"THE GEMFIELDS"
"THE GLEN"
"THE GULF"
"THE GUMS"
"THE GURDIES"
"THE HATCH"
"THE HEAD"
"THE HEART"
"THE HERMITAGE"
"THE HILL"
"THE HONEYSUCKLES"
"THE JUNCTION"
"THE KEPPELS"
"THE LAGOON"
"THE LAKES"
"THE LEAP"
"THE LIMITS"
"THE MARRA"
"THE MEADOWS"
"THE MINE"
"THE NARROWS"
"THE OAKS"
"THE PALMS"
"THE PATCH"
"THE PERCY GROUP"
"THE PILLIGA"
"THE PINES"
"THE PINNACLES"
"THE PLAINS"
"THE POCKET"
"THE PONDS"
"THE RANGE"
"THE RIDGEWAY"
"THE RISK"
"THE ROCK"
"THE ROCKS"
"THE SANDON"
"THE SISTERS"
"THE SLOPES"
"THE SPECTACLES"
"THE SUMMIT"
"THE VINES"
"THE WHITEMAN"
*/
select distinct locality_name from rawdata.addresses where locality_name like 'VALLEY %' order by 1;
/*
"VALLEY BREEZE"
"VALLEY BROOK"
"VALLEY CREST"
"VALLEY FAIR"
"VALLEY FARM"
"VALLEY FIELD"
"VALLEY GROVE"
"VALLEY HEIGHTS"
"VALLEY HO"
"VALLEY LAKE"
"VALLEY MIST"
"VALLEY OF THE GIANTS"
"VALLEY PARK"
"VALLEY PLAIN"
"VALLEY POND"
"VALLEY RIDGES"
"VALLEY SIDE"
"VALLEY VIEW"
"VALLEY VIEWS"
"VALLEY VISTA"
*/

select distinct street_suffix_code, street_suffix_type from rawdata.addresses order by 1;
/*
"CN";"CENTRAL"
"DE";"DEVIATION"
"E";"EAST"
"EX";"EXTENSION"
"ML";"MALL"
"N";"NORTH"
"ON";"ON"
"S";"SOUTH"
"SE";"SOUTH EAST"
"SW";"SOUTH WEST"
"W";"WEST"
"";""
*/

select array_agg(distinct number_first_prefix) from rawdata.addresses;
-- "{A,B,C,D,E,F,G,GC,GE,GF,H,J,K,L,M,N,O,P,POR,R,S,T,U,V,W,X,Y,NULL}"

select array_agg(distinct number_last_prefix) from rawdata.addresses;
-- "{NULL}"

select array_agg(distinct number_first_suffix) from rawdata.addresses;
-- "{A,"\\A",AA,AB,B,BB,BC,C,D,E,F,G,GR,GX,GZ,H,I,J,K,L,M,M1,N,NW,O,OA,P,Q,R,S,SE,T,T3,TT,U,V,W,X,Y,Z,NULL}"

select array_agg(distinct number_last_suffix) from rawdata.addresses;
-- "{14,85,A,B,C,D,E,F,G,GR,H,I,J,K,L,M,N,O,P,R,S,T,U,W,X,Z,NULL}"

/*
[house_number]
NUMBER -> HOUSE -> 0.5
SINGLE NUMBER -> HOUSE HOUSE -> 0.5
NUMBER SINGLE -> HOUSE HOUSE -> 0.5
SINGLE NUMBER SINGLE -> HOUSE HOUSE HOUSE -> 0.7
DOUBLE NUMBER -> HOUSE HOUSE -> 0.5
NUMBER DOUBLE -> HOUSE HOUSE -> 0.5
DOUBLE NUMBER DOUBLE -> HOUSE HOUSE HOUSE -> 0.7
SINGLE NUMBER DOUBLE -> HOUSE HOUSE HOUSE -> 0.7
DOUBLE NUMBER SINGLE -> HOUSE HOUSE HOUSE -> 0.7
NUMBER NUMBER -> HOUSE HOUSE -> 0.6
WORD NUMBER -> HOUSE HOUSE -> 0.7
*/

---------------------------------------------------------------------------------------------

begin;

drop table rawdata.addresses cascade;
create table rawdata.addresses (
    id serial not null primary key,
    address_detail_pid text,
    street_locality_pid text,
    locality_pid text,
    building_name text,
    lot_number_prefix text,
    lot_number text,
    lot_number_suffix text,
    flat_type text,
    flat_number_prefix text,
    flat_number integer,
    flat_number_suffix text,
    level_type text,
    level_number_prefix text,
    level_number integer,
    level_number_suffix text,
    number_first_prefix text,
    number_first integer,
    number_first_suffix text,
    number_last_prefix text,
    number_last integer,
    number_last_suffix text,
    street_name text,
    street_class_code text,
    street_class_type text,
    street_type_code text,
    street_suffix_code text,
    street_suffix_type text,
    locality_name text,
    state_abbreviation text,
    postcode text,
    latitude float8,
    longitude float8,
    geocode_type text,
    confidence int,
    alias_principal text,
    primary_secondary text,
    legal_parcel_id text,
    date_created date,
    building text,
    address text
);
select addgeometrycolumn('rawdata', 'addresses', 'geom', 4326, 'POINT', 2);

insert into rawdata.addresses (
  address_detail_pid ,
  street_locality_pid ,
  locality_pid ,
  building_name ,
  lot_number_prefix ,
  lot_number ,
  lot_number_suffix ,
  flat_type ,
  flat_number_prefix ,
  flat_number ,
  flat_number_suffix ,
  level_type ,
  level_number_prefix ,
  level_number ,
  level_number_suffix ,
  number_first_prefix ,
  number_first ,
  number_first_suffix ,
  number_last_prefix ,
  number_last ,
  number_last_suffix ,
  street_name ,
  street_class_code ,
  street_class_type ,
  street_type_code ,
  street_suffix_code ,
  street_suffix_type ,
  locality_name ,
  state_abbreviation ,
  postcode ,
  latitude ,
  longitude ,
  geocode_type ,
  confidence ,
  alias_principal ,
  primary_secondary ,
  legal_parcel_id ,
  date_created ,
  building ,
  address ,
  geom
)
select  ad.address_detail_pid,
    ad.street_locality_pid,
    ad.locality_pid,
    ad.building_name,
    ad.lot_number_prefix,
    ad.lot_number,
    ad.lot_number_suffix,
    fta.name AS flat_type,
    ad.flat_number_prefix,
    ad.flat_number,
    ad.flat_number_suffix,
    lta.name AS level_type,
    ad.level_number_prefix,
    ad.level_number,
    ad.level_number_suffix,
    ad.number_first_prefix,
    ad.number_first,
    ad.number_first_suffix,
    ad.number_last_prefix,
    ad.number_last,
    ad.number_last_suffix,
    sl.street_name,
    sl.street_class_code,
    sca.name AS street_class_type,
    sl.street_type_code,
    sl.street_suffix_code,
    ssa.name AS street_suffix_type,
    l.locality_name,
    st.state_abbreviation,
    ad.postcode,
    adg.latitude,
    adg.longitude,
    gta.name AS geocode_type,
    ad.confidence,
    ad.alias_principal,
    ad.primary_secondary,
    ad.legal_parcel_id,
    ad.date_created,
    array_to_string(ARRAY[ad.building_name, fta.name, ad.flat_number_prefix, ad.flat_number::text,
                          ad.flat_number_suffix, ad.level_number_prefix, ad.level_number::text, ad.level_number_suffix], ' ') as building,
    array_to_string(ARRAY[
        coalesce(
            nullif(array_to_string(ARRAY[ad.number_first_prefix, ad.number_first::text, ad.number_first_suffix], ' '), ''),
            nullif(array_to_string(ARRAY['LOT', ad.lot_number_prefix, ad.lot_number, ad.lot_number_suffix], ' '),'LOT')
        ),
        sl.street_name, sl.street_type_code, sl.street_suffix_code, l.locality_name, st.state_abbreviation, ad.postcode], ' ') as address,
    adg.geom
   FROM rawdata.address_detail ad
     LEFT JOIN rawdata.flat_type_aut fta ON ad.flat_type_code::text = fta.code::text
     LEFT JOIN rawdata.level_type_aut lta ON ad.level_type_code::text = lta.code::text
     JOIN rawdata.street_locality sl ON ad.street_locality_pid::text = sl.street_locality_pid::text
     LEFT JOIN rawdata.street_suffix_aut ssa ON sl.street_suffix_code::text = ssa.code::text
     LEFT JOIN rawdata.street_class_aut sca ON sl.street_class_code = sca.code
     LEFT JOIN rawdata.street_type_aut sta ON sl.street_type_code::text = sta.code::text
     JOIN rawdata.locality l ON ad.locality_pid::text = l.locality_pid::text
     JOIN rawdata.address_default_geocode adg ON ad.address_detail_pid::text = adg.address_detail_pid::text
     LEFT JOIN rawdata.geocode_type_aut gta ON adg.geocode_type_code::text = gta.code::text
     LEFT JOIN rawdata.geocoded_level_type_aut glta ON ad.level_geocoded_code = glta.code
     JOIN rawdata.state st ON l.state_pid::text = st.state_pid::text
  WHERE ad.confidence > '-1'::integer::numeric;
-- Query returned successfully: 14015754 rows affected, 11:08 minutes execution time.

create index addresses_geom_gidx on rawdata.addresses using gist (geom);
-- Query returned successfully with no result in 07:49 minutes.

cluster rawdata.addresses using addresses_geom_gidx;
-- Query returned successfully with no result in 10:16 minutes.

commit;
-- Query returned successfully with no result in 29:25 minutes.

vacuum analyze rawdata.addresses;
-- Query returned successfully with no result in 16.0 secs.

-- review addresses ----------------------------------------------------------------------

select * from rawdata.addresses where state_abbreviation='ACT' limit 1000;
select * from rawdata.addresses where state_abbreviation='NSW' limit 1000;
select * from rawdata.addresses where state_abbreviation='NT' limit 1000;
select * from rawdata.addresses where state_abbreviation='OT' limit 1000;
select * from rawdata.addresses where state_abbreviation='QLD' limit 1000;
select * from rawdata.addresses where state_abbreviation='SA' limit 1000;
select * from rawdata.addresses where state_abbreviation='TAS' limit 1000;
select * from rawdata.addresses where state_abbreviation='VIC' limit 1000;
select * from rawdata.addresses where state_abbreviation='WA' limit 1000;


-- as_compile_lexicon(lexicon text)
-- as_parse(IN address text, IN lexicon text, IN locale text, IN filter text)
-- as_standardize( IN address text, IN grammar text, IN lexicon text, IN locale text, IN filter text)

'25 C STOCKYARD ROAD NORFOLK ISLAND OT 2899'
'104 A J.E. ROAD NORFOLK ISLAND OT 2899'
'145 J.E. ROAD NORFOLK ISLAND OT 2899'
'93 F NEW CASCADE ROAD NORFOLK ISLAND OT 2899'
'34 B QUEEN ELIZABETH AVENUE NORFOLK ISLAND OT 2899'
'4 LITTLE CUTTERS CORN NORFOLK ISLAND OT 2899'
'7 ESTATE DIANA ADAMS ROAD NORFOLK ISLAND OT 2899'

'403 UPPER CORNWALL STREET COORPAROO QLD 4151'
'58 BEENLEIGH REDLAND BAY ROAD LOGANHOLME QLD 4129'

'LOT 26 PRINCES HIGHWAY TAILEM BEND SA 5260'
'168 PRINCES HIGHWAY TAILEM BEND SA 5260'
'162 THE POINT ROAD JERVOIS SA 5259'
'LOT 4 LAKE ROAD LAKE ALEXANDRINA SA 5259'
'LOT 103 FERRIES MCDONALD ROAD MONARTO SOUTH SA 5254'
'LOT 605 LANGHORNE CREEK ROAD LANGHORNE CREEK SA 5255'
'LOT 3 CHAUNCEYS LINE ROAD HARTLEY SA 5255'


select * from as_standardize(
    '13 AIRPORT ROAD BROOKLYN PARK SA 5032',
    (select grammar from as_config where countrycode='au' and dataset='gnaf'),
    (select clexicon from as_config where countrycode='au' and dataset='gnaf'),
    'en_AU',
    (select filter from as_config where countrycode='au' and dataset='gnaf')
);

select * from as_parse(
    '9 WILLIAMS ROAD TWO WELLS SA 5501',
    (select clexicon from as_config where countrycode='au' and dataset='gnaf'),
    'en_AU',
    (select filter from as_config where countrycode='au' and dataset='gnaf')
);


--------------------------------------------------------------------------
-- create and populate stdstreets table
--------------------------------------------------------------------------

DROP TABLE IF EXISTS stdstreets CASCADE;
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

/*
select a.id, std.*, dmetaphone(std.name)
  from rawdata.addresses as a,
       as_config as cfg,
       LATERAL as_standardize( address, grammar, clexicon, 'en_AU', filter) as std
 where countrycode='au' and dataset='gnaf' and a.id between 1001 and 2000;

    1 rec =  0.382 sec
   10 rec =  0.611 sec
  100 rec =  7.6   sec
 1000 rec = 73.0   sec
 14M  rec = 14M*0.073/3600/24 = 11.8 days estimate
*/
insert into stdstreets
select a.id, std.*, dmetaphone(std.name)
  from rawdata.addresses as a,
       as_config as cfg,
       LATERAL as_standardize( address, grammar, clexicon, 'en_AU', filter) as std
 where countrycode='au' and dataset='gnaf';
-- 

-- check for records the failed to standardize
select a.id, a.address
  from rawdata.addresses a left outer join stdstreets b on a.id=b.id
 where b.id is null;

select * from stdstreets where id between 1001 and 2000;

select * from rawdata.addresses where id in (select id from stdstreets where coalesce( building, house_num, predir, qual, pretype, name, suftype, sufdir, ruralroute, extra, city, state, country, postcode, box, unit ) is NULL);

--------------------------------------------------------------------------
-- add GNAF lexicon and grammar to as_config
--------------------------------------------------------------------------

--------------------------------------------------------------------------
-- debug various issues
--------------------------------------------------------------------------
select a.id, std.*, dmetaphone(std.name)
  from rawdata.addresses as a,
       as_config as cfg,
       LATERAL as_standardize( address, grammar, clexicon, 'en_AU', filter) as std
 where countrycode='au' and dataset='gnaf' and 
       --a.id between 12001 and 13000
       a.state_abbreviation='NSW'
       limit 1000
       ;

-- check id city does not match locality_name

select a.*
  from stdstreets a left outer join rawdata.locality b on replace(a.city, '-', ' ')=replace(b.locality_name, '-', ' ')
 where b.locality_name is null order by id;

select a.*
  from stdstreets a left outer join rawdata.locality b on a.city=b.locality_name
 where b.locality_name is null order by id;

-- check for failed to stadardize

select * from rawdata.addresses where id in (select id from stdstreets where coalesce( building, house_num, predir, qual, pretype, name, suftype, sufdir, ruralroute, extra, city, state, country, postcode, box, unit ) is NULL);

/*
without cities in lexicon
id=1       .382 sec
id<11      .611 sec
id<101    7.6 sec
id<1001  73.0 sec (80.0 sec with some cities)

with cities in lexicon
id=1       .774 sec
id<11     2.4 sec
id<101   19.3 sec
id<1001 187.0 sec

*/


select a.id, std.*, dmetaphone(std.name), address
  from rawdata.addresses as a,
       as_config as cfg,
       LATERAL as_standardize( address, grammar, clexicon, 'en_AU', filter) as std
    --   LATERAL as_standardize( '116 PINE HILLS NO 2 ROAD WOMBELANO VIC 3409', grammar, clexicon, 'en_AU', filter) as std
 where countrycode='au' and dataset='gnaf'
   --and a.id=5703616;
   --and a.id in ( select id from stdstreets where coalesce( building, house_num, predir, qual, pretype, name, suftype, sufdir, ruralroute, extra, city, state, country, postcode, box, unit ) is NULL);
   --and a.id in (2070747,2081612,2154448,2158060,2158581	)
   and a.id in (select a.id
                  from stdstreets a 
                  left outer join rawdata.locality b on a.city=replace(b.locality_name, '-', ' ')
                 where b.locality_name is null)
   and std.city != a.locality_name;

select a.id, std.*
  from rawdata.addresses as a,
       as_config as cfg,
       LATERAL as_parse( address, clexicon, 'en_AU', filter) as std
 where countrycode='au' and dataset='gnaf'
   and a.id=5703616;



select distinct std.*
  from as_config as cfg,
       LATERAL as_match( '116 PINE HILLS NO 2 ROAD WOMBELANO VIC 3409', grammar, clexicon, 'en_AU', filter) as std
 where countrycode='au' and dataset='gnaf'
 order by score desc;

select distinct a.id, std.*
  from rawdata.addresses as a,
       as_config as cfg,
       LATERAL as_match( address, grammar, clexicon, 'en_AU', filter) as std
 where countrycode='au' and dataset='gnaf'
       and a.id=5703616
 order by score desc;


select * from rawdata.addresses where id=2685495;
--select * from rawdata.addresses where id in (150454,198151,223129,236956,937726);




select distinct locality_name from rawdata.addresses where locality_name like 'W %' order by 1;
select distinct locality_name from rawdata.addresses where street_suffix_code is not null order by 1;
select distinct street_name from rawdata.addresses where street_name like '% FRONTAGES' order by 1;

select distinct locality_name from rawdata.locality where locality_name like 'BLACK %' order by 1;

select distinct regexp_replace(address, E'^.* PARK AVENUE | \\w+ \\d+$', '', 'g')
  from rawdata.addresses where address like '% PARK AVENUE %' and not address like '% PARK AVENUE QLD %';


--------------------------------------------------------------------------------------------------------

with std as (
	select a.id, std.*, dmetaphone(std.name) as name_dmeta
	  from rawdata.addresses as a,
	       as_config as cfg,
	       LATERAL as_standardize( address, grammar, clexicon, 'en_AU', filter) as std
	 where countrycode='au' and dataset='gnaf'
	   and a.id in (select a.id
			  from stdstreets a 
			  left outer join rawdata.locality b on a.city=replace(b.locality_name, '-', ' ')
			 where b.locality_name is null)
--	   and std.city != a.locality_name
)
--select * from std order by id;
update stdstreets a set
	building = std.building,
	house_num = std.house_num,
	predir = std.predir,
	qual = std.qual,
	pretype = std.pretype,
	name = std.name,
	suftype = std.suftype,
	sufdir = std.sufdir,
	ruralroute = std.ruralroute,
	extra = std.extra,
	city = std.city,
	state = std.prov,
	country = std.country,
	postcode = std.postcode,
	box = std.box,
	unit = std.unit,
	--pattern = std.pattern,
	name_dmeta = std.name_dmeta
 from std
where a.id = std.id

----------------------------------------------------------------------------------------------

select a.*
  from stdstreets2 a left outer join rawdata.locality b on replace(a.city, '-', ' ')=replace(b.locality_name, '-', ' ')
 where b.locality_name is null order by id;


select count(*) as cnt, pattern from stdstreets2 group by pattern order by cnt desc;

