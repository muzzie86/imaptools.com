select * from rawdata.roadseg limit 500;

select l_adddirfg, count(*) from rawdata.roadseg group by l_adddirfg;
/*
"Not Applicable";638053
"Same Direction";1765660
"Opposite Direction";12881
*/
select r_adddirfg, count(*) from rawdata.roadseg group by r_adddirfg;
/*
"Not Applicable";636532
"Same Direction";1767959
"Opposite Direction";12103
*/

select datasetnam, count(*) from rawdata.roadseg group by datasetnam;
/*
"Prince Edward Island";17359         PE
"Alberta";403701                     AB
"Ontario";651430                     ON
"Québec";448856                      QC
"Yukon Territory";6424               YT
"Nunavut";4242                       NU
"British Columbia";258180            BC
"Saskatchewan";289968                SK
"Manitoba";110604                    MB
"Northwest Territories";6792         NT
"New Brunswick";64463                NB
"Nova Scotia";110091                 NS
"Newfoundland and Labrador";44484    NL
*/

select roadclass, count(*) from rawdata.roadseg group by roadclass;
/*
"Arterial";190374
"Local / Strata";55769
"Local / Street";1242288
"Expressway / Highway";102791
"Alleyway / Lane";39805
"Winter";362
"Service Lane";6771
"Freeway";20505
"Local / Unknown";15893
"Rapid Transit";339
"Ramp";35334
"Resource / Recreation";213304
"Collector";493059
*/
select mtfcc, count(*) from streets group by mtfcc order by mtfcc;
/*
1;14
1011;52
1100;366339
1200;3293246
1400;36239646
1500;661428
1630;364314
1640;244262
1710;41635
1720;268
1730;132661
1740;2624644
1750;498506
1780;35477
1820;4831
1830;34
2451;2
2459;2
4010;4
4020;3
4121;8
4140;1
4150;4
*/

select max(l_hnumf), max(l_hnuml), max(r_hnumf), max(r_hnuml) from rawdata.roadseg;
-- 594354600;594354616;8756714;8756716

select max(length(l_placenam)), max(length(r_placenam)) from rawdata.roadseg;  -- 97
alter table streets alter column r_ac4 type varchar(100), alter column l_ac4 type varchar(100);


insert into data.streets (link_id, name, l_refaddr, l_nrefaddr, l_sqlnumf, l_sqlfmt, r_refaddr, r_nrefaddr, r_sqlnumf, r_sqlfmt,
                          l_ac4, l_ac2, l_ac1, r_ac4, r_ac2, r_ac1, mtfcc, the_geom)
