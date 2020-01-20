/*
iMaptools SQL Geocoder for Tiger data

Copyright 2012, Stephen Woodbridge DBA iMaptools.com, All Rights Reserved.

TODO:

1. add offset distance calculations  -- DONE
2. add queries and indexes so we can also search for city OR USPS city
3. look at query planner for degenerate cases where too fields are absent
4. see if changing name-city-state index to name-state-city and clustering it improves performance, name_dmeta-state-city also. -- DONE
5. support for intersections  -- DONE
6. landmarks
7. parcels
8. look into lex, gaz and rules for streets that did not standardize
9. UTF8 support in standardizer - look into unaccent

*/
/*
-- this is for reference. it is defined when the pagc standardizer is installed.

DROP TYPE IF EXISTS data.stdaddr cascade;
CREATE TYPE stdaddr AS (
    --id integer,
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
    unit character varying
);

*/
begin;
drop table if exists stdstreets cascade;

create table stdstreets (
    id integer not null primary key,
    building varchar,
    house_num varchar,
    predir varchar,
    qual varchar,
    pretype varchar,
    name varchar,
    suftype varchar,
    sufdir varchar,
    ruralroute varchar,
    extra varchar,
    city varchar,
    state varchar,
    country varchar,
    postcode varchar,
    box varchar,
    unit varchar,
    name_dmeta varchar
);

insert into stdstreets (id,building,house_num,predir,qual,pretype,name,suftype,sufdir,ruralroute,extra,city,state,country,postcode,box,unit,name_dmeta)
    select id, (std).*, coalesce(nullif(dmetaphone((std).name),''), (std).name)
      from (
        select gid as id, standardize_address('lex'::text, 'gaz'::text, 'rules'::text,
               array_to_string(ARRAY[refaddr::varchar, predirabrv, pretypabrv, prequalabr, name, sufdirabrv, suftypabrv, sufqualabr], ' '),
               array_to_string(ARRAY[coalesce(usps,ac5,ac4,ac3),ac2,ac1,postcode], ',') ) as std
          from streets order by gid
       ) as foo
       --limit 10
       ;
       
commit;
-- rollback;
-- Query returned successfully with no result in 01:07:3649 hours. (tiger2016)

-- create indexes -----------------------------------------------------------------------------

create index stdstreets_name_city_state_idx on stdstreets using btree (name,state,city ASC NULLS LAST);
-- Query returned successfully with no result in 1315409 ms.
-- Query returned successfully with no result in 46:02 minutes.

create index stdstreets_name_postcode_idx on stdstreets using btree (name,postcode ASC NULLS LAST);
-- Query returned successfully with no result in 722995 ms.
-- Query returned successfully with no result in 34:36 minutes.

create index stdstreets_name_dmeta_city_state_idx on stdstreets using btree (name_dmeta,state,city ASC NULLS LAST);
-- Query returned successfully with no result in 923967 ms.
-- Query returned successfully with no result in 43:05 minutes.

create index stdstreets_name_dmeta_postcode_idx on stdstreets using btree (name_dmeta,postcode ASC NULLS LAST);
-- Query returned successfully with no result in 759968 ms.
-- Query returned successfully with no result in 28:08 minutes.

-- Query returned successfully with no result in 02:26:7236 hours.

vacuum analyze verbose stdstreets;
-- Query returned successfully with no result in 4893048 ms.
-- Query returned successfully with no result in 37.8 secs.

/*
-- update null standardizations after updating lexicon or gazeteer
-- ######### This needs to be updated using the pattern from the insert ###############

update stdstreets as a set building=b.building, house_num=b.house_num, predir=b.predir, qual=b.qual, pretype=b.pretype, name=b.name,
                            suftype=b.suftype, sufdir=b.sufdir, ruralroute=b.ruralroute, extra=b.extra, city=b.city, state=b.state,
                            country=b.country, postcode=b.postcode, box=b.box, unit=b.unit
   from (select * from standardize_address(
        'lex',
        'gaz',
        'rules',
        'select gid as id,
                array_to_string(ARRAY[refaddr::varchar, predirabrv, pretypabrv, prequalabr, name, sufdirabrv, suftypabrv, sufqualabr], '' '') as micro,
                array_to_string(ARRAY[coalesce(usps,ac5,ac4,ac3),ac2,ac1,postcode], '','') as macro
           from streets
          where gid in (
                    select id
                      from stdstreets
                     where coalesce(building, house_num, predir, qual, pretype, name,
                                    suftype, sufdir, ruralroute, extra,
                                    city, state, country, postcode, box, unit) is null
          ) and array_to_string(ARRAY[refaddr::varchar, predirabrv, pretypabrv, prequalabr, name, sufdirabrv, suftypabrv, sufqualabr], '' '') is not null
          order by gid')) as b
  where a.id=b.id;

*/
-- query to get records that did not standardize --

drop table if exists failures;
select count(*) as cnt,
       array_to_string(ARRAY[predirabrv, pretypabrv, prequalabr, name,
                             sufdirabrv, suftypabrv, sufqualabr], ' ') as micro
  into failures
  from streets
 where gid in (
    select id
      from stdstreets
     where coalesce(building, house_num, predir, qual, pretype, name,
                    suftype, sufdir, ruralroute, extra,
                    city, state, country, postcode, box, unit) is null
      )
 group by micro
 order by micro;
 -- Query returned successfully: 4420 rows affected, 13.5 secs execution time.


/*

-- After tiger 2016 load and ading some rules and lex entries

with std as (
    select (standardize_address('lex','gaz','rules','123 '||a.micro,'')).*, a.cnt, a.micro
      from failures a
)
select count(*), sum(std.cnt) from std where std.house_num is null;
-- 4319; 41916

*/

/*
-- After full tiger 2012 load
  cnt  micro
25872  ""
16170  "Avant St"
 9702  "Rte Route Cord 39 Alt"
 6468  "Rte Route County Road 121 Alt"
19404  "Rte Route County Road 40 W Alt"
16170  "Rte Route County Road 59 Alt"

-- add to lexicon.csv
"1","RTE ROUTE CORD",6,"COUNTY ROAD"
"1","RTE ROUTE COUNTY ROAD",2,"COUNTY ROAD"


*/


-------------------------------------------------------------------------------------------------------------------

create or replace function imt_geo_add_where_clauses(inp stdaddr, plan integer)
  returns text as
$body$
/*
the query planner might do something like:
0. query for exact terms input, ignoring building, extra, box, and unit
1. try 1 with postcode only or state only, if no postcode
2. try 1 with city, state only
3. then loosen up on house_num, predir, sufdir, qual, pretype, suftype
4. try 3 with postcode only or state only, if no postcode
5. try 3 with city, state only
--- only if the request want fuzzy searching
6. then try searching on metaphone name
7. try 6 with postcode only or state only, if no postcode
8. try 6 with city state only
*/
declare
  ret text := '';
begin
  if inp.name is not null and plan < 6 then
    ret := ret || ' and a.name='||quote_literal(inp.name);
  elsif inp.name is not null and plan >= 6 then
    ret := ret || ' and a.name_dmeta=coalesce(dmetaphone('||quote_literal(inp.name)||'),'||quote_literal(inp.name)||')';
  end if;
  if inp.house_num is not null then
      ret := ret || ' and ('||to_number(quote_literal(inp.house_num),'999999999')||' between b.refaddr and b.nrefaddr' ||
                      ' or '||to_number(quote_literal(inp.house_num),'999999999')||' between b.nrefaddr and b.refaddr )';
  end if;
  if plan < 3 then
    if inp.predir is not null then
      ret := ret || ' and case when a.predir is not null then a.predir='||quote_literal(inp.predir)||' else true end';
    end if;
    if inp.qual is not null then
      ret := ret || ' and case when a.qual is not null then a.qual='||quote_literal(inp.qual)||' else true end';
    end if;
    if inp.pretype is not null then
      ret := ret || ' and case when a.pretype is not null then a.pretype='||quote_literal(inp.pretype)||' else true end';
    end if;
    if inp.suftype is not null then
      ret := ret || ' and case when a.suftype is not null then a.suftype='||quote_literal(inp.suftype)||' else true end';
    end if;
    if inp.sufdir is not null then
      ret := ret || ' and case when a.sufdir is not null then a.sufdir='||quote_literal(inp.sufdir)||' else true end';
    end if;
  end if;
  if inp.city is not null and plan in (0,2,3,5,6,8) then
    ret := ret || ' and a.city='||quote_literal(inp.city);
  end if;
  if (inp.state is not null) and plan in (0,2,3,5,6,8) or (inp.postcode is null and inp.state is not null and plan in (1,4,7)) then
    ret := ret || ' and a.state='||quote_literal(inp.state);
  end if;
  if inp.country is not null then
    ret := ret || ' and case when a.country is not null then a.country='||quote_literal(inp.country)||' else true end';
  end if;
  if inp.postcode is not null and plan in (0,1,3,4,6,7) then
    ret := ret || ' and a.postcode='||quote_literal(inp.postcode);
  end if;

  return ret;
end;
$body$
  language plpgsql immutable;

-----------------------------------------------------------------------------------------------------

create or replace function imt_geo_planner(inp stdaddr)
  returns setof text as
$body$
declare
  sql text;
  i integer;
  rec record;
  
begin
  <<planner>>
  for i in 0..8 loop 
    sql := 'select a.*, '||i::text||' as plan from stdstreets a, streets b where a.id=b.gid ' || imt_geo_add_where_clauses(inp, i);
    return next sql;
    /*
    for rec in execute sql loop
      -- do something
    end loop;
    */
  end loop planner;
  return;
end;
$body$
  language plpgsql immutable
  cost 1
  rows 10;

-----------------------------------------------------------------------------------------------------

create or replace function imt_geo_planner2(inp stdaddr, opt integer)
  returns text as
$body$
declare
  sql text;
  i integer;
  rec record;
  mm integer;
  
begin
  mm := case when opt>0 then 8 else 5 end;
  sql := '';
  <<planner>>
  for i in 0..mm loop 
    sql := sql 
      || case when i>0 then '
    union
    ' else '' end
      || 'select a.*, '
                 ||i::text||' as plan, '
                 ||coalesce(to_number(quote_literal(inp.house_num),'999999999')||'::integer%2=b.refaddr::integer%2','null')||' as parity'
      || ' from stdstreets a, streets b'
      || ' where a.id=b.gid ' 
      || imt_geo_add_where_clauses(inp, i);
  end loop planner;

--  sql := 'select distinct on (id) foo.* from (' || sql || ') as foo order by id';
  -- added sub-select to enforce an order by id, plan so all servers return the same results
  sql := 'select distinct on (id) * from (select * from (' || sql || ') as bar order by id, plan) as foo order by id';

  return sql;
end;
$body$
  language plpgsql immutable;

-------------------------------------------------------------------------------------------------------

create or replace function imt_geo_test(myid integer)
  returns setof text as
$body$
declare
  rec stdaddr;
  
begin
  select * into rec from stdstreets where id=myid;
  return query select * from geo_planner(rec);
end;
$body$
  language plpgsql immutable;


create or replace function imt_geo_test2(myid integer)
  returns setof text as
$body$
declare
  rec stdaddr;
  
begin
  select * into rec from stdstreets where id=myid;
  return query select * from imt_geo_planner2(rec, 1);
end;
$body$
  language plpgsql immutable;


