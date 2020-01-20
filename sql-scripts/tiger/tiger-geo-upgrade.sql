-- Function: imt_compute_xy(integer, text, double precision)

-- DROP FUNCTION imt_compute_xy(integer, text, double precision);

CREATE OR REPLACE FUNCTION imt_compute_xy(
    IN id integer,
    IN num text,
    IN aoff double precision,
    OUT x double precision,
    OUT y double precision)
  RETURNS record AS
$BODY$
declare
  rec record;
  pnt geometry;
  d1 double precision;
  d2 double precision;
  our_side integer;
  geom geometry;
  pos double precision;
  p1 geometry;
  p2 geometry;
  dx double precision;
  dy double precision;
  px double precision;
  py double precision;
  ux double precision;
  uy double precision;
  len double precision;
  
begin

  -- num is null for intersections and we get intersection locations from another table
  if num is null or num='0' then
    x := NULL;
    y := NULL;
    return;
  end if;
  
  -- side: 1=R, 2=L
  select refaddr, nrefaddr, side, st_geometryn(the_geom,1) as the_geom into rec from streets where gid=id;
  if found then
    if rec.refaddr<rec.nrefaddr then
      d2 := to_number(num,'999999999') - rec.refaddr;
      d1 := rec.nrefaddr - rec.refaddr;
    else
      d2 := rec.refaddr - to_number(num,'999999999');
      d1 := rec.refaddr - rec.nrefaddr;
    end if;
    if d1 = 0.0 then -- check and avoid divide by zero
      pos := 0.5;    -- the start and end are the same, then pick the midpoint of the segment. 
    else
      pos := d2/d1;
    end if;

--raise notice 'num: %, ref: %, nref: %, d2: %, d1: %, pos: %',num, rec.refaddr, rec.nrefaddr, d2, d1, pos;

    /* compute the offset from the center line if requested. aoff=0.0 is the trivial case */
    if aoff = 0.0 then 
        pnt := ST_LineInterpolatePoint(rec.the_geom, pos);
        x := st_x(pnt);
        y := st_y(pnt);
    else
        /* compute the offset to the right or left for this address
           * if the parity does not match then we should probably reverse the R/L side of the offset
           * if the location it at the end of the line, ie: pos=1.0, then we reverse the line and the sides to compute the offset
           * algorithm is:
           *   trim the line to start at pos
           *   get the unit vector from pnt1 to pos2
           *   use vector algebra to compute the offset
        */
        our_side := rec.side;
        geom := rec.the_geom;
        if pos = 1.0 then
	    geom := st_reverse(geom);                              -- reverse the line
	    our_side := case when rec.side = 1 then 2 else 1 end;  -- reverse the side
	    pos := 0.0;                                            -- reverse the ends
	end if;
	geom := ST_LineSubstring(geom, pos, 1.0);  -- trim the line so pos is the start now
	p1 := st_pointn(geom, 1);
	p2 := st_pointn(geom, 2);
	dx := st_x(p2) - st_x(p1);
	dy := st_y(p2) - st_y(p1);
	len := sqrt(dx*dx + dy*dy);     -- this could be zero if coincident points
	if len = 0.0 then
	    x := NULL;
	    y := NULL;
	    return;
	end if;
	ux := dx/len;  -- unit x
	uy := dy/len;  -- unit y
	if our_side = 1 then            -- right side
	    px := uy * aoff / 111120.0;
	    py := -ux * aoff / 111120.0;
	else                            -- left side
	    px := -uy * aoff / 111120.0;
	    py := ux * aoff / 111120.0;
	end if;
	x := st_x(p1) + px;
	y := st_y(p1) + py;
    end if;
  else
    x := NULL;
    y := NULL;
  end if;
end;
$BODY$
  LANGUAGE plpgsql IMMUTABLE
  COST 100;
ALTER FUNCTION imt_compute_xy(integer, text, double precision)
  OWNER TO postgres;


-- Function: imt_geo_add_where_clauses(stdaddr, integer)

-- DROP FUNCTION imt_geo_add_where_clauses(stdaddr, integer);

CREATE OR REPLACE FUNCTION imt_geo_add_where_clauses(
    inp stdaddr,
    plan integer)
  RETURNS text AS
