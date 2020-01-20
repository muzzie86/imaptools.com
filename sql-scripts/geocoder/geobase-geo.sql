 CREATE EXTENSION address_standardizer  SCHEMA public  VERSION "2.2.2";
 CREATE EXTENSION fuzzystrmatch         SCHEMA public  VERSION "1.0";
 CREATE EXTENSION unaccent              SCHEMA public  VERSION "1.0";


-- psql -U postgres -h localhost -p 5434 canada_2016 -f us-gaz.sql
-- psql -U postgres -h localhost -p 5434 canada_2016 -f us-lex.sql
-- psql -U postgres -h localhost -p 5434 canada_2016 -f us-rules.sql


-- prepare Geobase data
-- prepare statcan data (below)

/*

CREATE INDEX addrange_nid_idx ON rawdata.addrange USING btree (nid ASC NULLS LAST);
CREATE INDEX altnamlink_nid_idx ON rawdata.altnamlink USING btree (nid ASC NULLS LAST);
CREATE INDEX strplaname_nid_idx ON rawdata.strplaname USING btree (nid ASC NULLS LAST);
CREATE INDEX roadseg_nid_idx ON rawdata.roadseg USING btree (nid ASC NULLS LAST);

*/


/*

select * from rawdata.roadseg limit 100;
select * from rawdata.roadseg where adrangenid='None' and l_stname_c != 'Unknown' limit 100; -- no record
select * from rawdata.strplaname limit 100;
select * from rawdata.altnamlink limit 100;
select * from rawdata.addrange order by nid limit 100;

select * from rawdata.roadseg where adrangenid='0000dca9db7c4868ae709d5dad85606a';
select * from rawdata.addrange where l_altnanid != 'None' limit 100;

SET enable_seqscan = off;

select count(*)from rawdata.roadseg where roadsegid=1;

select foo.* from (
select a.gid as ggid, *
  from rawdata.roadseg a left outer join rawdata.addrange b on a.adrangenid=b.nid
                         left outer join rawdata.altnamlink d on b.l_altnanid=d.nid
                         left outer join rawdata.strplaname e on b.l_offnanid=e.nid
union all
select a.gid as ggid, *
  from rawdata.roadseg a left outer join rawdata.addrange b on a.adrangenid=b.nid
                         left outer join rawdata.altnamlink d on b.l_altnanid=d.nid
                         left outer join rawdata.strplaname f on d.strnamenid=f.nid
) as foo where foo.ggid=725114

--------------

select *
  from rawdata.roadseg a left outer join rawdata.addrange b on a.adrangenid=b.nid
                         left outer join rawdata.altnamlink d on b.l_altnanid=d.nid
                         left outer join rawdata.strplaname f on d.strnamenid=f.nid
 where b.l_altnanid != 'None' limit 100;
-- Total query runtime: 840633 ms.

select * from rawdata.strplaname a, rawdata.altnamlink b
 where b.strnamenid=a.nid and b.nid in ('628435bf42d74a79ad6938cf18533960','c27b1d71cbcf4763afbe8bd67c4d99aa');

-----------------------------------------------------

select * from rawdata.st limit 100;

select * from rawdata.roadseg where gid=964373;

select distinct a.* , case when a.side='L' then e.placename else g.placename end as altplace
  from expand_roadseg() as a 
       left outer join rawdata.roadseg b on a.rsgid=b.gid
       left outer join rawdata.addrange c on b.adrangenid=c.nid
       left outer join rawdata.altnamlink d on d.nid=c.l_altnanid
       left outer join rawdata.strplaname e on d.strnamenid=e.nid
       left outer join rawdata.altnamlink f on f.nid=c.r_altnanid
       left outer join rawdata.strplaname g on f.strnamenid=g.nid
 where a.rsgid=964373


select * from rawdata.roadseg order by gid limit 100;

select * -- c.*, e.*
  from rawdata.roadseg b
       left outer join rawdata.addrange c on b.adrangenid=c.nid
       left outer join rawdata.altnamlink d on d.nid=c.l_altnanid
       left outer join rawdata.strplaname e on d.strnamenid=e.nid
       left outer join rawdata.strplaname h on c.l_offnanid=h.nid
   --    left outer join rawdata.altnamlink f on f.nid=c.r_altnanid
   --    left outer join rawdata.strplaname g on f.strnamenid=g.nid
 where b.gid=964373

*/