/*

select * from imt_geo_test(27);

select 0 as plan, a.*, '269'::integer % 2=b.refaddr::integer%2 as parity from stdstreets a, streets b where a.id=b.gid  and a.name='WASHINGTON' and case when b.refaddr<b.nrefaddr then '269' between b.refaddr and b.nrefaddr else '269' between b.nrefaddr and b.refaddr end and case when a.suftype is not null then a.suftype='AVENUE' else true end and case when a.city is not null then a.city='REVERE' else true end and case when a.state is not null then a.state='MASSACHUSETTS' else true end and case when a.country is not null then a.country='USA' else true end and case when a.postcode is not null then a.postcode='02151' else true end;
select 1 as plan, a.* from stdstreets a, streets b where a.id=b.gid  and a.name='WASHINGTON' and case when b.refaddr<b.nrefaddr then '269' between b.refaddr and b.nrefaddr else '269' between b.nrefaddr and b.refaddr end and case when a.suftype is not null then a.suftype='AVENUE' else true end and case when a.country is not null then a.country='USA' else true end and case when a.postcode is not null then a.postcode='02151' else true end;
select 2 as plan, a.* from stdstreets a, streets b where a.id=b.gid  and a.name='WASHINGTON' and case when b.refaddr<b.nrefaddr then '269' between b.refaddr and b.nrefaddr else '269' between b.nrefaddr and b.refaddr end and case when a.suftype is not null then a.suftype='AVENUE' else true end and case when a.city is not null then a.city='REVERE' else true end and case when a.state is not null then a.state='MASSACHUSETTS' else true end and case when a.country is not null then a.country='USA' else true end;
select 3 as plan, a.* from stdstreets a, streets b where a.id=b.gid  and a.name='WASHINGTON' and case when a.city is not null then a.city='REVERE' else true end and case when a.state is not null then a.state='MASSACHUSETTS' else true end and case when a.country is not null then a.country='USA' else true end and case when a.postcode is not null then a.postcode='02151' else true end;
select 4 as plan, a.* from stdstreets a, streets b where a.id=b.gid  and a.name='WASHINGTON' and case when a.country is not null then a.country='USA' else true end and case when a.postcode is not null then a.postcode='02151' else true end;
select 5 as plan, a.* from stdstreets a, streets b where a.id=b.gid  and a.name='WASHINGTON' and case when a.city is not null then a.city='REVERE' else true end and case when a.state is not null then a.state='MASSACHUSETTS' else true end and case when a.country is not null then a.country='USA' else true end;
select 6 as plan, a.* from stdstreets a, streets b where a.id=b.gid  and a.name_dmeta=dmetaphone('WASHINGTON') and case when a.city is not null then a.city='REVERE' else true end and case when a.state is not null then a.state='MASSACHUSETTS' else true end and case when a.country is not null then a.country='USA' else true end and case when a.postcode is not null then a.postcode='02151' else true end;
select 7 as plan, a.* from stdstreets a, streets b where a.id=b.gid  and a.name_dmeta=dmetaphone('WASHINGTON') and case when a.country is not null then a.country='USA' else true end and case when a.postcode is not null then a.postcode='02151' else true end;
select 8 as plan, a.* from stdstreets a, streets b where a.id=b.gid  and a.name_dmeta=dmetaphone('WASHINGTON') and case when a.city is not null then a.city='REVERE' else true end and case when a.state is not null then a.state='MASSACHUSETTS' else true end and case when a.country is not null then a.country='USA' else true end;

select 0 as plan, a.*, '269'::integer % 2=b.refaddr::integer%2 as parity from stdstreets a, streets b where a.id=b.gid  and a.name='WASHINGTON' and case when b.refaddr<b.nrefaddr then '269' between b.refaddr and b.nrefaddr else '269' between b.nrefaddr and b.refaddr end and case when a.suftype is not null then a.suftype='AVENUE' else true end and a.city='REVERE' and a.state='MASSACHUSETTS' and case when a.country is not null then a.country='USA' else true end and a.postcode='02151';


select * from imt_geo_test2(27);

select distinct on (id) foo.* from (select a.*, 0 as plan, 97::integer%2=b.refaddr::integer%2 as parity from stdstreets a, streets b, streets_hn c where a.id=b.gid and b.gid=c.gid  and a.name='WHITNEY' and (97 between c.refaddr and c.nrefaddr or 97 between c.nrefaddr and c.refaddr ) and case when a.suftype is not null then a.suftype='STREET' else true end and a.city='SAN FRANCISCO' and a.state='CALIFORNIA' and case when a.country is not null then a.country='USA' else true end and a.postcode='94131'
    union
    select a.*, 1 as plan, 97::integer%2=b.refaddr::integer%2 as parity from stdstreets a, streets b, streets_hn c where a.id=b.gid and b.gid=c.gid  and a.name='WHITNEY' and (97 between c.refaddr and c.nrefaddr or 97 between c.nrefaddr and c.refaddr ) and case when a.suftype is not null then a.suftype='STREET' else true end and case when a.country is not null then a.country='USA' else true end and a.postcode='94131'
    union
    select a.*, 2 as plan, 97::integer%2=b.refaddr::integer%2 as parity from stdstreets a, streets b, streets_hn c where a.id=b.gid and b.gid=c.gid  and a.name='WHITNEY' and (97 between c.refaddr and c.nrefaddr or 97 between c.nrefaddr and c.refaddr ) and case when a.suftype is not null then a.suftype='STREET' else true end and a.city='SAN FRANCISCO' and a.state='CALIFORNIA' and case when a.country is not null then a.country='USA' else true end
    union
    select a.*, 3 as plan, 97::integer%2=b.refaddr::integer%2 as parity from stdstreets a, streets b, streets_hn c where a.id=b.gid and b.gid=c.gid  and a.name='WHITNEY' and (97 between c.refaddr and c.nrefaddr or 97 between c.nrefaddr and c.refaddr ) and a.city='SAN FRANCISCO' and a.state='CALIFORNIA' and case when a.country is not null then a.country='USA' else true end and a.postcode='94131'
    union
    select a.*, 4 as plan, 97::integer%2=b.refaddr::integer%2 as parity from stdstreets a, streets b, streets_hn c where a.id=b.gid and b.gid=c.gid  and a.name='WHITNEY' and (97 between c.refaddr and c.nrefaddr or 97 between c.nrefaddr and c.refaddr ) and case when a.country is not null then a.country='USA' else true end and a.postcode='94131'
    union
    select a.*, 5 as plan, 97::integer%2=b.refaddr::integer%2 as parity from stdstreets a, streets b, streets_hn c where a.id=b.gid and b.gid=c.gid  and a.name='WHITNEY' and (97 between c.refaddr and c.nrefaddr or 97 between c.nrefaddr and c.refaddr ) and a.city='SAN FRANCISCO' and a.state='CALIFORNIA' and case when a.country is not null then a.country='USA' else true end
    union
    select a.*, 6 as plan, 97::integer%2=b.refaddr::integer%2 as parity from stdstreets a, streets b, streets_hn c where a.id=b.gid and b.gid=c.gid  and a.name_dmeta=coalesce(dmetaphone('WHITNEY'),'WHITNEY') and (97 between c.refaddr and c.nrefaddr or 97 between c.nrefaddr and c.refaddr ) and a.city='SAN FRANCISCO' and a.state='CALIFORNIA' and case when a.country is not null then a.country='USA' else true end and a.postcode='94131'
    union
    select a.*, 7 as plan, 97::integer%2=b.refaddr::integer%2 as parity from stdstreets a, streets b, streets_hn c where a.id=b.gid and b.gid=c.gid  and a.name_dmeta=coalesce(dmetaphone('WHITNEY'),'WHITNEY') and (97 between c.refaddr and c.nrefaddr or 97 between c.nrefaddr and c.refaddr ) and case when a.country is not null then a.country='USA' else true end and a.postcode='94131'
    union
    select a.*, 8 as plan, 97::integer%2=b.refaddr::integer%2 as parity from stdstreets a, streets b, streets_hn c where a.id=b.gid and b.gid=c.gid  and a.name_dmeta=coalesce(dmetaphone('WHITNEY'),'WHITNEY') and (97 between c.refaddr and c.nrefaddr or 97 between c.nrefaddr and c.refaddr ) and a.city='SAN FRANCISCO' and a.state='CALIFORNIA' and case when a.country is not null then a.country='USA' else true end) as foo order by id

select distinct on (id) * from (select * from (
    select a.*, 0 as plan, 3495::integer%2=b.refaddr::integer%2 as parity from stdstreets a, streets b where a.id=b.gid  and a.name='AUTAUGA COUNTY 26' and (3495 between b.refaddr and b.nrefaddr or 3495 between b.nrefaddr and b.refaddr ) and a.city='BILLINGSLEY' and a.state='ALABAMA' and case when a.country is not null then a.country='USA' else true end and a.postcode='36006'
    union
    select a.*, 1 as plan, 3495::integer%2=b.refaddr::integer%2 as parity from stdstreets a, streets b where a.id=b.gid  and a.name='AUTAUGA COUNTY 26' and (3495 between b.refaddr and b.nrefaddr or 3495 between b.nrefaddr and b.refaddr ) and case when a.country is not null then a.country='USA' else true end and a.postcode='36006'
    union
    select a.*, 2 as plan, 3495::integer%2=b.refaddr::integer%2 as parity from stdstreets a, streets b where a.id=b.gid  and a.name='AUTAUGA COUNTY 26' and (3495 between b.refaddr and b.nrefaddr or 3495 between b.nrefaddr and b.refaddr ) and a.city='BILLINGSLEY' and a.state='ALABAMA' and case when a.country is not null then a.country='USA' else true end
    union
    select a.*, 3 as plan, 3495::integer%2=b.refaddr::integer%2 as parity from stdstreets a, streets b where a.id=b.gid  and a.name='AUTAUGA COUNTY 26' and (3495 between b.refaddr and b.nrefaddr or 3495 between b.nrefaddr and b.refaddr ) and a.city='BILLINGSLEY' and a.state='ALABAMA' and case when a.country is not null then a.country='USA' else true end and a.postcode='36006'
    union
    select a.*, 4 as plan, 3495::integer%2=b.refaddr::integer%2 as parity from stdstreets a, streets b where a.id=b.gid  and a.name='AUTAUGA COUNTY 26' and (3495 between b.refaddr and b.nrefaddr or 3495 between b.nrefaddr and b.refaddr ) and case when a.country is not null then a.country='USA' else true end and a.postcode='36006'
    union
    select a.*, 5 as plan, 3495::integer%2=b.refaddr::integer%2 as parity from stdstreets a, streets b where a.id=b.gid  and a.name='AUTAUGA COUNTY 26' and (3495 between b.refaddr and b.nrefaddr or 3495 between b.nrefaddr and b.refaddr ) and a.city='BILLINGSLEY' and a.state='ALABAMA' and case when a.country is not null then a.country='USA' else true end
    union
    select a.*, 6 as plan, 3495::integer%2=b.refaddr::integer%2 as parity from stdstreets a, streets b where a.id=b.gid  and a.name_dmeta=coalesce(dmetaphone('AUTAUGA COUNTY 26'),'AUTAUGA COUNTY 26') and (3495 between b.refaddr and b.nrefaddr or 3495 between b.nrefaddr and b.refaddr ) and a.city='BILLINGSLEY' and a.state='ALABAMA' and case when a.country is not null then a.country='USA' else true end and a.postcode='36006'
    union
    select a.*, 7 as plan, 3495::integer%2=b.refaddr::integer%2 as parity from stdstreets a, streets b where a.id=b.gid  and a.name_dmeta=coalesce(dmetaphone('AUTAUGA COUNTY 26'),'AUTAUGA COUNTY 26') and (3495 between b.refaddr and b.nrefaddr or 3495 between b.nrefaddr and b.refaddr ) and case when a.country is not null then a.country='USA' else true end and a.postcode='36006'
    union
    select a.*, 8 as plan, 3495::integer%2=b.refaddr::integer%2 as parity from stdstreets a, streets b where a.id=b.gid  and a.name_dmeta=coalesce(dmetaphone('AUTAUGA COUNTY 26'),'AUTAUGA COUNTY 26') and (3495 between b.refaddr and b.nrefaddr or 3495 between b.nrefaddr and b.refaddr ) and a.city='BILLINGSLEY' and a.state='ALABAMA' and case when a.country is not null then a.country='USA' else true end) as bar order by id, plan) as foo order by id;
    

*/