select gid as link_id,
       coalesce(nullif(l_stname_c, 'None'), nullif(l_stname_c, 'None'), nullif(l_stname_c, 'None'), nullif(l_stname_c, 'None')) as name,
       case when l_adddirfg = 'Same Direction' then l_hnumf when l_adddirfg = 'Opposite Direction' then l_hnumf else NULL end as l_refaddr,
       case when l_adddirfg = 'Same Direction' then l_hnuml when l_adddirfg = 'Opposite Direction' then l_hnuml else NULL end as l_nrefaddr,
       'FM999999999' as l_sqlnumf,
       '{}' as l_sqlfmt,
       case when r_adddirfg = 'Same Direction' then r_hnumf when r_adddirfg = 'Opposite Direction' then r_hnumf else NULL end as r_refaddr,
       case when r_adddirfg = 'Same Direction' then r_hnuml when r_adddirfg = 'Opposite Direction' then r_hnuml else NULL end as r_nrefaddr,
       'FM999999999' as r_sqlnumf,
       '{}' as r_sqlfmt,
       l_placenam as l_ac4, 
       case when datasetnam = 'Prince Edward Island' then 'PE'
            when datasetnam = 'Alberta' then 'AB'
            when datasetnam = 'Ontario' then 'ON'
            when datasetnam = 'Québec' then 'QC'
            when datasetnam = 'Yukon Territory' then 'YT'
            when datasetnam = 'Nunavut' then 'NU'
            when datasetnam = 'British Columbia' then 'BC'
            when datasetnam = 'Saskatchewan' then 'SK'
            when datasetnam = 'Manitoba' then 'MB'
            when datasetnam = 'Northwest Territories' then 'NT'
            when datasetnam = 'New Brunswick' then 'NB'
            when datasetnam = 'Nova Scotia' then 'NS'
            when datasetnam = 'Newfoundland and Labrador' then 'NL'
            else NULL end as l_ac2, 
       'Canada' as l_ac1, 
       r_placenam as r_ac4, 
       case when datasetnam = 'Prince Edward Island' then 'PE'
            when datasetnam = 'Alberta' then 'AB'
            when datasetnam = 'Ontario' then 'ON'
            when datasetnam = 'Québec' then 'QC'
            when datasetnam = 'Yukon Territory' then 'YT'
            when datasetnam = 'Nunavut' then 'NU'
            when datasetnam = 'British Columbia' then 'BC'
            when datasetnam = 'Saskatchewan' then 'SK'
            when datasetnam = 'Manitoba' then 'MB'
            when datasetnam = 'Northwest Territories' then 'NT'
            when datasetnam = 'New Brunswick' then 'NB'
            when datasetnam = 'Nova Scotia' then 'NS'
            when datasetnam = 'Newfoundland and Labrador' then 'NL'
            else NULL end as r_ac2, 
       'Canada' as r_ac1, 
       case when roadclass = 'Arterial'              then 1200
            when roadclass = 'Local / Strata'        then 1400
            when roadclass = 'Local / Street'        then 1400
            when roadclass = 'Expressway / Highway'  then 1100
            when roadclass = 'Alleyway / Lane'       then 1730
            when roadclass = 'Winter'                then 1500
            when roadclass = 'Service Lane'          then 1640
            when roadclass = 'Freeway'               then 1200
            when roadclass = 'Local / Unknown'       then 1400
            when roadclass = 'Rapid Transit'         then 1300
            when roadclass = 'Ramp'                  then 1630
            when roadclass = 'Resource / Recreation' then 1500
            when roadclass = 'Collector'             then 1200
            else NULL end as mtfcc, 
       st_transform(geom, 4326) as the_geom
  from rawdata.roadseg;
-- Query returned successfully: 2416594 rows affected, 231579 ms execution time.

-- loaded  imt-rgeo-pgis-2.0.sql
-- running imt-rgeo-pgis-2.0-part2.sql

select imt_make_intersections('streets', 0.000001, 'the_geom', 'gid', ' name is not null ');
-- Total query runtime: 18315596 ms.

alter function imt_make_intersections(character varying, double precision, character varying, character varying, text) set schema rawdata;


analyze;
-- Query returned successfully with no result in 28716 ms.

update intersections_tmp as a set
          cnt  = (select count(*) from st_vert_tmp b where b.vid=a.id),
          dcnt = (select count(distinct name) from st_vert_tmp b, streets c where b.gid=c.gid and b.vid=a.id and c.name is not null);
-- Query returned successfully: 28610029 rows affected, 3603829 ms execution time.

vacuum analyze verbose;
-- Query returned successfully with no result in 571569 ms.

select * from imt_reverse_geocode_flds(-74, 42);
-- "Leggs Mills Rd";"";"Ulster";"Ulster";"NY";"US";"";66.328152156;41886936  682 ms.
select * from imt_rgeo_intersections_flds(-74, 42);
-- "Old Kings Hwy, Co Rd 31, Leggs Mills Rd";"";"Ulster";"Ulster";"NY";"US";"";99.1;16347163  283 ms.
select * from imt_reverse_geocode_flds(-75, 45);
-- "Ault Drive";"";"Township Of South Stormont";"";"ON";"Canada";"";177.48630323;1929193  11 ms.
select * from imt_rgeo_intersections_flds(-75, 45);
-- "Ault Drive, Beech Street";"";"Township Of South Stormont";"";"ON";"Canada";"";224.8;28144111  73 ms.