$BODY$
/*
the query planner might do something like:
0. query for exact terms input, ignoring building, extra, box, and unit
1. try 1 with postcode only or state only, if no postcode
2. try 1 with city, state only
3. then loosen up on house_num, predir, sufdir, qual, pretype, suftype
4. try 3 with postcode only or state only, if no postcode
5. try 3 with city, state only
--- only if the request want fuzzy searching
6. then try searching on metaphone name
7. try 6 with postcode only or state only, if no postcode
8. try 6 with city state only
*/
declare
  ret text := '';
begin
  if inp.name is not null and plan < 6 then
    ret := ret || ' and a.name='||quote_literal(inp.name);
  elsif inp.name is not null and plan >= 6 then
    ret := ret || ' and a.name_dmeta=coalesce(dmetaphone('||quote_literal(inp.name)||'),'||quote_literal(inp.name)||')';
  end if;
  if inp.house_num is not null then
      ret := ret || ' and ('||to_number(quote_literal(inp.house_num),'999999999')||' between b.refaddr and b.nrefaddr' ||
                      ' or '||to_number(quote_literal(inp.house_num),'999999999')||' between b.nrefaddr and b.refaddr )';
  end if;
  if plan < 3 then
    if inp.predir is not null then
      ret := ret || ' and case when a.predir is not null then a.predir='||quote_literal(inp.predir)||' else true end';
    end if;
    if inp.qual is not null then
      ret := ret || ' and case when a.qual is not null then a.qual='||quote_literal(inp.qual)||' else true end';
    end if;
    if inp.pretype is not null then
      ret := ret || ' and case when a.pretype is not null then a.pretype='||quote_literal(inp.pretype)||' else true end';
    end if;
    if inp.suftype is not null then
      ret := ret || ' and case when a.suftype is not null then a.suftype='||quote_literal(inp.suftype)||' else true end';
    end if;
    if inp.sufdir is not null then
      ret := ret || ' and case when a.sufdir is not null then a.sufdir='||quote_literal(inp.sufdir)||' else true end';
    end if;
  end if;
  if inp.city is not null and plan in (0,2,3,5,6,8) then
    ret := ret || ' and a.city='||quote_literal(inp.city);
  end if;
  if (inp.state is not null) and plan in (0,2,3,5,6,8) or (inp.postcode is null and inp.state is not null and plan in (1,4,7)) then
    ret := ret || ' and a.state='||quote_literal(inp.state);
  end if;
  if inp.country is not null then
    ret := ret || ' and case when a.country is not null then a.country='||quote_literal(inp.country)||' else true end';
  end if;
  if inp.postcode is not null and plan in (0,1,3,4,6,7) then
    ret := ret || ' and a.postcode='||quote_literal(inp.postcode);
  end if;

  return ret;
end;
$BODY$
  LANGUAGE plpgsql IMMUTABLE
  COST 100;
ALTER FUNCTION imt_geo_add_where_clauses(stdaddr, integer)
  OWNER TO postgres;


-- Function: imt_geo_planner(stdaddr)

-- DROP FUNCTION imt_geo_planner(stdaddr);

CREATE OR REPLACE FUNCTION imt_geo_planner(inp stdaddr)
  RETURNS SETOF text AS
$BODY$
declare
  sql text;
  i integer;
  rec record;
  
begin
  <<planner>>
  for i in 0..8 loop 
    sql := 'select a.*, '||i::text||' as plan from stdstreets a, streets b where a.id=b.gid ' || imt_geo_add_where_clauses(inp, i);
    return next sql;
    /*
    for rec in execute sql loop
      -- do something
    end loop;
    */
  end loop planner;
  return;
end;
$BODY$
  LANGUAGE plpgsql IMMUTABLE
  COST 1
  ROWS 10;
ALTER FUNCTION imt_geo_planner(stdaddr)
  OWNER TO postgres;


-- Function: imt_geo_planner2(stdaddr, integer)

-- DROP FUNCTION imt_geo_planner2(stdaddr, integer);

CREATE OR REPLACE FUNCTION imt_geo_planner2(
    inp stdaddr,
    opt integer)
  RETURNS text AS
$BODY$
declare
  sql text;
  i integer;
  rec record;
  mm integer;
  
begin
  mm := case when opt>0 then 8 else 5 end;
  sql := '';
  <<planner>>
  for i in 0..mm loop 
    sql := sql 
      || case when i>0 then '
    union
    ' else '' end
      || 'select a.*, '
                 ||i::text||' as plan, '
                 ||coalesce(to_number(quote_literal(inp.house_num),'999999999')||'::integer%2=b.refaddr::integer%2','null')||' as parity'
      || ' from stdstreets a, streets b'
      || ' where a.id=b.gid ' 
      || imt_geo_add_where_clauses(inp, i);
  end loop planner;

