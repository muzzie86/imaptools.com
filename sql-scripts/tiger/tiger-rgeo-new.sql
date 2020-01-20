-- Function: imt_reverse_geocode(double precision, double precision)

-- DROP FUNCTION imt_reverse_geocode(double precision, double precision);

CREATE OR REPLACE FUNCTION imt_reverse_geocode(double precision, double precision)
  RETURNS rgeo_result AS
$BODY$

DECLARE
    x ALIAS FOR $1;
    y ALIAS FOR $2;
    rrow RECORD;

BEGIN
    select into rrow address||':'||array_to_string(ARRAY[village,city,county,state,country,postcode], ', '), distance, linkid
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
    select into rrow address||':'||array_to_string(ARRAY[village,city,county,state,country,postcode], ', '), distance, linkid
 from imt_reverse_geocode_flds(g);
    return rrow;
END;
$BODY$
  LANGUAGE plpgsql VOLATILE
  COST 100;
ALTER FUNCTION imt_reverse_geocode(geometry)
  OWNER TO postgres;


-- Function: imt_reverse_geocode_flds(geometry)

-- DROP FUNCTION imt_reverse_geocode_flds(geometry);

CREATE OR REPLACE FUNCTION imt_reverse_geocode_flds(geometry)
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
    pnt ALIAS FOR $1;
    pnt2 GEOMETRY;
    radius FLOAT;
    address TEXT;
    location TEXT;
    mdistance FLOAT;
    rr RECORD;
    pct FLOAT;
    right BOOLEAN;
    ret rgeo_result_flds;

BEGIN
    IF GeometryType(pnt) != 'POINT' THEN
        RETURN NULL;
    END IF;

    radius := 0.0013;
    LOOP
        SELECT * INTO rr FROM data.streets
            WHERE st_expand(pnt, radius) && the_geom
            ORDER BY st_distance(pnt, the_geom) ASC limit 1;
        IF FOUND THEN
--raise notice 'rr: %', rr;
            IF GeometryType(rr.the_geom)='MULTILINESTRING' THEN
                rr.the_geom := st_GeometryN(rr.the_geom, 1);
            END IF;
            pct := st_line_locate_point(rr.the_geom, pnt);
            pnt2 := st_line_interpolate_point(rr.the_geom, pct);
            mdistance := st_distance_sphere(pnt, pnt2);

            /*
             * we will arbitrarily assume the right side of the street
             * unless it has no address ranges, then the left else we
             * only return the street name if it exits.
             */
--raise notice '%, if % and % elsif % and %', rr.link_id, rr.l_refaddr IS NOT NULL, rr.l_sqlfmt IS NOT NULL, rr.r_refaddr IS NOT NULL, rr.r_sqlfmt IS NOT NULL;
            IF rr.l_refaddr IS NOT NULL AND rr.l_sqlfmt IS NOT NULL THEN
                /* linear interpolate the house number based on the pct */
--RAISE NOTICE 'LEFT: %, %, %', pct, TO_CHAR((rr.l_nrefaddr-rr.l_refaddr)*pct + rr.l_refaddr, rr.l_sqlnumf), (rr.l_nrefaddr-rr.l_refaddr)*pct + rr.l_refaddr;
                address := REPLACE(rr.l_sqlfmt, '{}',
                    TO_CHAR((rr.l_nrefaddr-rr.l_refaddr)*pct + rr.l_refaddr,
                        rr.l_sqlnumf)) || ' ';
                right := false;
            ELSIF rr.r_refaddr IS NOT NULL AND rr.r_sqlfmt IS NOT NULL THEN
--RAISE NOTICE 'RIGHT: %, %', pct, TO_CHAR((rr.r_nrefaddr-rr.r_refaddr)*pct + rr.r_refaddr, rr.r_sqlnumf);
                /* linear interpolate the house number based on the pct */
                address := REPLACE(rr.r_sqlfmt, '{}',
                    TO_CHAR((rr.r_nrefaddr-rr.r_refaddr)*pct + rr.r_refaddr,
                        rr.r_sqlnumf)) || ' ';
                right := true;
            ELSE
                /* no house numbers, we might still have a street name */
                right := NULL;
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
                select into rr * from imt_rgeo_intersections_flds(pnt);
                return rr;
            end if;

            
