-- select * from zipcode limit 500;
-- select pc_type, count(*) from zipcode group by pc_type;

drop table if exists placenames cascade;
create table placenames (
  gid serial not null primary key,
  state varchar(2),
  place varchar(35),
  dmeta_place varchar(4)
);
select addgeometrycolumn('data','placenames','the_geom',4326,'POINT',2);
insert into placenames (state, place, dmeta_place, the_geom)
    select state, pc_name, dmetaphone(pc_name), st_centroid(st_collect(the_geom)) as the_geom from zipcode where pc_type != 'UNIQUE ORGANIZATION' group by state, pc_name;
-- Query returned successfully: 29660 rows affected, 1432 ms execution time.

-- select * from placenames limit 500;

select state, place, null as postcode, the_geom from placenames where state=? and (place=? or dmeta_place=dmetaphone(?))
union
select state, pc_name as place, postcode, the_geom from zipcode where postcode=?;

create or replace function imt_geocode_place(instate text, inplace text, inzip text, fuzzy integer, method integer)
  RETURNS SETOF geo_place AS
$BODY$
declare
  sql text;
  
begin
  sql := 'select state, place, null as postcode, the_geom from placenames where state=? and (place=? or dmeta_place=dmetaphone(?))
union
select state, pc_name as place, postcode, the_geom from zipcode where postcode=?';

end;
$BODY$
  LANGUAGE plpgsql IMMUTABLE STRICT
  COST 5
  ROWS 100;
