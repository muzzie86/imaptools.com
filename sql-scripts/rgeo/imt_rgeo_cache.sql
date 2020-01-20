create schema data;
alter database pl_rgeo_cache set search_path to data, public;
set search_path to data, public;

drop table if exists rgeo_cache cascade;

create table rgeo_cache (
  id serial not null primary key,
  address1 text,
  address2 text,
  city text,
  county text,
  state text,
  country text,
  postalcode text,
  createdate date default now(),
  geom geometry('POINT', 4326)
);

create index rgeo_cache_gidx on rgeo_cache using gist (geom);
create index rgeo_cache_address1_idx on rgeo_cache using btree (coalesce(address1, ''));

create or replace function imt_rgeo_cache(
   OUT id integer, 
   OUT address1 text, 
   OUT address2 text,
   OUT city text,
   OUT county text,
   OUT state text,
   OUT country text,
   OUT postalcode text,
   OUT createdate date,
   INOUT lat float8,
   INOUT lon float8,
   IN tol float8,
   OUT dist float8)
returns record as
$body$
declare
  rec record;
  pnt geometry('Point', 4326) := st_setsrid(st_makepoint(lon, lat), 4326);

begin
  select *, st_distance_sphere(geom, pnt) as dist into rec
    from rgeo_cache
   where st_dwithin(geom, pnt, tol/111120.0)
   order by dist asc
   limit 1;

  if found then
    id         := rec.id;
    address1   := rec.address1;
    address2   := rec.address2;
    city       := rec.city;
    county     := rec.county;
    state      := rec.state;
    country    := rec.country;
    postalcode := rec.postalcode;
    createdate := rec.createdate;
    lat        := st_y(rec.geom);
    lon        := st_x(rec.geom);
    dist       := rec.dist;
  end if;
    
  return;
end;
$body$
   language plpgsql stable strict;

create or replace function imt_update_rgeo_cache(
    iid integer,    -- ignored unless mode=0
    iaddress1 text,
    iaddress2 text,
    icity text,
    icounty text,
    istate text,
    icountry text,
    ipostalcode text,
    lat float8,
    lon float8,
    mode integer,   -- action mode (see below)
    tol float8      -- distance in meters
) 
returns integer as
$body$
/*
    mode: integer flag that controls the behavior
      0 : replace, requires a valid id
      1 : replace if one exists within tol
      2 : replace if one exists within tol or add as new record
      3 : replace if one exists that matchs the address1, address2, city, county, state, country, postalcode
      4 : replace if one exists that matchs the address1, address2, city, county, state, country, postalcode or add as new record
      5 : add as new record regardless if it exists
      6 : delete, requires a valid id

    results:
      0 : failed
      1 : failed, more than one item within tol
      2 : replaced
      3 : added
      4 : deleted
*/
declare
  ret integer := 0;
  cnt integer;

