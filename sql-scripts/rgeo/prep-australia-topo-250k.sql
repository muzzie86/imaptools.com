--------------------------------------------------------------------------------------------------------
-- polygons
--------------------------------------------------------------------------------------------------------

select count(*) from buildingareas where not st_isvalid(geom);
update buildingareas set geom=st_forceRHR(geom), name=initcap(name);

select count(*) from builtupareas where not st_isvalid(geom);		-- 17
select count(*) from builtupareas where not st_isvalid(geom) and not st_isvalid(st_makevalid(geom));
update builtupareas set geom=st_makevalid(geom) where not st_isvalid(geom);
update builtupareas set geom=st_forceRHR(geom), name=initcap(name);

select count(*) from canalareas where not st_isvalid(geom);
update canalareas set geom=st_forceRHR(geom), name=initcap(name);

select count(*) from cemeteryareas where not st_isvalid(geom);
update cemeteryareas set geom=st_forceRHR(geom), name=initcap(name);

select count(*) from craters where not st_isvalid(geom);
update craters set geom=st_forceRHR(geom), name=initcap(name);

select count(*) from cultivatedareas where not st_isvalid(geom);	-- 8
select count(*) from cultivatedareas where not st_isvalid(geom) and not st_isvalid(st_makevalid(geom));
update cultivatedareas set geom=st_makevalid(geom) where not st_isvalid(geom);
update cultivatedareas set geom=st_forceRHR(geom), textnote=initcap(textnote);

select count(*) from deformationareas where not st_isvalid(geom);
update deformationareas set geom=st_forceRHR(geom), name=initcap(name);

select count(*) from flats where not st_isvalid(geom);			-- 100
select count(*) from flats where not st_isvalid(geom) and not st_isvalid(st_makevalid(geom));
update flats set geom=st_makevalid(geom) where not st_isvalid(geom);
update flats set geom=st_forceRHR(geom), name=initcap(name);

select count(*) from foreshoreflats where not st_isvalid(geom);		-- 6
select count(*) from foreshoreflats where not st_isvalid(geom) and not st_isvalid(st_makevalid(geom));
update foreshoreflats set geom=st_makevalid(geom) where not st_isvalid(geom);
update foreshoreflats set geom=st_forceRHR(geom);

select count(*) from geodataindexes where not st_isvalid(geom);
update geodataindexes set geom=st_forceRHR(geom);

select count(*) from islands where not st_isvalid(geom);
update islands set geom=st_forceRHR(geom), name=initcap(name);

select count(*) from lakes where not st_isvalid(geom);			-- 2
select count(*) from lakes where not st_isvalid(geom) and not st_isvalid(st_makevalid(geom));
update lakes set geom=st_makevalid(geom) where not st_isvalid(geom);
update lakes set geom=st_forceRHR(geom), name=initcap(name);

select count(*) from largeareafeatures where not st_isvalid(geom);
update largeareafeatures set geom=st_forceRHR(geom), name=initcap(name);

select count(*) from mainlands where not st_isvalid(geom);
update mainlands set geom=st_forceRHR(geom);

select count(*) from mapindexes where not st_isvalid(geom);
update mapindexes set geom=st_forceRHR(geom);

select count(*) from marinehazardareas where not st_isvalid(geom);	-- 13
select count(*) from marinehazardareas where not st_isvalid(geom) and not st_isvalid(st_makevalid(geom));
update marinehazardareas set geom=st_makevalid(geom) where not st_isvalid(geom);
update marinehazardareas set geom=st_forceRHR(geom), name=initcap(name);

select count(*) from mineareas where not st_isvalid(geom);
update mineareas set geom=st_forceRHR(geom), name=initcap(name);

select count(*) from nativevegetationareas where not st_isvalid(geom);	-- 572
select count(*) from nativevegetationareas where not st_isvalid(geom) and not st_isvalid(st_makevalid(geom));
update nativevegetationareas set geom=st_makevalid(geom) where not st_isvalid(geom);
update nativevegetationareas set geom=st_forceRHR(geom), textnote=initcap(textnote);

select count(*) from pondageareas where not st_isvalid(geom);
update pondageareas set geom=st_forceRHR(geom), textnote=initcap(textnote);

select count(*) from prohibitedareas where not st_isvalid(geom);
update prohibitedareas set geom=st_forceRHR(geom), name=initcap(name);

select count(*) from rapidareas where not st_isvalid(geom);
update rapidareas set geom=st_forceRHR(geom), textnote=initcap(textnote);

select count(*) from recreationareas where not st_isvalid(geom);	-- 1
select count(*) from recreationareas where not st_isvalid(geom) and not st_isvalid(st_makevalid(geom));
update recreationareas set geom=st_makevalid(geom) where not st_isvalid(geom);
update recreationareas set geom=st_forceRHR(geom), name=initcap(name);

select count(*) from reserves where not st_isvalid(geom);		-- 32
select count(*) from reserves where not st_isvalid(geom) and not st_isvalid(st_makevalid(geom));
update reserves set geom=st_makevalid(geom) where not st_isvalid(geom);
update reserves set geom=st_forceRHR(geom), name=initcap(name);

select count(*) from reservoirs where not st_isvalid(geom);
update reservoirs set geom=st_forceRHR(geom), name=initcap(name);

select count(*) from sands where not st_isvalid(geom);
update sands set geom=st_forceRHR(geom);

select count(*) from seas where not st_isvalid(geom);			-- 1
select count(*) from seas where not st_isvalid(geom) and not st_isvalid(st_makevalid(geom));
update seas set geom=st_makevalid(geom) where not st_isvalid(geom);
update seas set geom=st_forceRHR(geom), seaname=initcap(seaname);

select count(*) from watercourseareas where not st_isvalid(geom);	-- 25
select count(*) from watercourseareas where not st_isvalid(geom) and not st_isvalid(st_makevalid(geom));
update watercourseareas set geom=st_makevalid(geom) where not st_isvalid(geom);
update watercourseareas set geom=st_forceRHR(geom), name=initcap(name);

--------------------------------------------------------------------------------------------------------
-- lines
--------------------------------------------------------------------------------------------------------

select distinct featwidth from roadcrossinglines;
alter table roadcrossinglines add column featwidth1 float;
alter table roadcrossinglines add column featwidth2 float;
update roadcrossinglines set featwidth1 = coalesce(nullif(featwidth*10,0),1), featwidth2 = featwidth*10+4;

select distinct featwidth from railwaycrossinglines;
alter table railwaycrossinglines add column featwidth1 float;
alter table railwaycrossinglines add column featwidth2 float;
update railwaycrossinglines set featwidth1 = coalesce(nullif(featwidth*10,0),1), featwidth2 = featwidth*10+4;

select distinct featwidth1 from roads;
alter table roads add column featwidth1 float;
alter table roads add column featwidth2 float;
update roads set featwidth1 = coalesce(nullif(featwidth*10,0),1), featwidth2 = featwidth*10+4;

select distinct featwidth from waterfallpoints;
alter table waterfallpoints add column featwidth1 float;
update waterfallpoints set featwidth1 = coalesce(nullif(featwidth,0),1)*10;

select distinct featwidth1 from railwaybridgepoints;
alter table railwaybridgepoints add column featwidth1 float;
update railwaybridgepoints set featwidth1 = coalesce(nullif(featwidth,0)*100,10);


--------------------------------------------------------------------------------------------------------
-- points
--------------------------------------------------------------------------------------------------------

