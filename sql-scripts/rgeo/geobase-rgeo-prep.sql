--geobase-rgeo-prep.sql
-- geobase 2016

select max(length(l_placenam)), max(length(r_placenam)) from rawdata.roadseg;
-- 97, 97

select count(*) from streets where name = 'None';   -- 518,580 -- 0
select count(*) from streets where name is null;    -- 522,775
select count(*) from streets where name='Unknown';  --  43,979 -- 46,539
select count(*) from streets where l_ac4='Unknown'; -- 174,605 -- 0
select count(*) from streets where l_ac4='None';    --   4,061 -- 0
select count(*) from rawdata.roadseg where rtename1en='Unknown'; -- 0

select * from streets where l_refaddr is not null limit 500;
select * from streets limit 500;

-- Table: streets

DROP TABLE if exists data.streets cascade;

CREATE TABLE data.streets
(
  gid serial NOT NULL,
  link_id numeric(10,0),
  name character varying(80),
  l_refaddr numeric(10,0),
  l_nrefaddr numeric(10,0),
  l_sqlnumf character varying(12),
  l_sqlfmt character varying(16),
  l_cformat character varying(16),
  r_refaddr numeric(10,0),
  r_nrefaddr numeric(10,0),
  r_sqlnumf character varying(12),
  r_sqlfmt character varying(16),
  r_cformat character varying(16),
  l_ac5 character varying(100),
  l_ac4 character varying(100),
  l_ac3 character varying(41),
  l_ac2 character varying(35),
  l_ac1 character varying(35),
  r_ac5 character varying(100),
  r_ac4 character varying(100),
  r_ac3 character varying(41),
  r_ac2 character varying(35),
  r_ac1 character varying(35),
  l_postcode character varying(11),
  r_postcode character varying(11),
  mtfcc integer,
  passflg character varying(1),
  divroad character varying(1),
  the_geom geometry NOT NULL,
  CONSTRAINT streets_pkey PRIMARY KEY (gid),
  CONSTRAINT enforce_dims_the_geom CHECK (st_ndims(the_geom) = 2),
  CONSTRAINT enforce_geotype_the_geom CHECK (geometrytype(the_geom) = 'MULTILINESTRING'::text OR the_geom IS NULL),
  CONSTRAINT enforce_srid_the_geom CHECK (st_srid(the_geom) = 4326)
)
WITH (
  OIDS=FALSE
);
ALTER TABLE streets OWNER TO postgres;

drop table if exists data.link_id cascade;
create table data.link_id (
    id serial not null primary key,
    nid character(32) not null unique
    );
    
CREATE INDEX link_id_nid_idx ON data.link_id USING btree (nid ASC NULLS LAST);

------------------------------------------------------------------------------------------------
-- do the following for each province
------------------------------------------------------------------------------------------------

insert into link_id (nid)
   select foo.nid from (
     select nid from rawdata.roadseg
     union
     select nid from rawdata.ferryseg
     ) as foo;
-- Query returned successfully: 2136667 rows affected, 02:59 minutes execution time.
-- Query returned successfully: 2146540 rows affected, 03:22 minutes execution time. -- 2017


analyze;
-- Query returned successfully with no result in 01:07 minutes.
-- Query returned successfully with no result in 48.0 secs.

insert into data.streets (link_id, name, l_refaddr, l_nrefaddr, r_refaddr, r_nrefaddr, 
                          l_ac4, r_ac4, l_ac2, r_ac2, l_ac1, r_ac1, mtfcc, the_geom)
