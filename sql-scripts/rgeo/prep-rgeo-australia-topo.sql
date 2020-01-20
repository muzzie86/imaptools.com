select * from rawdata.roads limit 500;

select class, count(*) from rawdata.roads group by class;
/*
"Track";206842
"Minor Road";193566
"Dual Carriageway";1328
"Principal Road";29555
"Secondary Road";28234
*/

select count(*) from rawdata.roads where coalesce(name, nrn, srn) is not null;
-- 84,433 out of 459,525 or 18.4%

select count(*) from rawdata.roads where class != 'Track' and coalesce(name, nrn, srn) is not null;
-- 81,890 out of 252,683 or 32.4%


set search_path = data, public;

DROP TABLE IF EXISTS streets CASCADE;

CREATE TABLE streets
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
  l_ac5 character varying(50),
  l_ac4 character varying(50),
  l_ac3 character varying(50),
  l_ac2 character varying(50),
  l_ac1 character varying(35),
  r_ac5 character varying(50),
  r_ac4 character varying(50),
  r_ac3 character varying(50),
  r_ac2 character varying(50),
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


-- run imt-rgeo-pgis-2.0.sql

insert into streets (link_id, name, the_geom)
  select gid, array_to_string(array[name, nrn, srn], ' '), st_transform(geom, 4326) from rawdata.roads
  union all
  select gid+500000, textnote, st_transform(geom, 4326) from rawdata.roadtunnellines
  union all
  select gid+510000, array_to_string(array[name, textnote], ' '), st_transform(geom, 4326) from rawdata.foottracks
  union all
  select gid+520000, array_to_string(array[name, textnote], ' '), st_transform(geom, 4326) from rawdata.railways
  union all
  select gid+530000, textnote, st_transform(geom, 4326) from rawdata.railwaytunnellines
  union all
  select gid+540000, array_to_string(array[name, textnote], ' '), st_transform(geom, 4326) from rawdata.ferryroutelines
  ;

CREATE INDEX ON streets USING gist (the_geom);

CREATE INDEX ON rawdata.m_state USING btree (state_pid ASC NULLS LAST);
CREATE INDEX ON rawdata.state USING btree (state_pid ASC NULLS LAST);
CREATE INDEX ON rawdata.state USING gist (geom);


create view rawdata.v_state as
  select state_name, geom
    from rawdata.state a, rawdata.m_state b
   where a.state_pid=b.state_pid;


CREATE UNIQUE INDEX ON rawdata.m_locality USING btree (loc_pid ASC NULLS LAST);
CREATE INDEX ON rawdata.locality USING btree (loc_pid ASC NULLS LAST);
CREATE INDEX ON rawdata.locality USING gist (geom);

create view rawdata.v_locality as
  select state_name,  name, a.geom
    from rawdata.locality a, rawdata.m_locality b, rawdata.m_state c
   where a.loc_pid=b.loc_pid and b.state_pid=c.state_pid;


update streets set r_ac1='AU',                     l_ac1='AU',
                   r_ac2=coalesce(r_ac2, foo.ac2), l_ac2=coalesce(l_ac2, foo.ac2),
                   r_ac4=coalesce(r_ac4, foo.ac4), l_ac4=coalesce(l_ac4, foo.ac4)
  from (
         select distinct on (a.gid) a.gid as gid, state_name as ac2, b.name as ac4
           from streets a
           join rawdata.v_locality b ON st_dwithin(b.geom, st_transform(a.the_geom, 4283), 0.0)
           order by a.gid, st_length(st_intersection(b.geom, st_transform(a.the_geom, 4283))) desc
       ) as foo
 where streets.gid=foo.gid;
-- Query returned successfully: 462631 rows affected, 01:00:3613 hours execution time.

select count(*) from streets where l_ac2 is null; -- 560

CREATE INDEX ON rawdata.islands USING gist (geom);

