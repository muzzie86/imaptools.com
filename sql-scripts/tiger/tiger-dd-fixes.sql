-- Function: imt_getfarmdd(integer, double precision, double precision)

-- DROP FUNCTION imt_getfarmdd(integer, double precision, double precision);

CREATE OR REPLACE FUNCTION imt_getfarmdd(farm_id integer, farm_lat double precision, farm_lon double precision)
  RETURNS SETOF imt_farmdd AS
$BODY$
declare
    mkttable text;
    sql text;
    nid integer;
    ext record;
    rec record;
    rec2 imt_farmdd;

begin
    mkttable := 'mkttab_' || farm_id::text;
    
    if not pgr_isColumnInTable(mkttable, 'mktid') then
        raise exception 'No markets have been added for id: %', farm_id;
    end if;
    
    nid := imt_xyGetNode(farm_lon, farm_lat);

    execute 'select st_expand(st_envelope(st_collect(the_geom)), 0.1) as bb
               from ' || quote_ident(mkttable) || ' a, vertices_tmp b
              where a.nid=b.id or b.id=$1' into ext using nid;

    execute 'select array_agg(nid) as targets from (
                 select distinct nid from ' || quote_ident(mkttable) || ') as foo' into rec;

    sql := 'select gid as id, source, target, cost from streets where the_geom && ''' || ext.bb::text || '''::geometry';
    --raise notice 'sql: %', sql;
    --raise notice 'nid: %, targets: %', nid, rec.targets;

    for rec2 in execute 'select b.mktid as id, cost as dist
                from pgr_kdijkstracost( $1, $2, $3, false, false) a,
                    ' || mkttable || ' b where a.id2=b.nid ' using sql, nid, rec.targets
        loop
	    return next rec2;
    end loop;

    -- clean up remove tmp table
    execute 'drop table ' || quote_ident(mkttable) ||' cascade';
    return;
end;
$BODY$
  LANGUAGE plpgsql VOLATILE STRICT
  COST 100
  ROWS 500;
ALTER FUNCTION imt_getfarmdd(integer, double precision, double precision)
  OWNER TO postgres;
