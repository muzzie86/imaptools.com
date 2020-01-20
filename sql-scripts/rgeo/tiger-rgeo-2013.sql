set search_path to data, public;

create index streets_gidx on data.streets using gist (the_geom);
-- Query returned successfully with no result in 1695047 ms.

alter table streets cluster on streets_gidx;
vacuum analyze;


-----------------------------------------------------------------------------------------------------
-- imaptools reverse geocoder
-----------------------------------------------------------------------------------------------------

/*
 -- this is for reference, it is create by the loaf-rgeo script
 
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
  l_ac5 character varying(35),
  l_ac4 character varying(35),
  l_ac3 character varying(35),
  l_ac2 character varying(35),
  l_ac1 character varying(35),
  r_ac5 character varying(35),
  r_ac4 character varying(35),
  r_ac3 character varying(35),
  r_ac2 character varying(35),
  r_ac1 character varying(35),
  l_postcode character varying(11),
  r_postcode character varying(11),
  mtfcc integer,
  passflg character varying(1),
  divroad character varying(1),
  the_geom geometry(MultiLineString,4326),
  CONSTRAINT streets_pkey PRIMARY KEY (gid)
)
WITH (
  OIDS=FALSE
);

*/
---------------------------------------------------------------------------------------------------------------


CREATE OR REPLACE FUNCTION data.max(a numeric, b numeric)
  RETURNS numeric AS
$BODY$
begin
  return case when a>b then a else b end;
end;
$BODY$
  LANGUAGE plpgsql IMMUTABLE;


CREATE OR REPLACE FUNCTION comma_cat(text[], text)
  RETURNS text[] AS
$BODY$
  SELECT
    CASE WHEN $1 @> ARRAY[$2] THEN $1
    ELSE $1 || $2
  END
$BODY$
  LANGUAGE 'sql' VOLATILE;

CREATE OR REPLACE FUNCTION comma_finish(text[])
  RETURNS text AS
$BODY$
    SELECT array_to_string($1, ', ')
$BODY$
  LANGUAGE 'sql' VOLATILE
  COST 100;

DROP AGGREGATE IF EXISTS list (text);
CREATE AGGREGATE list (BASETYPE = text, SFUNC = comma_cat, STYPE = text[], FINALFUNC = comma_finish, INITCOND = '{NULL, NULL}');


-- Function: imt_intersection_point_to_id(geometry, double precision)

-- DROP FUNCTION imt_intersection_point_to_id(geometry, double precision);

CREATE OR REPLACE FUNCTION imt_intersection_point_to_id(point geometry, tolerance double precision)
  RETURNS integer AS
$BODY$
DECLARE
        row record;
        point_id int;
BEGIN
        LOOP
                SELECT INTO row id, the_geom FROM intersections_tmp WHERE
                   st_dwithin(point, the_geom, tolerance);

                point_id := row.id;

                IF NOT FOUND THEN
                        INSERT INTO intersections_tmp (the_geom) VALUES (point);
                ELSE
                        EXIT;
                END IF;
        END LOOP;
        RETURN point_id;
END;
$BODY$
  LANGUAGE plpgsql VOLATILE STRICT
  COST 100;
ALTER FUNCTION imt_intersection_point_to_id(geometry, double precision) OWNER TO postgres;


DROP TYPE IF EXISTS rgeo_result_flds2 CASCADE;
CREATE TYPE rgeo_result_flds2 AS
    (linkid int,
     housenum text,
     street text,
     city    text,
     state   text,
     country text,
     postcode text,
     mtfcc integer,
     distance float,
     the_geom geometry
     );


DROP TYPE IF EXISTS rgeo_result_flds CASCADE;
CREATE TYPE rgeo_result_flds AS
    (address text,
     village text,
     city    text,
     county  text,
     state   text,
     country text,
     postcode text,
     distance float,
     linkid int);

DROP TYPE IF EXISTS  rgeo_result CASCADE;
CREATE TYPE rgeo_result AS
    (address text,
     distance float,
     linkid int);

-- Function: imt_rgeo_intersections(double precision, double precision, double precision)

-- DROP FUNCTION imt_rgeo_intersections(double precision, double precision, double precision);

CREATE OR REPLACE FUNCTION imt_rgeo_intersections(x double precision, y double precision, srad double precision)
  RETURNS rgeo_result AS
