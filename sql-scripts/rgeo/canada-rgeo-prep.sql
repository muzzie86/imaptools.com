select * from imt_reverse_geocode_flds(-75.5,45.45);


select * from streets order by gid limit 100;

update streets set l_refaddr=l_nrefaddr where l_refaddr is null and l_nrefaddr is not null;
update streets set l_nrefaddr=l_refaddr where l_nrefaddr is null and l_refaddr is not null;
update streets set r_refaddr=r_nrefaddr where r_refaddr is null and r_nrefaddr is not null;
update streets set r_nrefaddr=r_refaddr where r_nrefaddr is null and r_refaddr is not null;


update streets set l_sqlnumf='FM'||repeat('9',case when length(l_refaddr::text)>length(l_nrefaddr::text) then length(l_refaddr::text) else length(l_nrefaddr::text) end),
                   l_sqlfmt='{}',
                   l_cformat='%'||case when length(l_refaddr::text)>length(l_nrefaddr::text) then length(l_refaddr::text) else length(l_nrefaddr::text) end||'d',
                   r_sqlnumf='FM'||repeat('9',case when length(r_refaddr::text)>length(r_nrefaddr::text) then length(r_refaddr::text) else length(r_nrefaddr::text) end),
                   r_sqlfmt='{}',
                   r_cformat='%'||case when length(r_refaddr::text)>length(r_nrefaddr::text) then length(r_refaddr::text) else length(r_nrefaddr::text) end||'d'
 where l_refaddr is not null or r_refaddr is not null or l_nrefaddr is not null or r_nrefaddr is not null;

-- Function: prep.update_streets_attrs()

-- DROP FUNCTION prep.update_streets_attrs();

CREATE OR REPLACE FUNCTION prep.update_streets_attrs()
  RETURNS text AS
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
ALTER FUNCTION prep.update_streets_attrs() OWNER TO postgres;

create schema prep;
alter table gccs000a06a_e set schema prep;
alter table gccs000b06a_e set schema prep;
alter table gcd_000a06a_e set schema prep;
alter table gcd_000b06a_e set schema prep;
alter table gcma000a06a_e set schema prep;
alter table gcma000b06a_e set schema prep;
alter table gcsd000a06a_e set schema prep;
alter table gcsd000b06a_e set schema prep;
alter table ger_000a06a_e set schema prep;
alter table ger_000b06a_e set schema prep;
alter table ghy_000c06a_e set schema prep;
alter table ghy_000d06a_e set schema prep;
alter table ghy_000f06a_e set schema prep;
alter table gpr_000a06a_e set schema prep;
alter table gpr_000b06a_e set schema prep;
alter table grnf000r10a_e set schema prep;
alter function update_streets_attrs() set schema prep;