drop type if exists geo_result cascade;
create type geo_result as (
  id integer,
  building text,
  housenum text,
  predir text,
  qual text,
  pretype text,
  name text,
  suftype text,
  sufdir text,
  ruralroute text,
  extra text,
  city text,
  state text,
  country text,
  postcode text,
  box text,
  unit text,
  name_dmeta text,
  plan integer,
  parity boolean,
  score double precision,
  x double precision,
  y double precision
);



drop type if exists geo_result2 cascade;
create type geo_result2 as (
  rid integer,                -- sfld.id as rid,
  id integer,                 -- a.id,
  id1 integer,                -- b.tlid as id1,
  id2 integer,                -- b.side as id2,
  id3 integer,                -- b.tfid as id3,
  completeaddressnumber text, -- sfld.house_num as completeaddressnumber,
  predirectional text,        -- b.predirabrv as predirectional,
  premodifier text,           -- b.prequalabr as premodifier,
  postmodifier text,          -- b.sufqualabr as postmodifier,
  pretype text,               -- b.pretypabrv as pretype,
  streetname text,            -- b.name as streetname,
  posttype text,              -- b.suftypabrv as posttype,
  postdirectional text,       -- b.sufdirabrv as postdirectional,
  placename text,             -- coalesce(b.ac5,b.ac4,b.ac3) as placename,
  placename_usps text,        -- b.usps as placename_usps,
  statename text,             -- b.ac2 as statename,
  countrycode text,           -- b.ac1 as countrycode,
  zipcode text,               -- b.zipcode,
  building text,              -- a.building,
  unit text,                  -- a.unit,
  plan integer,               -- plan,
  parity boolean,             -- parity,
  geocodematchcode double precision, -- a.geocodematchcode,
  x double precision,         -- a.x,
  y double precision          -- a.y
);


-----------------------------------------------------------------------------------------------

create or replace function max(a numeric, b numeric)
  returns numeric as
$body$
begin
  return case when a>b then a else b end;
end;
$body$
  language plpgsql immutable;

-----------------------------------------------------------------------------------------------

create or replace function imt_compute_xy(id integer, num text, aoff double precision, OUT x double precision, OUT y double precision) as
$body$
declare
  rec record;
  pnt geometry;
  d1 double precision;
  d2 double precision;
  our_side integer;
  geom geometry;
  pos double precision;
  p1 geometry;
  p2 geometry;
  dx double precision;
  dy double precision;
  px double precision;
  py double precision;
  ux double precision;
  uy double precision;
  len double precision;
  
begin

  -- num is null for intersections and we get intersection locations from another table
  if num is null or num='0' then
    x := NULL;
    y := NULL;
    return;
  end if;
  
  -- side: 1=R, 2=L
  select refaddr, nrefaddr, side, st_geometryn(the_geom,1) as the_geom into rec from streets where gid=id;
  if found then
    if rec.refaddr<rec.nrefaddr then
      d2 := to_number(num,'999999999') - rec.refaddr;
      d1 := rec.nrefaddr - rec.refaddr;
    else
      d2 := rec.refaddr - to_number(num,'999999999');
      d1 := rec.refaddr - rec.nrefaddr;
    end if;
    if d1 = 0.0 then -- check and avoid divide by zero
      pos := 0.5;    -- the start and end are the same, then pick the midpoint of the segment. 
    else
      pos := d2/d1;
    end if;

--raise notice 'num: %, ref: %, nref: %, d2: %, d1: %, pos: %',num, rec.refaddr, rec.nrefaddr, d2, d1, pos;

    /* compute the offset from the center line if requested. aoff=0.0 is the trivial case */
    if aoff = 0.0 then 
        pnt := ST_Line_Interpolate_Point(rec.the_geom, pos);
        x := st_x(pnt);
        y := st_y(pnt);
    else
        /* compute the offset to the right or left for this address
           * if the parity does not match then we should probably reverse the R/L side of the offset
           * if the location it at the end of the line, ie: pos=1.0, then we reverse the line and the sides to compute the offset
           * algorithm is:
           *   trim the line to start at pos
           *   get the unit vector from pnt1 to pos2
           *   use vector algebra to compute the offset
        */
        our_side := rec.side;
        geom := rec.the_geom;
        if pos = 1.0 then
	    geom := st_reverse(geom);                              -- reverse the line
	    our_side := case when rec.side = 1 then 2 else 1 end;  -- reverse the side
	    pos := 0.0;                                            -- reverse the ends
	end if;
	geom := ST_LineSubstring(geom, pos, 1.0);  -- trim the line so pos is the start now
	p1 := st_pointn(geom, 1);
	p2 := st_pointn(geom, 2);
	dx := st_x(p2) - st_x(p1);
	dy := st_y(p2) - st_y(p1);
	len := sqrt(dx*dx + dy*dy);     -- this could be zero if coincident points
	if len = 0.0 then
	    x := NULL;
	    y := NULL;
	    return;
	end if;
	ux := dx/len;  -- unit x
	uy := dy/len;  -- unit y
	if our_side = 1 then            -- right side
	    px := uy * aoff / 111120.0;
	    py := -ux * aoff / 111120.0;
	else                            -- left side
	    px := -uy * aoff / 111120.0;
	    py := ux * aoff / 111120.0;
	end if;
	x := st_x(p1) + px;
	y := st_y(p1) + py;
    end if;
  else
    x := NULL;
    y := NULL;
  end if;
end;
$body$
  language plpgsql immutable;
  

-----------------------------------------------------------------------------------------------

create or replace function imt_geo_score(sql text, sin stdaddr, aoff double precision)
  returns setof geo_result as
$body$
declare
  rec geo_result;
  score double precision;
  w1 double precision := 0.05;
  w2 double precision := 0.25;
  w3 double precision := 0.125;
  len integer;
  xy record;
  
begin

  for rec in execute sql loop
    select * into xy from imt_compute_xy(rec.id, sin.house_num, aoff);
    rec.x := xy.x;
    rec.y := xy.y;
    score := case when coalesce(rec.parity,true) then 0.0 else 0.1 end;
    len := max(length(coalesce(rec.predir,'')), length(coalesce(sin.predir,'')));
    score := score + case when len>0 then levenshtein(coalesce(rec.predir,''), coalesce(sin.predir,''))::double precision / len::double precision * w1 else 0.0 end;
    len := max(length(coalesce(rec.sufdir,'')), length(coalesce(sin.sufdir,'')));
    score := score + case when len>0 then levenshtein(coalesce(rec.sufdir,''), coalesce(sin.sufdir,''))::double precision / len::double precision * w1 else 0.0 end;
    len := max(length(coalesce(rec.pretype,'')), length(coalesce(sin.pretype,'')));
    score := score + case when len>0 then levenshtein(coalesce(rec.pretype,''), coalesce(sin.pretype,''))::double precision / len::double precision * w1 else 0.0 end;
    len := max(length(coalesce(rec.suftype,'')), length(coalesce(sin.suftype,'')));
    score := score + case when len>0 then levenshtein(coalesce(rec.suftype,''), coalesce(sin.suftype,''))::double precision / len::double precision * w1 else 0.0 end;
    len := max(length(coalesce(rec.qual,'')), length(coalesce(sin.qual,'')));
    score := score + case when len>0 then levenshtein(coalesce(rec.qual,''), coalesce(sin.qual,''))::double precision / len::double precision * w1 else 0.0 end;
    len := max(length(coalesce(rec.name,'')), length(coalesce(sin.name,'')));
    score := score + case when len>0 then levenshtein(coalesce(rec.name,''), coalesce(sin.name,''))::double precision / len::double precision * w2 else 0.0 end;
    len := max(length(coalesce(rec.city,'')), length(coalesce(sin.city,'')));
    score := score + case when len>0 then levenshtein(coalesce(rec.city,''), coalesce(sin.city,''))::double precision / len::double precision * w3 else 0.0 end;
    len := max(length(coalesce(rec.state,'')), length(coalesce(sin.state,'')));
    score := score + case when len>0 then levenshtein(coalesce(rec.state,''), coalesce(sin.state,''))::double precision / len::double precision * w3 else 0.0 end;
    len := max(length(coalesce(rec.postcode,'')), length(coalesce(sin.postcode,'')));
    score := score + case when len>0 then levenshtein(coalesce(rec.postcode,''), coalesce(sin.postcode,''))::double precision / len::double precision * w3 else 0.0 end;
    len := max(length(coalesce(rec.country,'')), length(coalesce(sin.country,'')));
    score := score + case when len>0 then levenshtein(coalesce(rec.country,''), coalesce(sin.country,''))::double precision / len::double precision * w3 else 0.0 end;
    rec.score := 1.0 - score;
    return next rec;
  end loop;
  return;
end;
$body$
  language plpgsql immutable strict cost 1 rows 100;

-----------------------------------------------------------------------------------------------

create or replace function imt_geo_score2(sql text, sin stdaddr, aoff double precision)
  returns setof geo_result as
$body$
-- score based on similarity function. This does not discriminate as well as levenshtein function
declare
  rec geo_result;
  score double precision;
  w1 double precision := 0.05;
  w2 double precision := 0.25;
  w3 double precision := 0.125;
  len integer;
  xy record;
  
