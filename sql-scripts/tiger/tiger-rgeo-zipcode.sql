select * from imt_reverse_geocode_flds(-88.3106, 39.5223);

select * from streets where link_id=5454162;

select count(*) from streets where l_postcode is null and r_postcode is null;
-- 24,006,682 in 290 sec

select count(*) from streets where l_postcode is null or r_postcode is null;
-- 29,185,139 in 290 sec

select * from zcta5 where st_dwithin(st_setsrid(st_makepoint(-88.3106, 39.5223), 4326), the_geom, 0.0);

select b.*
  from zcta5 a, zipcode b 
 where a.zcta5ce10=b.zip and st_dwithin(st_setsrid(st_makepoint(-88.7039,39.5223), 4326), the_geom, 0.0);

select st_distance_sphere(the_geom, st_setsrid(st_makepoint(-88.7039,39.5223), 4326))
  from zcta5 where zcta5ce10='61938'; -- 19972.0170433429

select st_astext(st_centroid(the_geom))
  from zcta5 where zcta5ce10='61938';  -- "POINT(-88.3742568488428 39.4838390544647)"

select st_distance_sphere(the_geom, st_setsrid(st_makepoint(-88.7039,39.5223), 4326))
  from zcta5 where zcta5ce10='61951'; -- 0.0

select st_astext(st_centroid(the_geom))
  from zcta5 where zcta5ce10='61951';  -- "POINT(-88.5867894952255 39.5939068402963)"