$BODY$

DECLARE
    rrow rgeo_result;

BEGIN
    select into rrow address||':'||array_to_string(ARRAY[village,city,county,state,country,postcode], ', '), distance, /* speed, maxspeed, */ linkid
 from imt_rgeo_intersections_flds(x, y, srad);
    return rrow;
END;
$BODY$
  LANGUAGE plpgsql STABLE
  COST 100;
ALTER FUNCTION imt_rgeo_intersections(double precision, double precision, double precision)
  OWNER TO postgres;

-- Function: imt_rgeo_intersections(geometry, double precision)

-- DROP FUNCTION imt_rgeo_intersections(geometry, double precision);

CREATE OR REPLACE FUNCTION imt_rgeo_intersections(g geometry, srad double precision)
  RETURNS rgeo_result AS
$BODY$

DECLARE
    rrow rgeo_result;

BEGIN
    select into rrow address||':'||array_to_string(ARRAY[village,city,county,state,country,postcode], ', '), distance, /* speed, maxspeed, */ linkid
 from imt_rgeo_intersections_flds(g, srad);
    return rrow;
END;
$BODY$
  LANGUAGE plpgsql STABLE
  COST 100;
ALTER FUNCTION imt_rgeo_intersections(geometry, double precision)
  OWNER TO postgres;

-- Function: imt_rgeo_intersections(geometry)

-- DROP FUNCTION imt_rgeo_intersections(geometry);

CREATE OR REPLACE FUNCTION imt_rgeo_intersections(geometry)
  RETURNS rgeo_result AS
$BODY$

DECLARE
    g ALIAS FOR $1;
    rrow rgeo_result;

BEGIN
    select into rrow address||':'||array_to_string(ARRAY[village,city,county,state,country,postcode], ', '), distance, /* speed, maxspeed, */ linkid
 from imt_rgeo_intersections_flds(g, 30.0::double precision);
    return rrow;
END;
$BODY$
  LANGUAGE plpgsql STABLE
  COST 100;
ALTER FUNCTION imt_rgeo_intersections(geometry)
  OWNER TO postgres;

-- Function: imt_rgeo_intersections(double precision, double precision)

-- DROP FUNCTION imt_rgeo_intersections(double precision, double precision);

CREATE OR REPLACE FUNCTION imt_rgeo_intersections(x double precision, y double precision)
  RETURNS rgeo_result AS
$BODY$

DECLARE
    rrow rgeo_result;

BEGIN
    select into rrow address||':'||array_to_string(ARRAY[village,city,county,state,country,postcode], ', '), distance, /* speed, maxspeed, */ linkid
 from imt_rgeo_intersections_flds(x, y, 30.0::double precision);
    return rrow;
END;
$BODY$
  LANGUAGE plpgsql STABLE
  COST 100;
ALTER FUNCTION imt_rgeo_intersections(double precision, double precision)
  OWNER TO postgres;


-- Function: imt_rgeo_intersections_flds(geometry)

-- DROP FUNCTION imt_rgeo_intersections_flds(geometry);

CREATE OR REPLACE FUNCTION imt_rgeo_intersections_flds(pnt geometry)
  RETURNS rgeo_result_flds AS
$BODY$
DECLARE
  r record;
BEGIN

  r := imt_rgeo_intersections_flds(pnt, 30.0::double precision);
  return r;

END
$BODY$
  LANGUAGE plpgsql STABLE
  COST 100;
ALTER FUNCTION imt_rgeo_intersections_flds(geometry)
  OWNER TO postgres;

-- Function: imt_rgeo_intersections_flds(double precision, double precision)

-- DROP FUNCTION imt_rgeo_intersections_flds(double precision, double precision);

CREATE OR REPLACE FUNCTION imt_rgeo_intersections_flds(x double precision, y double precision)
  RETURNS rgeo_result_flds AS
$BODY$

DECLARE
    rrow RECORD;

BEGIN
    rrow = imt_rgeo_intersections_flds(st_SetSRID(st_MakePoint(x, y), 4326), 30.0::double precision);
    return rrow;
END;
$BODY$
  LANGUAGE plpgsql STABLE
  COST 100;
