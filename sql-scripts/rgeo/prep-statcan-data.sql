CREATE TABLE streets
(
  gid serial NOT NULL,
  link_id integer,
  "name" character varying(80),
  l_refaddr integer,
  l_nrefaddr integer,
  l_sqlnumf character varying(12),
  l_sqlfmt character varying(16),
  l_cformat character varying(16),
  r_refaddr integer,
  r_nrefaddr integer,
  r_sqlnumf character varying(12),
  r_sqlfmt character varying(16),
  r_cformat character varying(16),
  l_ac5 character varying(35),
  l_ac4 character varying(35),
  l_ac3 character varying(35),
  l_ac2 character varying(35),
  l_ac1 character varying(35),
  r_ac5 character varying(35),
  r_ac4 character varying(35),
  r_ac3 character varying(35),
  r_ac2 character varying(35),
  r_ac1 character varying(35),
  l_postcode character varying(11),
  r_postcode character varying(11),
  the_geom geometry NOT NULL,
  CONSTRAINT streets_pkey PRIMARY KEY (gid),
  CONSTRAINT enforce_dims_the_geom CHECK (ndims(the_geom) = 2),
  CONSTRAINT enforce_geotype_the_geom CHECK (geometrytype(the_geom) = 'MULTILINESTRING'::text OR the_geom IS NULL),
  CONSTRAINT enforce_srid_the_geom CHECK (srid(the_geom) = 4326)
)
WITH (
  OIDS=FALSE
);

select Probe_geometry_columns();
select Populate_geometry_Columns();

insert into streets (link_id, name, l_refaddr, l_nrefaddr, r_refaddr, r_nrefaddr, l_ac1, r_ac1, l_ac2, r_ac2, the_geom)
  select rb_uid::integer,
         name || coalesce(' ' || nullif(initcap(type), ''), '') || coalesce(' ' || nullif(upper(direction), ''), '') as name,
         addr_fm_le, addr_to_le, addr_fm_rg, addr_to_rg,
         'CA', 'CA', 'NL', 'NL', st_transform(the_geom, 4326) from grnf010r10a_e;

insert into streets (link_id, name, l_refaddr, l_nrefaddr, r_refaddr, r_nrefaddr, l_ac1, r_ac1, l_ac2, r_ac2, the_geom)
  select rb_uid::integer,
         name || coalesce(' ' || nullif(initcap(type), ''), '') || coalesce(' ' || nullif(upper(direction), ''), '') as name,
         addr_fm_le, addr_to_le, addr_fm_rg, addr_to_rg,
         'CA', 'CA', 'PE', 'PE', st_transform(the_geom, 4326) from grnf011r10a_e;

insert into streets (link_id, name, l_refaddr, l_nrefaddr, r_refaddr, r_nrefaddr, l_ac1, r_ac1, l_ac2, r_ac2, the_geom)
  select rb_uid::integer,
         name || coalesce(' ' || nullif(initcap(type), ''), '') || coalesce(' ' || nullif(upper(direction), ''), '') as name,
         addr_fm_le, addr_to_le, addr_fm_rg, addr_to_rg,
         'CA', 'CA', 'NS', 'NS', st_transform(the_geom, 4326) from grnf012r10a_e;

insert into streets (link_id, name, l_refaddr, l_nrefaddr, r_refaddr, r_nrefaddr, l_ac1, r_ac1, l_ac2, r_ac2, the_geom)
  select rb_uid::integer,
         name || coalesce(' ' || nullif(initcap(type), ''), '') || coalesce(' ' || nullif(upper(direction), ''), '') as name,
         addr_fm_le, addr_to_le, addr_fm_rg, addr_to_rg,
         'CA', 'CA', 'NB', 'NB', st_transform(the_geom, 4326) from grnf013r10a_e;

insert into streets (link_id, name, l_refaddr, l_nrefaddr, r_refaddr, r_nrefaddr, l_ac1, r_ac1, l_ac2, r_ac2, the_geom)
  select rb_uid::integer,
         name || coalesce(' ' || nullif(initcap(type), ''), '') || coalesce(' ' || nullif(upper(direction), ''), '') as name,
         addr_fm_le, addr_to_le, addr_fm_rg, addr_to_rg,
         'CA', 'CA', 'QC', 'QC', st_transform(the_geom, 4326) from grnf024r10a_e;

insert into streets (link_id, name, l_refaddr, l_nrefaddr, r_refaddr, r_nrefaddr, l_ac1, r_ac1, l_ac2, r_ac2, the_geom)
  select rb_uid::integer,
         name || coalesce(' ' || nullif(initcap(type), ''), '') || coalesce(' ' || nullif(upper(direction), ''), '') as name,
         addr_fm_le, addr_to_le, addr_fm_rg, addr_to_rg,
         'CA', 'CA', 'ON', 'ON', st_transform(the_geom, 4326) from grnf035r10a_e;

insert into streets (link_id, name, l_refaddr, l_nrefaddr, r_refaddr, r_nrefaddr, l_ac1, r_ac1, l_ac2, r_ac2, the_geom)
  select rb_uid::integer,
         name || coalesce(' ' || nullif(initcap(type), ''), '') || coalesce(' ' || nullif(upper(direction), ''), '') as name,
         addr_fm_le, addr_to_le, addr_fm_rg, addr_to_rg,
         'CA', 'CA', 'MB', 'MB', st_transform(the_geom, 4326) from grnf046r10a_e;

