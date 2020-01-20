set search_path to data, rawdata, public;
SET enable_seqscan = off;

CREATE INDEX altstreets_linkid_idx on altstreets (LINK_ID);
CREATE INDEX mtdarea_aid_idx on mtdarea (AREA_ID);
CREATE INDEX mtdzonerec_zoneid_idx on mtdzonerec (ZONE_ID);
CREATE INDEX streets_linkid_idx on streets (LINK_ID);
CREATE INDEX zones_linkid_idx on zones (LINK_ID);
CREATE INDEX zones_zoneid_idx on zones (ZONE_ID);
CREATE INDEX mtdarea_ac_idx on mtdarea (ADMIN_LVL,AREACODE_1,AREACODE_2,AREACODE_3,AREACODE_4,AREACODE_5,AREACODE_6,AREACODE_7);

select count(*) from (
select distinct a.zone_id, a.area_id1, a.zone_type, a.area_type, 
       a.area_name as city, 
       (select area_name as county 
          from mtdarea 
         where admin_lvl=3 and 
               areacode_1=a.areacode_1 and 
               areacode_2=a.areacode_2 and 
               areacode_3=a.areacode_3 and 
               lang_code='ENG' and 
               area_type='B' ) as county, 
        (select area_name as state 
           from mtdarea 
          where admin_lvl=2 and 
                areacode_1=a.areacode_1 and 
                areacode_2=a.areacode_2 and 
                lang_code='ENG' and 
                area_type='B' ) as state, 
        (select area_name as state 
           from mtdarea 
          where admin_lvl=1 and 
                areacode_1=a.areacode_1 and 
                lang_code='ENG' and 
                area_type='A' ) as country 
  from (select b.area_id as area_id1, * 
          from mtdarea b, 
               mtdzonerec c 
         where b.area_id=c.area_id and 
               area_type != 'E' ) as a
) foo;



select count(*) from (
select a.link_id, a.st_name, a.st_langcd, a.st_nm_pref, a.st_typ_bef, a.st_nm_base, a.st_nm_suff, a.st_typ_aft, a.st_typ_att, 
       a.addr_type, a.l_refaddr, a.l_nrefaddr, a.l_addrsch, a.l_addrform, a.l_area_id, a.l_postcode, 'L' as side,
       c.zone_id, coalesce(d.area_id, a.l_area_id) as area_id
  from streets a left outer join zones c on a.link_id=c.link_id left outer join mtdzonerec d on c.zone_id=d.zone_id
 where c.side is null or c.side='L' or c.side='B'
union all
select a.link_id, a.st_name, a.st_langcd, a.st_nm_pref, a.st_typ_bef, a.st_nm_base, a.st_nm_suff, a.st_typ_aft, a.st_typ_att, 
       a.addr_type, a.r_refaddr, a.r_nrefaddr, a.r_addrsch, a.r_addrform, a.r_area_id, a.r_postcode, 'R' as side,
       c.zone_id, coalesce(d.area_id, a.r_area_id) as area_id
  from streets a left outer join zones c on a.link_id=c.link_id left outer join mtdzonerec d on c.zone_id=d.zone_id
 where c.side is null or c.side='R' or c.side='B'
union all
select a.link_id, a.st_name, a.st_langcd, a.st_nm_pref, a.st_typ_bef, a.st_nm_base, a.st_nm_suff, a.st_typ_aft, a.st_typ_att, 
       a.addr_type, a.l_refaddr, a.l_nrefaddr, a.l_addrsch, a.l_addrform, b.l_area_id, b.l_postcode, 'L' as side,
       c.zone_id, coalesce(d.area_id, b.l_area_id) as area_id
  from altstreets a, streets b left outer join zones c on b.link_id=c.link_id left outer join mtdzonerec d on c.zone_id=d.zone_id
 where a.link_id=b.link_id and (c.side is null or c.side='L' or c.side='B')
union all
select a.link_id, a.st_name, a.st_langcd, a.st_nm_pref, a.st_typ_bef, a.st_nm_base, a.st_nm_suff, a.st_typ_aft, a.st_typ_att, 
       a.addr_type, a.r_refaddr, a.r_nrefaddr, a.r_addrsch, a.r_addrform, b.r_area_id, b.r_postcode, 'R' as side,
       c.zone_id, coalesce(d.area_id, b.r_area_id) as area_id
  from altstreets a, streets b left outer join zones c on b.link_id=c.link_id left outer join mtdzonerec d on c.zone_id=d.zone_id
 where a.link_id=b.link_id and  (c.side is null or c.side='R' or c.side='B')
) as foo;
-- 3,198,302 Total query runtime: 13541 ms. - without area_ids
-- 2,789,019 Total query runtime: 24617 ms. - with area_ids
-- 3,526,730 Total query runtime: 26130 ms. - changing to left outer joins. Many streets records have numzones=0