ALTER FUNCTION imt_rgeo_intersections_flds(double precision, double precision)
  OWNER TO postgres;

-- Function: imt_rgeo_intersections_flds(geometry, double precision)

-- DROP FUNCTION imt_rgeo_intersections_flds(geometry, double precision);

CREATE OR REPLACE FUNCTION imt_rgeo_intersections_flds(pnt geometry, srad double precision)
  RETURNS rgeo_result_flds AS
$BODY$
/*
 * ROW(street & street, village, city, county, state, country, postcode, distance_meters, id) = rgeo(point);
 * ROW(street & street, village, city, county, state, country, postcode, distance_meters, id) = rgeo(long, lat);
 * 
 * This function performs reverse geocoding on that iMaptools intersection table
 * data. It returns a rgeo_result record for the closest intersection(s), or
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
    mdistance FLOAT;
    rr RECORD;
    pct FLOAT;
    rightvar BOOLEAN;
    ret rgeo_result_flds;
    mspeed integer;
    sraddd double precision;

BEGIN
    IF GeometryType(pnt) != 'POINT' THEN
        RETURN NULL;
    END IF;

    radius := 0.0013;
    LOOP
       select into rr a.id, round(st_distance_sphere(pnt, a.the_geom)::numeric,1) as dist, list(c.name) as name,
              c.r_ac5, c.r_ac4, c.r_ac3, c.r_ac2, c.r_ac1, c.r_postcode, a.the_geom
         from intersections_tmp a, st_vert_tmp b, streets c
        where a.id=b.vid and b.gid=c.gid and a.dcnt>1 and c.name is not null
              and st_dwithin(pnt, a.the_geom, radius)
        group by a.id, dist, c.r_ac5, c.r_ac4, c.r_ac3, c.r_ac2, c.r_ac1, c.r_postcode, a.the_geom
        order by dist, a.id, length(list(c.name)) desc limit 1;
        IF FOUND THEN
--raise notice 'rr: %', rr;

            ret.village  := rr.r_ac5;
            ret.city     := rr.r_ac4;
            ret.county   := rr.r_ac3;
            ret.state    := rr.r_ac2;
            ret.country  := rr.r_ac1;
            ret.postcode := rr.r_postcode;
            ret.address  := rr.name;
            ret.distance := rr.dist;
            ret.linkid   := rr.id;

/*
            -- get the max speed at the intersection
            sraddd := coalesce(srad, 30.0) / 111120.0; -- meter to degree conversion
            select max(max(speed, ave_speed)) into mspeed from streets
             where st_dwithin(rr.the_geom, the_geom, sraddd);

            ret.speed    := mspeed;
            ret.maxspeed := mspeed;
*/
            
            RETURN ret;
        END IF;
        radius := radius * 2.0;
        IF radius > 0.5 THEN
            RETURN NULL;
        END IF;
    END LOOP;
END
$BODY$
  LANGUAGE plpgsql STABLE
  COST 100;
ALTER FUNCTION imt_rgeo_intersections_flds(geometry, double precision)
  OWNER TO postgres;

-- Function: imt_rgeo_intersections_flds(double precision, double precision, double precision)

-- DROP FUNCTION imt_rgeo_intersections_flds(double precision, double precision, double precision);

CREATE OR REPLACE FUNCTION imt_rgeo_intersections_flds(x double precision, y double precision, srad double precision)
  RETURNS rgeo_result_flds AS
$BODY$

DECLARE
    rrow RECORD;

BEGIN
    rrow = imt_rgeo_intersections_flds(st_SetSRID(st_MakePoint(x, y), 4326), srad);
    return rrow;
END;
$BODY$
  LANGUAGE plpgsql STABLE
  COST 100;
ALTER FUNCTION imt_rgeo_intersections_flds(double precision, double precision, double precision)
  OWNER TO postgres;

-----------------------------------------------------------------------------------------------------------------

-- Function: imt_reverse_geocode(double precision, double precision)

-- DROP FUNCTION imt_reverse_geocode(double precision, double precision);

CREATE OR REPLACE FUNCTION imt_reverse_geocode(x double precision, y double precision)
  RETURNS rgeo_result AS
$BODY$

DECLARE
    rrow RECORD;