--  sql := 'select distinct on (id) foo.* from (' || sql || ') as foo order by id';
  -- added sub-select to enforce an order by id, plan so all servers return the same results
  sql := 'select distinct on (id) * from (select * from (' || sql || ') as bar order by id, plan) as foo order by id';

  return sql;
end;
$BODY$
  LANGUAGE plpgsql IMMUTABLE
  COST 100;
ALTER FUNCTION imt_geo_planner2(stdaddr, integer)
  OWNER TO postgres;


-- Function: imt_geo_score(text, stdaddr, double precision)

-- DROP FUNCTION imt_geo_score(text, stdaddr, double precision);

CREATE OR REPLACE FUNCTION imt_geo_score(
    sql text,
    sin stdaddr,
    aoff double precision)
  RETURNS SETOF geo_result AS
$BODY$
declare
  rec geo_result;
  score double precision;
  w1 double precision := 0.05;
  w2 double precision := 0.25;
  w3 double precision := 0.125;
  len integer;
  xy record;
  
begin

  for rec in execute sql loop
    select * into xy from imt_compute_xy(rec.id, sin.house_num, aoff);
    rec.x := xy.x;
    rec.y := xy.y;
    score := case when coalesce(rec.parity,true) then 0.0 else 0.1 end;
    len := max(length(coalesce(rec.predir,'')), length(coalesce(sin.predir,'')));
    score := score + case when len>0 then levenshtein(coalesce(rec.predir,''), coalesce(sin.predir,''))::double precision / len::double precision * w1 else 0.0 end;
    len := max(length(coalesce(rec.sufdir,'')), length(coalesce(sin.sufdir,'')));
    score := score + case when len>0 then levenshtein(coalesce(rec.sufdir,''), coalesce(sin.sufdir,''))::double precision / len::double precision * w1 else 0.0 end;
    len := max(length(coalesce(rec.pretype,'')), length(coalesce(sin.pretype,'')));
    score := score + case when len>0 then levenshtein(coalesce(rec.pretype,''), coalesce(sin.pretype,''))::double precision / len::double precision * w1 else 0.0 end;
    len := max(length(coalesce(rec.suftype,'')), length(coalesce(sin.suftype,'')));
    score := score + case when len>0 then levenshtein(coalesce(rec.suftype,''), coalesce(sin.suftype,''))::double precision / len::double precision * w1 else 0.0 end;
    len := max(length(coalesce(rec.qual,'')), length(coalesce(sin.qual,'')));
    score := score + case when len>0 then levenshtein(coalesce(rec.qual,''), coalesce(sin.qual,''))::double precision / len::double precision * w1 else 0.0 end;
    len := max(length(coalesce(rec.name,'')), length(coalesce(sin.name,'')));
    score := score + case when len>0 then levenshtein(coalesce(rec.name,''), coalesce(sin.name,''))::double precision / len::double precision * w2 else 0.0 end;
    len := max(length(coalesce(rec.city,'')), length(coalesce(sin.city,'')));
    score := score + case when len>0 then levenshtein(coalesce(rec.city,''), coalesce(sin.city,''))::double precision / len::double precision * w3 else 0.0 end;
    len := max(length(coalesce(rec.state,'')), length(coalesce(sin.state,'')));
    score := score + case when len>0 then levenshtein(coalesce(rec.state,''), coalesce(sin.state,''))::double precision / len::double precision * w3 else 0.0 end;
    len := max(length(coalesce(rec.postcode,'')), length(coalesce(sin.postcode,'')));
    score := score + case when len>0 then levenshtein(coalesce(rec.postcode,''), coalesce(sin.postcode,''))::double precision / len::double precision * w3 else 0.0 end;
    len := max(length(coalesce(rec.country,'')), length(coalesce(sin.country,'')));
    score := score + case when len>0 then levenshtein(coalesce(rec.country,''), coalesce(sin.country,''))::double precision / len::double precision * w3 else 0.0 end;
    rec.score := 1.0 - score;
    return next rec;
  end loop;
  return;
end;
$BODY$
  LANGUAGE plpgsql IMMUTABLE STRICT
  COST 1
  ROWS 100;