begin

  if mode=0 then
    --  0 : replace, requires a valid id
    begin
      update rgeo_cache set address1=iaddress1, address2=iaddress2, city=icity, county=icounty, state=istate, country=icountry,
                            postalcode=ipostalcode, geom=st_setsrid(st_makepoint(lon, lat), 4326)
                        where id=iid;
      if found then
        return 2;
      else
        return 0;
      end if;
    exception when others then
      return 0;
    end;
  elsif mode=6 then
    --  6 : delete, requires a valid id
    begin
      delete from rgeo_cache where id=iid;
      if found then
        return 4;
      else
        return 0;
      end if;
    exception when others then
      return 0;
    end;
  elsif mode in (1,2) then
    -- 1 : replace if one exists within tol
    -- 2 : replace if one exists within tol or add as new record
    select count(*) into cnt
      from rgeo_cache
     where st_dwithin(geom, st_setsrid(st_makepoint(lon, lat), 4326), tol/111120.0);
    if cnt=0 then
      if mode=2 then
        mode := 5;
      else
        return 0;
      end if;
    elsif cnt=1 then
      update rgeo_cache set address1=iaddress1, address2=iaddress2, city=icity, county=icounty, state=istate, country=icountry,
                            postalcode=ipostalcode, geom=st_setsrid(st_makepoint(lon, lat), 4326), createdate=now()
                        where st_dwithin(geom, st_setsrid(st_makepoint(lon, lat), 4326), tol/111120.0);
      if found then
        return 2;
      else
        return 0;
      end if;
    else
      return 1;
    end if;
  elsif mode in (3,4) then
    -- 3 : replace if one exists that matchs the address1, address2, city, county, state, country
    -- 4 : replace if one exists that matchs the address1, address2, city, county, state, country or ass as new record
    select count(*) into cnt
      from rgeo_cache
     where coalesce(address1, '')=coalesce(iaddress1, '') 
       and coalesce(address2, '')=coalesce(iaddress2, '')
       and coalesce(city, '')=coalesce(icity, '') 
       and coalesce(county, '')=coalesce(icounty, '')
       and coalesce(state, '')=coalesce(istate, '') 
       and coalesce(country, '')=coalesce(icountry, '')
       and coalesce(postalcode, '')=coalesce(ipostalcode, '');
    if cnt=0 then
      if mode=4 then
        mode := 5;
      else
        return 0;
      end if;
    elsif cnt=1 then
      update rgeo_cache set address1=iaddress1, address2=iaddress2, city=icity, county=icounty, state=istate, country=icountry,
                            postalcode=ipostalcode, geom=st_setsrid(st_makepoint(lon, lat), 4326), createdate=now()
       where coalesce(address1, '')=coalesce(iaddress1, '') 
         and coalesce(address2, '')=coalesce(iaddress2, '')
         and coalesce(city, '')=coalesce(icity, '') 
         and coalesce(county, '')=coalesce(icounty, '')
         and coalesce(state, '')=coalesce(istate, '') 
         and coalesce(country, '')=coalesce(icountry, '')
         and coalesce(postalcode, '')=coalesce(ipostalcode, '');
      if found then
        return 2;
      else
        return 0;
      end if;
    else
      return 1;
    end if;
  end if;

  if mode=5 then
    -- 5 : add as new record regardless if it exists
    insert into rgeo_cache (address1, address2, city, county, state, country, postalcode, geom)
           values (iaddress1, iaddress2, icity, icounty, istate, icountry, ipostalcode, st_setsrid(st_makepoint(lon, lat), 4326));
    if found then
      return 3;
    else
      return 0;
    end if;
  end if;

  return ret;
end;
$body$
   language plpgsql volatile;

/*
    iid integer,    -- ignored unless mode=0
    iaddress1 text,
    iaddress2 text,
    icity text,
    icounty text,
    istate text,
    icountry text,
    ipostalcode text,
    lat float8,
    lon float8,
    mode integer,   -- action mode (see below)
    tol float8      -- distance in meters

    mode: integer flag that controls the behavior
      0 : replace, requires a valid id
      1 : replace if one exists within tol
      2 : replace if one exists within tol or add as new record
      3 : replace if one exists that matchs the address1, address2, city, county, state, country, postalcode
      4 : replace if one exists that matchs the address1, address2, city, county, state, country, postalcode or add as new record
      5 : add as new record regardless if it exists
      6 : delete, requires a valid id

    results:
      0 : failed
      1 : failed, more than one item within tol
      2 : replaced
      3 : added
      4 : deleted

*/