begin

  for rec in execute sql loop
    select * into xy from imt_compute_xy(rec.id, sin.house_num, aoff);
    rec.x := xy.x;
    rec.y := xy.y;
    score := case when coalesce(rec.parity,true) then 0.0 else 0.1 end;
    len := max(length(coalesce(rec.predir,'')), length(coalesce(sin.predir,'')));
    score := score + case when len>0 then similarity(coalesce(rec.predir,''), coalesce(sin.predir,''))::double precision / len::double precision * w1 else 0.0 end;
    len := max(length(coalesce(rec.sufdir,'')), length(coalesce(sin.sufdir,'')));
    score := score + case when len>0 then similarity(coalesce(rec.sufdir,''), coalesce(sin.sufdir,''))::double precision / len::double precision * w1 else 0.0 end;
    len := max(length(coalesce(rec.pretype,'')), length(coalesce(sin.pretype,'')));
    score := score + case when len>0 then similarity(coalesce(rec.pretype,''), coalesce(sin.pretype,''))::double precision / len::double precision * w1 else 0.0 end;
    len := max(length(coalesce(rec.suftype,'')), length(coalesce(sin.suftype,'')));
    score := score + case when len>0 then similarity(coalesce(rec.suftype,''), coalesce(sin.suftype,''))::double precision / len::double precision * w1 else 0.0 end;
    len := max(length(coalesce(rec.qual,'')), length(coalesce(sin.qual,'')));
    score := score + case when len>0 then similarity(coalesce(rec.qual,''), coalesce(sin.qual,''))::double precision / len::double precision * w1 else 0.0 end;
    len := max(length(coalesce(rec.name,'')), length(coalesce(sin.name,'')));
    score := score + case when len>0 then similarity(coalesce(rec.name,''), coalesce(sin.name,''))::double precision / len::double precision * w2 else 0.0 end;
    len := max(length(coalesce(rec.city,'')), length(coalesce(sin.city,'')));
    score := score + case when len>0 then similarity(coalesce(rec.city,''), coalesce(sin.city,''))::double precision / len::double precision * w3 else 0.0 end;
    len := max(length(coalesce(rec.state,'')), length(coalesce(sin.state,'')));
    score := score + case when len>0 then similarity(coalesce(rec.state,''), coalesce(sin.state,''))::double precision / len::double precision * w3 else 0.0 end;
    len := max(length(coalesce(rec.postcode,'')), length(coalesce(sin.postcode,'')));
    score := score + case when len>0 then similarity(coalesce(rec.postcode,''), coalesce(sin.postcode,''))::double precision / len::double precision * w3 else 0.0 end;
    len := max(length(coalesce(rec.country,'')), length(coalesce(sin.country,'')));
    score := score + case when len>0 then similarity(coalesce(rec.country,''), coalesce(sin.country,''))::double precision / len::double precision * w3 else 0.0 end;
    rec.score := score;
    return next rec;
  end loop;
  return;
end;
$body$
  language plpgsql immutable strict cost 1 rows 100;

---------------------------------------------------------------------------------------------
/*
-- FIXME: intersection and address geocoders return different geo_results!!

create or replace function imt_geocoder(address_in text, fuzzy integer, aoffset double precision, method integer)
  returns setof geo_result2 as
$body$
declare
  sfld stdaddr;
  sql text;
  rec geo_result2;
  cnt integer := 0;
  paddr record;
  micro_in text;
  macro_in text;

begin
  select * into paddr from parse_address(address_in);
  if not found then
    return;
  end if;

  macro_in := array_to_string(ARRAY[paddr.city, paddr.state, paddr.zip, paddr.county], ',');
  
  if paddr.street is not null and paddr.street2 is not null then
    -- we are doing an intersection
    return query select * from imt_geocode_intersection(paddr.street, paddr.street2, macro_in, fuzzy, method);
  else
    -- we are doing an address
    micro_in := paddr.address1;

    sql := 'select 0::int4 as id, '||trim(both from quote_literal(micro_in))||'::text as micro, '||trim(both from quote_literal(macro_in))||'::text as macro';
--raise notice 'sql: %', sql;
    select * into sfld from standardize_address(
          'lex',
          'gaz',
          'rules',
          sql);
--raise notice 'sfld: %', sfld;

    return query select * from imt_geocoder(sfld, fuzzy, aoffset, method);
  end if;
end;
$body$
  language plpgsql immutable strict cost 5 rows 100;

*/
---------------------------------------------------------------------------------------------

create or replace function imt_geocoder(micro_in text, macro_in text, fuzzy integer, aoffset double precision, method integer)
  returns setof geo_result2 as
$body$
declare
  sfld stdaddr;

begin
  select * into sfld 
    from standardize_address('lex', 'gaz', 'rules', 
           trim(both from micro_in)::text, 
           trim(both from macro_in)::text);
--raise notice 'sfld: %', sfld;
  if sfld.state is null and sfld.postcode is null then
    return;
  end if;

  return query select * from imt_geocoder(sfld, fuzzy, aoffset, method);
end;
$body$
  language plpgsql immutable strict cost 5 rows 100;

---------------------------------------------------------------------------------------------

create or replace function imt_geocoder_best(sql text, fuzzy integer, aoffset double precision, method integer)
  returns setof geo_result2 as
$body$
declare
  sfld stdaddr;
  rec geo_result2;
  t0 timestamp with time zone;

begin
  for sfld in execute sql loop
    continue when sfld.state is null and sfld.postcode is null;
    t0 := clock_timestamp();
    for rec in select * from imt_geocoder(sfld, fuzzy, aoffset, method) loop
      return next rec;
      exit;
    end loop;
    if method < 0 then
      raise notice 'id=%, time=%', sfld.id, clock_timestamp()-t0;
    end if;
  end loop;

  return;
end;
$body$
  language plpgsql immutable strict cost 5 rows 100;

---------------------------------------------------------------------------------------------

create or replace function imt_geocoder(sfld stdaddr, fuzzy integer, aoffset double precision, method integer)
  returns setof geo_result2 as
$body$
declare
  sql text;
  rec geo_result2;
  cnt integer := 0;

begin

--raise notice 'sfld: %', sfld;
  if sfld.state is null and sfld.postcode is null then
    return;
  end if;

  -- make sure we got an address that we can use
  if coalesce(sfld.house_num,sfld.predir,sfld.qual,sfld.pretype,sfld.name,sfld.suftype,sfld.sufdir) is not null and
     coalesce(sfld.city,sfld.state,sfld.postcode /*,sfld.country */) is not null then
    select * into sql from imt_geo_planner2(sfld, fuzzy);
--raise notice '%', sql;
    if found then
      for rec in select 
                             0::int4 as rid,
                             a.id,
                             b.tlid as id1,
                             b.side as id2,
                             b.tfid as id3,
                             sfld.house_num as completeaddressnumber,
                             b.predirabrv as predirectional,
                             b.prequalabr as premodifier,
                             b.sufqualabr as postmodifier,
                             b.pretypabrv as pretype,
                             b.name as streetname,
                             b.suftypabrv as posttype,
                             b.sufdirabrv as postdirectional,
                             coalesce(b.ac5,b.ac4,b.ac3) as placename,
                             b.usps as placename_usps,
                             b.ac2 as statename,
                             b.ac1 as countrycode,
                             b.postcode as zipcode,
                             sfld.building,
                             sfld.unit,
                             plan,
                             parity,
                             a.score as geocodematchcode,
                             a.x,
                             a.y
                   from (select * from imt_geo_score(sql, sfld, aoffset)) as a, streets b where a.id=b.gid order by a.score desc, plan asc loop
        cnt := cnt + 1;
        rec.rid := cnt;
        return next rec;
      end loop;
    end if;
  end if;
  return;
end;
$body$
  language plpgsql immutable strict cost 5 rows 100;

----------------------------------------------------------------------------------------------
/*
-- DEPRECATED --

create or replace function imt_geocoder2(micro_in text, macro_in text, opts integer)
  returns setof geo_result as
$body$
declare
  sfld stdaddr;
  sql text;
  rec geo_result;
  cnt integer := 0;

begin
  sql := 'select 0::int4 as id, '||trim(both from quote_literal(micro_in))||'::text as micro, '||trim(both from quote_literal(macro_in))||'::text as macro';
--raise notice 'sql: %', sql;
  select * into sfld from standardize_address(
        'lex',
        'gaz',
        'rules',
        sql);
--raise notice 'sfld: %', sfld;

  -- make sure we got a result that we can use
  if found  and coalesce(sfld.house_num,sfld.predir,sfld.qual,sfld.pretype,sfld.name,sfld.suftype,sfld.sufdir,sfld.city,sfld.state,sfld.postcode,sfld.country) is not null then
    select * into sql from imt_geo_planner2(sfld, opts);
--raise notice '%', sql;
    if found then
      for rec in select * from (select * from imt_geo_score(sql, sfld, 0.0)) as a order by a.score desc, plan asc loop
        cnt := cnt + 1;
        return next rec;
      end loop;
    end if;
  end if;
  return;
end;
$body$
  language plpgsql immutable strict;

*/
-------------------------------------------------------------------------------------------------
-- to add support for intersections

-- Function: imt_intersection_point_to_id(geometry, double precision)

-- DROP FUNCTION imt_intersection_point_to_id(geometry, double precision);

CREATE OR REPLACE FUNCTION imt_intersection_point_to_id(point geometry, tolerance double precision)
  RETURNS integer AS
$BODY$
DECLARE
        row record;
        point_id int;
BEGIN
        SELECT id
          FROM intersections_tmp
         WHERE st_dwithin(point, the_geom, tolerance)
          INTO row;

        IF NOT FOUND THEN
                INSERT INTO intersections_tmp (the_geom) VALUES (point) RETURNING id INTO row;
        END IF;

        RETURN row.id;
END;
$BODY$
  LANGUAGE plpgsql VOLATILE STRICT
  COST 100;
ALTER FUNCTION imt_intersection_point_to_id(geometry, double precision) OWNER TO postgres;


-- Function: imt_make_intersections(character varying, double precision, character varying, character varying)

-- DROP FUNCTION imt_make_intersections(character varying, double precision, character varying, character varying);

CREATE OR REPLACE FUNCTION imt_make_intersections(geom_table character varying, tolerance double precision, geo_cname character varying, gid_cname character varying)
  RETURNS character varying AS
$BODY$
DECLARE
    points record;
    source_id int;
    target_id int;
    srid integer;
    countids integer;

BEGIN
    BEGIN
        DROP TABLE intersections_tmp;
        EXCEPTION
            WHEN UNDEFINED_TABLE THEN
        END;
    BEGIN
        DROP TABLE st_vert_tmp;
        EXCEPTION
            WHEN UNDEFINED_TABLE THEN
        END;

        EXECUTE 'CREATE TABLE intersections_tmp (id serial, cnt integer, dcnt integer)';

        EXECUTE 'SELECT srid FROM geometry_columns WHERE f_table_name='|| quote_literal(geom_table) INTO srid;

        EXECUTE 'SELECT max('||quote_ident(gid_cname)||') as countids FROM '|| quote_ident(geom_table) INTO countids;

        EXECUTE 'SELECT addGeometryColumn(''intersections_tmp'', ''the_geom'', '||srid||', ''POINT'', 2)';

        CREATE INDEX intersections_tmp_idx ON intersections_tmp USING GIST (the_geom);

        EXECUTE 'CREATE TABLE st_vert_tmp (gid integer, vid integer, wend char(1))';

        FOR points IN EXECUTE 'SELECT ' || quote_ident(gid_cname) || ' AS id,'
                || ' ST_PointN(ST_GeometryN('|| quote_ident(geo_cname) ||', 1), 1) AS source,'
                || ' ST_PointN(ST_GeometryN('|| quote_ident(geo_cname) ||', 1), ST_NumPoints(ST_GeometryN('
                || quote_ident(geo_cname) ||', 1))) AS target'
                || ' FROM ' || quote_ident(geom_table) || ' ORDER BY '
                || quote_ident(gid_cname)
            loop

                IF points.id%1000=0 THEN
                      RAISE NOTICE '% out of % edges processed', points.id, countids;
                END IF;

                source_id := imt_intersection_point_to_id(st_setsrid(points.source, srid), tolerance);
                target_id := imt_intersection_point_to_id(st_setsrid(points.target, srid), tolerance);

                IF source_id is null OR target_id is null THEN
                      RAISE EXCEPTION 'source_id: %, target_id: % for gid: %', source_id, target_id, points.id;
                END IF;

                INSERT INTO st_vert_tmp values(points.id, source_id, 'S');
                INSERT INTO st_vert_tmp values(points.id, target_id, 'T');

        END LOOP;


        create index st_vert_tmp_vid_idx on st_vert_tmp using btree (vid);
        create index st_vert_tmp_gid_idx on st_vert_tmp using btree (gid);
        CREATE INDEX intersections_tmp_id_idx ON intersections_tmp USING btree (id ASC NULLS LAST);
        CREATE INDEX intersections_tmp_dcnt_idx ON intersections_tmp USING btree (dcnt ASC NULLS LAST);
        DROP INDEX IF EXISTS streets_gid_idx;
        CREATE INDEX streets_gid_idx ON streets USING btree (gid ASC NULLS LAST);
