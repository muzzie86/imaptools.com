-- For STATCAN data

CREATE OR REPLACE FUNCTION update_streets_attrs()
  RETURNS TEXT AS
$BODY$
DECLARE
    sgid INTEGER;
    rec RECORD;
    ac3 text;
    ac4 text;
    ac5 text;
    
BEGIN

    FOR sgid IN SELECT gid FROM streets
    LOOP
        SELECT b.csdname, b.csdtype, b.cdname FROM streets AS a, gcsd000a06a_e AS b
          INTO rec
         WHERE st_transform(a.the_geom,st_srid(b.the_geom)) && b.the_geom AND a.gid=sgid
         ORDER BY st_length(st_intersection(st_transform(a.the_geom,st_srid(b.the_geom)), b.the_geom)) DESC LIMIT 1;

        IF FOUND THEN
            ac5 := CASE WHEN rec.csdtype in ('CC','CG','COM','HAM','NH','NV','NVL','RV','SC','SÉ','S-É','SET','SV','V','VC','VK','VL,','VN') THEN rec.csdname ELSE NULL END;
            ac4 := CASE WHEN rec.csdtype in ('C','CÉ','CN','CT','CU','CY','IM','IRI','LGD','LOT','M','MÉ','MU','NL','RCR','RGM','RM','SM','SNO','T','TC','TI','TK','TP','TV') THEN rec.csdname ELSE NULL END;
            ac3 := CASE WHEN rec.csdtype in ('CM','DM','ID','IGD','MD','NO','P','PE','RDA','RG','TL') THEN rec.csdname ELSE rec.cdname END;

            UPDATE streets SET r_ac3=ac3, l_ac3=ac3, r_ac4=ac4, l_ac4=ac4, r_ac5=ac5, l_ac5=ac5
             WHERE gid=sgid;

        END IF;
        
    END LOOP;

    RETURN 'OK';
END
$BODY$
  LANGUAGE plpgsql VOLATILE
  COST 100;


/*

select max(length(csdname)) as ac5len from gcsd000a06a_e where csdtype in ('CC','CG','COM','HAM','NH','NV','NVL','RV','SC','SÉ','S-É','SET','SV','V','VC','VK','VL,','VN');
select max(length(csdname)) as ac4len from gcsd000a06a_e where csdtype in ('C','CÉ','CN','CT','CU','CY','IM','IRI','LGD','LOT','M','MÉ','MU','NL','RCR','RGM','RM','SM','SNO','T','TC','TI','TK','TP','TV');
select max(length(csdname)) as as3len from gcsd000a06a_e where csdtype in ('CM','DM','ID','IGD','MD','NO','P','PE','RDA','RG','TL');
select max(length(cdname)) as cdnlen from gcsd000a06a_e;

select count(*) from gcsd000a06a_e where not isvalid(the_geom) and not isvalid(st_buffer(the_geom,0));
update gcsd000a06a_e set the_geom=st_multi(st_buffer(the_geom,0)) where not isvalid(the_geom);

select addgeometrycolumn('data','gcsd000a06a_e','geom4326',4326,'MULTIPOLYGON',2);
update gcsd000a06a_e set geom4326=st_transform(the_geom, 4326);
CREATE INDEX gcsd000a06a_e_geom4326_gist ON gcsd000a06a_e USING gist (geom4326);
VACUUM ANALYZE gcsd000a06a_e;

--select update_streets_attrs(); -- this is obsolete, use the UPDATE query below

UPDATE streets SET r_ac3=c.ac3, l_ac3=c.ac3, r_ac4=c.ac4, l_ac4=c.ac4, r_ac5=c.ac5, l_ac5=c.ac5 FROM (
  SELECT DISTINCT ON (a.gid) a.gid, 
      CASE WHEN b.csdtype in ('CC','CG','COM','HAM','NH','NV','NVL','RV','SC','SÉ','S-É','SET','SV','V','VC','VK','VL','VN') THEN b.csdname ELSE NULL END AS ac5,
      CASE WHEN b.csdtype in ('C','CÉ','CN','CT','CU','CY','IM','IRI','LGD','LOT','M','MÉ','MU','NL','P','PE','RCR','RGM','RM','SM','SNO','T','TC','TI','TK','TP','TV') THEN b.csdname ELSE NULL END AS ac4,
      CASE WHEN b.csdtype in ('CM','DM','ID','IGD','MD','NO','RDA','RG','TL') THEN b.csdname ELSE b.cdname END AS ac3
    FROM
      --(SELECT * from streets where L_ac2='QC' LIMIT 200)
      streets
         AS a INNER JOIN gcsd000a06a_e AS b ON a.the_geom && b.geom4326
   WHERE
     st_intersects(a.the_geom, b.geom4326)
   ORDER BY a.gid, st_length(st_intersection(a.the_geom, b.geom4326)) ASC
  ) AS c
 WHERE streets.gid=c.gid;

-- Query returned successfully: 1956944 rows affected, 17237199 ms execution time.

select st_length(st_intersection(a.geom4326, b.the_geom)) as len, * from gcsd000a06a_e a, streets b
 where a.geom4326 && b.the_geom and st_distance(a.geom4326, b.the_geom)=0 and b.gid=199324

*/