/*
-- test cases: run this in sequence on an empty database to test the code

select imt_update_rgeo_cache(null, '11 Radcliffe Rd', null, 'North Chelmsford', 'Middlesex', 'MA', 'US', '01863', 42.61977982686991, -71.38810467045815, 0, 10.0); -- 0
select imt_update_rgeo_cache(null, '11 Radcliffe Rd', null, 'North Chelmsford', 'Middlesex', 'MA', 'US', '01863', 42.61977982686991, -71.38810467045815, 1, 10.0); -- 0
select imt_update_rgeo_cache(null, '11 Radcliffe Rd', null, 'North Chelmsford', 'Middlesex', 'MA', 'US', '01863', 42.61977982686991, -71.38810467045815, 3, 10.0); -- 0
select imt_update_rgeo_cache(null, '11 Radcliffe Rd', null, 'North Chelmsford', 'Middlesex', 'MA', 'US', '01863', 42.61977982686991, -71.38810467045815, 6, 10.0); -- 0
select imt_update_rgeo_cache(1234, '11 Radcliffe Rd', null, 'North Chelmsford', 'Middlesex', 'MA', 'US', '01863', 42.61977982686991, -71.38810467045815, 6, 10.0); -- 0
select imt_update_rgeo_cache(null, '11 Radcliffe Rd', null, 'North Chelmsford', 'Middlesex', 'MA', 'US', '01863', 42.61977982686991, -71.38810467045815, 2, 10.0); -- 3
select imt_update_rgeo_cache(null, '11 Radcliffe Rd', null, 'North Chelmsford', 'Middlesex', 'MA', 'US', '01863', 42.61977982686991, -71.38810467045815, 2, 10.0); -- 2
select imt_update_rgeo_cache(null, '11 Radcliffe Rd', null, 'North Chelmsford', 'Middlesex', 'MA', 'US', '01863', 42.61977982686991, -71.38810467045815, 3, 10.0); -- 2
select * from imt_rgeo_cache(42.61977982686991, -71.38810467045815, 10.0);
-- 1;"11 Radcliffe Rd";"";"North Chelmsford";"Middlesex";"MA";"US";"01863";"2014-07-14";42.6197798268699;-71.3881046704581;0
select imt_update_rgeo_cache(1, '11 Radcliffe Rd', null, 'North Chelmsford', 'Middlesex', 'MA', 'US', '01863', 42.61977982686991, -71.38810467045815, 6, 10.0); -- 4
select imt_update_rgeo_cache(1, '11 Radcliffe Rd', null, 'North Chelmsford', 'Middlesex', 'MA', 'US', '01863', 42.61977982686991, -71.38810467045815, 0, 10.0); -- 0
select imt_update_rgeo_cache(null, '11 Radcliffe Rd', null, 'North Chelmsford', 'Middlesex', 'MA', 'US', '01863', 42.61977982686991, -71.38810467045815, 2, 10.0); -- 3
select imt_update_rgeo_cache(null, '11 Radcliffe Rd', null, 'North Chelmsford', 'Middlesex', 'MA', 'US', '01863', 42.61977982686991, -71.38810467045815, 4, 10.0); -- 2
select * from imt_rgeo_cache(42.61977982686991, -71.38810467045815, 10.0);
-- 2;"11 Radcliffe Rd";"";"North Chelmsford";"Middlesex";"MA";"US";"01863";"2014-07-14";42.6197798268699;-71.3881046704581;0
select imt_update_rgeo_cache(null, '11 Radcliffe Rd', null, 'North Chelmsford', 'Middlesex', 'MA', 'US', '01863', 42.61977982686991, -71.38810467045815, 5, 10.0); -- 3
select imt_update_rgeo_cache(null, '11 Radcliffe Rd', null, 'North Chelmsford', 'Middlesex', 'MA', 'US', '01863', 42.61977982686991, -71.38810467045815, 1, 10.0); -- 1
select imt_update_rgeo_cache(null, '11 Radcliffe Rd', null, 'North Chelmsford', 'Middlesex', 'MA', 'US', '01863', 42.61977982686991, -71.38810467045815, 4, 10.0); -- 1
select imt_update_rgeo_cache(1, '11 Radcliffe Rd', null, 'North Chelmsford', 'Middlesex', 'MA', 'US', '01863', 42.61977982686991, -71.38810467045815, 6, 10.0); -- 0
select imt_update_rgeo_cache(3, '11 Radcliffe Rd', null, 'North Chelmsford', 'Middlesex', 'MA', 'US', '01863', 42.61977982686991, -71.38810467045815, 6, 10.0); -- 4
select imt_update_rgeo_cache(null, '11 Radcliffe Rd', null, 'North Chelmsford', 'Middlesex', 'MA', 'US', '01863', 42.61977982686991, -71.38810467045815, 1, 10.0); -- 2
select imt_update_rgeo_cache(null, '11 Radcliffe Rd', null, 'North Chelmsford', 'Middlesex', 'MA', 'US', '01863', 42.61977982686991, -71.38810467045815, 2, 10.0); -- 2
select imt_update_rgeo_cache(null, '11 Radcliffe Rd', null, 'North Chelmsford', 'Middlesex', 'MA', 'US', '01863', 42.61977982686991, -71.38810467045815, 3, 10.0); -- 2
select imt_update_rgeo_cache(null, '11 Radcliffe Rd', null, 'North Chelmsford', 'Middlesex', 'MA', 'US', '01863', 42.61977982686991, -71.38810467045815, 4, 10.0); -- 2

*/