ALTER FUNCTION imt_geo_score(text, stdaddr, double precision)
  OWNER TO postgres;


-- Function: imt_geo_score2(text, stdaddr, double precision)

-- DROP FUNCTION imt_geo_score2(text, stdaddr, double precision);

CREATE OR REPLACE FUNCTION imt_geo_score2(
    sql text,
    sin stdaddr,
    aoff double precision)
  RETURNS SETOF geo_result AS
$BODY$
-- score based on similarity function. This does not discriminate as well as levenshtein function
declare
  rec geo_result;
  score double precision;
  w1 double precision := 0.05;
  w2 double precision := 0.25;
  w3 double precision := 0.125;
  len integer;
  xy record;
  
begin

  for rec in execute sql loop
    select * into xy from imt_compute_xy(rec.id, sin.house_num, aoff);
    rec.x := xy.x;
    rec.y := xy.y;
    score := case when coalesce(rec.parity,true) then 0.0 else 0.1 end;
    len := max(length(coalesce(rec.predir,'')), length(coalesce(sin.predir,'')));
    score := score + case when len>0 then similarity(coalesce(rec.predir,''), coalesce(sin.predir,''))::double precision / len::double precision * w1 else 0.0 end;
    len := max(length(coalesce(rec.sufdir,'')), length(coalesce(sin.sufdir,'')));
    score := score + case when len>0 then similarity(coalesce(rec.sufdir,''), coalesce(sin.sufdir,''))::double precision / len::double precision * w1 else 0.0 end;
    len := max(length(coalesce(rec.pretype,'')), length(coalesce(sin.pretype,'')));
    score := score + case when len>0 then similarity(coalesce(rec.pretype,''), coalesce(sin.pretype,''))::double precision / len::double precision * w1 else 0.0 end;
    len := max(length(coalesce(rec.suftype,'')), length(coalesce(sin.suftype,'')));
    score := score + case when len>0 then similarity(coalesce(rec.suftype,''), coalesce(sin.suftype,''))::double precision / len::double precision * w1 else 0.0 end;
    len := max(length(coalesce(rec.qual,'')), length(coalesce(sin.qual,'')));
    score := score + case when len>0 then similarity(coalesce(rec.qual,''), coalesce(sin.qual,''))::double precision / len::double precision * w1 else 0.0 end;
    len := max(length(coalesce(rec.name,'')), length(coalesce(sin.name,'')));
    score := score + case when len>0 then similarity(coalesce(rec.name,''), coalesce(sin.name,''))::double precision / len::double precision * w2 else 0.0 end;
    len := max(length(coalesce(rec.city,'')), length(coalesce(sin.city,'')));
    score := score + case when len>0 then similarity(coalesce(rec.city,''), coalesce(sin.city,''))::double precision / len::double precision * w3 else 0.0 end;
    len := max(length(coalesce(rec.state,'')), length(coalesce(sin.state,'')));
    score := score + case when len>0 then similarity(coalesce(rec.state,''), coalesce(sin.state,''))::double precision / len::double precision * w3 else 0.0 end;
    len := max(length(coalesce(rec.postcode,'')), length(coalesce(sin.postcode,'')));
    score := score + case when len>0 then similarity(coalesce(rec.postcode,''), coalesce(sin.postcode,''))::double precision / len::double precision * w3 else 0.0 end;
    len := max(length(coalesce(rec.country,'')), length(coalesce(sin.country,'')));
    score := score + case when len>0 then similarity(coalesce(rec.country,''), coalesce(sin.country,''))::double precision / len::double precision * w3 else 0.0 end;
    rec.score := score;
    return next rec;
  end loop;
  return;
end;
$BODY$
  LANGUAGE plpgsql IMMUTABLE STRICT
  COST 1
  ROWS 100;
ALTER FUNCTION imt_geo_score2(text, stdaddr, double precision)
  OWNER TO postgres;


-- Function: imt_geo_test(integer)

-- DROP FUNCTION imt_geo_test(integer);

CREATE OR REPLACE FUNCTION imt_geo_test(myid integer)
  RETURNS SETOF text AS
$BODY$
declare
  rec stdaddr;
  
begin
  select * into rec from stdstreets where id=myid;
  return query select * from geo_planner(rec);
end;
$BODY$
  LANGUAGE plpgsql IMMUTABLE
  COST 100
  ROWS 1000;