update streets set r_ac1='AU',                     l_ac1='AU',
                   r_ac2=coalesce(r_ac2, foo.ac2), l_ac2=coalesce(l_ac2, foo.ac2),
                   r_ac4=coalesce(r_ac4, foo.ac4), l_ac4=coalesce(l_ac4, foo.ac4)
  from (
         select distinct on (a.gid) a.gid as gid, b.state as ac2, b.name as ac4
           from streets a
           join rawdata.islands b ON st_dwithin(b.geom, st_transform(a.the_geom, 4283), 0.0)
           where r_ac1 is null
           order by a.gid, st_length(st_intersection(b.geom, st_transform(a.the_geom, 4283))) desc
       ) as foo
 where streets.gid=foo.gid and r_ac1 is null;
-- Query returned successfully: 142 rows affected, 498 msec execution time.

select count(*) from streets where l_ac2 is null; -- 418
select count(*) from streets where l_ac2 is null and name is null; -- 0


update streets set r_ac1='AU',                     l_ac1='AU',
                   r_ac2=coalesce(r_ac2, foo.ac2), l_ac2=coalesce(l_ac2, foo.ac2)
  from (
         select distinct on (a.gid) a.gid as gid, state_name as ac2
           from streets a
           join rawdata.v_state b ON st_dwithin(b.geom, st_transform(a.the_geom, 4283), 0.0)
           where r_ac1 is null
           order by a.gid, st_length(st_intersection(b.geom, st_transform(a.the_geom, 4283))) desc
       ) as foo
 where streets.gid=foo.gid and r_ac1 is null;
-- Query returned successfully: 399 rows affected, 01:45 minutes execution time.

select * from streets limit 500;

------------------------------------------
-- run run imt-rgeo-pgis-2.0-part2.sql
------------------------------------------

select count(*) from streets where name = ''; -- 375,638
update streets set name=NULL where name='';
-- Query returned successfully: 375638 rows affected, 25.8 secs execution time.



select imt_make_intersections('streets', 0.000001, 'the_geom', 'gid', ' name is not null ');
-- Query returned successfully: 399 rows affected, 01:45 minutes execution time.
-- Total query runtime: 35.5 secs

analyze;
-- Query returned successfully with no result in 4.2 secs.

update intersections_tmp as a set
          cnt  = (select count(*) from st_vert_tmp b where b.vid=a.id),
          dcnt = (select count(distinct name) from st_vert_tmp b, streets c where b.gid=c.gid and b.vid=a.id and c.name is not null);
-- Query returned successfully: 333465 rows affected, 38.9 secs execution time.
-- Query returned successfully: 83955 rows affected, 8.8 secs execution time.


alter function imt_make_intersections(character varying, double precision, character varying, character varying, text) set schema rawdata;
-- alter function rawdata.imt_make_intersections(character varying, double precision, character varying, character varying, text) set schema data;

CREATE INDEX streets_link_id_idx  ON streets  USING btree  (link_id);
-- Query returned successfully with no result in 623 msec.

CREATE INDEX streets_gidx  ON streets  USING gist  (the_geom);
ALTER TABLE streets CLUSTER ON streets_gidx;
-- Query returned successfully with no result in 10.5 secs.

vacuum analyze;
-- Query returned successfully with no result in 4.3 secs.

select * from imt_reverse_geocode_flds(143.93990, -37.25201);
-- "CRESWICK NEWSTEAD ROAD C283";"";"CAMPBELLTOWN";"";"VICTORIA";"AU";"";32.351468655;45102

select * from imt_rgeo_intersections_flds(143.93990, -37.25201);
-- "GLENGOWER ROAD, CRESWICK NEWSTEAD ROAD C283";"";"CAMPBELLTOWN";"";"VICTORIA";"AU";"";5379.3;23005

select * from imt_reverse_geocode_flds(143.80841, -37.55129);
-- "BALLARAT BURRUMBEET ROAD C805";"";"ALFREDTON";"";"VICTORIA";"AU";"";0.026490491;286008

select * from imt_rgeo_intersections_flds(143.80841, -37.55129);
-- "GLENGOWER ROAD, CRESWICK NEWSTEAD ROAD C283";"";"CAMPBELLTOWN";"";"VICTORIA";"AU";"";5379.3;23005