select 
    b.id as link_id,
    coalesce(nullif(nullif(l_stname_c, 'None'), 'Unknown'),
             nullif(nullif(r_stname_c, 'None'),  'Unknown'),
             nullif(nullif(l_stname_c, 'None'), 'Unknown'),
             nullif(nullif(r_stname_c, 'None'), 'Unknown'),
             nullif(nullif(rtename1en, 'None'), 'Unknown'),
             nullif(nullif(rtnumber1, 'None'), 'Unknown')
            ) as name,
     case when l_adddirfg='Opposite Direction' and l_hnuml>0 then l_hnuml
          when l_adddirfg='Same Direction' and l_hnumf>0 then l_hnumf
          when l_adddirfg='Not Applicable' and l_hnumf>0 then l_hnumf end as l_refaddr,
     case when l_adddirfg='Opposite Direction' and l_hnumf>0 then l_hnumf
          when l_adddirfg='Same Direction' and l_hnuml>0 then l_hnuml
          when l_adddirfg='Not Applicable' and l_hnuml>0 then l_hnuml end as l_nrefaddr,
     case when l_adddirfg='Opposite Direction' and r_hnuml>0 then r_hnuml
          when l_adddirfg='Same Direction' and r_hnumf>0 then r_hnumf
          when l_adddirfg='Not Applicable' and r_hnumf>0 then r_hnumf end as r_refaddr,
     case when l_adddirfg='Opposite Direction' and r_hnumf>0 then r_hnumf
          when l_adddirfg='Same Direction' and r_hnuml>0 then r_hnuml
          when l_adddirfg='Not Applicable' and r_hnuml>0 then r_hnuml end as r_nrefaddr,
    nullif(nullif(l_placenam, 'Unknown'), 'None') as l_ac4,
    nullif(nullif(r_placenam, 'Unknown'), 'None') as r_ac4,
    datasetnam as l_ac2,
    datasetnam as r_ac2,
    'CA' as l_ac1,
    'CA' as r_ac1,
    case 
      when roadclass='Expressway / Highway'  then 1000
      when roadclass='Rapid Transit'         then 1100
      when roadclass='Arterial'              then 1200
      when roadclass='Collector'             then 1300
      when roadclass='Local / Strata'        then 1400
      when roadclass='Local / Street'        then 1400
      when roadclass='Local / Unknown'       then 1400
      when roadclass='Ramp'                  then 1500
      when roadclass='Alleyway / Lane'       then 1600
      when roadclass='Service Lane'          then 1700
      when roadclass='Resource / Recreation' then 1800
      when roadclass='Winter'                then 1900
      else 9999 end as mtfcc,
    st_transform(geom, 4326) as the_geom
from rawdata.roadseg a, data.link_id b
where a.nid=b.nid;
-- Query returned successfully: 2418665 rows affected, 01:47 minutes execution time.
-- Query returned successfully: 2434316 rows affected, 01:45 minutes execution time. -- 2017

insert into data.streets (link_id, name, l_ac2, r_ac2, l_ac1, r_ac1, mtfcc, the_geom)
select 
    b.id as link_id,
    coalesce(nullif(rtename1en, 'None'), nullif(rtnumber1, 'None')) as name,
    datasetnam as l_ac2,
    datasetnam as r_ac2,
    'CA' as l_ac1,
    'CA' as r_ac1,
    case 
      when roadclass='Expressway / Highway'  then 2000
      when roadclass='Rapid Transit'         then 2100
      when roadclass='Arterial'              then 2200
      when roadclass='Collector'             then 2300
      when roadclass='Local / Strata'        then 2400
      when roadclass='Local / Street'        then 2400
      when roadclass='Local / Unknown'       then 2400
      when roadclass='Ramp'                  then 2500
      when roadclass='Alleyway / Lane'       then 2600
      when roadclass='Service Lane'          then 2700
      when roadclass='Resource / Recreation' then 2800
      when roadclass='Winter'                then 2900
      else 9999 end as mtfcc,
    st_transform(geom, 4326) as the_geom
from rawdata.ferryseg a, data.link_id b
where a.nid=b.nid;
-- Query returned successfully: 230 rows affected, 42 msec execution time.
-- Query returned successfully: 225 rows affected, 53 msec execution time. --2017