BEGIN
    select into rrow address||':'||array_to_string(ARRAY[village,city,county,state,country,postcode], ', '), distance, /* speed, maxspeed, */ linkid
 from imt_reverse_geocode_flds(st_SetSRID(st_MakePoint(x, y), 4326));
    return rrow;
END;
$BODY$
  LANGUAGE plpgsql VOLATILE
  COST 100;
ALTER FUNCTION imt_reverse_geocode(double precision, double precision)
  OWNER TO postgres;

-- Function: imt_reverse_geocode(geometry)

-- DROP FUNCTION imt_reverse_geocode(geometry);

CREATE OR REPLACE FUNCTION imt_reverse_geocode(geometry)
  RETURNS rgeo_result AS
$BODY$

DECLARE
    g ALIAS FOR $1;
    rrow RECORD;

BEGIN
    select into rrow address||':'||array_to_string(ARRAY[village,city,county,state,country,postcode], ', '), distance, /* speed, maxspeed, */ linkid
 from imt_reverse_geocode_flds(g);
    return rrow;
END;
$BODY$
  LANGUAGE plpgsql VOLATILE
  COST 100;
ALTER FUNCTION imt_reverse_geocode(geometry)
  OWNER TO postgres;

-- Function: imt_reverse_geocode(double precision, double precision, double precision)

-- DROP FUNCTION imt_reverse_geocode(double precision, double precision, double precision);

CREATE OR REPLACE FUNCTION imt_reverse_geocode(x double precision, y double precision, srad double precision)
  RETURNS rgeo_result AS
$BODY$

DECLARE
    rrow RECORD;

BEGIN
    select into rrow address||':'||array_to_string(ARRAY[village,city,county,state,country,postcode], ', '), distance, /* speed, maxspeed, */ linkid
 from imt_reverse_geocode_flds(st_SetSRID(st-MakePoint(x, y), 4326), srad);
    return rrow;
END;
$BODY$
  LANGUAGE plpgsql VOLATILE
  COST 100;
ALTER FUNCTION imt_reverse_geocode(double precision, double precision, double precision)
  OWNER TO postgres;

-- Function: imt_reverse_geocode(geometry, double precision)

-- DROP FUNCTION imt_reverse_geocode(geometry, double precision);

CREATE OR REPLACE FUNCTION imt_reverse_geocode(g geometry, srad double precision)
  RETURNS rgeo_result AS
$BODY$

DECLARE
    rrow RECORD;

BEGIN
    select into rrow address||':'||array_to_string(ARRAY[village,city,county,state,country,postcode], ', '), distance, /* speed, maxspeed, */ linkid
 from imt_reverse_geocode_flds(g, srad);
    return rrow;
END;
$BODY$
  LANGUAGE plpgsql VOLATILE
  COST 100;
ALTER FUNCTION imt_reverse_geocode(geometry, double precision)
  OWNER TO postgres;

-- Function: imt_reverse_geocode_flds(double precision, double precision)

-- DROP FUNCTION imt_reverse_geocode_flds(double precision, double precision);

CREATE OR REPLACE FUNCTION imt_reverse_geocode_flds(x double precision, y double precision)
  RETURNS rgeo_result_flds AS
$BODY$

DECLARE
    rrow RECORD;

BEGIN
    rrow := imt_reverse_geocode_flds(st_SetSRID(st_MakePoint(x, y), 4326), 30.0::double precision);
    return rrow;
END;
$BODY$
  LANGUAGE plpgsql STABLE
  COST 100;
ALTER FUNCTION imt_reverse_geocode_flds(double precision, double precision)
  OWNER TO postgres;

-- Function: imt_reverse_geocode_flds(geometry, double precision)

-- DROP FUNCTION imt_reverse_geocode_flds(geometry, double precision);

CREATE OR REPLACE FUNCTION imt_reverse_geocode_flds(pnt geometry, srad double precision)
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
    mdistance FLOAT;
    rr RECORD;
    geom geometry;
    pct FLOAT;
    rightvar BOOLEAN;
    ret rgeo_result_flds;
    mspeed integer;
    sraddd double precision;

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
            geom := st_GeometryN(rr.the_geom, 1);
            pct := ST_LineLocatePoint(geom, pnt);
            pnt2 := ST_LineInterpolatePoint(geom, pct);
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

            if (address='') and exists(SELECT * FROM pg_catalog.pg_tables WHERE tablename='intersections_tmp') then
                select into rr * from imt_rgeo_intersections_flds(pnt, srad);
                return rr;
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