select * from imt_reverse_geocode_flds(143.76798, -37.54238);
-- "BALLARAT SKIPTON RAIL TRAIL";"";"CARDIGAN";"";"VICTORIA";"AU";"";3.541153595;540403

select * from imt_rgeo_intersections_flds(143.76798, -37.54238);
-- "gauge 1600mm abandoned";"";"ALFREDTON";"";"VICTORIA";"AU";"";3688.9;81557

--------------------------------------------------------------------------------------------------------------
-- integrating G-NAF data
--------------------------------------------------------------------------------------------------------------

-- Function: imt_reverse_geocode_flds(geometry, double precision)

-- DROP FUNCTION imt_reverse_geocode_flds(geometry, double precision);

CREATE OR REPLACE FUNCTION imt_reverse_geocode_flds(
    pnt geometry,
    srad double precision)
  RETURNS rgeo_result_flds AS
$BODY$
/*
 * ROW(address, village, city, county, state, country, postcode, distance_meters, linkid) = rgeo(point);
 * ROW(address, village, city, county, state, country, postcode, distance_meters, linkid) = rgeo(long, lat);
 * 
 * This function performs reverse geocoding on that iMaptools Navteq
 * data. It returns a rgeo_result record for the closest segment, or
 * NULL if it fails.
 *
 * We are ignoring the fact the degrees longitude are shortened as we
 * move toword the poles.
 *
 * Algorithm:
 * We create a small radius (0.0013 degrees, .0897 miles) and search
 * for the closest segment within that radius and compute the results.
 * If we fail to find any results in that radius we double it and search
 * again until we exceed a one degree radius which is roughtly 69*69 sq-miles.
 *
 */

DECLARE
    pnt2 GEOMETRY;
    radius FLOAT;
    address TEXT;
    location TEXT;
    city TEXT;
    mdistance FLOAT;
    rr RECORD;
    rr2 RECORD;
    pct FLOAT;
    rightvar BOOLEAN;
    ret rgeo_result_flds;
    mspeed integer;
    sraddd double precision;
    geom geometry;

BEGIN
    IF GeometryType(pnt) != 'POINT' THEN
        RETURN NULL;
    END IF;

    radius := 0.0013;  -- approx. 0.1 mi or 160 m
    LOOP
        IF radius < 0.00131 THEN
            SELECT * INTO rr FROM data.streets
                WHERE st_dwithin(pnt, the_geom, radius) AND name is not null
                ORDER BY st_distance(pnt, the_geom) ASC limit 1;
        END IF;
        IF radius > 0.0013 OR NOT FOUND THEN
            SELECT * INTO rr FROM data.streets
                WHERE st_dwithin(pnt, the_geom, radius)
                ORDER BY st_distance(pnt, the_geom) ASC limit 1;
        END IF;
        IF FOUND THEN
--raise notice 'rr: %', rr;
            IF GeometryType(rr.the_geom)='MULTILINESTRING' THEN
                geom := st_GeometryN(rr.the_geom, 1);
            ELSE
                geom := rr.the_geom;
            END IF;
            pct := st_line_locate_point(geom, pnt);
            pnt2 := st_line_interpolate_point(geom, pct);
            mdistance := st_distance_sphere(pnt, pnt2);

            /*
             * we will arbitrarily assume the right side of the street
             * unless it has no address ranges, then the left else we
             * only return the street name if it exits.
             */
--raise notice '%, if % and % elsif % and %', rr.link_id, rr.l_refaddr IS NOT NULL, rr.l_sqlfmt IS NOT NULL, rr.r_refaddr IS NOT NULL, rr.r_sqlfmt IS NOT NULL;
            IF nullif(rr.l_refaddr, 0) IS NOT NULL AND rr.l_sqlfmt IS NOT NULL THEN
                /* linear interpolate the house number based on the pct */
