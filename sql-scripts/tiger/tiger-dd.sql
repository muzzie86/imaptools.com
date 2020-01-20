/*
drop extension pgrouting;
create extension pgrouting;
set search_path to data, public, pg_catalog;
*/

create or replace function imt_xyGetNode(x float8, y float8)
    returns integer as
$body$
declare
    radius float8 := 0.013;
    pnt geometry;
    rec record;

begin
    loop
        pnt := st_setsrid(st_makepoint(x,y),4326);
        select gid, source, target, the_geom into rec from streets where st_dwithin(the_geom, pnt, radius);
        if FOUND then
            if st_line_locate_point(st_geometryn(rec.the_geom,1), pnt) < 0.5 then
                return rec.source;
            else
                return rec.target;
            end if;
        end if;

        if radius > 1.0 then
            exit;
        end if;

        radius := radius * 2.0;
    end loop;

    raise exception 'Could not find a node within 69 mile radius of lat: %, lon: %', y, x;
end;
$body$
language plpgsql strict cost 1;

-----------------------------------------------------------------------------------------------

create or replace function imt_addMarket(farm_id integer, mkt_id integer, lat float8, lon float8)
    returns integer as
$body$
declare
    mkttable text;
    id integer;

begin
    mkttable := 'mkttab_' || farm_id::text;
    
    if not pgr_isColumnInTable(mkttable, 'mktid') then
        execute 'drop table if exists ' || mkttable || ' cascade';
        execute 'create table ' || mkttable || ' (
            mktid integer not null primary key,
            lat float8,
            lon float8,
            nid integer
            )';
    end if;

    -- map lat long to nearest edge
    id := imt_xyGetNode(lon, lat);

    begin
        -- insert record
        execute 'insert into ' || quote_ident(mkttable) || ' values ($1, $2, $3, $4)'
          using mkt_id, lat, lon, id;
    exception
        when unique_violation then
            -- if primary key violation return 1 for failure
            return 1;
    end;

    -- return 0 for success
    return 0;
end;
$body$
language plpgsql strict volatile;

-------------------------------------------------------------------------------------------------------

create or replace function imt_getFarmDD(farm_id integer, farm_lat float8, farm_lon float8, OUT id integer, OUT dist float8)
    returns setof record as
$body$
declare
    mkttable text;
    sql text;
    nid integer;
    ext record;
    rec record;

begin
    mkttable := 'mkttab_' || farm_id::text;
    
    if not pgr_isColumnInTable(mkttable, 'mktid') then
        raise exception 'No markets have been added for id: %', farm_id;
    end if;
    
    nid := imt_xyGetNode(farm_lon, farm_lat);

    execute 'select st_expand(st_envelope(st_collect(pnt)), 0.1) as bb from (
        select st_setsrid(st_makepoint(lon, lat), 4326) as pnt from ' || quote_ident(mkttable) || '
        union all
        select st_setsrid(st_makepoint($1, $2), 4326) as pnt
        ) as foo' into ext using farm_lon, farm_lat;

    execute 'select array_agg(nid) as targets from ' || quote_ident(mkttable) into rec;
    execute 'drop table ' || quote_ident(mkttable) ||' cascade';

    sql := 'select gid as id, source, target, cost from streets where the_geom && ''' || ext.bb::text || '''::geometry';
    raise notice 'sql: %', sql;

    return query select id2 as id, cost as dist from pgr_kdijkstracost(sql, nid, rec.targets, false, false);
end;
$body$
language plpgsql strict volatile cost 100 rows 500;

-------------------------------------------------------------------------------------------------------

create or replace function imt_listMktTables()
    returns setof text as
$body$
declare

begin
    -- this assumes all "mkttab_*" tables will be in the "data" schema
    return query SELECT c.relname::text as name
                   FROM pg_catalog.pg_class c
                        LEFT JOIN pg_catalog.pg_namespace n ON n.oid = c.relnamespace
                  WHERE c.relkind = 'r' 
                        AND n.nspname = 'data'
                        AND c.relname ~ '^(mkttab_.*)$'
                        AND pg_catalog.pg_table_is_visible(c.oid)
                  ORDER BY 1;
end;
$body$
language plpgsql;

---------------------------------------------------------------------------------------------------------

create or replace function imt_purgeMktTables()
    returns integer as
$body$
declare
    tab text;
    cnt integer := 0;

begin
    -- this assumes all "mkttab_*" tables will be in the "data" schema
    for tab in select * from imt_listMktTables() loop
        execute 'drop table if exists data.' || quote_ident(tab) || ' cascade';
        cnt := cnt + 1;
    end loop;

    return cnt;
end;
$body$
language plpgsql;

---------------------------------------------------------------------------------------------------------

create or replace function imt_dd_test(OUT id integer, OUT dist float8)
    returns setof record as
$body$
declare
    fx float8 := -102.0;
    fy float8 :=   40.0;
    r record;

begin
    raise notice 'Adding 500 random markets in 200 mi radius of lat: %, lon: %', fy, fx;
    for r in select foo.id, random()*5.8-2.9+fx as x, random()*5.8-2.9+fy as y from (
                 select generate_series(1,500) as id ) as foo loop
        if imt_addMarket(27, r.id, r.y, r.x) < 0 then
            raise notice 'imt_addMarket(27, %, %, %) failed', r.id, r.y, r.x;
        end if;
    end loop;

    raise notice 'Calling imt_getFarmDD(27, %, %)', fy, fx;
    return query select * from imt_getFarmDD(27, fy, fx);
end;
$body$
language plpgsql;

alter table data.streets
    drop column l_sqlnumf, 
    drop column l_sqlfmt, 
    drop column l_cformat, 
    drop column r_refaddr,
    drop column r_nrefaddr,
    drop column r_sqlnumf,
    drop column r_sqlfmt,
    drop column r_cformat,
    drop column r_ac5,
    drop column r_ac4,
    drop column r_ac3,
    drop column r_ac2,
    drop column r_ac1,
    drop column r_postcode,
    add column source integer,
    add column target integer,
    add column cost float8;


update streets set cost = st_length2d_spheroid(the_geom, 'SPHEROID["GRS_1980",6378137,298.257222101]'::spheroid);
-- Query returned successfully: 45115474 rows affected, 48592320 ms execution time.

select pgr_createTopology('streets', 0.000001, 'the_geom', 'gid');
-- Total query runtime: 246433723 ms.

vacuum full verbose;
-- Query returned successfully with no result in 8635847 ms.

create index streets_gidx on streets using gist (the_geom);
-- Query returned successfully with no result in 4817307 ms.

cluster streets using streets_gidx;
-- Query returned successfully with no result in 10466161 ms.

analyze streets;
-- Query returned successfully with no result in 96237 ms.

select * from imt_dd_test();
-- Total query runtime: 335565 ms.  500 rows retrieved.

select * from imt_listMktTables();
select * from imt_purgeMktTables();