drop table if exists data.streets cascade;
create table data.streets (
  gid serial not null primary key,
  link_id numeric(10,0),
  st_name varchar,
  st_langcd char(3),
  st_nm_pref varchar,
  st_typ_bef varchar,
  st_nm_base varchar,
  st_nm_suff varchar,
  st_typ_aft varchar,
  st_typ_att char(1),
  addr_type char(1),
  refaddr varchar(10),
  nrefaddr varchar(10),
  addrsch char(1),
  addrform varchar(2),
  side char(1),
  postcode varchar(11),
  city varchar,
  county varchar,
  state varchar,
  country varchar,
  area_id numeric(10,0),
  zone_id numeric(10,0)
);
select addgeometrycolumn('data','streets','the_geom',4326,'MULTILINESTRING',2);

insert into data.streets (link_id, st_name, st_langcd, st_nm_pref, st_typ_bef, st_nm_base, st_nm_suff, st_typ_aft, st_typ_att, addr_type, 
                     refaddr, nrefaddr, addrsch, addrform, side, postcode, city, county, state, country, area_id, zone_id, the_geom)
select link_id, st_name, st_langcd, st_nm_pref, st_typ_bef, st_nm_base, st_nm_suff, st_typ_aft, st_typ_att, addr_type, 
       refaddr, nrefaddr, addrsch, addrform, side, postcode, city, county, state, country, g.area_id, g.zone_id, the_geom
 from (
select a.link_id, a.st_name, a.st_langcd, a.st_nm_pref, a.st_typ_bef, a.st_nm_base, a.st_nm_suff, a.st_typ_aft, a.st_typ_att, 
       a.addr_type, a.l_refaddr as refaddr, a.l_nrefaddr as nrefaddr, a.l_addrsch as addrsch, a.l_addrform as addrform, 
       a.l_area_id, a.l_postcode as postcode, 'L' as side,
       c.zone_id, coalesce(nullif(d.area_id,0), a.l_area_id) as area_id, the_geom
  from rawdata.streets a left outer join zones c on a.link_id=c.link_id left outer join mtdzonerec d on c.zone_id=d.zone_id
 where (c.side is null or c.side='L' or c.side='B') and a.l_refaddr is not null
union all
select a.link_id, a.st_name, a.st_langcd, a.st_nm_pref, a.st_typ_bef, a.st_nm_base, a.st_nm_suff, a.st_typ_aft, a.st_typ_att, 
       a.addr_type, a.r_refaddr, a.r_nrefaddr, a.r_addrsch, a.r_addrform, a.r_area_id, a.r_postcode, 'R' as side,
       c.zone_id, coalesce(nullif(d.area_id,0), a.r_area_id) as area_id, the_geom
  from rawdata.streets a left outer join zones c on a.link_id=c.link_id left outer join mtdzonerec d on c.zone_id=d.zone_id
 where (c.side is null or c.side='R' or c.side='B') and a.r_refaddr is not null
union all
select a.link_id, a.st_name, a.st_langcd, a.st_nm_pref, a.st_typ_bef, a.st_nm_base, a.st_nm_suff, a.st_typ_aft, a.st_typ_att, 
       a.addr_type, a.l_refaddr, a.l_nrefaddr, a.l_addrsch, a.l_addrform, b.l_area_id, b.l_postcode, 'L' as side,
       c.zone_id, coalesce(nullif(d.area_id,0), b.l_area_id) as area_id, the_geom
  from altstreets a, rawdata.streets b left outer join zones c on b.link_id=c.link_id left outer join mtdzonerec d on c.zone_id=d.zone_id
 where a.link_id=b.link_id and (c.side is null or c.side='L' or c.side='B') and a.l_refaddr is not null
union all
select a.link_id, a.st_name, a.st_langcd, a.st_nm_pref, a.st_typ_bef, a.st_nm_base, a.st_nm_suff, a.st_typ_aft, a.st_typ_att, 
       a.addr_type, a.r_refaddr, a.r_nrefaddr, a.r_addrsch, a.r_addrform, b.r_area_id, b.r_postcode, 'R' as side,
       c.zone_id, coalesce(nullif(d.area_id,0), b.r_area_id) as area_id, the_geom
  from altstreets a, rawdata.streets b left outer join zones c on b.link_id=c.link_id left outer join mtdzonerec d on c.zone_id=d.zone_id
 where a.link_id=b.link_id and  (c.side is null or c.side='R' or c.side='B') and a.r_refaddr is not null
) as f,
(select distinct a.zone_id, a.area_id1 as area_id, a.zone_type, a.area_type, 
       a.area_name as city, 
       (select area_name as county 
          from mtdarea 
         where admin_lvl=3 and 
               areacode_1=a.areacode_1 and 
               areacode_2=a.areacode_2 and 
               areacode_3=a.areacode_3 and 
               lang_code='ENG' and 
               area_type='B' ) as county, 
        (select area_name as state 
           from mtdarea 
          where admin_lvl=2 and 
                areacode_1=a.areacode_1 and 
                areacode_2=a.areacode_2 and 
                lang_code='ENG' and 
                area_type='B' ) as state, 
        (select area_name as state 
           from mtdarea 
          where admin_lvl=1 and 
                areacode_1=a.areacode_1 and 
                lang_code='ENG' and 
                area_type='A' ) as country 
  from (select b.area_id as area_id1, * 
          from mtdarea b left outer join mtdzonerec c 
         on b.area_id=c.area_id where 
               area_type != 'E') as a
) as g
where f.area_id=g.area_id
order by link_id;