--RAISE NOTICE 'LEFT: %, %, %', pct, TO_CHAR((rr.l_nrefaddr-rr.l_refaddr)*pct + rr.l_refaddr, rr.l_sqlnumf), (rr.l_nrefaddr-rr.l_refaddr)*pct + rr.l_refaddr;
                address := REPLACE(rr.l_sqlfmt, '{}',
                    TO_CHAR((rr.l_nrefaddr-rr.l_refaddr)*pct + rr.l_refaddr,
                        rr.l_sqlnumf)) || ' ';
                rightvar := false;
            ELSIF nullif(rr.r_refaddr, 0) IS NOT NULL AND rr.r_sqlfmt IS NOT NULL THEN
--RAISE NOTICE 'RIGHT: %, %', pct, TO_CHAR((rr.r_nrefaddr-rr.r_refaddr)*pct + rr.r_refaddr, rr.r_sqlnumf);
                /* linear interpolate the house number based on the pct */
                address := REPLACE(rr.r_sqlfmt, '{}',
                    TO_CHAR((rr.r_nrefaddr-rr.r_refaddr)*pct + rr.r_refaddr,
                        rr.r_sqlnumf)) || ' ';
                rightvar := true;
            ELSE
                /* no house numbers, we might still have a street name */
                rightvar := NULL;
            END IF;
            address := coalesce(address, '');
            
            /* get the street name */
            IF rr.name IS NOT NULL THEN
                --address := address || rr.name || ', ';
                address := address || rr.name;
            ELSE
                address := '';
            END IF;
            ret.address := address;

            if exists(SELECT * FROM pg_catalog.pg_tables WHERE tablename='addresses') then
                select into rr2 *, st_distance_sphere(pnt, a.geom) as dist
                  from addresses a
                 where st_dwithin(pnt, a.geom, 150.0/111120.0) 
                 order by st_distance(pnt, a.geom) asc limit 1;

                if FOUND and mdistance>rr2.dist then
                    ret.address  := rr2.address;
                    ret.village  := NULL;
                    ret.city     := rr2.locality_name;
                    ret.county   := NULL;
                    ret.state    := rr2.state_abbreviation;
                    ret.country  := 'AU';
                    ret.postcode := rr2.postcode;
                    ret.distance := rr2.dist;
                    ret.linkid   := rr2.id+100000000;
		    return ret;
		end if;
            end if;

            if (address='') and exists(SELECT * FROM pg_catalog.pg_tables WHERE tablename='intersections_tmp') then
                select into rr2 * from imt_rgeo_intersections_flds(pnt, srad);
                if coalesce(rr2.address,'') != '' then
		    return rr2;
		end if;
            end if;

            if ( rightvar IS NULL OR rightvar) and rr.r_ac5 is null and rr.r_ac5 is null and
                 exists(SELECT * FROM pg_catalog.pg_tables WHERE tablename='cities') then
                select into city cityname from cities a where st_dwithin(a.geom, pnt, 0.0);
                rr.r_ac4 = city;
            end if;
            
            if ( rightvar IS NULL OR NOT rightvar) and rr.l_ac5 is null and rr.l_ac5 is null and
                 exists(SELECT * FROM pg_catalog.pg_tables WHERE tablename='cities') then
                select into city cityname from cities a where st_dwithin(a.geom, pnt, 0.0);
                rr.l_ac4 = city;
            end if;
            