drop table if exists rawdata.st cascade;

create table rawdata.st (
 gid serial not null primary key,
 source integer,
 link_id integer, 
 name text, 
 hnumf integer, 
 hnuml integer, 
 side char(1), 
 city text, 
 state text, 
 country text
);

insert into rawdata.st (source, linkid, name, hnumf, hnuml, side, city, state, country)
select 2::integer as sourcefoo.*, 'CANADA' as country from (
select b.gid as link_ID,
       array_to_string(ARRAY[
           nullif(h.dirprefix,'None'),
           nullif(h.strtypre,'None'),
           nullif(h.starticle,'None'),
           nullif(h.namebody,'None'),
           nullif(h.strtysuf,'None'),
           nullif(h.dirsuffix,'None')],' ') as name,
       c.l_hnumf, 
       c.l_hnuml, 
       'L' as side,
       nullif(h.placename,'Unknown') as city,
       h.province as state
  from rawdata.roadseg b, rawdata.addrange c, rawdata.strplaname h
 where b.adrangenid=c.nid and c.l_offnanid=h.nid
       and not ((c.l_hnumf = 0 and c.l_hnuml = 0) or (c.l_hnumf = -1 and c.l_hnuml = -1))
union 
select b.gid as rsgid,
       array_to_string(ARRAY[
           nullif(h.dirprefix,'None'),
           nullif(h.strtypre,'None'),
           nullif(h.starticle,'None'),
           nullif(h.namebody,'None'),
           nullif(h.strtysuf,'None'),
           nullif(h.dirsuffix,'None')],' ') as name,
       c.r_hnumf, 
       c.r_hnuml, 
       'R' as side,
       nullif(h.placename,'Unknown') as city,
       h.province as state
  from rawdata.roadseg b, rawdata.addrange c, rawdata.strplaname h
 where b.adrangenid=c.nid and c.r_offnanid=h.nid
       and not ((c.r_hnumf = 0 and c.r_hnuml = 0) or (c.r_hnumf = -1 and c.r_hnuml = -1))
union 
select b.gid as rsgid,
       array_to_string(ARRAY[
           nullif(h.dirprefix,'None'),
           nullif(h.strtypre,'None'),
           nullif(h.starticle,'None'),
           nullif(h.namebody,'None'),
           nullif(h.strtysuf,'None'),
           nullif(h.dirsuffix,'None')],' ') as name,
       c.l_hnumf, 
       c.l_hnuml, 
       'L' as side,
       nullif(h.placename,'Unknown') as city,
       h.province as state
  from rawdata.roadseg b, rawdata.addrange c, rawdata.altnamlink d, rawdata.strplaname h
 where b.adrangenid=c.nid and c.l_altnanid=d.nid and d.strnamenid=h.nid
       and not ((c.l_hnumf = 0 and c.l_hnuml = 0) or (c.l_hnumf = -1 and c.l_hnuml = -1))
union 
select b.gid as rsgid,
       array_to_string(ARRAY[
           nullif(h.dirprefix,'None'),
           nullif(h.strtypre,'None'),
           nullif(h.starticle,'None'),
           nullif(h.namebody,'None'),
           nullif(h.strtysuf,'None'),
           nullif(h.dirsuffix,'None')],' ') as name,
       c.r_hnumf, 
       c.r_hnuml, 
       'R' as side,
       nullif(h.placename,'Unknown') as city,
       h.province as state
  from rawdata.roadseg b, rawdata.addrange c, rawdata.altnamlink d, rawdata.strplaname h
 where b.adrangenid=c.nid and c.r_altnanid=d.nid and d.strnamenid=h.nid
       and not ((c.r_hnumf = 0 and c.r_hnuml = 0) or (c.r_hnumf = -1 and c.r_hnuml = -1))
union
select b.gid as rsgid,
       b.l_stname_c as name,
       coalesce(nullif(b.l_hnumf, -1), b.l_hnuml) as hnumf,
       coalesce(nullif(b.l_hnuml, -1), b.l_hnumf) as hnuml,
       'L' as side,
       nullif(b.l_placenam,'Unknown') as city,
       b.datasetnam as state
  from rawdata.roadseg b
 where b.l_hnumf != 0 and b.l_hnuml != 0 and (b.l_hnumf != -1 or b.l_hnuml != -1) and b.l_stname_c != 'None'
       and not ((b.l_hnumf = 0 and b.l_hnuml = 0) or (b.l_hnumf = -1 and b.l_hnuml = -1))
union
select b.gid as rsgid,
       b.rtnumber1 as name,
       coalesce(nullif(b.l_hnumf, -1), b.l_hnuml) as hnumf,
       coalesce(nullif(b.l_hnuml, -1), b.l_hnumf) as hnuml,
       'L' as side,
       nullif(b.l_placenam,'Unknown') as city,
       b.datasetnam as state
  from rawdata.roadseg b
 where b.rtnumber1 != 'None'
       and not ((b.l_hnumf = 0 and b.l_hnuml = 0) or (b.l_hnumf = -1 and b.l_hnuml = -1))
union
select b.gid as rsgid,
       b.rtnumber2 as name,
       coalesce(nullif(b.l_hnumf, -1), b.l_hnuml) as hnumf,
       coalesce(nullif(b.l_hnuml, -1), b.l_hnumf) as hnuml,
       'L' as side,
       nullif(b.l_placenam,'Unknown') as city,
       b.datasetnam as state
  from rawdata.roadseg b
 where b.rtnumber2 != 'None'
       and not ((b.l_hnumf = 0 and b.l_hnuml = 0) or (b.l_hnumf = -1 and b.l_hnuml = -1))
union
select b.gid as rsgid,
       b.rtnumber3 as name,
       coalesce(nullif(b.l_hnumf, -1), b.l_hnuml) as hnumf,
       coalesce(nullif(b.l_hnuml, -1), b.l_hnumf) as hnuml,
       'L' as side,
       nullif(b.l_placenam,'Unknown') as city,
       b.datasetnam as state
  from rawdata.roadseg b
 where b.rtnumber3 != 'None'
       and not ((b.l_hnumf = 0 and b.l_hnuml = 0) or (b.l_hnumf = -1 and b.l_hnuml = -1))
union
select b.gid as rsgid,
       b.rtnumber4 as name,
       coalesce(nullif(b.l_hnumf, -1), b.l_hnuml) as hnumf,
       coalesce(nullif(b.l_hnuml, -1), b.l_hnumf) as hnuml,
       'L' as side,
       nullif(b.l_placenam,'Unknown') as city,
       b.datasetnam as state
  from rawdata.roadseg b
 where b.rtnumber4 != 'None'
       and not ((b.l_hnumf = 0 and b.l_hnuml = 0) or (b.l_hnumf = -1 and b.l_hnuml = -1))
union
select b.gid as rsgid,
       b.rtnumber5 as name,
       coalesce(nullif(b.l_hnumf, -1), b.l_hnuml) as hnumf,
       coalesce(nullif(b.l_hnuml, -1), b.l_hnumf) as hnuml,
       'L' as side,
       nullif(b.l_placenam,'Unknown') as city,
       b.datasetnam as state
  from rawdata.roadseg b
 where b.rtnumber5 != 'None'
       and not ((b.l_hnumf = 0 and b.l_hnuml = 0) or (b.l_hnumf = -1 and b.l_hnuml = -1))
union
select b.gid as rsgid,
       b.rtename1fr as name,
       coalesce(nullif(b.l_hnumf, -1), b.l_hnuml) as hnumf,
       coalesce(nullif(b.l_hnuml, -1), b.l_hnumf) as hnuml,
       'L' as side,
       nullif(b.l_placenam,'Unknown') as city,
       b.datasetnam as state
  from rawdata.roadseg b
 where b.rtename1fr != 'None'
       and not ((b.l_hnumf = 0 and b.l_hnuml = 0) or (b.l_hnumf = -1 and b.l_hnuml = -1))
union
select b.gid as rsgid,
       b.rtename2fr as name,
       coalesce(nullif(b.l_hnumf, -1), b.l_hnuml) as hnumf,
       coalesce(nullif(b.l_hnuml, -1), b.l_hnumf) as hnuml,
       'L' as side,
       nullif(b.l_placenam,'Unknown') as city,
       b.datasetnam as state
  from rawdata.roadseg b
 where b.rtename2fr != 'None'
       and not ((b.l_hnumf = 0 and b.l_hnuml = 0) or (b.l_hnumf = -1 and b.l_hnuml = -1))
union
select b.gid as rsgid,
       b.rtename3fr as name,
       coalesce(nullif(b.l_hnumf, -1), b.l_hnuml) as hnumf,
       coalesce(nullif(b.l_hnuml, -1), b.l_hnumf) as hnuml,
       'L' as side,
       nullif(b.l_placenam,'Unknown') as city,
       b.datasetnam as state
  from rawdata.roadseg b
 where b.rtename3fr != 'None'
       and not ((b.l_hnumf = 0 and b.l_hnuml = 0) or (b.l_hnumf = -1 and b.l_hnuml = -1))
union
select b.gid as rsgid,
       b.rtename4fr as name,
       coalesce(nullif(b.l_hnumf, -1), b.l_hnuml) as hnumf,
       coalesce(nullif(b.l_hnuml, -1), b.l_hnumf) as hnuml,
       'L' as side,
       nullif(b.l_placenam,'Unknown') as city,
       b.datasetnam as state
  from rawdata.roadseg b
 where b.rtename4fr != 'None'
       and not ((b.l_hnumf = 0 and b.l_hnuml = 0) or (b.l_hnumf = -1 and b.l_hnuml = -1))
union
select b.gid as rsgid,
       b.rtename1en as name,
       coalesce(nullif(b.l_hnumf, -1), b.l_hnuml) as hnumf,
       coalesce(nullif(b.l_hnuml, -1), b.l_hnumf) as hnuml,
       'L' as side,
       nullif(b.l_placenam,'Unknown') as city,
       b.datasetnam as state
  from rawdata.roadseg b
 where b.rtename1en != 'None'
       and not ((b.l_hnumf = 0 and b.l_hnuml = 0) or (b.l_hnumf = -1 and b.l_hnuml = -1))
union
select b.gid as rsgid,
       b.rtename2en as name,
       coalesce(nullif(b.l_hnumf, -1), b.l_hnuml) as hnumf,
       coalesce(nullif(b.l_hnuml, -1), b.l_hnumf) as hnuml,
       'L' as side,
       nullif(b.l_placenam,'Unknown') as city,
       b.datasetnam as state
  from rawdata.roadseg b
 where b.rtename2en != 'None'
       and not ((b.l_hnumf = 0 and b.l_hnuml = 0) or (b.l_hnumf = -1 and b.l_hnuml = -1))
union
select b.gid as rsgid,
       b.rtename3en as name,
       coalesce(nullif(b.l_hnumf, -1), b.l_hnuml) as hnumf,
       coalesce(nullif(b.l_hnuml, -1), b.l_hnumf) as hnuml,
       'L' as side,
       nullif(b.l_placenam,'Unknown') as city,
       b.datasetnam as state
  from rawdata.roadseg b
 where b.rtename3en != 'None'
       and not ((b.l_hnumf = 0 and b.l_hnuml = 0) or (b.l_hnumf = -1 and b.l_hnuml = -1))
union
select b.gid as rsgid,
       b.rtename4en as name,
       coalesce(nullif(b.l_hnumf, -1), b.l_hnuml) as hnumf,
       coalesce(nullif(b.l_hnuml, -1), b.l_hnumf) as hnuml,
       'L' as side,
       nullif(b.l_placenam,'Unknown') as city,
       b.datasetnam as state
  from rawdata.roadseg b
 where b.rtename4en != 'None'
       and not ((b.l_hnumf = 0 and b.l_hnuml = 0) or (b.l_hnumf = -1 and b.l_hnuml = -1))
union -- right side
select b.gid as rsgid,
       b.r_stname_c as name,
       coalesce(nullif(b.r_hnumf, -1), b.r_hnuml) as hnumf,
       coalesce(nullif(b.r_hnuml, -1), b.r_hnumf) as hnuml,
       'R' as side,
       nullif(b.r_placenam,'Unknown') as city,
       b.datasetnam as state
  from rawdata.roadseg b
 where b.r_stname_c != 'None'
       and not ((b.r_hnumf = 0 and b.r_hnuml = 0) or (b.r_hnumf = -1 and b.r_hnuml = -1))
union
select b.gid as rsgid,
       b.rtnumber1 as name,
       coalesce(nullif(b.r_hnumf, -1), b.r_hnuml) as hnumf,
       coalesce(nullif(b.r_hnuml, -1), b.r_hnumf) as hnuml,
       'R' as side,
       nullif(b.r_placenam,'Unknown') as city,
       b.datasetnam as state
  from rawdata.roadseg b
 where b.rtnumber1 != 'None'
       and not ((b.r_hnumf = 0 and b.r_hnuml = 0) or (b.r_hnumf = -1 and b.r_hnuml = -1))
union
select b.gid as rsgid,
       b.rtnumber2 as name,
       coalesce(nullif(b.r_hnumf, -1), b.r_hnuml) as hnumf,
       coalesce(nullif(b.r_hnuml, -1), b.r_hnumf) as hnuml,
       'R' as side,
       nullif(b.r_placenam,'Unknown') as city,
       b.datasetnam as state
  from rawdata.roadseg b
 where b.rtnumber2 != 'None'
       and not ((b.r_hnumf = 0 and b.r_hnuml = 0) or (b.r_hnumf = -1 and b.r_hnuml = -1))
union
select b.gid as rsgid,
       b.rtnumber3 as name,
       coalesce(nullif(b.r_hnumf, -1), b.r_hnuml) as hnumf,
       coalesce(nullif(b.r_hnuml, -1), b.r_hnumf) as hnuml,
       'R' as side,
       nullif(b.r_placenam,'Unknown') as city,
       b.datasetnam as state
  from rawdata.roadseg b
 where b.rtnumber3 != 'None'
       and not ((b.r_hnumf = 0 and b.r_hnuml = 0) or (b.r_hnumf = -1 and b.r_hnuml = -1))
union
select b.gid as rsgid,
       b.rtnumber4 as name,
       coalesce(nullif(b.r_hnumf, -1), b.r_hnuml) as hnumf,
       coalesce(nullif(b.r_hnuml, -1), b.r_hnumf) as hnuml,
       'R' as side,
       nullif(b.r_placenam,'Unknown') as city,
       b.datasetnam as state
  from rawdata.roadseg b
 where b.rtnumber4 != 'None'
       and not ((b.r_hnumf = 0 and b.r_hnuml = 0) or (b.r_hnumf = -1 and b.r_hnuml = -1))
union
select b.gid as rsgid,
       b.rtnumber5 as name,
       coalesce(nullif(b.r_hnumf, -1), b.r_hnuml) as hnumf,
       coalesce(nullif(b.r_hnuml, -1), b.r_hnumf) as hnuml,
       'R' as side,
       nullif(b.r_placenam,'Unknown') as city,
       b.datasetnam as state
  from rawdata.roadseg b
 where b.rtnumber5 != 'None'
       and not ((b.r_hnumf = 0 and b.r_hnuml = 0) or (b.r_hnumf = -1 and b.r_hnuml = -1))
union
select b.gid as rsgid,
       b.rtename1fr as name,
       coalesce(nullif(b.r_hnumf, -1), b.r_hnuml) as hnumf,
       coalesce(nullif(b.r_hnuml, -1), b.r_hnumf) as hnuml,
       'R' as side,
       nullif(b.r_placenam,'Unknown') as city,
       b.datasetnam as state
  from rawdata.roadseg b
 where b.rtename1fr != 'None'
       and not ((b.r_hnumf = 0 and b.r_hnuml = 0) or (b.r_hnumf = -1 and b.r_hnuml = -1))
union
select b.gid as rsgid,
       b.rtename2fr as name,
       coalesce(nullif(b.r_hnumf, -1), b.r_hnuml) as hnumf,
       coalesce(nullif(b.r_hnuml, -1), b.r_hnumf) as hnuml,
       'R' as side,
       nullif(b.r_placenam,'Unknown') as city,
       b.datasetnam as state
  from rawdata.roadseg b
 where b.rtename2fr != 'None'
       and not ((b.r_hnumf = 0 and b.r_hnuml = 0) or (b.r_hnumf = -1 and b.r_hnuml = -1))
union
select b.gid as rsgid,
       b.rtename3fr as name,
       coalesce(nullif(b.r_hnumf, -1), b.r_hnuml) as hnumf,
       coalesce(nullif(b.r_hnuml, -1), b.r_hnumf) as hnuml,
       'R' as side,
       nullif(b.r_placenam,'Unknown') as city,
       b.datasetnam as state
  from rawdata.roadseg b
 where b.rtename3fr != 'None'
       and not ((b.r_hnumf = 0 and b.r_hnuml = 0) or (b.r_hnumf = -1 and b.r_hnuml = -1))
union
select b.gid as rsgid,
       b.rtename4fr as name,
       coalesce(nullif(b.r_hnumf, -1), b.r_hnuml) as hnumf,
       coalesce(nullif(b.r_hnuml, -1), b.r_hnumf) as hnuml,
       'R' as side,
       nullif(b.r_placenam,'Unknown') as city,
       b.datasetnam as state
  from rawdata.roadseg b
 where b.rtename4fr != 'None'
       and not ((b.r_hnumf = 0 and b.r_hnuml = 0) or (b.r_hnumf = -1 and b.r_hnuml = -1))
union
select b.gid as rsgid,
       b.rtename1en as name,
       coalesce(nullif(b.r_hnumf, -1), b.r_hnuml) as hnumf,
       coalesce(nullif(b.r_hnuml, -1), b.r_hnumf) as hnuml,
       'R' as side,
       nullif(b.r_placenam,'Unknown') as city,
       b.datasetnam as state
  from rawdata.roadseg b
 where b.rtename1en != 'None'
       and not ((b.r_hnumf = 0 and b.r_hnuml = 0) or (b.r_hnumf = -1 and b.r_hnuml = -1))
union
select b.gid as rsgid,
       b.rtename2en as name,
       coalesce(nullif(b.r_hnumf, -1), b.r_hnuml) as hnumf,
       coalesce(nullif(b.r_hnuml, -1), b.r_hnumf) as hnuml,
       'R' as side,
       nullif(b.r_placenam,'Unknown') as city,
       b.datasetnam as state
  from rawdata.roadseg b
 where b.rtename2en != 'None'
       and not ((b.r_hnumf = 0 and b.r_hnuml = 0) or (b.r_hnumf = -1 and b.r_hnuml = -1))
union
select b.gid as rsgid,
       b.rtename3en as name,
       coalesce(nullif(b.r_hnumf, -1), b.r_hnuml) as hnumf,
       coalesce(nullif(b.r_hnuml, -1), b.r_hnumf) as hnuml,
       'R' as side,
       nullif(b.r_placenam,'Unknown') as city,
       b.datasetnam as state
  from rawdata.roadseg b
 where b.rtename3en != 'None'
       and not ((b.r_hnumf = 0 and b.r_hnuml = 0) or (b.r_hnumf = -1 and b.r_hnuml = -1))
union
select b.gid as rsgid,
       b.rtename4en as name,
       coalesce(nullif(b.r_hnumf, -1), b.r_hnuml) as hnumf,
       coalesce(nullif(b.r_hnuml, -1), b.r_hnumf) as hnuml,
       'R' as side,
       nullif(b.r_placenam,'Unknown') as city,
       b.datasetnam as state
  from rawdata.roadseg b
 where b.rtename4en != 'None'
       and not ((b.r_hnumf = 0 and b.r_hnuml = 0) or (b.r_hnumf = -1 and b.r_hnuml = -1))
) as foo
 -- where foo.rsgid=1
