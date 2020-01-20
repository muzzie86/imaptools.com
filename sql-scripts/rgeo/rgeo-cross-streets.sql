select * from imt_reverse_geocode_flds(-71.387804, 42.619795);
-- "16 Radcliffe Rd";"NORTH CHELMSFORD";"Chelmsford";"Middlesex";"MA";"US";"01863";5.385777571;86893125

select * from imt_rgeo_intersections_flds(-71.387804, 42.619795);
-- "Radcliffe Rd, Crooked Spring Rd";"NORTH CHELMSFORD";"Chelmsford";"Middlesex";"MA";"US";"01863";49.3;5906082


create or replace function imt_rgeo_cross_streets(x double precision, y double precision)
  returns text as
$body$

declare
  txt text;
begin

select into txt list(distinct e.name)
  from imt_reverse_geocode_flds(x, y) a,
       streets b,
       intersections_tmp c,
       st_vert_tmp d,
       st_vert_tmp dd,
       streets e
 where a.linkid=b.link_id and c.id=d.vid and c.dcnt>1 and b.gid=d.gid and d.vid=dd.vid and dd.gid=e.gid and a.linkid != e.link_id;

return txt;
end;
$body$
  language plpgsql stable;

select imt_rgeo_cross_streets(-71.387804, 42.619795);
