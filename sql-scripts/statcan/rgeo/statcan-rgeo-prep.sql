
-- run imt-rgeo-pgis-2.0.sql

DROP TABLE if exists data.streets cascade;

CREATE TABLE data.streets
(
  gid serial NOT NULL,
  link_id numeric(10,0),
  name character varying(80),
  l_refaddr numeric(10,0),
  l_nrefaddr numeric(10,0),
  l_sqlnumf character varying(12),
  l_sqlfmt character varying(16),
  l_cformat character varying(16),
  r_refaddr numeric(10,0),
  r_nrefaddr numeric(10,0),
  r_sqlnumf character varying(12),
  r_sqlfmt character varying(16),
  r_cformat character varying(16),
  l_ac5 character varying(100),
  l_ac4 character varying(100),
  l_ac3 character varying(41),
  l_ac2 character varying(35),
  l_ac1 character varying(35),
  r_ac5 character varying(100),
  r_ac4 character varying(100),
  r_ac3 character varying(41),
  r_ac2 character varying(35),
  r_ac1 character varying(35),
  l_postcode character varying(11),
  r_postcode character varying(11),
  mtfcc integer,
  passflg character varying(1),
  divroad character varying(1),
  the_geom geometry NOT NULL,
  CONSTRAINT streets_pkey PRIMARY KEY (gid),
  CONSTRAINT enforce_dims_the_geom CHECK (st_ndims(the_geom) = 2),
  CONSTRAINT enforce_geotype_the_geom CHECK (geometrytype(the_geom) = 'MULTILINESTRING'::text OR the_geom IS NULL),
  CONSTRAINT enforce_srid_the_geom CHECK (st_srid(the_geom) = 4326)
)
WITH (
  OIDS=FALSE
);

insert into data.streets (link_id, name, l_refaddr, l_nrefaddr, r_refaddr, r_nrefaddr,
                          l_ac4, r_ac4, l_ac2, r_ac2, l_ac1, r_ac1, mtfcc, the_geom)
select
    b.id as link_id,
    coalesce(nullif(nullif(l_stname_c, 'None'), 'Unknown'),
             nullif(nullif(r_stname_c, 'None'),  'Unknown'),
             nullif(nullif(l_stname_c, 'None'), 'Unknown'),
             nullif(nullif(r_stname_c, 'None'), 'Unknown'),
             nullif(nullif(rtename1en, 'None'), 'Unknown'),
             nullif(nullif(rtnumber1, 'None'), 'Unknown')
            ) as name,
     case when l_adddirfg='Opposite Direction' and l_hnuml>0 then l_hnuml
          when l_adddirfg='Same Direction' and l_hnumf>0 then l_hnumf
          when l_adddirfg='Not Applicable' and l_hnumf>0 then l_hnumf end as l_refaddr,
     case when l_adddirfg='Opposite Direction' and l_hnumf>0 then l_hnumf
          when l_adddirfg='Same Direction' and l_hnuml>0 then l_hnuml
          when l_adddirfg='Not Applicable' and l_hnuml>0 then l_hnuml end as l_nrefaddr,
     case when l_adddirfg='Opposite Direction' and r_hnuml>0 then r_hnuml
          when l_adddirfg='Same Direction' and r_hnumf>0 then r_hnumf
          when l_adddirfg='Not Applicable' and r_hnumf>0 then r_hnumf end as r_refaddr,
     case when l_adddirfg='Opposite Direction' and r_hnumf>0 then r_hnumf
          when l_adddirfg='Same Direction' and r_hnuml>0 then r_hnuml
          when l_adddirfg='Not Applicable' and r_hnuml>0 then r_hnuml end as r_nrefaddr,
    nullif(nullif(l_placenam, 'Unknown'), 'None') as l_ac4,
    nullif(nullif(r_placenam, 'Unknown'), 'None') as r_ac4,
    datasetnam as l_ac2,
    datasetnam as r_ac2,
    'CA' as l_ac1,
    'CA' as r_ac1,
    case
      when roadclass='Expressway / Highway'  then 1000
      when roadclass='Rapid Transit'         then 1100
      when roadclass='Arterial'              then 1200
      when roadclass='Collector'             then 1300
      when roadclass='Local / Strata'        then 1400
      when roadclass='Local / Street'        then 1400
      when roadclass='Local / Unknown'       then 1400
      when roadclass='Ramp'                  then 1500
      when roadclass='Alleyway / Lane'       then 1600
      when roadclass='Service Lane'          then 1700
      when roadclass='Resource / Recreation' then 1800
      when roadclass='Winter'                then 1900
      else 9999 end as mtfcc,
    st_transform(st_multi(geom), 4326) as the_geom
from rawdata.roadseg a, data.link_id b
where a.nid=b.nid;


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


