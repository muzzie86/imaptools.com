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
    mdistance FLOAT;
    rr RECORD;
    rr2 RECORD;
    pct FLOAT;
    rightvar BOOLEAN;
    ret rgeo_result_flds;
    mspeed integer;
    sraddd double precision;
    geom geometry;
    excl integer;

BEGIN
    IF GeometryType(pnt) != 'POINT' THEN
        RETURN NULL;
    END IF;

    if exists(SELECT * FROM pg_catalog.pg_tables WHERE tablename='exclusion_areas') then
	select into excl id from exclusion_areas a where st_dwithin(pnt, a.geom, 0.0) limit 1;
	if FOUND then
	    return NULL;
	end if;
    end if;

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

            if (address='') and exists(SELECT * FROM pg_catalog.pg_tables WHERE tablename='intersections_tmp') then
                select into rr2 * from imt_rgeo_intersections_flds(pnt, srad);
                if coalesce(rr2.address,'') != '' then
		    return rr2;
		end if;
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