ALTER FUNCTION imt_geo_test(integer)
  OWNER TO postgres;


-- Function: imt_geo_test2(integer)

-- DROP FUNCTION imt_geo_test2(integer);

CREATE OR REPLACE FUNCTION imt_geo_test2(myid integer)
  RETURNS SETOF text AS
$BODY$
declare
  rec stdaddr;
  
begin
  select * into rec from stdstreets where id=myid;
  return query select * from imt_geo_planner2(rec, 1);
end;
$BODY$
  LANGUAGE plpgsql IMMUTABLE
  COST 100
  ROWS 1000;
ALTER FUNCTION imt_geo_test2(integer)
  OWNER TO postgres;


-- Function: imt_geocode_intersection(text, text, text, integer, integer)

-- DROP FUNCTION imt_geocode_intersection(text, text, text, integer, integer);

CREATE OR REPLACE FUNCTION imt_geocode_intersection(
    micro1_in text,
    micro2_in text,
    macro_in text,
    fuzzy integer,
    method integer)
  RETURNS SETOF geo_intersection AS
$BODY$
declare
  srec stdaddr;
  sql text;
  sql2 text;
  asql text[];
  sfld stdaddr[];
  rec geo_intersection;
  cnt integer := 0;

begin

  -- add house num for standardizer
  srec := standardize_address('lex', 'gaz', 'rules', trim(both from '10 '||micro1_in)::text, trim(both from macro_in)::text);
  -- kill house num now because we don't want it for intersections
  srec.house_num := null;
--raise notice 'st1: srec: %', srec;

  -- make sure we got a result that we can use
  if coalesce(srec.house_num,srec.predir,srec.qual,srec.pretype,srec.name,srec.suftype,srec.sufdir,srec.city,srec.state,srec.postcode,srec.country) is not null then
      select * into sql2 from imt_geo_planner2(srec, fuzzy);
      if found then
          asql[cnt] := sql2;
--raise notice 'st1: asql: %', sql2;
          sfld[cnt] := srec;
          cnt := cnt + 1;
      else
          return;
      end if;
  else
      return;
  end if;

  -- add house num for standardizer
  srec := standardize_address('lex', 'gaz', 'rules', trim(both from '10 '||micro2_in)::text, trim(both from macro_in)::text);
  -- kill house num now because we don't want it for intersections
  srec.house_num := null;
--raise notice 'st2: srec: %', srec;

  -- make sure we got a result that we can use
  if coalesce(srec.house_num,srec.predir,srec.qual,srec.pretype,srec.name,srec.suftype,srec.sufdir,srec.city,srec.state,srec.postcode,srec.country) is not null then
      select * into sql2 from imt_geo_planner2(srec, fuzzy);
      if found then
          asql[cnt] := sql2;
--raise notice 'st2: asql: %', sql2;
          sfld[cnt] := srec;
          cnt := cnt + 1;
      else
          return;
      end if;
  else
      return;
  end if;

  if (sfld[0].state is null and sfld[0].postcode is null) or (sfld[0].state is null and sfld[0].postcode is null) then
    return;
  end if;

  for rec in select * from (
               select distinct on (x, y)
                 a.id as id1,
                 c.id as id2,
                 f.tlid as id1a,
                 g.tlid as id2a,
                 array_to_string(ARRAY[f.predirabrv, f.prequalabr, f.pretypabrv, f.name, f.suftypabrv, f.sufdirabrv, f.prequalabr], ' ') as street1,
                 array_to_string(ARRAY[g.predirabrv, g.prequalabr, g.pretypabrv, g.name, g.suftypabrv, g.sufdirabrv, g.prequalabr], ' ') as street2,
                 coalesce(f.ac5,f.ac4,f.ac3) as placename,
                 f.usps as placename_usps,
                 f.ac2 as statename,
                 f.ac1 as countrycode,
                 f.postcode as zipcode,
                 a.plan,
                 (a.score+c.score)/2.0 as geocodematchcode,
                 st_x(e.the_geom) as x,
                 st_y(e.the_geom) as y
               from 
                   (select * from imt_geo_score(asql[0], sfld[0], 0.0)) as a, st_vert_tmp b, streets f,
                   (select * from imt_geo_score(asql[1], sfld[1], 0.0)) as c, st_vert_tmp d, streets g,
                   intersections_tmp e
              where
                   a.id=b.gid and b.gid=f.gid and
                   c.id=d.gid and d.gid=g.gid and
                   b.vid=d.vid and
                   b.vid=e.id and
                   array_to_string(ARRAY[f.predirabrv, f.prequalabr, f.pretypabrv, f.name, f.suftypabrv, f.sufdirabrv, f.prequalabr], ' ') != array_to_string(ARRAY[g.predirabrv, g.prequalabr, g.pretypabrv, g.name, g.suftypabrv, g.sufdirabrv, g.prequalabr], ' ')
              order by x, y ) as foo
            order by geocodematchcode desc
  loop
        return next rec;
  end loop;
  return;