-- Query returned successfully: 2293914 rows affected, 268695 ms execution time.

CREATE INDEX streets_link_id_idx  ON streets  USING btree  (link_id);

select count(*) from rawdata.pointaddress a left outer join data.streets b on a.link_id=b.link_id where b.link_id is null;
-- 542

select distinct a.link_id from rawdata.pointaddress a left outer join data.streets b on a.link_id=b.link_id where b.link_id is null;
--251

select count(*)
  from (select aa.* from rawdata.pointaddress aa left outer join data.streets bb on aa.link_id=bb.link_id where bb.link_id is null) p
       join rawdata.altstreets a on p.link_id=a.link_id left outer join zones c on a.link_id=c.link_id left outer join mtdzonerec d on c.zone_id=d.zone_id
-- 556 rawdata.streets
--  89 rawdata.altstreets

insert into data.streets (link_id, st_name, st_langcd, st_nm_pref, st_typ_bef, st_nm_base, st_nm_suff, st_typ_aft, st_typ_att, addr_type, 
                     refaddr, nrefaddr, addrsch, addrform, side, postcode, city, county, state, country, area_id, zone_id, the_geom)
select link_id, st_name, st_langcd, st_nm_pref, st_typ_bef, st_nm_base, st_nm_suff, st_typ_aft, st_typ_att, addr_type, 
       refaddr, nrefaddr, addrsch, addrform, side, postcode, city, county, state, country, g.area_id, g.zone_id, the_geom
 from (
select a.link_id, a.st_name, a.st_langcd, a.st_nm_pref, a.st_typ_bef, a.st_nm_base, a.st_nm_suff, a.st_typ_aft, a.st_typ_att, 
       p.addr_type, p.address as refaddr, p.address as nrefaddr, a.l_addrsch as addrsch, a.l_addrform as addrform, 
       a.l_area_id, a.l_postcode as postcode, p.side,
       c.zone_id, coalesce(nullif(d.area_id,0), a.l_area_id) as area_id, a.the_geom
  from (select aa.* from rawdata.pointaddress aa left outer join data.streets bb on aa.link_id=bb.link_id where bb.link_id is null) p
       join rawdata.streets a on p.link_id=a.link_id left outer join zones c on a.link_id=c.link_id left outer join mtdzonerec d on c.zone_id=d.zone_id
 where (c.side is null or c.side=p.side or c.side='B')
union -- all
select a.link_id, a.st_name, a.st_langcd, a.st_nm_pref, a.st_typ_bef, a.st_nm_base, a.st_nm_suff, a.st_typ_aft, a.st_typ_att, 
       p.addr_type, p.address as refaddr, p.address as nrefaddr, a.l_addrsch as addrsch, a.l_addrform as addrform, 
       b.l_area_id, b.l_postcode as postcode, p.side,
       c.zone_id, coalesce(nullif(d.area_id,0), b.l_area_id) as area_id, b.the_geom
  from (select aa.* from rawdata.pointaddress aa left outer join data.streets bb on aa.link_id=bb.link_id where bb.link_id is null) p
       join altstreets a on p.link_id=a.link_id join rawdata.streets b on p.link_id=b.link_id left outer join zones c on a.link_id=c.link_id left outer join mtdzonerec d on c.zone_id=d.zone_id
 where (c.side is null or c.side=p.side or c.side='B')
) as f,
(select distinct a.zone_id, a.area_id1 as area_id, a.zone_type, a.area_type, 
       a.area_name as city, 
       (select area_name as county 
          from mtdarea 
         where admin_lvl=3 and 
               areacode_1=a.areacode_1 and 
               areacode_2=a.areacode_2 and 
               areacode_3=a.areacode_3 and 
               lang_code='ENG' and 
               area_type='B' ) as county, 
        (select area_name as state 
           from mtdarea 
          where admin_lvl=2 and 
                areacode_1=a.areacode_1 and 
                areacode_2=a.areacode_2 and 
                lang_code='ENG' and 
                area_type='B' ) as state, 
        (select area_name as state 
           from mtdarea 
          where admin_lvl=1 and 
                areacode_1=a.areacode_1 and 
                lang_code='ENG' and 
                area_type='A' ) as country 
  from (select b.area_id as area_id1, * 
          from mtdarea b left outer join mtdzonerec c 
         on b.area_id=c.area_id where 
               area_type != 'E') as a
) as g
where f.area_id=g.area_id
order by link_id;



/*  looks like a navteq data bug

select distinct a.link_id
  from rawdata.streets a, rawdata.pointaddress b
 where a.link_id=b.link_id and a.st_name is null;
-- 250

select * from rawdata.streets where link_id in (23622856,105568974,120851030,832926824,845597071,105407554,116923905,105603882);

*/