/*
        update intersections_tmp as a set
          cnt  = (select count(*) from st_vert_tmp b where b.vid=a.id),
          dcnt = (select count(distinct name) from st_vert_tmp b, streets c where b.gid=c.gid and b.vid=a.id and c.name is not null);
*/
    RETURN 'OK';

/*
    -- The following are various uses of these tables

    -- get a count of intersections with multiple street names
    select count(*) from intersections_tmp where dcnt>1;

    -- see a sample of the intersections and the street names
    select distinct a.id, c.name
      from intersections_tmp a, st_vert_tmp b, streets c
     where a.id=b.vid and b.gid=c.gid and a.dcnt>1 and c.name is not null
     order by a.id, c.name
     limit 100;

    -- find the nearest intersection to a point
     select a.id, round(distance_sphere(setsrid(makepoint(-90.51791, 14.61327), 4326), a.the_geom)::numeric,1) as dist, list(c.name),
            c.r_ac5, c.r_ac4, c.r_ac3, c.r_ac2, c.r_ac1
      from intersections_tmp a, st_vert_tmp b, streets c
     where a.id=b.vid and b.gid=c.gid and a.dcnt>1 and c.name is not null
       and expand(setsrid(makepoint(-90.51791, 14.61327), 4326), 0.0013) && a.the_geom
     group by a.id, dist, c.r_ac5, c.r_ac4, c.r_ac3, c.r_ac2, c.r_ac1
     order by dist, a.id;


*/

END;
$BODY$
  LANGUAGE plpgsql VOLATILE STRICT
  COST 100;
ALTER FUNCTION imt_make_intersections(character varying, double precision, character varying, character varying) OWNER TO postgres;


-------------------------------------------------------------------------------------------------
-- preparing tables for searching for intersections

select imt_make_intersections('streets', 0.000001, 'the_geom', 'gid');
-- Total query runtime: 29103656 ms.
-- Total query runtime: 05:10:18024 hours

DROP INDEX intersections_tmp_dcnt_idx;
DROP INDEX intersections_tmp_id_idx;
DROP INDEX intersections_tmp_idx;

update intersections_tmp as a set
          cnt  = (select count(*) from st_vert_tmp b where b.vid=a.id),
          dcnt = (select count(distinct name) from st_vert_tmp b, streets c where b.gid=c.gid and b.vid=a.id and c.name is not null);
-- Query returned successfully: 20487647 rows affected, 12578580 ms execution time.
-- Query returned successfully: 20694608 rows affected, 48:24 minutes execution time.

CREATE INDEX intersections_tmp_dcnt_idx
  ON intersections_tmp
  USING btree
  (dcnt);

CREATE INDEX intersections_tmp_id_idx
  ON intersections_tmp
  USING btree
  (id);

CREATE INDEX intersections_tmp_idx
  ON intersections_tmp
  USING gist
  (the_geom);
--

select count(*) from intersections_tmp where dcnt>1;
-- 10,441,965
-- 10,457,781 (tiger2016)

begin;
drop index if exists st_vert_tmp_gid_idx;
delete from st_vert_tmp where vid in (select id from intersections_tmp where dcnt<2);
-- Query returned successfully: 29063468 rows affected, 62809203 ms execution time.
-- Query returned successfully: 26440345 rows affected, 07:07 minutes execution time.

CREATE INDEX st_vert_tmp_gid_idx ON st_vert_tmp USING btree (gid);
-- Query returned successfully with no result in 03:15 minutes.

commit;

vacuum analyze st_vert_tmp;
-- Query returned successfully with no result in 03:13 minutes.

-- sample query to search for an intersection

select * from stdstreets where id=20726192;
select * from stdstreets a where a.name='RADCLIFFE'
         and ( a.city='NORTH CHELMSFORD' or a.city='CHELMSFORD' )
         and a.state='MASSACHUSETTS';
select * from stdstreets c where c.name='CROOKED SPRING'
         and ( c.city='NORTH CHELMSFORD' or c.city='CHELMSFORD' )
         and c.state='MASSACHUSETTS';

explain analyze
select distinct st_x(e.the_geom), st_y(e.the_geom)
  from stdstreets a, st_vert_tmp b, stdstreets c, st_vert_tmp d, intersections_tmp e
 where a.id=b.gid and c.id=d.gid and b.vid=d.vid and b.vid=e.id
   and a.name='RADCLIFFE' and a.city='CHELMSFORD' and a.state='MASSACHUSETTS'
   and c.name='CROOKED SPRING' and c.city='CHELMSFORD' and c.state='MASSACHUSETTS'

select * from parse_address('radcliffe rd @ crooked spring rd, north chelmsford ma 01863');
-- "";"radcliffe rd";"crooked spring rd";"";"north chelmsford";"MA";"01863";"";"US"

drop type if exists geo_intersection cascade;
create type geo_intersection as (
    id1 integer,                        -- a.id as id1,
    id2 integer,                        -- c.id as id2,
    id1a integer,                       -- f.tlid as id1a,
    id2a integer,                       -- g.tlid as id2a,
    street1 text,                       -- array_to_string(ARRAY[f.predirabrv, f.prequalabr, f.pretypabrv, f.name, f.suftypabrv, f.sufdirabrv, f.prequalabr], ' ') as street1,
    street2 text,                       -- array_to_string(ARRAY[g.predirabrv, g.prequalabr, g.pretypabrv, g.name, g.suftypabrv, g.sufdirabrv, g.prequalabr], ' ') as street2,
    placename text,                     -- coalesce(b.ac5,b.ac4,b.ac3) as placename,
    placename_usps text,                -- b.usps as placename_usps,
    statename text,                     -- b.ac2 as statename,
    countrycode text,                   -- b.ac1 as countrycode,
    postcode text,                      -- b.postcode as zipcode,
    plan integer,                       -- plan,
    geocodematchcode double precision,  -- (a.score+c.score)/2.0 as geocodematchcode,
    x double precision,                 -- st_x(e.the_geom) as x,
    y double precision                  -- st_y(e.the_geom) as y
);



create or replace function imt_geocode_intersection(micro1_in text, micro2_in text, macro_in text, fuzzy integer, method integer)
  returns setof geo_intersection as
$body$
declare
  srec stdaddr;
  sql text;
  sql2 text;
  asql text[];
  sfld stdaddr[];
  rec geo_intersection;
  cnt integer := 0;

begin

  -- add house num for standardizer
  srec := standardize_address('lex', 'gaz', 'rules', trim(both from '10 '||micro1_in)::text, trim(both from macro_in)::text);
  -- kill house num now because we don't want it for intersections
  srec.house_num := null;
--raise notice 'st1: srec: %', srec;

  -- make sure we got a result that we can use
  if coalesce(srec.house_num,srec.predir,srec.qual,srec.pretype,srec.name,srec.suftype,srec.sufdir,srec.city,srec.state,srec.postcode,srec.country) is not null then
      select * into sql2 from imt_geo_planner2(srec, fuzzy);
      if found then
          asql[cnt] := sql2;
--raise notice 'st1: asql: %', sql2;
          sfld[cnt] := srec;
          cnt := cnt + 1;
      else
          return;
      end if;
  else
      return;
  end if;

  -- add house num for standardizer
  srec := standardize_address('lex', 'gaz', 'rules', trim(both from '10 '||micro2_in)::text, trim(both from macro_in)::text);
  -- kill house num now because we don't want it for intersections
  srec.house_num := null;
--raise notice 'st2: srec: %', srec;

  -- make sure we got a result that we can use
  if coalesce(srec.house_num,srec.predir,srec.qual,srec.pretype,srec.name,srec.suftype,srec.sufdir,srec.city,srec.state,srec.postcode,srec.country) is not null then
      select * into sql2 from imt_geo_planner2(srec, fuzzy);
      if found then
          asql[cnt] := sql2;
--raise notice 'st2: asql: %', sql2;
          sfld[cnt] := srec;
          cnt := cnt + 1;
      else
          return;
      end if;
  else
      return;
  end if;

  if (sfld[0].state is null and sfld[0].postcode is null) or (sfld[0].state is null and sfld[0].postcode is null) then
    return;
  end if;

  for rec in select * from (
               select distinct on (x, y)
                 a.id as id1,
                 c.id as id2,
                 f.tlid as id1a,
                 g.tlid as id2a,
                 array_to_string(ARRAY[f.predirabrv, f.prequalabr, f.pretypabrv, f.name, f.suftypabrv, f.sufdirabrv, f.prequalabr], ' ') as street1,
                 array_to_string(ARRAY[g.predirabrv, g.prequalabr, g.pretypabrv, g.name, g.suftypabrv, g.sufdirabrv, g.prequalabr], ' ') as street2,
                 coalesce(f.ac5,f.ac4,f.ac3) as placename,
                 f.usps as placename_usps,
                 f.ac2 as statename,
                 f.ac1 as countrycode,
                 f.postcode as zipcode,
                 a.plan,
                 (a.score+c.score)/2.0 as geocodematchcode,
                 st_x(e.the_geom) as x,
                 st_y(e.the_geom) as y
               from 
                   (select * from imt_geo_score(asql[0], sfld[0], 0.0)) as a, st_vert_tmp b, streets f,
                   (select * from imt_geo_score(asql[1], sfld[1], 0.0)) as c, st_vert_tmp d, streets g,
                   intersections_tmp e
              where
                   a.id=b.gid and b.gid=f.gid and
                   c.id=d.gid and d.gid=g.gid and
                   b.vid=d.vid and
                   b.vid=e.id and
                   array_to_string(ARRAY[f.predirabrv, f.prequalabr, f.pretypabrv, f.name, f.suftypabrv, f.sufdirabrv, f.prequalabr], ' ') != array_to_string(ARRAY[g.predirabrv, g.prequalabr, g.pretypabrv, g.name, g.suftypabrv, g.sufdirabrv, g.prequalabr], ' ')
              order by x, y ) as foo
            order by geocodematchcode desc
  loop
        return next rec;
  end loop;
  return;
end;
$body$
  language plpgsql immutable strict cost 5 rows 100;


-- site/parcel geocoding -------------------------------------------------------------------------------------------

-- create a table and view to add dmetaphone to basque data
CREATE TABLE basques.dmeta
(
   id integer NOT NULL, 
   name_dmeta character varying(4), 
    PRIMARY KEY (id)
) 
WITH (
  OIDS = FALSE
);

insert into basques.dmeta select id, coalesce(dmetaphone(name),name) from basques.stdstreets;
-- Query returned successfully: 217569 rows affected, 4196 ms execution time.

update basques.dmeta a set name_dmeta=coalesce(dmetaphone(b.name),b.name)
  from basques.stdstreets b where a.id=b.id and length(a.name_dmeta)=0;
-- Query returned successfully: 21 rows affected, 250 ms execution time.

