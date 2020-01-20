-- run work/australia/load-gnaf-geo script
-- load work/australia/lex-gmr-v2.sql 
-- load as_config table
-- run ./geo-standardize2 script (14-15 days to run)

-- create indexes -----------------------------------------------------------------------------

create index stdstreets_name_city_state_idx on stdstreets using btree (name,state,city ASC NULLS LAST);
-- Query returned successfully with no result in 05:19 minutes.

create index stdstreets_name_postcode_idx on stdstreets using btree (name,postcode ASC NULLS LAST);
-- Query returned successfully with no result in 02:40 minutes.

create index stdstreets_name_dmeta_city_state_idx on stdstreets using btree (name_dmeta,state,city ASC NULLS LAST);
-- Query returned successfully with no result in 02:47 minutes.

create index stdstreets_name_dmeta_postcode_idx on stdstreets using btree (name_dmeta,postcode ASC NULLS LAST);
-- Query returned successfully with no result in 02:29 minutes.


vacuum analyze verbose stdstreets;
-- Query returned successfully with no result in 12.7 secs.



-----------------------------------------------------------------------------------------------
---------------------------------------------------------------------------------------------
---------------------------------------------------------------------------------------------

drop type if exists stdaddr cascade;
create type stdaddr as (
    id integer,
    building text,
    house_num text,
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
    pattern text,
    dmetaphone text
);

---------------------------------------------------------------------------------------------

drop type if exists georesult cascade;
create type georesult as (
    rid integer,
    id integer,
    building text,
    house_num text,
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
    dmetaphone text,
    address text,
    plan integer,
    score float8,
    x float8,
    y float8,
    confidence integer,
    geotype text
);

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
  num integer;
  
begin
  if inp.name is not null and plan < 6 then
    ret := ret || ' and a.name='||quote_literal(inp.name);
  elsif inp.name is not null and plan >= 6 then
    ret := ret || ' and a.name_dmeta=coalesce(dmetaphone('||quote_literal(inp.name)||'),'||quote_literal(inp.name)||')';
  end if;
  if inp.house_num is not null then
      num := substring(quote_literal(inp.house_num) from E'(\\d+)')::integer;
      ret := ret || ' and (' || num || ' between b.number_first and coalesce(b.number_last, b.number_first) or ' || num 
                 || ' = substring(nullif(array_to_string(ARRAY[''LOT'', b.lot_number_prefix, b.lot_number, b.lot_number_suffix], '' ''),''LOT'') from E''(\\d+)'')::integer )';
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

-----------------------------------------------------------------------------------------------

create or replace function max(a numeric, b numeric)
  returns numeric as
$body$
begin
  return case when a>b then a else b end;
end;
$body$
  language plpgsql immutable;

--------------------------------------------------------------------------------------------------

create or replace function imt_geo_planner(inp stdaddr, opt integer)
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
      || 'select 0::integer as rid, a.*, b.address, ' || i::text || ' as plan '
      || ' from stdstreets a, rawdata.addresses b'
      || ' where a.id=b.id ' 
      || imt_geo_add_where_clauses(inp, i);
  end loop planner;

  -- added sub-select to enforce an order by id, plan so all servers return the same results
  sql := 'select distinct on (id) * from (select * from (' || sql || ') as bar order by id, plan) as foo order by id';

  return sql;
end;
$body$
  language plpgsql immutable;

-------------------------------------------------------------------------------------------------------

create or replace function imt_compute_xy(id_in integer, num_in text,
    OUT x double precision, 
    OUT y double precision,
    OUT confidence integer,
    OUT geotype text,
    OUT distance integer,
    OUT num integer
    ) as
$body$
declare
  rec record;
  
begin

  num := substring(num_in from E'(\\d+)')::integer;

  select *, case when num between number_first and coalesce(number_last, number_first) then 1 else 0 end as distance into rec 
    from rawdata.addresses a
   where a.id=id_in;

  if FOUND then
    x := rec.longitude;
    y := rec.latitude;
    confidence := rec.confidence;
    geotype := rec.geocode_type;
    distance := rec.distance;
  else
    x := NULL;
    y := NULL;
    confidence := NULL;
    distance := NULL;
  end if;