--RAISE NOTICE 'RIGHT: %, address: %', right, address;
            /* get the city, state, country postcode*/
            location := '';
            IF right IS NULL OR right THEN
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
            
            IF (right IS NULL AND LENGTH(address)=0) OR NOT right THEN
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
                RETURN ret;
            END IF;
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
ALTER FUNCTION imt_reverse_geocode_flds(geometry)
  OWNER TO postgres;


-- Function: imt_reverse_geocode_flds(double precision, double precision)

-- DROP FUNCTION imt_reverse_geocode_flds(double precision, double precision);

CREATE OR REPLACE FUNCTION imt_reverse_geocode_flds(double precision, double precision)
  RETURNS rgeo_result_flds AS
$BODY$

DECLARE
    x ALIAS FOR $1;
    y ALIAS FOR $2;
    rrow RECORD;

BEGIN
    rrow = imt_reverse_geocode_flds(st_SetSRID(st_MakePoint(x, y), 4326));
    return rrow;
END;
$BODY$
  LANGUAGE plpgsql STABLE
  COST 100;
ALTER FUNCTION imt_reverse_geocode_flds(double precision, double precision)
  OWNER TO postgres;


-- Function: imt_rgeo_intersections(double precision, double precision)

-- DROP FUNCTION imt_rgeo_intersections(double precision, double precision);

CREATE OR REPLACE FUNCTION imt_rgeo_intersections(double precision, double precision)
  RETURNS rgeo_result AS
$BODY$

DECLARE
    x ALIAS FOR $1;
    y ALIAS FOR $2;
    rrow rgeo_result;

BEGIN
    select into rrow address||':'||array_to_string(ARRAY[village,city,county,state,country,postcode], ', '), distance, linkid
 from imt_rgeo_intersections_flds(x, y);
    return rrow;
END;
$BODY$
  LANGUAGE plpgsql STABLE
  COST 100;
ALTER FUNCTION imt_rgeo_intersections(double precision, double precision)
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
    select into rrow address||':'||array_to_string(ARRAY[village,city,county,state,country,postcode], ', '), distance, linkid
 from imt_rgeo_intersections_flds(g);
    return rrow;
END;
$BODY$
  LANGUAGE plpgsql STABLE
  COST 100;
ALTER FUNCTION imt_rgeo_intersections(geometry)
  OWNER TO postgres;


-- Function: imt_rgeo_intersections_flds(double precision, double precision)

-- DROP FUNCTION imt_rgeo_intersections_flds(double precision, double precision);

CREATE OR REPLACE FUNCTION imt_rgeo_intersections_flds(double precision, double precision)
  RETURNS rgeo_result_flds AS
$BODY$

DECLARE
    x ALIAS FOR $1;
    y ALIAS FOR $2;
    rrow RECORD;

BEGIN
    rrow = imt_rgeo_intersections_flds(st_SetSRID(st_MakePoint(x, y), 4326));
    return rrow;
END;
$BODY$
  LANGUAGE plpgsql STABLE
  COST 100;
ALTER FUNCTION imt_rgeo_intersections_flds(double precision, double precision)
  OWNER TO postgres;


-- Function: imt_rgeo_intersections_flds(geometry)

-- DROP FUNCTION imt_rgeo_intersections_flds(geometry);

CREATE OR REPLACE FUNCTION imt_rgeo_intersections_flds(geometry)
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
    pnt ALIAS FOR $1;
    pnt2 GEOMETRY;
    radius FLOAT;
    address TEXT;
    location TEXT;
    mdistance FLOAT;
    rr RECORD;
    pct FLOAT;
    right BOOLEAN;
    ret rgeo_result_flds;

BEGIN
    IF GeometryType(pnt) != 'POINT' THEN
        RETURN NULL;
    END IF;

    radius := 0.0013;
    LOOP
       select into rr a.id, round(st_distance_sphere(pnt, a.the_geom)::numeric,1) as dist, list(c.name) as name,
              c.r_ac5, c.r_ac4, c.r_ac3, c.r_ac2, c.r_ac1, c.r_postcode
         from intersections_tmp a, st_vert_tmp b, streets c
        where a.id=b.vid and b.gid=c.gid and a.dcnt>1 and c.name is not null
              and st_expand(pnt, radius) && a.the_geom
        group by a.id, dist, c.r_ac5, c.r_ac4, c.r_ac3, c.r_ac2, c.r_ac1, c.r_postcode
        order by dist, a.id limit 1;
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
            ret.linkid := rr.id;
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
ALTER FUNCTION imt_rgeo_intersections_flds(geometry)
  OWNER TO postgres;

select * from imt_reverse_geocode_flds(-72.5,42);