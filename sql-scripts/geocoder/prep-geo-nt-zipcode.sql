/*
 * Adding placename and zipcode geocoders
*/

/*
create index placenames_state_place_idx on placenames using btree (state, place);
create index placenames_state_dmeta_place_idx on placenames using btree (state, dmeta_place);

-- we need a way to standardize the state name into an abbreviation
create or replace function imt_get_state_abbr(state text)
    returns text as
$body$
declare
  abbr text;

begin

  select word into abbr
    from gaz
   where stdword ilike state and length(word)=2 and word!= 'NF' limit 1;

  return abbr;
end;
$body$
language plpgsql immutable strict;

create or replace function imt_geocode_placename(state text, place text, fuzzy boolean)
    returns geometry as
$body$
declare

begin

end;
$body$
language plpgsql immutable strict;

*/
select * from zipcode limit 500;

create index zipcode_idx on zipcode using btree (postalcode);

drop function imt_geocode_postcode(text);
create or replace function imt_geocode_postcode(INOUT postcode text, OUT pnt geometry, OUT name text, OUT state text)
    as
$body$
declare
    rec record;
    
begin
    postcode := substr(btrim(postcode), 1, 5);    -- truncate to 5 characters
    select * into rec from zipcode where postalcode=postcode;
    if found then
        pnt := rec.the_geom;
        name := rec.pa_name;
        state := rec.state;
    end if;
end;
$body$
language plpgsql immutable strict;

select pa_name, state, the_geom from zipcode where postalcode='01863';

select * from imt_geocode_postcode('90120-1234');
select * from imt_geocode_postcode('01863');

