------------------------------------------------------------------------------------------------------------------
-------------------- imaptools reverse geocoder prep intersections -----------------------------------------------
------------------------------------------------------------------------------------------------------------------

-- Function: imt_make_intersections(character varying, double precision, character varying, character varying, text)

-- DROP FUNCTION imt_make_intersections(character varying, double precision, character varying, character varying, text);

SET search_path to data, public;

CREATE OR REPLACE FUNCTION imt_make_intersections(geom_table character varying, tolerance double precision, geo_cname character varying, gid_cname character varying, where_str text DEFAULT(''))
  RETURNS character varying AS
$BODY$
DECLARE
    points record;
    source_id int;
    target_id int;
    srid integer;
    countids integer;
    where_clause text;

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

        where_clause := '';
        if where_str != '' then
            where_clause := ' where ' || where_str;
        end if;

        EXECUTE 'CREATE TABLE intersections_tmp (id serial, cnt integer, dcnt integer)';

        EXECUTE 'SELECT srid FROM geometry_columns WHERE f_table_name='|| quote_literal(geom_table) INTO srid;

        EXECUTE 'SELECT count(*) as countids FROM '|| quote_ident(geom_table) || ' ' || where_clause INTO countids;

        EXECUTE 'SELECT addGeometryColumn(''intersections_tmp'', ''the_geom'', '||srid||', ''POINT'', 2)';

        CREATE INDEX intersections_tmp_idx ON intersections_tmp USING GIST (the_geom);

        EXECUTE 'CREATE TABLE st_vert_tmp (gid integer, vid integer, wend char(1))';

       FOR points IN EXECUTE 'SELECT ' || quote_ident(gid_cname) || ' AS id,'
                || ' st_PointN(st_geometryn('|| quote_ident(geo_cname) ||', 1), 1) AS source,'
                || ' st_PointN(st_geometryn('|| quote_ident(geo_cname) ||', 1),'
                || ' st_NumPoints(st_geometryn('|| quote_ident(geo_cname) ||', 1))) AS target'
                || ' FROM ' || quote_ident(geom_table) || ' '
                || where_clause
                || ' ORDER BY ' || quote_ident(gid_cname)
            loop

                IF points.id%1000=0 THEN
                      RAISE NOTICE '% out of % edges processed', points.id, countids;
                END IF;

                source_id := imt_intersection_point_to_id(st_setsrid(points.source, srid), tolerance);
                target_id := imt_intersection_point_to_id(st_setsrid(points.target, srid), tolerance);

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
     select a.id, round(st_distance_sphere(st_setsrid(st_makepoint(-90.51791, 14.61327), 4326), a.the_geom)::numeric,1) as dist, list(c.name),
            c.r_ac5, c.r_ac4, c.r_ac3, c.r_ac2, c.r_ac1
      from intersections_tmp a, st_vert_tmp b, streets c
     where a.id=b.vid and b.gid=c.gid and a.dcnt>1 and c.name is not null
       and st_expand(st_setsrid(st_makepoint(-90.51791, 14.61327), 4326), 0.0013) && a.the_geom
     group by a.id, dist, c.r_ac5, c.r_ac4, c.r_ac3, c.r_ac2, c.r_ac1
     order by dist, a.id;


*/

END;
$BODY$
  LANGUAGE plpgsql VOLATILE STRICT
  COST 100;
ALTER FUNCTION imt_make_intersections(character varying, double precision, character varying, character varying, text) OWNER TO postgres;

-------------------------------------------------------------------------------------------------------------

select populate_geometry_columns();
SELECT srid FROM geometry_columns WHERE f_table_name='streets';

select imt_make_intersections('streets', 0.000001, 'the_geom', 'gid', ' name is not null ');
-- Total query runtime: 1787649 ms.
-- Total query runtime: 25612307 ms. pl_na3
-- Total query runtime: 24205815 ms. tiger2013_rgeo
-- Total query runtime: 19709056 ms. pl_na3 ca update
-- Total query runtime: 18239013 ms. tiger2013 & canada 2014
-- Total query runtime: 17301240 ms. tiger2014
-- Total query runtime: 18454355 ms. us_canada_rgeo_2014
-- Total query runtime: 17034024 ms. tiger2015_rgeo
-- Total query runtime: 18315596 ms. tiger2015_rgeo & geobase canada 2015
-- Total query runtime: 04:55:14409 hours. tiger2016_rgeo
-- Total query runtime: 05:01:18003 hours. tiger2017_rgeo
-- Total query runtime: 05:07:18029 hours. tiger2017_rgeo fixed
-- Total query runtime: 04:47:14407 hours. tiger2017_rgeo fixed encoding
-- Total query runtime: 01:48 minutes pl_ecuador5
-- Total query runtime: 05:14 minutes pl_switzerland
-- Total query runtime: 01:08 minutes pl_uruguay3
-- Successfully run. Total query runtime: 4 hr 49 min.  tiger2018_rgeo pg11
-- Successfully run. Total query runtime: 5 hr 4 min. pl_na6
-- 8069731 edges processed Time: 3880244.620 ms (01:04:40.245) inegi 