end;
$BODY$
  LANGUAGE plpgsql IMMUTABLE STRICT
  COST 5
  ROWS 100;
ALTER FUNCTION imt_geocode_intersection(text, text, text, integer, integer)
  OWNER TO postgres;


-- Function: imt_geocoder(text, text, integer, double precision, integer)

-- DROP FUNCTION imt_geocoder(text, text, integer, double precision, integer);

CREATE OR REPLACE FUNCTION imt_geocoder(
    micro_in text,
    macro_in text,
    fuzzy integer,
    aoffset double precision,
    method integer)
  RETURNS SETOF geo_result2 AS
$BODY$
declare
  sfld stdaddr;

begin
  select * into sfld 
    from standardize_address('lex', 'gaz', 'rules', 
           trim(both from micro_in)::text, 
           trim(both from macro_in)::text);
--raise notice 'sfld: %', sfld;
  if sfld.state is null and sfld.postcode is null then
    return;
  end if;

  return query select * from imt_geocoder(sfld, fuzzy, aoffset, method);
end;
$BODY$
  LANGUAGE plpgsql IMMUTABLE STRICT
  COST 5
  ROWS 100;
ALTER FUNCTION imt_geocoder(text, text, integer, double precision, integer)
  OWNER TO postgres;


-- Function: imt_geocoder(stdaddr, integer, double precision, integer)

-- DROP FUNCTION imt_geocoder(stdaddr, integer, double precision, integer);

CREATE OR REPLACE FUNCTION imt_geocoder(
    sfld stdaddr,
    fuzzy integer,
    aoffset double precision,
    method integer)
  RETURNS SETOF geo_result2 AS
$BODY$
declare
  sql text;
  rec geo_result2;
  cnt integer := 0;

begin

--raise notice 'sfld: %', sfld;
  if sfld.state is null and sfld.postcode is null then
    return;
  end if;

  -- make sure we got an address that we can use
  if coalesce(sfld.house_num,sfld.predir,sfld.qual,sfld.pretype,sfld.name,sfld.suftype,sfld.sufdir) is not null and
     coalesce(sfld.city,sfld.state,sfld.postcode /*,sfld.country */) is not null then
    select * into sql from imt_geo_planner2(sfld, fuzzy);
--raise notice '%', sql;
    if found then
      for rec in select 
                             0::int4 as rid,
                             a.id,
                             b.tlid as id1,
                             b.side as id2,
                             b.tfid as id3,
                             sfld.house_num as completeaddressnumber,
                             b.predirabrv as predirectional,
                             b.prequalabr as premodifier,
                             b.sufqualabr as postmodifier,
                             b.pretypabrv as pretype,
                             b.name as streetname,
                             b.suftypabrv as posttype,
                             b.sufdirabrv as postdirectional,
                             coalesce(b.ac5,b.ac4,b.ac3) as placename,
                             b.usps as placename_usps,
                             b.ac2 as statename,
                             b.ac1 as countrycode,
                             b.postcode as zipcode,
                             sfld.building,
                             sfld.unit,
                             plan,
                             parity,
                             a.score as geocodematchcode,
                             a.x,
                             a.y
                   from (select * from imt_geo_score(sql, sfld, aoffset)) as a, streets b where a.id=b.gid order by a.score desc, plan asc loop
        cnt := cnt + 1;
        rec.rid := cnt;
        return next rec;
      end loop;
    end if;
  end if;
  return;
end;
$BODY$
  LANGUAGE plpgsql IMMUTABLE STRICT
  COST 5
  ROWS 100;
ALTER FUNCTION imt_geocoder(stdaddr, integer, double precision, integer)
  OWNER TO postgres;


-- Function: imt_geocoder_best(text, integer, double precision, integer)

-- DROP FUNCTION imt_geocoder_best(text, integer, double precision, integer);