order by foo.rsgid;
-- Query returned successfully: 2606012 rows affected, 1055873 ms. execution time.
-- Query returned successfully: 2417328 rows affected, 03:58 minutes execution time. (2016)

select * from rawdata.st limit 100;

select city, count(*) from rawdata.st group by city order by city;
select * from rawdata.st where city ilike '% Subdivision';

-- Clean up city names
update rawdata.st
  set city = regexp_replace(city, E'^((The )?Corporation Of The Municipality|City|Town|County|District|Municipality|Township|United Townships|Village) Of |^DSL de |, (County|M.D.|Municipality) Of$|, Unorganized$| \\(Part\\)$| County$| Subdivision$', '', 'i');
-- Query returned successfully: 2417328 rows affected, 01:34 minutes execution time.

vacuum analyze rawdata.st;


-----------------------------------------------------------------------------------------------------------------------------------------------------
-- statcan data prep
-----------------------------------------------------------------------------------------------------------------------------------------------------

/*
CREATE TABLE rawdata.st_statcan
(
  gid serial NOT NULL,
  ngd_uid character varying(10),
  name character varying(50),
  type character varying(6),
  dir character varying(2),
  afl_val character varying(9),
  atl_val character varying(9),
  afr_val character varying(9),
  atr_val character varying(9),
  csduid_l character varying(7),
  csdname_l character varying(55),
  csdtype_l character varying(3),
  csduid_r character varying(7),
  csdname_r character varying(55),
  csdtype_r character varying(3),
  pruid_l character varying(2),
  prname_l character varying(55),
  pruid_r character varying(2),
  prname_r character varying(55),
  class character varying(2),
  geom geometry(MultiLineString,3347),
  CONSTRAINT st_statcan_pkey PRIMARY KEY (gid)
)
*/