analyze;
-- Query returned successfully with no result in 33683 ms.
-- Query returned successfully with no result in 48653 ms.
-- Query returned successfully with no result in 280299 ms. pl_na3 ca update
-- Query returned successfully with no result in 57660 ms. tiger2014
-- Query returned successfully with no result in 21487 ms. us_canada_rgeo_2014
-- Query returned successfully with no result in 8101 ms. tiger2015_rgeo
-- Query returned successfully with no result in 8.3 secs. tiger2016_rgeo
-- Query returned successfully with no result in 4.8 secs. tiger2017_rgeo
-- Query returned successfully in 6 secs 715 msec. tiger2018_rgeo pg11
-- Time: 118856.282 ms (01:58.856) inegi

update intersections_tmp as a set
          cnt  = (select count(*) from st_vert_tmp b where b.vid=a.id),
          dcnt = (select count(distinct name) from st_vert_tmp b, streets c where b.gid=c.gid and b.vid=a.id and c.name is not null);
-- Query returned successfully: 2470983 rows affected, 265548 ms execution time.
-- Query returned successfully: 30745288 rows affected, 7030406 ms execution time.  pl_na3
-- Query returned successfully: 27043736 rows affected, 5838199 ms execution time. tiger2013_rgeo
-- Query returned successfully: 31186666 rows affected, 7320075 ms execution time. pl_na3 ca update
-- Query returned successfully: 28564703 rows affected, 5002531 ms execution time. tiger2013 & canada2014 rgeo
-- Query returned successfully: 27079244 rows affected, 5626841 ms execution time. tiger2014
-- Query returned successfully: 28600211 rows affected, 5849221 ms execution time. us_canada_rgeo_2014
-- Query returned successfully: 27104278 rows affected, 3936197 ms execution time. tiger2015_rgeo
-- Query returned successfully: 27017543 rows affected, 01:02:3655 hours execution time. tiger2016_rgeo
-- Query returned successfully: 27105606 rows affected, 52:50 minutes execution time. tiger2017_rgeo
-- Query returned successfully: 27105606 rows affected, 56:25 minutes execution time. tiger2017_rgeo fixed
-- Query returned successfully: 27105606 rows affected, 54:11 minutes execution time. tiger2017_rgeo fixed encoding
-- Query returned successfully: 201803 rows affected, 18.6 secs execution time. pl_ecuador5
-- Query returned successfully: 655539 rows affected, 01:03 minutes execution time. pl_switzerland
-- Query returned successfully in 1 hr 12 min.: UPDATE 26844620 tiger2018_rgeo pg11
-- UPDATE 29371504 Query returned successfully in 1 hr 36 min. pl_na6
-- UPDATE 6053906 Time: 785730.418 ms (13:05.730) inegi

create schema rawdata;
alter function imt_make_intersections(character varying, double precision, character varying, character varying, text) set schema rawdata;
-- alter function rawdata.imt_make_intersections(character varying, double precision, character varying, character varying, text) set schema data;


-- CREATE INDEX streets_gid_idx  ON streets  USING btree  (gid);

CREATE INDEX streets_gidx  ON streets  USING gist  (the_geom);
-- Query returned successfully with no result in 115298 ms.
-- Query returned successfully with no result in 1947764 ms.
-- Query returned successfully with no result in 2851196 ms. tiger2014
--   inegi

ALTER TABLE streets CLUSTER ON streets_gidx;

CREATE INDEX streets_link_id_idx  ON streets  USING btree  (link_id);
-- Query returned successfully with no result in 14592 ms.
-- Query returned successfully with no result in 509165 ms.
-- Query returned successfully with no result in 526027 ms. tiger2014
-- Query returned successfully with no result in 578385 ms. tiger2015_rgeo
-- Query returned successfully with no result in 03:54 minutes. tiger2016_rgeo
-- Query returned successfully with no result in 02:56 minutes. tiger2017_rgeo
-- Query returned successfully with no result in 03:23 minutes. tiger2017_rgeo fixed
-- Query returned successfully with no result in 03:34 minutes. tiger2017_rgeo fixed encoding
-- Query returned successfully in 1 min 7 secs. tiger2018_rgeo
-- Time: 16247.774 ms (00:16.248) inegi

-- vacuum analyze streets;               -- Query returned successfully with no result in 135379 ms.
-- vacuum analyze intersections_tmp;     -- Query returned successfully with no result in 97090 ms.
-- vacuum analyze st_vert_tmp;           -- Query returned successfully with no result in 38703 ms.

vacuum analyze verbose;
-- Query returned successfully with no result in 2519845 ms.  pl_na3 ca update
-- Query returned successfully with no result in 429455 ms. tiger2014
-- Query returned successfully with no result in 171472 ms. tiger2015_rgeo
-- Query returned successfully with no result in 05:01 minutes. tiger2016_rgeo
-- Query returned successfully with no result in 01:07 minutes. tiger2017_rgeo
-- Time: 348667.979 ms (05:48.668)  inegi

select * from imt_reverse_geocode_flds(-74, 42);
select * from imt_rgeo_intersections_flds(-74, 42);
select * from imt_reverse_geocode_flds(-75, 45);
select * from imt_rgeo_intersections_flds(-75, 45);
select * from imt_reverse_geocode_flds(-71.2577232, 42.901641);
select * from imt_rgeo_intersections_flds(-71.2577232, 42.901641);
select * from imt_reverse_geocode_flds(-97.095630,18.847630);
select * from imt_reverse_geocode_flds(-99.1497957330831,19.3750727445655);