end;
$body$
  language plpgsql immutable;


-----------------------------------------------------------------------------------------------

create or replace function imt_geo_score(sql text, sin stdaddr)
  returns setof georesult as
$body$
declare
  rec georesult;
  score double precision;
  w1 double precision := 0.05;
  w2 double precision := 0.25;
  w3 double precision := 0.125;
  len integer;
  xy record;
  
begin

--raise notice 'sin: %', sin;

  for rec in execute sql loop
--raise notice 'rec: %', rec;
    rec.rid := null;
    select * into xy from imt_compute_xy( rec.id, sin.house_num );
--raise notice 'xy: %', xy;
    rec.x := xy.x;
    rec.y := xy.y;
    rec.confidence = xy.confidence;
    rec.geotype = xy.geotype;
    score := xy.distance * 0.1;
    len := max(length(coalesce(rec.house_num,'')), length(coalesce(sin.house_num,'')));
    score := score + case when len>0 then levenshtein(coalesce(rec.house_num,''), coalesce(sin.house_num,''))::double precision / len::double precision * w1 else 0.0 end;
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
--raise notice 'rec: %', rec;
    return next rec;
  end loop;
  return;
end;
$body$
  language plpgsql immutable strict cost 1 rows 100;

---------------------------------------------------------------------------------------------

create or replace function imt_geocoder(sfld stdaddr, fuzzy integer)
  returns setof georesult as
$body$
declare
  sql text;
  rec georesult;
  cnt integer := 0;

begin

--raise notice 'sfld: %', sfld;
  if sfld.state is null and sfld.postcode is null then
    return;
  end if;

  -- make sure we got an address that we can use
  if coalesce(sfld.house_num,sfld.predir,sfld.qual,sfld.pretype,sfld.name,sfld.suftype,sfld.sufdir) is not null and
     coalesce(sfld.city,sfld.state,sfld.postcode /*,sfld.country */) is not null then
    select * into sql from imt_geo_planner(sfld, fuzzy);
--raise notice 'sql: %', sql;
    if found then
      for rec in select a.*
                   from (select distinct on (address) * from imt_geo_score(sql, sfld) order by address, score desc) as a order by a.score desc, plan asc loop
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

create or replace function imt_geocoder(address_in text, fuzzy integer, cfgid_in integer)
  returns setof georesult as
$body$
declare
  sfld stdaddr;

begin
  select 0::integer, std.*, dmetaphone(std.name) into sfld 
    from as_config as cfg,
         LATERAL as_standardize(replace(address_in, '/', ' / '), grammar, clexicon, 'en_AU', filter) as std
   where cfg.id=cfgid_in;
   
--raise notice 'state: %, postcode: %', sfld.state, sfld.postcode;
  if sfld.state is null and sfld.postcode is null then
    return;
  end if;

  return query select * from imt_geocoder(sfld, fuzzy);
end;
$body$
  language plpgsql immutable strict cost 5 rows 100;

-----------------------------------------------------------------------------------------------------

create or replace function imt_geo_test(myid integer)
  returns setof text as
$body$
declare
  rec stdaddr;
  
begin
  select * into rec from stdstreets where id=myid;
  return query select * from imt_geo_planner(rec, 1);
end;
$body$
  language plpgsql immutable;

---------------------------------------------------------------------------------------------

select * from imt_geo_test(5703616);

select * from imt_geocoder('lot 11 warcowie rd black hill station sa 5434', 0, 26);
select * from imt_geocoder('lot 11 warcowie rd black hill station sa', 0, 26);
select * from imt_geocoder('lot 11 warcowie rd black hills station sa', 0, 26);
select * from imt_geocoder('lot 11 warcwie rd black station sa', 1, 26);
select * from imt_geocoder('123 Collins St, Melbourne VIC 3000, Australia', 0, 26);
select * from imt_geocoder('110-114 James Ruse Dr, Parramatta NSW 2142, Australia', 0, 26);