select csdname_l, count(*) as cnt from rawdata.st_statcan group by csdname_l order by csdname_l;
select prname_l, count(*) as cnt from rawdata.st_statcan group by prname_l order by prname_l;
select * from rawdata.st_statcan limit 100;


drop table if exists rawdata.st2 cascade;

create table rawdata.st2 (
 gid serial not null primary key,
 link_id integer, 
 name text, 
 hnumf integer, 
 hnuml integer, 
 side char(1), 
 city text, 
 state text, 
 country text
);


insert into rawdata.st2 (link_id, name, hnumf, hnuml, side, city, state, country)

select foo.*, 'CANADA' as country from (
select ngd_uid::integer as link_id,
       array_to_string(ARRAY[a.name, a.type, a.dir],' ') as name,
       coalesce(a.afl_val,a.atl_val)::integer, 
       coalesce(a.atl_val,a.afl_val)::integer, 
       'L' as side,
       regexp_replace(csdname_l, E', Unorganized,|\\(Part\\) |, Unorganized$', '', 'i') as city,
       case when a.pruid_l='10' then 'NL'
            when a.pruid_l='11' then 'PE'
            when a.pruid_l='12' then 'NS'
            when a.pruid_l='13' then 'NB'
            when a.pruid_l='24' then 'QC'
            when a.pruid_l='35' then 'ON'
            when a.pruid_l='46' then 'MB'
            when a.pruid_l='47' then 'SK'
            when a.pruid_l='48' then 'AB'
            when a.pruid_l='59' then 'BC'
            when a.pruid_l='60' then 'YT'
            when a.pruid_l='61' then 'NT'
            when a.pruid_l='62' then 'NU'
            else NULL end as state
  from rawdata.st_statcan a
 where a.afl_val is not null and a.atl_val is not null
union 
select ngd_uid::integer as link_id,
       array_to_string(ARRAY[a.name, lower(a.type), a.dir],' ') as name,
       coalesce(a.afr_val,a.atr_val)::integer, 
       coalesce(a.atr_val,a.afr_val)::integer, 
       'R' as side,
       regexp_replace(csdname_r, E', Unorganized,|\\(Part\\) |, Unorganized$', '', 'i') as city,
       case when a.pruid_r='10' then 'NL'
            when a.pruid_r='11' then 'PE'
            when a.pruid_r='12' then 'NS'
            when a.pruid_r='13' then 'NB'
            when a.pruid_r='24' then 'QC'
            when a.pruid_r='35' then 'ON'
            when a.pruid_r='46' then 'MB'
            when a.pruid_r='47' then 'SK'
            when a.pruid_r='48' then 'AB'
            when a.pruid_r='59' then 'BC'
            when a.pruid_r='60' then 'YT'
            when a.pruid_r='61' then 'NT'
            when a.pruid_r='62' then 'NU'
            else NULL end as state
  from rawdata.st_statcan a
 where a.afr_val is not null and a.atr_val is not null
) as foo
order by link_id;
-- Query returned successfully: 2282154 rows affected, 01:26 minutes execution time.