--RAISE NOTICE 'RIGHT: %, address: %', rightvar, address;
            /* get the city, state, country postcode*/
            location := '';
            IF rightvar IS NULL OR rightvar THEN
                location := COALESCE(NULLIF(rr.r_ac5, ''), NULLIF(rr.r_ac4, ''),
                  NULLIF(rr.r_ac3, '')) || ', ' || rr.r_ac2 || ', ' || rr.r_ac1
                  || COALESCE(NULLIF(', ' || rr.r_postcode, ', '), '');
                  ret.village := rr.r_ac5;
                  ret.city    := rr.r_ac4;
                  ret.county  := rr.r_ac3;
                  ret.state   := rr.r_ac2;
                  ret.country := rr.r_ac1;
                  ret.postcode := rr.r_postcode;
            END IF;
            
            IF (rightvar IS NULL AND LENGTH(address)=0) OR NOT rightvar THEN
                location := COALESCE(NULLIF(rr.l_ac5, ''), NULLIF(rr.l_ac4, ''),
                  NULLIF(rr.l_ac3, '')) || ', ' || rr.l_ac2 || ', ' || rr.l_ac1
                  || COALESCE(NULLIF(', ' || rr.l_postcode, ', '), '');
                  ret.village := rr.l_ac5;
                  ret.city    := rr.l_ac4;
                  ret.county  := rr.l_ac3;
                  ret.state   := rr.l_ac2;
                  ret.country := rr.l_ac1;
                  ret.postcode := rr.l_postcode;
            END IF;
            
            IF LENGTH(location)=0 THEN
            --raise notice 'no location: %', rr.link_id;
                RETURN NULL;
            ELSE
                --ret := ROW(address||location, mdistance, rr.link_id);
                ret.distance := mdistance;
                ret.linkid := rr.link_id;

/*                
                -- get the max speed of near by roads
                sraddd := coalesce(srad, 30.0) / 111120.0; -- meter to degree conversion
                select max(max(speed, ave_speed)) into mspeed from streets
                 where st_dwithin(pnt, the_geom, sraddd);

                ret.speed    := max(rr.speed, rr.ave_speed);
                ret.maxspeed := max(max(rr.speed, rr.ave_speed), mspeed);

                -- RAISE NOTICE 'gid=%, speed=%, ave_speed=%, mspeed=%', rr.gid, rr.speed, rr.ave_speed, mspeed;
*/
                RETURN ret;
            END IF;
        END IF;
        radius := radius * 2.0;
        IF radius > 0.5 THEN
            RETURN NULL;
        END IF;
    END LOOP;
END;
$BODY$
  LANGUAGE plpgsql STABLE
  COST 100;
ALTER FUNCTION imt_reverse_geocode_flds(geometry, double precision)
  OWNER TO postgres;


select * from imt_reverse_geocode_flds(117.66031, -23.20236);
-- "PARABURDOO TOM PRICE ROAD";"";"PARABURDOO";"";"WESTERN AUSTRALIA";"AU";"";107.220697335;310941  -- without addresses
-- "246 BARROW AVENUE";"";"PARABURDOO";"";"WA";"AU";"6754";2.326906835;112422334                    -- with addresses

select * from imt_rgeo_intersections_flds(117.66031, -23.20236);
-- "HAMERSLEY IRON RAILWAY gauge 1435mm, HAMERSLEY IRON ORE RAILWAY gauge 1435mm";"";"ROCKLEA";"";"WESTERN AUSTRALIA";"AU";"";32365;80928


-----------------------------------------------------------------------------------------------------
-- create new minimal addresses table
-----------------------------------------------------------------------------------------------------

select count(*), max(length(address)) as address from (
select distinct array_to_string(ARRAY[number_first_prefix, number_first::text, number_first_suffix, street_name, 
                          street_type_code, street_suffix_code], ' ') as address, locality_name, state_abbreviation, postcode
                  from addresses a
) as foo;
-- 9653011;40
-- Total query runtime: 06:19 minutes

alter table addresses set schema rawdata;

create table addresses (
  id serial not null primary key,
  address text,
  locality_name text,
  state_abbreviation text,
  postcode text
);
select addgeometrycolumn('data', 'addresses', 'geom', 4326, 'POINT', 2);

insert into addresses (address, locality_name, state_abbreviation, postcode, geom)
select distinct array_to_string(ARRAY[number_first_prefix, number_first::text, number_first_suffix, street_name, 
                          street_type_code, street_suffix_code], ' ') as address, locality_name, state_abbreviation, postcode, geom
  from rawdata.addresses;
-- Query returned successfully: 10606709 rows affected, 10:06 minutes execution time.

create index addresses_geom_gidx on addresses using gist (geom);
-- Query returned successfully with no result in 05:30 minutes.

alter table addresses cluster on addresses_geom_gidx;
-- Query returned successfully with no result in 11 msec.

vacuum analyze;
-- Query returned successfully with no result in 10.5 secs.


