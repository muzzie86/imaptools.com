create or replace function imt_geocode_place(istate text, icity text, zip text, fuzzy integer, method integer)
  returns setof geo_result2 as
$body$

declare
  sql text;
  rec geo_result2;
  cnt integer := 0;

/*    Stuff TODO
      1. need to standardize input
      2. need a sql planner for place and fuzzy
      3. need to score results
      4. need two functions: 1. to pass city, state, zip; 2. to pass stdaddr rec to
*/

begin

  -- got a zipcode? if so check this first
  if length(btrim(coalesce(zip, ''))) > 0 then  
    for rec in select *  -- <#### expand this
                 from zipcode
                where postcode = zip
      loop
        cnt := cnt + 1;
        return next rec;
    end loop;
  end if;

  -- if we got a zip result, don't bother to do this
  -- if we have city state as input then we can try this
  if cnt == 0 and length(btrim(coalesce(icity, ''))) > 0 and length(btrim(coalesce(istate, ''))) > 0 then
    for rec in select *  -- <#### expand this
                 from citystate
                where city=icity and state=istate
      loop
        cnt := cnt + 1;
        return next rec;
    end loop;
  end if;
  
  return;
end;
$body$
  language plpgsql stable
  cost 5
  rows 10;