CREATE INDEX st_statcan_ngd_uid_idx  ON rawdata.st_statcan  USING btree  (ngd_uid COLLATE pg_catalog."default");


/*
* using prep-tiger-geo-new.sql as a pattern
* create data.streets
* copy rawdata.st with geom into streets
* copy rawdata.st_statcan with geom into streets (add 10,000,000 to gid for link_id)
* finish preping geocoder
*/


-- Table: streets

DROP TABLE if exists streets cascade;

CREATE TABLE streets
(
  gid serial NOT NULL,
  tlid numeric(10,0),
  refaddr numeric(10,0),
  nrefaddr numeric(10,0),
  sqlnumf character varying(12),
  sqlfmt character varying(16),
  cformat character varying(16),
  name character varying(80),
  predirabrv character varying(15),
  pretypabrv character varying(50),
  prequalabr character varying(15),
  sufdirabrv character varying(15),
  suftypabrv character varying(50),
  sufqualabr character varying(15),
  side smallint,
  tfid numeric(10,0),
  usps character varying(35),
  ac5 character varying(35),
  ac4 character varying(100),
  ac3 character varying(35),
  ac2 character varying(35),
  ac1 character varying(35),
  postcode character varying(11),
  the_geom geometry(MultiLineString,4326),
  CONSTRAINT streets_pkey PRIMARY KEY (gid)
)
WITH (
  OIDS=FALSE
);