CREATE OR REPLACE FUNCTION imt_geocoder_best(
    sql text,
    fuzzy integer,
    aoffset double precision,
    method integer)
  RETURNS SETOF geo_result2 AS
$BODY$
declare
  sfld stdaddr;
  rec geo_result2;
  t0 timestamp with time zone;

begin
  for sfld in execute sql loop
    continue when sfld.state is null and sfld.postcode is null;
    t0 := clock_timestamp();
    for rec in select * from imt_geocoder(sfld, fuzzy, aoffset, method) loop
      return next rec;
      exit;
    end loop;
    if method < 0 then
      raise notice 'id=%, time=%', sfld.id, clock_timestamp()-t0;
    end if;
  end loop;

  return;
end;
$BODY$
  LANGUAGE plpgsql IMMUTABLE STRICT
  COST 5
  ROWS 100;
ALTER FUNCTION imt_geocoder_best(text, integer, double precision, integer)
  OWNER TO postgres;


-- Function: imt_intersection_point_to_id(geometry, double precision)

-- DROP FUNCTION imt_intersection_point_to_id(geometry, double precision);

CREATE OR REPLACE FUNCTION imt_intersection_point_to_id(
    point geometry,
    tolerance double precision)
  RETURNS integer AS
$BODY$
DECLARE
        row record;
        point_id int;
BEGIN
        SELECT id
          FROM intersections_tmp
         WHERE st_dwithin(point, the_geom, tolerance)
          INTO row;

        IF NOT FOUND THEN
                INSERT INTO intersections_tmp (the_geom) VALUES (point) RETURNING id INTO row;
        END IF;

        RETURN row.id;
END;
$BODY$
  LANGUAGE plpgsql VOLATILE STRICT
  COST 100;
ALTER FUNCTION imt_intersection_point_to_id(geometry, double precision)
  OWNER TO postgres;


-- Function: imt_make_intersections(character varying, double precision, character varying, character varying)

-- DROP FUNCTION imt_make_intersections(character varying, double precision, character varying, character varying);

CREATE OR REPLACE FUNCTION imt_make_intersections(
    geom_table character varying,
    tolerance double precision,
    geo_cname character varying,
    gid_cname character varying)
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

        EXECUTE 'SELECT max('||quote_ident(gid_cname)||') as countids FROM '|| quote_ident(geom_table) INTO countids;

        EXECUTE 'SELECT addGeometryColumn(''intersections_tmp'', ''the_geom'', '||srid||', ''POINT'', 2)';

        CREATE INDEX intersections_tmp_idx ON intersections_tmp USING GIST (the_geom);

        EXECUTE 'CREATE TABLE st_vert_tmp (gid integer, vid integer, wend char(1))';

        FOR points IN EXECUTE 'SELECT ' || quote_ident(gid_cname) || ' AS id,'
                || ' ST_PointN(ST_GeometryN('|| quote_ident(geo_cname) ||', 1), 1) AS source,'
                || ' ST_PointN(ST_GeometryN('|| quote_ident(geo_cname) ||', 1), ST_NumPoints(ST_GeometryN('
                || quote_ident(geo_cname) ||', 1))) AS target'
                || ' FROM ' || quote_ident(geom_table) || ' ORDER BY '
                || quote_ident(gid_cname)
            loop

                IF points.id%1000=0 THEN
                      RAISE NOTICE '% out of % edges processed', points.id, countids;
                END IF;

                source_id := imt_intersection_point_to_id(st_setsrid(points.source, srid), tolerance);
                target_id := imt_intersection_point_to_id(st_setsrid(points.target, srid), tolerance);

                IF source_id is null OR target_id is null THEN
                      RAISE EXCEPTION 'source_id: %, target_id: % for gid: %', source_id, target_id, points.id;
                END IF;

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
ALTER FUNCTION imt_make_intersections(character varying, double precision, character varying, character varying)
  OWNER TO postgres;


-- Function: max(numeric, numeric)

-- DROP FUNCTION max(numeric, numeric);

CREATE OR REPLACE FUNCTION max(
    a numeric,
    b numeric)
  RETURNS numeric AS
$BODY$
begin
  return case when a>b then a else b end;
end;
$BODY$
  LANGUAGE plpgsql IMMUTABLE
  COST 100;
ALTER FUNCTION max(numeric, numeric)
  OWNER TO postgres;