-- Function: imt_reverse_geocode_flds(double precision, double precision, double precision)

-- DROP FUNCTION imt_reverse_geocode_flds(double precision, double precision, double precision);

CREATE OR REPLACE FUNCTION imt_reverse_geocode_flds(x double precision, y double precision, srad double precision)
  RETURNS rgeo_result_flds AS
$BODY$

DECLARE
    rrow RECORD;

BEGIN
    rrow := imt_reverse_geocode_flds(st_SetSRID(st_MakePoint(x, y), 4326), srad);
    return rrow;
END;
$BODY$
  LANGUAGE plpgsql STABLE
  COST 100;
ALTER FUNCTION imt_reverse_geocode_flds(double precision, double precision, double precision)
  OWNER TO postgres;

-- Function: imt_reverse_geocode_flds(geometry)

-- DROP FUNCTION imt_reverse_geocode_flds(geometry);

CREATE OR REPLACE FUNCTION imt_reverse_geocode_flds(pnt geometry)
  RETURNS rgeo_result_flds AS
$BODY$
DECLARE
    rrow record;

BEGIN
    rrow := imt_reverse_geocode_flds(pnt, 30.0::double precision);
    return rrow;
END;
$BODY$
  LANGUAGE plpgsql STABLE
  COST 100;
ALTER FUNCTION imt_reverse_geocode_flds(geometry)
  OWNER TO postgres;


------------------------------------------------------------------------------------------------------------------

-- Function: imt_reverse_geocode_flds2(geometry, boolean)

-- DROP FUNCTION imt_reverse_geocode_flds2(geometry, boolean);

CREATE OR REPLACE FUNCTION imt_reverse_geocode_flds2(geometry, boolean)
  RETURNS rgeo_result_flds2 AS
$BODY$
/*
 * ROW(linkid, housenum, street, city, state, country, postcode, mtfcc, distance_meters, geom) = rgeo(point, boolean);
 * 
 * This function performs reverse geocoding on that iMaptools prepared
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
    pnt ALIAS FOR $1;
    dohnum ALIAS FOR $2;
    pnt2 GEOMETRY;
    radius FLOAT;
    hnum TEXT;
    location TEXT;
    mdistance FLOAT;
    rr RECORD;
    geom geometry;
    pct FLOAT;
    rightvar BOOLEAN;
    ret rgeo_result_flds2;

BEGIN
    IF GeometryType(pnt) != 'POINT' THEN
        RETURN NULL;
    END IF;

    radius := 0.0013;
    LOOP
        SELECT * INTO rr FROM data.streets
            WHERE st_dwithin(pnt, the_geom, radius)
            ORDER BY st_distance(pnt, the_geom) ASC limit 1;
        IF FOUND THEN
--raise notice 'rr: %', rr;
            geom := st_GeometryN(rr.the_geom, 1);
            pct := ST_LineLocatePoint(geom, pnt);
            pnt2 := ST_LineInterpolatePoint(geom, pct);
            mdistance := round(st_distance_sphere(pnt, pnt2)::numeric,1);

            IF dohnum THEN
                /*
                 * we will arbitrarily assume the right side of the street
                 * unless it has no address ranges, then the left else we
                 * only return the street name if it exits.
                 */
--raise notice '%, if % and % elsif % and %', rr.link_id, rr.l_refaddr IS NOT NULL, rr.l_sqlfmt IS NOT NULL, rr.r_refaddr IS NOT NULL, rr.r_sqlfmt IS NOT NULL;
                IF rr.l_refaddr IS NOT NULL AND rr.l_sqlfmt IS NOT NULL THEN
                    /* linear interpolate the house number based on the pct */
--RAISE NOTICE 'LEFT: %, %, %', pct, TO_CHAR((rr.l_nrefaddr-rr.l_refaddr)*pct + rr.l_refaddr, rr.l_sqlnumf), (rr.l_nrefaddr-rr.l_refaddr)*pct + rr.l_refaddr;
                    hnum := REPLACE(rr.l_sqlfmt, '{}',
                        TO_CHAR((rr.l_nrefaddr-rr.l_refaddr)*pct + rr.l_refaddr,
                            rr.l_sqlnumf));
                    rightvar := false;
                ELSIF rr.r_refaddr IS NOT NULL AND rr.r_sqlfmt IS NOT NULL THEN