CREATE INDEX streets_gid_idx  ON streets  USING btree  (gid);


-- load the gebaase data into streets

insert into streets (tlid, refaddr, nrefaddr, name, side, ac4, ac2, ac1, the_geom)
select rsgid, hnumf, hnuml, name, case when side='R' then 1 when side='L' then 2 else 0 end as side, city, state, country, st_transform(b.geom, 4326) as the_geom
  from rawdata.st a, rawdata.roadseg b
 where rsgid=b.gid
 order by rsgid;
-- Query returned successfully: 2417328 rows affected, 01:37 minutes execution time.


--select min(rsgid), max(rsgid) from rawdata.st; -- 3; 2,418,660
--select min(ngd_uid::integer), max(ngd_uid::integer) from rawdata.st_statcan; -- 1;5,572,822

-- load the statcan data into streets

insert into streets (tlid, refaddr, nrefaddr, name, side, ac4, ac2, ac1, the_geom)
select a.link_id+10000000 as tlid, hnumf, hnuml, a.name, case when side='R' then 1 when side='L' then 2 else 0 end as side, city, state, country, st_transform(b.geom, 4326) as the_geom
  from rawdata.st2 a, rawdata.st_statcan b
 where link_id::text=b.ngd_uid
 order by link_id;
-- Query returned successfully: 2282154 rows affected, 02:18 minutes execution time.

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
           --    replace(unaccent(array_to_string(ARRAY[coalesce(nullif(refaddr, -1),nrefaddr)::varchar, predirabrv, pretypabrv, prequalabr, name, sufdirabrv, suftypabrv, sufqualabr], ' ')), '''', ' '),
               unaccent(array_to_string(ARRAY[coalesce(nullif(refaddr, -1),nrefaddr)::varchar, predirabrv, pretypabrv, prequalabr, name, sufdirabrv, suftypabrv, sufqualabr], ' ')),
               unaccent(array_to_string(ARRAY[coalesce(usps,ac5,ac4,ac3),ac2,ac1,postcode], ',')) ) as std
          from streets
         where nullif(replace(unaccent(array_to_string(ARRAY[predirabrv, pretypabrv, prequalabr, name, sufdirabrv, suftypabrv, sufqualabr], ' ')), '''', ' '), '') is not null
          order by gid
       ) as foo
       --limit 10
       --where id=415463
       ;

