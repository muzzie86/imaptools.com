select st_distance_sphere(the_geom, st_setsrid(st_makepoint(-73.619101, 45.723795), 4326)) as dist, * 
  from streets where st_dwithin(the_geom, st_setsrid(st_makepoint(-73.619101, 45.723795), 4326), .001)
  order by dist asc;

select count(*) from streets where name='Voie' and r_ac1='CA'; -- 12458
select count(*) from streets where name='Voie' and (l_refaddr is not null or r_refaddr is not null); -- 58
select count(*) from streets where name='Voie' and not (l_refaddr is not null or r_refaddr is not null); -- 12400

select * into voie from streets where name='Voie' and not (l_refaddr is not null or r_refaddr is not null);

select count(*) from voie;

delete from streets where name='Voie' and not (l_refaddr is not null or r_refaddr is not null);

vacuum analyze streets;

select * from imt_reverse_geocode_flds(-73.619101, 45.723795);
