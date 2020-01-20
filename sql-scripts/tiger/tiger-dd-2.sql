-- Function: imt_dd_test()

-- DROP FUNCTION imt_dd_test();

CREATE OR REPLACE FUNCTION imt_dd_test(cnt integer, radius float8, lat float8, lon float8, OUT id integer, OUT dist double precision)
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

select * from imt_dd_test(50, 10, 40.0, -120.0);
-- Total query runtime: 98037 ms.
-- Total query runtime: 124028 ms.

select * from imt_dd_test(500, 10, 40.0, -120.0);
-- Total query runtime: 74276 ms.

select * from imt_dd_test(500, 50, 40.0, -120.0);
-- Total query runtime: 145039 ms.

select * from imt_dd_test(50, 10, 40.0, -120.0);
-- 915 mb, Total query runtime: 91008 ms.

select * from imt_dd_test(500, 10, 40.0, -120.0);
-- 1055 mb, Total query runtime: 129569 ms.

select * from imt_dd_test(500, 50, 40.0, -120.0);
-- ?? mb, Total query runtime: 152373 ms.

select * from imt_dd_test();
-- Total query runtime: 273051 ms.