create index stdstreets_name_city_state_idx on stdstreets using btree (name,state,city ASC NULLS LAST);
create index stdstreets_name_postcode_idx on stdstreets using btree (name,postcode ASC NULLS LAST);
create index stdstreets_name_dmeta_city_state_idx on stdstreets using btree (name_dmeta,state,city ASC NULLS LAST);
create index stdstreets_name_dmeta_postcode_idx on stdstreets using btree (name_dmeta,postcode ASC NULLS LAST);

commit;
-- Query returned successfully with no result in 18:24 minutes.

vacuum analyze verbose stdstreets;


drop table if exists failures;
select count(*) as cnt,
       unaccent(array_to_string(ARRAY[predirabrv, pretypabrv, prequalabr, name,
                             sufdirabrv, suftypabrv, sufqualabr], ' ')) as micro
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
-- Query returned successfully: 870 rows affected, 1.3 secs execution time.

select * from standardize_address('lex','gaz','rules', unaccent('123 86B Street'), '');

drop table if exists failures2;
select unaccent(array_to_string(ARRAY[predirabrv, pretypabrv, prequalabr, b.name,
                             sufdirabrv, suftypabrv, sufqualabr], ' ')) as micro, a.*
  into failures2
  from stdstreets a, streets b
 where a.id=b.gid and a.unit is not null; -- 9955 rows

select * from failures2 limit 500;
select micro, count(*) as cnt from failures2 group by micro order by cnt desc;
select * from failures2 where micro ilike 'no %';

select st_astext(the_geom), * from streets where gid=411419;


/*
-- ISSUES --