-- cleanup and fix a bad attempt to change the process
-- update basques.dmeta a set name_dmeta=nullif(coalesce(dmetaphone(b.name),b.name),'')
--   from basques.stdstreets b where a.id=b.id and length(a.name_dmeta)=0;
-- Query returned successfully: 13392 rows affected, 6724 ms execution time.

create index dmeta_name_dmeta_idx on basques.dmeta using btree (name_dmeta);
-- Query returned successfully with no result in 730 ms.

create view basques.v_stdstreets as select a.*, b.name_dmeta from basques.stdstreets a, basques.dmeta b;



-----------------------------------------------------------------------------------------------------------------------
set search_path to data, public;

select * from standardize_address('lex', 'gaz', 'rules', '11 radcliff rd', 'north chelmsford, ma 01863');


select * from imt_geocode_intersection('radcliff rd', 'crooked spring rd', 'north chelmsford, ma 01863', 1, 0);
select * from imt_geocode_intersection('bayard ln', 'hodge rd', 'princeton nj 08540', 1, 0);
select * from imt_geocode_intersection('stanworth ln', 's stanworth dr', 'princeton nj 08540', 1, 0);          -- NOT FOUND
select * from imt_geocode_intersection('witherspoon st', 'witherspoon ln', 'princeton nj 08540', 1, 0);
select * from imt_geocode_intersection('quarry st', 'berrien ct', 'princeton nj 08540', 1, 0);


-------------------------------------------------------------------------------------------------

select * from imt_geocoder('11 radcliffe rd', 'north chelmsford, ma, 01863', 0, 10.0, 0);
select * from imt_geocoder('11 radcliffe rd', 'north chelmsford, ma, 01863', 1, 0.0, 0);
select * from imt_geocoder('11 radcliffe rd', 'north chelmsford, ma, 01863', 0, 0.0, 0);
select * from imt_geocoder('11 radcliff rd', 'north chelmsford, ma, 01863', 1, 0.0, 0);
select * from imt_geocoder('11 radcliff rd', 'chelmsford, ma, 01863', 1, 0.0, 0);
select * from imt_geocoder('11 radcliff rd', 'chelmsford, ma', 1, 0.0, 0);
select * from imt_geocoder('11 ratcliff rd', 'chelmsford, ma', 1, 0.0, 0);
select * from imt_geocoder('11 ratcliff rd', 'chelmsford, ma 01824', 1, 0.0, 0);
select * from imt_geocoder('11 ratcliff rd', 'north chelmsford, ma 01824', 1, 0.0, 0); -- NOT FOUND
select * from imt_geocoder('1 ridgewood ln', 'burlington, ma 01803', 0, 10.0, 0);
select * from imt_geocoder('1 ridgewood ln', 'burlington, ma 01803', 1, 0.0, 0);
select * from imt_geocoder('116 commonwealth ave, apt a', 'west concord, ma 01742', 0, 0.0, 0);
select * from imt_geocoder('116 commonwealth ave, apt a', 'west concord, ma 01742', 1, 0.0, 0);
select * from imt_geocoder('116 commonwealth ave', 'concord, ma 01742', 0, 0.0, 0);
select * from imt_geocoder('116 commonwealth ave', 'west concord, ma 01742', 1, 0.0, 0);
select * from imt_geocoder('101 newbury st', 'boston, ma 02116', 0, 0.0, 0);
select * from imt_geocoder('101 newbury st', 'boston, ma 02116', 1, 0.0, 0);
select * from imt_geocoder('84 bayard ln', 'princeton, nj 08540', 0, 0.0, 0);
select * from imt_geocoder('84 bayard ln', 'princeton, nj 08540', 1, 0.0, 0);
select * from imt_geocoder('10 stanworth ln', 'princeton, nj 08540', 1, 0.0, 0);
select * from imt_geocoder('64 depot hill rd', 'amenia, ny 12501', 1, 0.0, 0);
select * from imt_geocoder('49 maple st, apt 127', 'manchester center, vt 05255 US', 1, 0.0, 0);
select * from imt_geocoder('2099 university ave w', 'st paul, mn, 55104', 0, 0.0, 0);
select * from imt_geocoder('2099 university ave w', 'ste paul, mn, 55104', 0, 0.0, 0);
select * from imt_geocoder('2099 university ave w', 'saint paul, mn, 55104-3431', 0, 0.0, 0);
select * from imt_geocoder('7 Co Hwy 34', 'WILLIAMSPORT,PA,US,17701', 1, 0.0, 0);            -- Fixed -- FAILS(2016)
select * from imt_geocoder('1071 B Ave', 'Loxley, AL 36551', 1, 0.0, 0);                     -- FAILS, google finds closest number ~3000
select * from imt_geocoder('1071 Chicago Ave', 'Loxley, AL 36551', 1, 0.0, 0);               -- looks like B Ave === Chicago Ave

select * from imt_geocoder('101 7th St', 'St. Paul, MN', 1, 0.0, 0);  
-- Total query runtime: 67303 ms.  259 rows retrieved.
-- Total query runtime: 2.4 secs.  264 rows retrieved. (2016)
select * from imt_geocoder('101 7th St', 'Saint Paul, MN', 0, 0.0, 0);  -- Total query runtime: 452 ms.  259 rows retrieved.
select * from imt_geocoder('101 7th St', 'St. Paul, MN 55102', 0, 0.0, 0);  -- Total query runtime: 93 ms.  1 row retrieved.

'101 7th St, Saint Paul, MN'                       -- Takes 10 sec !!!!  FIX THIS
'101 7th St, Saint Paul, MN 55102'                 -- Better
'123-125 Washington Avenue, Boston MA 02130'       -- FAILS
'8525 COTTAGEWOOD TERR, Blaine, MN 55434'          -- good
'8525 COTTAGE WOOD TERR, Blaine, MN 55434'         -- good
'3420 RHODE ISLAND AVE S, ST. LOUIS PARK, MN 55426'     -- fixed
'3420 RHODEISLAND AVE S, ST. LOUIS PARK, MN 55426'      -- fixed
'3420 RHODE ISLAND AVE S, SAINT LOUIS PARK, MN 55426'   -- ok
'1701 Main Street, Hopkins, MN 55343'                   -- FAILS
'1701 Mainstreet, Hopkins, MN 55343'                    -- ok
'156 Galewski Dr Airport Industrial Park, Winona, MN 55987'   -- works great
'477 Camino del Rio South, San Diego, CA 94115'         -- ok
'18208 N COUNTY ROAD 241, ALACHUA, FL 32615'            -- SLOW 808 ms
'6226 E PIMA ST STE 999, TUCSON AZ'                     -- SLOW 4.3 sec fuzzy, 108 ms, hot, no fuzzy
'100 E BROADWAY AVE STE A, TUCSON, AZ'                  -- SLOW 2.79 sec not fuzzy, 2.89 sec hot, fuzzy
'2601 24TH AVE,  NY 11102'                              -- FAILS seems to be missing Tiger records -- RESEARCH
'2601 24TH AVE, ASTORIA NY 11102'                       -- 
'2601 24TH AVE, ASTORIA NY 111022337'                   -- 
'2601 24TH AVE, ASTORIA NY 11102-2337'                  -- 

select length(dmetaphone('34'));

select distinct coalesce(ac5, ac4) from streets where coalesce(ac5, ac4) ilike 'st. %';  -- 122 rows added to gazeteer.csv

'SH 121 @ Denton Tap, Coppell, TX'
select * from imt_geocode_intersection('SH 121', 'Denton Tap', 'Coppell', 1, 0);                  -- FAST FAILS (missing state)
select * from imt_geocode_intersection('State Highway 121', 'Denton Tap', 'Coppell', 0, 0);       -- FAST FAILS (missing state)
select * from imt_geocode_intersection('SH 121', 'Denton Tap', 'Coppell TX', 1, 0);               -- SLOW 6-12 sec FAILS because SH -> 'SHOPPING CENTER'
select * from imt_geocode_intersection('Hwy 121', 'Denton Tap', 'Coppell TX', 1, 0);              -- SLOW 7-14 sec cold FAILS, 531 ms hot
select * from imt_geocode_intersection('State Highway 121', 'Denton Tap', 'Coppell, TX', 1, 0);   -- 499 ms hot, FAILS

'SH 121 @ Business 121, Coppell, TX'
'S. Belt Line @ Dividend, Coppell, TX'
select * from imt_geocode_intersection('S. Belt Line', 'Dividend', 'Coppell, TX', 1, 0);   -- SLOW 9 sec FAILS

select * from stdstreets a, streets b where a.id=b.gid and a.name='B' and city='LOXLEY' and state='ALABAMA';

select * from imt_geocode_intersection('N. Belt Line', 'I-635', 'Coppell, TX', 0, 0);   -- SLOW 2.6 sec FAILS
select * from imt_geocode_intersection('N. Belt Line', 'I-635', 'Coppell, TX', 1, 0);   -- SLOW 8.9 sec FAILS
select * from parse_address('N. Belt Line @ I- 635, Coppell TX');
select * from parse_address('N. Belt Line @ I-635, Coppell TX');

/*
-- fix, we do not pass sql as argument!!

select * from standardize_address(
        'lex',
        'gaz',
        'rules',
        'select 0::int4 as id, ''116 I- 635''::text as micro, ''Coppell TX''::text as macro
         union all
         select 1::int4 as id, ''116 I-635''::text as micro, ''Coppell TX''::text as macro');

select * from standardize_address('lex', 'gaz', 'rules',
        'select 0::int4 as id, ''116 I- 635''::text as micro, ''Coppell TX''::text as macro
         union all
         select 1::int4 as id, ''116 I-635''::text as micro, ''Coppell TX''::text as macro');



select * from parse_address('212 n 3rd ave, Minneapolis, mn 55401, USA');                    -- FIX THIS
select * from standardize_address(
        'lex',
        'gaz',
        'rules',
        'select 0::int4 as id, ''212 n 3rd ave''::text as micro, ''Minneapolis, mn 55401, USA''::text as macro');

'501 7TH ST NW, ROCHESTER, MN 55901'
select * from parse_address('501 7TH ST NW, ROCHESTER, MN 55901');                    -- good
select * from standardize_address(                                                    -- good
        'lex',
        'gaz',
        'rules',
        'select 0::int4 as id, ''501 7TH ST NW''::text as micro, ''ROCHESTER, MN 55901''::text as macro');
select * from standardize_address(                                                    -- good
        'lex',
        'gaz',
        'rules',
        'select 0::int4 as id, ''501 7TH NW ST''::text as micro, ''ROCHESTER, MN 55901''::text as macro');
*/

select * from parse_address('2099 university ave w, saint paul, mn, 55104-3431');
select * from parse_address('university ave w @ main st, saint paul, mn, 55104-3431');

select * from parse_address('385 Landgrove Rd  Landgrove VT 05148');
-- "385";"Landgrove Rd";"";"385 Landgrove Rd";"Landgrove";"VT";"05148";"";"US"

select * from parse_address('385 Landgrove Rd  Landgrove VT 05148 US');                      -- FIX THIS, this is seriously broken!!!!!!!
-- "385";"Landgrove Rd Landgrove VT 05148";"";"385 Landgrove Rd Landgrove VT 05148";"US";"";"";"";"US"
select * from parse_address('385 Landgrove Rd,  Landgrove VT 05148 US');                     -- FIX THIS, this is seriously broken!!!!!!!
-- "385";"Landgrove Rd";"";"385 Landgrove Rd";"Landgrove VT 05148 US";"";"";"";"US"

