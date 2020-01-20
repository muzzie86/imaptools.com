select * from imt_dd_test(500, 50, 40.0, -120.0);
select * from imt_dd_test(5, 5, 40.0, -120.0);  -- (90s, 140s), 78s, 146s
select * from imt_getFarmDD(27, 40, -120);

select gid as id, source, target, cost from streets where the_geom && '0103000020E61000000100000005000000C7F0D8CF62085EC0A50F5D50DFEC4340C7F0D8CF62085EC0BBB4E1B034164440E660360186F55DC0BBB4E1B034164440E660360186F55DC0A50F5D50DFEC4340C7F0D8CF62085EC0A50F5D50DFEC4340'::geometry;
select count(*) from streets where the_geom && '0103000020E61000000100000005000000C7F0D8CF62085EC0A50F5D50DFEC4340C7F0D8CF62085EC0BBB4E1B034164440E660360186F55DC0BBB4E1B034164440E660360186F55DC0A50F5D50DFEC4340C7F0D8CF62085EC0A50F5D50DFEC4340'::geometry;
-- 1879

select * from pgr_kdijkstracost(
                 'select gid as id, source, target, cost from streets where the_geom && ''0103000020E610000001000000050000007EE71725E8075EC00853944BE3EF43407EE71725E8075EC0E89FE06245174440A22AA6D24FF45DC0E89FE06245174440A22AA6D24FF45DC00853944BE3EF43407EE71725E8075EC00853944BE3EF4340''::geometry',
                 19959348,
                 '{3271949,3268730,19959296,19959193,3281998}'::integer[], false, false);
                 

-- Function: imt_dd_test(integer, double precision, double precision, double precision)

-- DROP FUNCTION imt_dd_test(integer, double precision, double precision, double precision);

CREATE OR REPLACE FUNCTION imt_dd_test(IN cnt integer, IN radius double precision, IN lat double precision, IN lon double precision, OUT id integer, OUT dist double precision)
  RETURNS SETOF record AS
$BODY$
declare
    fx float8 := lon;
    fy float8 := lat;
    rad float8 := radius / 69.047; -- convert miles to degrees
    r record;

begin
    if cnt < 1 then
        raise exception 'CNT must be greater than 0';
    end if;

    raise notice 'Adding % random markets in % mi radius of lat: %, lon: %', cnt, radius, fy, fx;
    for r in select foo.id, random()*rad*2.0-rad+fx as x, random()*rad*2.0-rad+fy as y from (
                 select generate_series(1,cnt) as id ) as foo loop
        if imt_addMarket(27, r.id, r.y, r.x) < 0 then
            raise notice 'imt_addMarket(27, %, %, %) failed', r.id, r.y, r.x;
        end if;
    end loop;

    raise notice 'Calling imt_getFarmDD(27, %, %)', fy, fx;
    return query select * from imt_getFarmDD(27, fy, fx);
end;
$BODY$
  LANGUAGE plpgsql VOLATILE STRICT
  COST 100
  ROWS 1000;
ALTER FUNCTION imt_dd_test(integer, double precision, double precision, double precision)
  OWNER TO postgres;