--RAISE NOTICE 'RIGHT: %, %', pct, TO_CHAR((rr.r_nrefaddr-rr.r_refaddr)*pct + rr.r_refaddr, rr.r_sqlnumf);
                    /* linear interpolate the house number based on the pct */
                    hnum := REPLACE(rr.r_sqlfmt, '{}',
                        TO_CHAR((rr.r_nrefaddr-rr.r_refaddr)*pct + rr.r_refaddr,
                            rr.r_sqlnumf));
                    rightvar := true;
                ELSE
                    /* no house numbers, we might still have a street name */
                    rightvar := NULL;
                END IF;
            ELSE
                hnum := NULL;
            END IF;
            
            ret.housenum := hnum;
            ret.street := rr.name;

--RAISE NOTICE 'RIGHT: %, address: %', rightvar, address;
            /* get the city, state, country postcode*/
            IF rightvar IS NULL OR rightvar THEN
                  ret.city    := COALESCE(rr.r_ac5, rr.r_ac4);
                  ret.state   := rr.r_ac2;
                  ret.country := rr.r_ac1;
                  ret.postcode := rr.r_postcode;
            END IF;
            
            IF NOT rightvar THEN
                  ret.city    := COALESCE(rr.l_ac5, rr.l_ac4);
                  ret.state   := rr.l_ac2;
                  ret.country := rr.l_ac1;
                  ret.postcode := rr.l_postcode;
            END IF;
            
            ret.distance := mdistance;
            ret.linkid := rr.link_id;
            ret.mtfcc := rr.mtfcc;
            ret.the_geom := rr.the_geom;
            RETURN ret;
        END IF;
        radius := radius * 2.0;
        IF radius > 0.5 THEN
            RETURN NULL;
        END IF;
    END LOOP;
END
$BODY$
  LANGUAGE plpgsql STABLE
  COST 100;
ALTER FUNCTION imt_reverse_geocode_flds2(geometry, boolean)
  OWNER TO postgres;


-- Function: imt_reverse_geocode_hnum(double precision, double precision)

-- DROP FUNCTION imt_reverse_geocode_hnum(double precision, double precision);

CREATE OR REPLACE FUNCTION imt_reverse_geocode_hnum(double precision, double precision)
  RETURNS rgeo_result_flds2 AS
$BODY$

DECLARE
    x ALIAS FOR $1;
    y ALIAS FOR $2;
    rrow RECORD;

BEGIN
    rrow = imt_reverse_geocode_flds2(st_SetSRID(st_MakePoint(x, y), 4326), true);
    return rrow;
END;
$BODY$
  LANGUAGE plpgsql STABLE
  COST 100;
ALTER FUNCTION imt_reverse_geocode_hnum(double precision, double precision)
  OWNER TO postgres;


-- Function: imt_reverse_geocode_nhnum(double precision, double precision)

-- DROP FUNCTION imt_reverse_geocode_nhnum(double precision, double precision);

CREATE OR REPLACE FUNCTION imt_reverse_geocode_nhnum(double precision, double precision)
  RETURNS rgeo_result_flds2 AS
$BODY$

DECLARE
    x ALIAS FOR $1;
    y ALIAS FOR $2;
    rrow RECORD;

BEGIN
    rrow = imt_reverse_geocode_flds2(st_SetSRID(st_MakePoint(x, y), 4326), false);
    return rrow;
END;
$BODY$
  LANGUAGE plpgsql STABLE
  COST 100;
ALTER FUNCTION imt_reverse_geocode_nhnum(double precision, double precision)
  OWNER TO postgres;

