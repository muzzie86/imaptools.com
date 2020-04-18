create or replace function imt_county_geojson(fips text, simp numeric default 0.0)
  returns text as
$BODY$
begin
  return replace(
    json_build_object(
        'type', 'Feature',
        'geometry', '@@@@',
        'properties', array_to_json(ARRAY[
            'fips', stccc,
            'name', name,
            'namelsad', namelsad,
            'stusps', stusps,
            'stname', stname]))::text,
    '@@@@'::text,
    st_asGeoJson(st_simplifypreservetopology(geom,simp))::text
  ) from county where stccc=fips;
end;
$BODY$
  language plpgsql stable
  cost 100;

create or replace function imt_zipcode_geojson(zip text, simp numeric default 0.0)
  returns text as
$BODY$
begin
  return replace(
    json_build_object(
        'type', 'Feature',
        'geometry', '@@@@',
        'properties', array_to_json(ARRAY[
            'zipcode', zipcode]))::text,
    '@@@@'::text,
    st_asGeoJson(st_simplifypreservetopology(geom,simp))::text
  ) from zcta5 where zipcode=zip;
end;
$BODY$
  language plpgsql stable
  cost 100;

create or replace function imt_county_geojson(lon numeric, lat numeric, simp numeric default 0.0)
  returns text as
$BODY$
declare
  fips text;

begin
    select a.fips into fips from imt_rgeo_countyzip(lon, lat) as a;
    return imt_county_geojson(fips, simp);
end;
$BODY$
  language plpgsql stable
  cost 100;

create or replace function imt_zipcode_geojson(lon numeric, lat numeric, simp numeric default 0.0)
  returns text as
$BODY$
declare
  zip text;

begin
    select a.zip into zip from imt_rgeo_countyzip(lon, lat) as a;
    return imt_zipcode_geojson(zip, simp);
end;
$BODY$
  language plpgsql stable
  cost 100;