/*
-- FIXME --

select * from standardize_address(
--        'lex',
        'select seq, word::text, stdword::text, token from gaz union all select seq, word::text, stdword::text, token from lex ',
        'gaz',
        'rules',
        'select 0::int4 as id, ''1071 B Ave''::text as micro, ''Loxley, AL 36551''::text as macro');

select * from standardize_address(
        'lex',
        'gaz',
        'rules',
        'select 0::int4 as id, ''116A commonwealth ave''::text as micro, ''west concord, ma 01742''::text as macro');

select * from standardize_address(
        'lex',
        'gaz',
        'rules',
        'select 0::int4 as id, ''116 commonwealth ave apt a''::text as micro, ''west concord, ma 01742''::text as macro');
-- 0;"";"116";"";"";"";"COMMONWEALTH";"AVENUE";"";"";"";"WEST CONCORD";"MASSACHUSETTS";"";"01742";"";"APARTMENT A"

select * from standardize_address(
        'lex',
        'gaz',
        'rules',
        'select 0::int4 as id, ''116 A commonwealth ave''::text as micro, ''west concord, ma 01742''::text as macro');
-- 0;"";"116";"";"ALTERNATE";"";"COMMONWEALTH";"AVENUE";"";"";"";"WEST CONCORD";"MASSACHUSETTS";"";"01742";"";""


select * from standardize_address(
        'lex',
        'gaz',
        'rules',
        'select 0::int4 as id, ''7 Co Hwy 34''::text as micro, ''WILLIAMSPORT,PA,US,17701''::text as macro');
*/


select *
  from streets
 where gid in (
    select id
      from stdstreets
     where coalesce(building, house_num, predir, qual, pretype, name,
                    suftype, sufdir, ruralroute, extra,
                    city, state, country, postcode, box, unit) is null
      )
      and array_to_string(ARRAY[predirabrv, pretypabrv, prequalabr, name,
                             sufdirabrv, suftypabrv, sufqualabr], ' ') = 'Co Hwy 34';
-- gid: 38505445

select * from standardize_address(
        'lex',
        'gaz',
        'rules',
        'select gid as id,
                array_to_string(ARRAY[refaddr::varchar, predirabrv, pretypabrv, prequalabr, name, sufdirabrv, suftypabrv, sufqualabr], '' '') as micro,
                array_to_string(ARRAY[coalesce(usps,ac5,ac4,ac3),ac2,ac1,postcode], '','') as macro
           from streets
          where gid=38505445');

select gid as id,
                array_to_string(ARRAY[refaddr::varchar, predirabrv, pretypabrv, prequalabr, name, sufdirabrv, suftypabrv, sufqualabr], ' ') as micro,
                array_to_string(ARRAY[coalesce(usps,ac5,ac4,ac3),ac2,ac1,postcode], ',') as macro, *
           from streets
          where gid=38505445;
-- 7 Co Hwy 34

select state, count(*) from stdstreets group by state order by state;
select ac2, count(*) from streets group by ac2 order by ac2;
select * from stdstreets limit 100;

select * from standardize_address(
  'lex', 
  'gaz', 
  'rules', 
  'select 0::int4 as id, '||quote_literal(trim(both from  '0 newbury st' ))||'::text as micro, '||quote_literal(trim(both from  'boston, ma 02116' ))||'::text as macro');


select to_number('116 a', '999999');
select 10 between 1 and 10, 10 between 10 and 1;
select max(gid), min(gid) from streets;



select * from stdstreets where id=20726192;
select * from streets where gid=20726192;

select * from imt_geocoder('11 radcliffe rd', 'north chelmsford, ma', 1, 0.0, 0);
select * from imt_geocoder('11 radcliff rd', 'chelmsford, ma', 1, 0.0, 0);


-- internal query from imt_geocode_instersection

explain analyze
select distinct on (e.id)
                 a.id as id1,
                 c.id as id2,
                 f.tlid as id1a,
                 g.tlid as id2a,
                 array_to_string(ARRAY[f.predirabrv, f.prequalabr, f.pretypabrv, f.name, f.suftypabrv, f.sufdirabrv, f.prequalabr], ' ') as street1,
                 array_to_string(ARRAY[g.predirabrv, g.prequalabr, g.pretypabrv, g.name, g.suftypabrv, g.sufdirabrv, g.prequalabr], ' ') as street2,
                 coalesce(f.ac5,f.ac4,f.ac3) as placename,
                 f.usps as placename_usps,
                 f.ac2 as statename,
                 f.ac1 as countrycode,
                 f.postcode as zipcode,
                 a.plan,
                 (a.score+c.score)/2.0 as geocodematchcode,
                 st_x(e.the_geom) as x,
                 st_y(e.the_geom) as y
               from 
                   (select * from imt_geo_score(
'select distinct on (id) foo.* from (select a.*, 0 as plan, null as parity from stdstreets a, streets b where a.id=b.gid  and a.name=''RADCLIFFE'' and case when a.suftype is not null then a.suftype=''ROAD'' else true end and a.city=''NORTH CHELMSFORD'' and a.state=''MASSACHUSETTS'' and a.postcode=''01863''
    union
    select a.*, 1 as plan, null as parity from stdstreets a, streets b where a.id=b.gid  and a.name=''RADCLIFFE'' and case when a.suftype is not null then a.suftype=''ROAD'' else true end and a.postcode=''01863''
    union
    select a.*, 2 as plan, null as parity from stdstreets a, streets b where a.id=b.gid  and a.name=''RADCLIFFE'' and case when a.suftype is not null then a.suftype=''ROAD'' else true end and a.city=''NORTH CHELMSFORD'' and a.state=''MASSACHUSETTS''
    union
    select a.*, 3 as plan, null as parity from stdstreets a, streets b where a.id=b.gid  and a.name=''RADCLIFFE'' and a.city=''NORTH CHELMSFORD'' and a.state=''MASSACHUSETTS'' and a.postcode=''01863''
    union
    select a.*, 4 as plan, null as parity from stdstreets a, streets b where a.id=b.gid  and a.name=''RADCLIFFE'' and a.postcode=''01863''
    union
    select a.*, 5 as plan, null as parity from stdstreets a, streets b where a.id=b.gid  and a.name=''RADCLIFFE'' and a.city=''NORTH CHELMSFORD'' and a.state=''MASSACHUSETTS'') as foo order by id' , 
(select standardize_address(
  'lex', 
  'gaz', 
  'rules', 
  'select 0::int4 as id, '||quote_literal(trim(both from  '0 radcliffe rd' ))||'::text as micro, '||quote_literal(trim(both from  'north chelmsford, ma 01863' ))||'::text as macro'))
                   , 0.0)) as a, st_vert_tmp b, streets f,
                   (select * from imt_geo_score(
'select distinct on (id) foo.* from (select a.*, 0 as plan, null as parity from stdstreets a, streets b where a.id=b.gid  and a.name=''CROOKED SPRING'' and case when a.suftype is not null then a.suftype=''ROAD'' else true end and a.city=''NORTH CHELMSFORD'' and a.state=''MASSACHUSETTS'' and a.postcode=''01863''
    union
    select a.*, 1 as plan, null as parity from stdstreets a, streets b where a.id=b.gid  and a.name=''CROOKED SPRING'' and case when a.suftype is not null then a.suftype=''ROAD'' else true end and a.postcode=''01863''
    union
    select a.*, 2 as plan, null as parity from stdstreets a, streets b where a.id=b.gid  and a.name=''CROOKED SPRING'' and case when a.suftype is not null then a.suftype=''ROAD'' else true end and a.city=''NORTH CHELMSFORD'' and a.state=''MASSACHUSETTS''
    union
    select a.*, 3 as plan, null as parity from stdstreets a, streets b where a.id=b.gid  and a.name=''CROOKED SPRING'' and a.city=''NORTH CHELMSFORD'' and a.state=''MASSACHUSETTS'' and a.postcode=''01863''
    union
    select a.*, 4 as plan, null as parity from stdstreets a, streets b where a.id=b.gid  and a.name=''CROOKED SPRING'' and a.postcode=''01863''
    union
    select a.*, 5 as plan, null as parity from stdstreets a, streets b where a.id=b.gid  and a.name=''CROOKED SPRING'' and a.city=''NORTH CHELMSFORD'' and a.state=''MASSACHUSETTS'') as foo order by id' , 
(select standardize_address(
  'lex', 
  'gaz', 
  'rules', 
  'select 0::int4 as id, '||quote_literal(trim(both from  '0 crooked spring rd' ))||'::text as micro, '||quote_literal(trim(both from  'north chelmsford, ma 01863' ))||'::text as macro'))
                   , 0.0)) as c, st_vert_tmp d, streets g,
                   intersections_tmp e
              where
                   a.id=b.gid and 
                   c.id=d.gid and 
                   b.vid=d.vid and
                   b.gid=f.gid and
                   d.gid=g.gid and
                   d.vid=e.id
              order by e.id, geocodematchcode desc;


-- Playing around with trigrams package

CREATE INDEX name_city_state_words_trgm_gist ON name_city_state USING gist (words gist_trgm_ops);
-- Query returned successfully with no result in 19602362 ms.


select distinct name, similarity(name, 'radtcliff') as sml, levenshtein(name, 'radtcliff') as lev from data.stdstreets where name % 'radtcliff' order by lev asc, sml desc, name limit 20;
-- Total query runtime: 4742 ms.

"RADCLIFF";0.583333;9
"RATCLIFF";0.583333;9
"RACLIFF";0.5;9
"RADLIFF";0.5;9
"RAWCLIFF";0.461538;9
"RAYCLIFF";0.461538;9
"RADCLIFFE";0.428571;9
"RAHNCLIFF";0.428571;9
"RATCLIFFE";0.428571;9
"RAYECLIFF";0.428571;9
"RADCLIF";0.384615;9
"RAFLIFF";0.384615;9
"RATLIFF";0.384615;9
"ROCLIFF";0.384615;9
"RUCLIFF";0.384615;9
"RYCLIFF";0.384615;9
"ANTCLIFF";0.357143;9
"CUTCLIFF";0.357143;9
"RACKLIFF";0.357143;9
"RADCLIFT";0.357143;9


select distinct name, similarity(name, 'radtcliff') as sml from data.stdstreets where name % 'radtcliff' order by sml desc, name limit 50;
-- Total query runtime: 72475 ms.
-- Total query runtime: 4634 ms. (cached)

"RADCLIFF";0.583333
"RATCLIFF";0.583333
"RACLIFF";0.5
"RADLIFF";0.5
"RADCLIFF RUN";0.466667
"RAWCLIFF";0.461538
"RAYCLIFF";0.461538
"J B RATCLIFF";0.4375
"RADCLIFFE";0.428571
"RAHNCLIFF";0.428571
"RATCLIFFE";0.428571
"RAYECLIFF";0.428571
"DOCK RATCLIFF";0.411765
"RADCLIFF EAST";0.411765
"RADCLIFF WEST";0.411765
"RATCLIFF BLUFF";0.411765
"RATCLIFF COVE";0.411765
"RATCLIFF LOOP";0.411765
"RATCLIFF POND";0.411765
"RAHN CLIFF";0.4
"RAVENCLIFF";0.4
"BETTS RADCLIFF";0.388889
"EARLY RATCLIFF";0.388889
"FRANK RADCLIFF";0.388889
"JAMES RATCLIFF";0.388889
"LAURA RATCLIFF";0.388889
"MCKEE RATCLIFF";0.388889
"RADCLIFF NORTH";0.388889
"RADCLIFF SOUTH";0.388889
"RATCLIFF MANOR";0.388889
"RADCLIF";0.384615
"RAFLIFF";0.384615
"RATLIFF";0.384615
"ROCLIFF";0.384615
"RUCLIFF";0.384615
"RYCLIFF";0.384615
"CLIFF RAPER";0.375
"RAVEN CLIFF";0.375
"RAVENSCLIFF";0.375
"RADCLIFF DOCTOR";0.368421
"RADCLIFF HOLLER";0.368421
"RADCLIFF HOLLOW";0.368421
"RATCLIFF BRANCH";0.368421
"SAMUEL RATCLIFF";0.368421
"ANTCLIFF";0.357143
"CUTCLIFF";0.357143
"RACKLIFF";0.357143
"RADCLIFT";0.357143
"REDCLIFF";0.357143
"ROWCLIFF";0.357143



