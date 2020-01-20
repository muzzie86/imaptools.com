select * from geonames limit 500;

select concise_code, concise_term_en, count(*) from geonames group by concise_code, concise_term_en order by concise_code;

select addgeometrycolumn('geonames', 'the_geom', 4326, 'POINT', 2);
update geonames set the_geom=st_setsrid(st_makepoint(londec, latdec), 4326);
update geonames set prov=case when region_code=10 then 'NL'
                              when region_code=11 then 'PE'
                              when region_code=12 then 'NS'
                              when region_code=13 then 'NB'
                              when region_code=24 then 'QC'
                              when region_code=35 then 'ON'
                              when region_code=46 then 'MB'
                              when region_code=47 then 'SK'
                              when region_code=48 then 'AB'
                              when region_code=59 then 'BC'
                              when region_code=60 then 'YT'
                              when region_code=61 then 'NT'
                              when region_code=62 then 'NU'
                              when region_code=72 then 'UF'
                              when region_code=125 then 'CA' END;

select gid, geoname, cgndb_key, concise_code as type, concise_term_en as type_nm, prov, the_geom
  from geonames
 where concise_code in ('BCH','CAPE','CAVE','CLF','CRAT','GEOG','GLAC','ISL','MTN','PLN','VALL');
 where concise_code in ('BAY','CHAN','FALL','HYDR','LAKE','RAP','RIV','RIVF','SEA','SEAF','SEAU','SHL');
 where concise_code in ('AIR','CAMP','IR','MAR','MIL','RES','ROAD','SITE');
 where concise_code in ('CITY','HAM','MUN1','MUN2','PROV','TERR','TOWN','UNP','VILG');
 where concise_code in ('FOR','PARK','RECR','VEGL');