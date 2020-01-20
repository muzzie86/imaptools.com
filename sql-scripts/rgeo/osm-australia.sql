select * from osm_roads limit 100;
select * from osm_polygon limit 100;
select admin_level, count(*) as cnt from osm_polygon group by admin_level; -- 10, 9, 8, null
/*
"";205942
"10";147
"9";6
"8";2
*/
select place, count(*) as cnt from osm_polygon where place is not null group by place; -- 86
/*
"farm";1
"island";62
"town";1
"village";3
"islet";13
"hamlet";4
"locality";2
*/
select * from osm_polygon where place is not null;
select * from osm_line where admin_level='4' ;
select admin_level, count(*) as cnt from osm_line group by admin_level;
/*
"";831455
"2";60
"4";323
"5";1
"6";811
"7";24
"8";258
"10";13358
"Tinara Walk";1
"parish";1
*/
select st_distance_sphere(setsrid(makepoint(144.9633,-37.814),4326), way) as dist, * 
from osm_roads where st_dwithin(setsrid(makepoint(144.9633,-37.814),4326), way, 0.002) 
order by dist asc;

select st_distance_sphere(setsrid(makepoint(144.9633,-37.814),4326), way) as dist, * 
from osm_point where st_dwithin(setsrid(makepoint(144.9633,-37.814),4326), way, 0.002) 
order by dist asc;

select st_distance_sphere(setsrid(makepoint(144.9633,-37.814),4326), way) as dist, * 
from osm_line where st_dwithin(setsrid(makepoint(144.9633,-37.814),4326), way, 0.002) 
order by dist asc;

select st_distance_sphere(setsrid(makepoint(144.9633,-37.814),4326), way) as dist, * 
from osm_polygon where st_dwithin(setsrid(makepoint(144.9633,-37.814),4326), way, 0.002) 
order by dist asc;

select distinct geometrytype(geometry) from country_naturalearthdata;
select populate_geometry_columns();
update worldboundaries set the_geom4326=st_transform(the_geom, 4326);

select * from osm_polygon where name ilike '%Dandenong%';