select distinct name, similarity(name, 'radtcliff north chelmsford massachusetts') as sml from data.stdstreets where name % 'radtcliff north chelmsford massachusetts' order by sml desc, name limit 50;
-- Total query runtime: 168297 ms.

"MASSACHUSETTS NORTH";0.487805
"CHELMSFORD NORTH";0.414634
"MASSACHUSETTS NORTHEAST";0.413043
"MASSACHUSETTS NORTHWEST";0.413043
"CHELMSFORD NORTHEAST";0.347826
"CHELMSFORD NORTHWEST";0.347826
"MASSACHUSETTS";0.341463
"5 MASSACHUSETTS";0.325581
"MASSACHUSETTS BAY";0.311111
"CLIFFORD NORTH";0.302326
"MASSACHUSSETTS";0.302326
"RADCLIFF NORTH";0.302326


-----------------------------------------------------------------------------------------------------------------------------

stdaddr=# select * from imt_geocode_intersection('radcliff rd', 'crooked spring rd', 'north chelmsford, ma 01863', 1, 0);
NOTICE:  sql: select 0::int4 as id, '10 radcliff rd'::text as micro, 'north chelmsford, ma 01863'::text as macro
    union all
select 1::int4 as id, '10 crooked spring rd'::text as micro, 'north chelmsford, ma 01863'::text as macro
NOTICE:  srec: (0,,,,,,RADCLIFF,ROAD,,,,"NORTH CHELMSFORD",MASSACHUSETTS,,01863,,)
NOTICE:  sql1: select distinct on (id) foo.* from (select a.*, 0 as plan, null as parity from stdstreets a, streets b where a.id=b.gid  and a.name='RADCLIFF' and case when a.suftype is not null then a.suftype='ROAD' else true end and a.city='NORTH CHELMSFORD' and a.state='MASSACHUSETTS' and a.postcode='01863'
    union
    select a.*, 1 as plan, null as parity from stdstreets a, streets b where a.id=b.gid  and a.name='RADCLIFF' and case when a.suftype is not null then a.suftype='ROAD' else true end and a.postcode='01863'
    union
    select a.*, 2 as plan, null as parity from stdstreets a, streets b where a.id=b.gid  and a.name='RADCLIFF' and case when a.suftype is not null then a.suftype='ROAD' else true end and a.city='NORTH CHELMSFORD' and a.state='MASSACHUSETTS'
    union
    select a.*, 3 as plan, null as parity from stdstreets a, streets b where a.id=b.gid  and a.name='RADCLIFF' and a.city='NORTH CHELMSFORD' and a.state='MASSACHUSETTS' and a.postcode='01863'
    union
    select a.*, 4 as plan, null as parity from stdstreets a, streets b where a.id=b.gid  and a.name='RADCLIFF' and a.postcode='01863'
    union
    select a.*, 5 as plan, null as parity from stdstreets a, streets b where a.id=b.gid  and a.name='RADCLIFF' and a.city='NORTH CHELMSFORD' and a.state='MASSACHUSETTS'
    union
    select a.*, 6 as plan, null as parity from stdstreets a, streets b where a.id=b.gid  and a.name_dmeta=dmetaphone('RADCLIFF') and a.city='NORTH CHELMSFORD' and a.state='MASSACHUSETTS' and a.postcode='01863'
    union
    select a.*, 7 as plan, null as parity from stdstreets a, streets b where a.id=b.gid  and a.name_dmeta=dmetaphone('RADCLIFF') and a.postcode='01863'
    union
    select a.*, 8 as plan, null as parity from stdstreets a, streets b where a.id=b.gid  and a.name_dmeta=dmetaphone('RADCLIFF') and a.city='NORTH CHELMSFORD' and a.state='MASSACHUSETTS') as foo order by id
NOTICE:  srec: (1,,,,,,"CROOKED SPRING",ROAD,,,,"NORTH CHELMSFORD",MASSACHUSETTS,,01863,,)
NOTICE:  sql2: select distinct on (id) foo.* from (select a.*, 0 as plan, null as parity from stdstreets a, streets b where a.id=b.gid  and a.name='CROOKED SPRING' and case when a.suftype is not null then a.suftype='ROAD' else true end and a.city='NORTH CHELMSFORD' and a.state='MASSACHUSETTS' and a.postcode='01863'
    union
    select a.*, 1 as plan, null as parity from stdstreets a, streets b where a.id=b.gid  and a.name='CROOKED SPRING' and case when a.suftype is not null then a.suftype='ROAD' else true end and a.postcode='01863'
    union
    select a.*, 2 as plan, null as parity from stdstreets a, streets b where a.id=b.gid  and a.name='CROOKED SPRING' and case when a.suftype is not null then a.suftype='ROAD' else true end and a.city='NORTH CHELMSFORD' and a.state='MASSACHUSETTS'
    union
    select a.*, 3 as plan, null as parity from stdstreets a, streets b where a.id=b.gid  and a.name='CROOKED SPRING' and a.city='NORTH CHELMSFORD' and a.state='MASSACHUSETTS' and a.postcode='01863'
    union
    select a.*, 4 as plan, null as parity from stdstreets a, streets b where a.id=b.gid  and a.name='CROOKED SPRING' and a.postcode='01863'
    union
    select a.*, 5 as plan, null as parity from stdstreets a, streets b where a.id=b.gid  and a.name='CROOKED SPRING' and a.city='NORTH CHELMSFORD' and a.state='MASSACHUSETTS'
    union
    select a.*, 6 as plan, null as parity from stdstreets a, streets b where a.id=b.gid  and a.name_dmeta=dmetaphone('CROOKED SPRING') and a.city='NORTH CHELMSFORD' and a.state='MASSACHUSETTS' and a.postcode='01863'
    union
    select a.*, 7 as plan, null as parity from stdstreets a, streets b where a.id=b.gid  and a.name_dmeta=dmetaphone('CROOKED SPRING') and a.postcode='01863'
    union
    select a.*, 8 as plan, null as parity from stdstreets a, streets b where a.id=b.gid  and a.name_dmeta=dmetaphone('CROOKED SPRING') and a.city='NORTH CHELMSFORD' and a.state='MASSACHUSETTS') as foo order by id
   id1    |   id2    |   id1a   |   id2a   |   street1    |      street2      | placename  |  placename_usps  | statename | countrycode | postcode | plan | geocodematchcode  |     x      |     y
----------+----------+----------+----------+--------------+-------------------+------------+------------------+-----------+-------------+----------+------+-------------------+------------+-----------
 20726192 | 20778764 | 86893125 | 87112558 | Radcliffe Rd | Crooked Spring Rd | Chelmsford | NORTH CHELMSFORD | MA        | US          | 01863    |    6 | 0.861111111111111 | -71.387469 | 42.620164
(1 row)


stdaddr=# select * from imt_geocoder('11 radcliffe rd', 'north chelmsford, ma, 01863', 0, 10.0, 0);
 rid |    id    |   id1    | id2 |    id3    | completeaddressnumber | predirectional | premodifier | postmodifier | pretype | streetname | posttype | postdirectional | placename  |  placename_usps  | statename | countrycode | zipcode | building | unit | plan | parity | geocodematchcode |         x         |        y
-----+----------+----------+-----+-----------+-----------------------+----------------+-------------+--------------+---------+------------+----------+-----------------+------------+------------------+-----------+-------------+---------+----------+------+------+--------+------------------+-------------------+------------------
   0 | 20726192 | 86893125 |   2 | 227535277 | 11                    |                |             |              |         | Radcliffe  | Rd       |                 | Chelmsford | NORTH CHELMSFORD | MA        | US          | 01863   |          |      |    0 | t      |            0.875 | -71.3876948442439 | 42.6199158781142
   0 | 20726193 | 86893125 |   1 | 207656203 | 11                    |                |             |              |         | Radcliffe  | Rd       |                 | Chelmsford | NORTH CHELMSFORD | MA        | US          | 01863   |          |      |    0 | f      |            0.775 | -71.3875492601771 |  42.619806089686
(2 rows)


select distinct on (id) foo.* from (
    select a.*, 0 as plan, null as parity from stdstreets a, streets b where a.id=b.gid  and case when a.pretype is not null then a.pretype='INT' else true end and a.city='MASSACHUSETTS' and a.state='USA' and case when a.country is not null then a.country='01863''TEXT AS' else true end
    union
    select a.*, 1 as plan, null as parity from stdstreets a, streets b where a.id=b.gid  and case when a.pretype is not null then a.pretype='INT' else true end and a.state='USA' and case when a.country is not null then a.country='01863''TEXT AS' else true end
    union
    select a.*, 2 as plan, null as parity from stdstreets a, streets b where a.id=b.gid  and case when a.pretype is not null then a.pretype='INT' else true end and a.city='MASSACHUSETTS' and a.state='USA' and case when a.country is not null then a.country='01863''TEXT AS' else true end
    union
    select a.*, 3 as plan, null as parity from stdstreets a, streets b where a.id=b.gid  and a.city='MASSACHUSETTS' and a.state='USA' and case when a.country is not null then a.country='01863''TEXT AS' else true end
    union
    select a.*, 4 as plan, null as parity from stdstreets a, streets b where a.id=b.gid  and a.state='USA' and case when a.country is not null then a.country='01863''TEXT AS' else true end
    union
    select a.*, 5 as plan, null as parity from stdstreets a, streets b where a.id=b.gid  and a.city='MASSACHUSETTS' and a.state='USA' and case when a.country is not null then a.country='01863''TEXT AS' else true end
    union
    select a.*, 6 as plan, null as parity from stdstreets a, streets b where a.id=b.gid  and a.city='MASSACHUSETTS' and a.state='USA' and case when a.country is not null then a.country='01863''TEXT AS' else true end
    union
    select a.*, 7 as plan, null as parity from stdstreets a, streets b where a.id=b.gid  and a.state='USA' and case when a.country is not null then a.country='01863''TEXT AS' else true end
    union
    select a.*, 8 as plan, null as parity from stdstreets a, streets b where a.id=b.gid  and a.city='MASSACHUSETTS' and a.state='USA' and case when a.country is not null then a.country='01863''TEXT AS' else true end) as foo order by id;

select * from standardize_address(
                 'lex',
                 'gaz',
                 'rules',
'select 0::int4 as id, ''10 radcliff rd''::text as micro, ''north chelmsford, ma 01863''::text as macro
    union all
select 1::int4 as id, ''10 crooked spring rd''::text as micro, ''north chelmsford, ma 01863''::text as macro'
);