-------------------------------------------------------------------------------------------------------
-------------------------------------------------------------------------------------------------------


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

        EXECUTE 'SELECT count(*) as countids FROM '|| quote_ident(geom_table) INTO countids;

        EXECUTE 'SELECT addGeometryColumn(''intersections_tmp'', ''the_geom'', '||srid||', ''POINT'', 2)';

        CREATE INDEX intersections_tmp_idx ON intersections_tmp USING GIST (the_geom);

        EXECUTE 'CREATE TABLE st_vert_tmp (gid integer, vid integer, wend char(1))';

        FOR points IN EXECUTE 'SELECT ' || quote_ident(gid_cname) || ' AS id,'
                || ' st_startpoint(st_geometryn('|| quote_ident(geo_cname) ||', 1)) AS source,'
                || ' st_endpoint(st_geometryn('|| quote_ident(geo_cname) ||', 1)) AS target'
                || ' FROM ' || quote_ident(geom_table) || ' ORDER BY '
                || quote_ident(gid_cname)
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


-- Function: imt_make_intersections(character varying, double precision, character varying, character varying, text)

-- DROP FUNCTION imt_make_intersections(character varying, double precision, character varying, character varying, text);

CREATE OR REPLACE FUNCTION imt_make_intersections(geom_table character varying, tolerance double precision, geo_cname character varying, gid_cname character varying, where_str text)
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

        where_clause := coalesce(where_str, '');

        EXECUTE 'CREATE TABLE intersections_tmp (id serial, cnt integer, dcnt integer)';

        EXECUTE 'SELECT srid FROM geometry_columns WHERE f_table_name='|| quote_literal(geom_table) INTO srid;

        EXECUTE 'SELECT count(*) as countids FROM '|| quote_ident(geom_table) || ' ' || where_clause INTO countids;

        EXECUTE 'SELECT addGeometryColumn(''intersections_tmp'', ''the_geom'', '||srid||', ''POINT'', 2)';

        CREATE INDEX intersections_tmp_idx ON intersections_tmp USING GIST (the_geom);

        EXECUTE 'CREATE TABLE st_vert_tmp (gid integer, vid integer, wend char(1))';

        FOR points IN EXECUTE 'SELECT ' || quote_ident(gid_cname) || ' AS id,'
                || ' st_startpoint(st_geometryn('|| quote_ident(geo_cname) ||', 1)) AS source,'
                || ' st_endpoint(st_geometryn('|| quote_ident(geo_cname) ||', 1)) AS target'
                || ' FROM ' || quote_ident(geom_table) 
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
ALTER FUNCTION imt_make_intersections(character varying, double precision, character varying, character varying, text) OWNER TO postgres;

-----------------------------------------------------------------------------------------------------------
-----------------------------------------------------------------------------------------------------------

/*
select max(gid) as countids FROM streets;

select populate_geometry_columns();

select imt_make_intersections('streets'::text, 0.000001, 'the_geom'::text, 'gid'::text);
-- Total query runtime: 30175409 ms.

update intersections_tmp as a set
    cnt  = (select count(*) from st_vert_tmp b where b.vid=a.id),
    dcnt = (select count(distinct name) from st_vert_tmp b, streets c where b.gid=c.gid and b.vid=a.id and c.name is not null);
-- Query returned successfully: 37312802 rows affected, 5269954 ms execution time.

DROP FUNCTION imt_make_intersections(character varying, double precision, character varying, character varying);
DROP FUNCTION imt_make_intersections(character varying, double precision, character varying, character varying, text);

vacuum full verbose analyze;
-- Total query runtime: 18954116 ms.

select * from imt_reverse_geocode_flds(-74.0, 42.0);
select * from imt_reverse_geocode_flds(-73.5, 44.5);
select * from imt_reverse_geocode_flds(-85.0, 42.0);
select * from imt_rgeo_intersections_flds(-74.0, 42.0);
select * from imt_rgeo_intersections_flds(-73.5, 44.5);
select * from imt_rgeo_intersections_flds(-85.0, 42.0);


create index streets_link_id_idx on streets using btree (link_id);
-- Query returned successfully with no result in 451891 ms.


select * from imt_reverse_geocode_flds(-74.223317,41.167848) a, streets b where b.link_id=a.linkid;

select * from imt_reverse_geocode_flds(ST_SetSRID(ST_MakePoint(-122.035464,47.601621), 4326));

select address, coalesce(village,city) as city, state, postcode, distance 
  from imt_reverse_geocode_flds(ST_SetSRID(ST_MakePoint(-122.035464,47.601621), 4326));

vacuum analyze;
-- Query returned successfully with no result in 153563 ms.

*/