update streets set l_refaddr=l_nrefaddr where l_refaddr is null and l_nrefaddr is not null;
update streets set l_nrefaddr=l_refaddr where l_nrefaddr is null and l_refaddr is not null;
update streets set r_refaddr=r_nrefaddr where r_refaddr is null and r_nrefaddr is not null;
update streets set r_nrefaddr=r_refaddr where r_nrefaddr is null and r_refaddr is not null;


update streets set l_sqlnumf='FM'||repeat('9',case when length(l_refaddr::text)>length(l_nrefaddr::text) then length(l_refaddr::text) else length(l_nrefaddr::text) end),
                   l_sqlfmt='{}',
                   l_cformat='%'||case when length(l_refaddr::text)>length(l_nrefaddr::text) then length(l_refaddr::text) else length(l_nrefaddr::text) end||'d',
                   r_sqlnumf='FM'||repeat('9',case when length(r_refaddr::text)>length(r_nrefaddr::text) then length(r_refaddr::text) else length(r_nrefaddr::text) end),
                   r_sqlfmt='{}',
                   r_cformat='%'||case when length(r_refaddr::text)>length(r_nrefaddr::text) then length(r_refaddr::text) else length(r_nrefaddr::text) end||'d'
 where l_refaddr is not null or r_refaddr is not null or l_nrefaddr is not null or r_nrefaddr is not null;



-----------------------------------------------------------------------------------------------------
-- imaptools reverse geocoder
-----------------------------------------------------------------------------------------------------

-- load imt-rgeo-pgis-2.0.sql

-- load imt-rgeo-pgis-2.0-part2.sql

select imt_make_intersections('streets', 0.000001, 'the_geom', 'gid', ' name is not null ');
-- Total query runtime: 42:54 minutes
-- Total query runtime: 45:21 minutes

analyze;
-- Query returned successfully with no result in 59.6 secs.
-- Query returned successfully with no result in 50.6 secs.

update intersections_tmp as a set
          cnt  = (select count(*) from st_vert_tmp b where b.vid=a.id),
          dcnt = (select count(distinct name) from st_vert_tmp b, streets c where b.gid=c.gid and b.vid=a.id and c.name is not null);
-- Query returned successfully: 1532039 rows affected, 03:50 minutes execution time.
-- Query returned successfully: 1894033 rows affected, 04:37 minutes execution time. --2017

create schema rawdata;
alter function imt_make_intersections(character varying, double precision, character varying, character varying, text) set schema rawdata;

CREATE INDEX streets_link_id_idx  ON streets  USING btree  (link_id);
-- Query returned successfully with no result in 26.2 secs.
-- Query returned successfully with no result in 25.3 secs.

CREATE INDEX streets_gidx ON streets USING gist (the_geom);
ALTER TABLE streets CLUSTER ON streets_gidx;
-- Query returned successfully with no result in 01:10 minutes.
-- Query returned successfully with no result in 01:01 minutes.

vacuum analyze verbose;
-- Query returned successfully with no result in 01:20 minutes.
-- Query returned successfully with no result in 01:10 minutes.


select * from imt_reverse_geocode_flds(-106.663771,52.128759);
select * from imt_rgeo_intersections_flds(-106.663771,52.128759);
select * from imt_reverse_geocode_flds(-75.663771,45.128759);
select * from imt_rgeo_intersections_flds(-75.663771,45.128759);
select * from imt_reverse_geocode_flds(-73.663771,45.53);
select * from imt_rgeo_intersections_flds(-73.663771,45.53);


/*
--create schema rawdata;
--alter table addrange set schema rawdata;
--alter table altnamlink set schema rawdata;
--alter table blkpassage set schema rawdata;
--alter table ferryseg set schema rawdata;
--alter table junction set schema rawdata;
alter table link_id set schema rawdata;
--alter table roadseg set schema rawdata;
--alter table strplaname set schema rawdata;
--alter table tollpoint set schema rawdata;

alter function imt_make_intersections(character varying, double precision, character varying, character varying) set schema rawdata;
alter function imt_make_intersections(character varying, double precision, character varying, character varying, text) set schema rawdata;
*/