insert into streets (link_id, name, l_refaddr, l_nrefaddr, r_refaddr, r_nrefaddr, l_ac1, r_ac1, l_ac2, r_ac2, the_geom)
  select rb_uid::integer,
         name || coalesce(' ' || nullif(initcap(type), ''), '') || coalesce(' ' || nullif(upper(direction), ''), '') as name,
         addr_fm_le, addr_to_le, addr_fm_rg, addr_to_rg,
         'CA', 'CA', 'SK', 'SK', st_transform(the_geom, 4326) from grnf047r10a_e;

insert into streets (link_id, name, l_refaddr, l_nrefaddr, r_refaddr, r_nrefaddr, l_ac1, r_ac1, l_ac2, r_ac2, the_geom)
  select rb_uid::integer,
         name || coalesce(' ' || nullif(initcap(type), ''), '') || coalesce(' ' || nullif(upper(direction), ''), '') as name,
         addr_fm_le, addr_to_le, addr_fm_rg, addr_to_rg,
         'CA', 'CA', 'AB', 'AB', st_transform(the_geom, 4326) from grnf048r10a_e;

insert into streets (link_id, name, l_refaddr, l_nrefaddr, r_refaddr, r_nrefaddr, l_ac1, r_ac1, l_ac2, r_ac2, the_geom)
  select rb_uid::integer,
         name || coalesce(' ' || nullif(initcap(type), ''), '') || coalesce(' ' || nullif(upper(direction), ''), '') as name,
         addr_fm_le, addr_to_le, addr_fm_rg, addr_to_rg,
         'CA', 'CA', 'BC', 'BC', st_transform(the_geom, 4326) from grnf059r10a_e;

insert into streets (link_id, name, l_refaddr, l_nrefaddr, r_refaddr, r_nrefaddr, l_ac1, r_ac1, l_ac2, r_ac2, the_geom)
  select rb_uid::integer,
         name || coalesce(' ' || nullif(initcap(type), ''), '') || coalesce(' ' || nullif(upper(direction), ''), '') as name,
         addr_fm_le, addr_to_le, addr_fm_rg, addr_to_rg,
         'CA', 'CA', 'YK', 'YK', st_transform(the_geom, 4326) from grnf060r10a_e;

insert into streets (link_id, name, l_refaddr, l_nrefaddr, r_refaddr, r_nrefaddr, l_ac1, r_ac1, l_ac2, r_ac2, the_geom)
  select rb_uid::integer,
         name || coalesce(' ' || nullif(initcap(type), ''), '') || coalesce(' ' || nullif(upper(direction), ''), '') as name,
         addr_fm_le, addr_to_le, addr_fm_rg, addr_to_rg,
         'CA', 'CA', 'NT', 'NT', st_transform(the_geom, 4326) from grnf061r10a_e;

insert into streets (link_id, name, l_refaddr, l_nrefaddr, r_refaddr, r_nrefaddr, l_ac1, r_ac1, l_ac2, r_ac2, the_geom)
  select rb_uid::integer,
         name || coalesce(' ' || nullif(initcap(type), ''), '') || coalesce(' ' || nullif(upper(direction), ''), '') as name,
         addr_fm_le, addr_to_le, addr_fm_rg, addr_to_rg,
         'CA', 'CA', 'NU', 'NU', st_transform(the_geom, 4326) from grnf062r10a_e;

drop table grnf010r10a_e cascade;
drop table grnf011r10a_e cascade;
drop table grnf012r10a_e cascade;
drop table grnf013r10a_e cascade;
drop table grnf024r10a_e cascade;
drop table grnf035r10a_e cascade;
drop table grnf046r10a_e cascade;
drop table grnf047r10a_e cascade;
drop table grnf048r10a_e cascade;
drop table grnf059r10a_e cascade;
drop table grnf060r10a_e cascade;
drop table grnf061r10a_e cascade;
drop table grnf062r10a_e cascade;

COMMENT ON TABLE gccs000a06a_e IS 'Census consolidated subdivision, digital boundary file';
COMMENT ON TABLE gccs000b06a_e IS 'Census consolidated subdivision, cartographic boundary file';
COMMENT ON TABLE gcd_000a06a_e IS 'Census division, digital boundary file';
COMMENT ON TABLE gcd_000b06a_e IS 'Census division, cartographic boundary file';
COMMENT ON TABLE gcma000a06a_e IS 'Census metropolitan area/census agglomeration, digital boundary file';
COMMENT ON TABLE gcma000b06a_e IS 'Census metropolitan area/census agglomeration, cartographic boundary file';
COMMENT ON TABLE gcsd000a06a_e IS 'Census subdivision, digital boundary file';
COMMENT ON TABLE gcsd000b06a_e IS 'Census subdivision, cartographic boundary file';
COMMENT ON TABLE ger_000a06a_e IS 'Economic region, digital boundary file';
COMMENT ON TABLE ger_000b06a_e IS 'Economic region, cartographic boundary file';
COMMENT ON TABLE ghy_000c06a_e IS 'Supporting hydrography (Great Lakes, St. Lawrence River, oceans, etc.), detailed interior lakes hydrographic coverage (polygon)';
COMMENT ON TABLE ghy_000d06a_e IS 'Supporting hydrography (Great Lakes, St. Lawrence River, oceans, etc.), detailed interior rivers hydrographic coverage (line)';
COMMENT ON TABLE ghy_000f06a_e IS 'Supporting hydrography (Great Lakes, St. Lawrence River, oceans, etc.), detailed interior lakes hydrographic coverage – closure lines (line)';
COMMENT ON TABLE gpr_000a06a_e IS 'National/provincial, digital boundary file';
COMMENT ON TABLE gpr_000b06a_e IS 'National/provincial, cartographic boundary file';

-------------------------------------------------------------------------------------------------------------------------

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

-------------------------------------------------------------------------------------------------------------------------

select update_streets_attrs();