select * from imt_geocoder('453/455 Deakin Ave, Mildura VIC 3500, Australia', 1, 26);
select * from imt_geocoder('Wine Country Dr, Nulkaba NSW 2325, Australia', 1, 26);
select * from imt_geocoder('Cumberland St, Cessnock NSW 2325, Australia', 1, 26);
select * from imt_geocoder('11/13 Oyster Bay Cres, Coles Bay TAS 7215, Australia', 1, 26);
select * from imt_geocoder('66 Aurora Ln, Docklands VIC 3008, Australia', 1, 26);

with addrs as (
select unnest(array[
'199 George St, Sydney NSW 2000, Australia',
'110-114 James Ruse Dr, Parramatta NSW 2142, Australia',
'97 Grafton St, Bondi Junction NSW 2022, Australia',
'100 Cumberland St, Sydney NSW 2000, Australia',
'515 Queen St, Brisbane City QLD 4000, Australia',
'38 Cunningham St, Dalby QLD 4405, Australia',
'1 Beach St, Fremantle WA 6160, Australia',
'453/455 Deakin Ave, Mildura VIC 3500, Australia',
'136 Wollombi Rd, Cessnock NSW 2325, Australia',
'485 King St, Newtown NSW 2042, Australia',
'11 Egan St, Newtown NSW 2042, Australia',
'9 Missenden Rd, Camperdown NSW 2050, Australia',
'23-33 Missenden Rd, Camperdown NSW 2050, Australia',
'43 Shedden St, Cessnock NSW 2325, Australia',
'216 Vincent St, Cessnock NSW 2325, Australia',
'221 Vincent St, Cessnock NSW 2325, Australia',
'123 Collins St, Melbourne VIC 3000, Australia',
'110 Aberdare St, Aberdare NSW 2325, Australia',
'30 Allandale Rd, Cessnock NSW 2325, Australia',
'1 Wine Country Dr, Nulkaba NSW 2325, Australia',
'388 Wollombi Rd, Bellbird NSW 2325, Australia',
'5 Darwin St, Cessnock NSW 2325, Australia',
'1 Cumberland St, Cessnock NSW 2325, Australia',
'430 Wine Country Dr, Lovedale NSW 2325, Australia',
'300 Maitland Rd, Cessnock NSW 2325, Australia',
'28 Cessnock Rd, Neath NSW 2326, Australia',
'336 Oakey Creek Rd, Pokolbin NSW 2320, Australia',
'35 Colliery St, Aberdare NSW 2325, Australia',
'2090 Broke Rd, Pokolbin NSW 2320, Australia',
'2352 Coles Bay Rd, Coles Bay TAS 7215, Australia',
'2308 Coles Bay Rd, Coles Bay TAS 7215, Australia',
'11/13 Oyster Bay Cres, Coles Bay TAS 7215, Australia',
'300 Spencer St, Melbourne VIC 3000, Australia',
'66 Aurora Ln, Docklands VIC 3008, Australia',
--'116 PINE HILLS NO 2 ROAD WOMBELANO VIC 3409',
'740 Bourke St, Docklands VIC 3008, Australia']) as adr
)
--select * from addrs;
select * from addrs, lateral (select * from imt_geocoder(adr, 0, 26) limit 1) as foo;
-- Total query runtime: 20.8 secs, 35 rows retrieved.


select * from imt_geocoder('116 PINE HILLS NO 2 ROAD WOMBELANO VIC 3409', 0, 26) limit 1;
select * from imt_geocoder('11/13 Oyster Bay Cres, Coles Bay TAS 7215, Australia', 0, 26) limit 1;


select std.*, dmetaphone(std.name)
  from as_config as cfg,
       --LATERAL as_standardize( '11/13 Oyster Bay Cres, Coles Bay TAS 7215, Australia', grammar, clexicon, 'en_AU', filter) as std
       --LATERAL as_standardize( '11 Wine Country Dr, Nulkaba NSW 2325, Australia', grammar, clexicon, 'en_AU', filter) as std
       LATERAL as_standardize( '116 PINE HILLS NO 2 ROAD WOMBELANO VIC 3409', grammar, clexicon, 'en_AU', filter) as std
       --LATERAL as_standardize( 'Cumberland St, Cessnock NSW 2325', grammar, clexicon, 'en_AU', filter) as std
 where countrycode='au' and dataset='gnaf'
