-- Function: imt_geocoder(stdaddr, integer, double precision, integer)

-- DROP FUNCTION imt_geocoder(stdaddr, integer, double precision, integer);

CREATE OR REPLACE FUNCTION imt_geocoder(sfld stdaddr, fuzzy integer, aoffset double precision, method integer)
  RETURNS SETOF geo_result2 AS
$BODY$
declare
  sql text;
  rec geo_result2;
  cnt integer := 0;

begin

--raise notice 'sfld: %', sfld;
  if (sfld.state is null and sfld.postcode is null) or sfld.house_num is null then
    return;
  end if;

  -- make sure we got an address that we can use
  if coalesce(sfld.house_num,sfld.predir,sfld.qual,sfld.pretype,sfld.name,sfld.suftype,sfld.sufdir) is not null and
     coalesce(sfld.city,sfld.state,sfld.postcode /* ,sfld.country */) is not null then
    select * into sql from imt_geo_planner2(sfld, fuzzy);
--raise notice '%', sql;
    if found then
      for rec in select 
                             0::int4 as rid,
                             a.id,
                             b.link_id as id1,
                             case when b.side='R' then 1::integer else 2::integer end as id2,
                             b.area_id as id3,
                             sfld.house_num as completeaddressnumber,
                             b.st_nm_pref as predirectional,
                             null as premodifier,
                             null as postmodifier,
                             b.st_typ_bef as pretype,
                             b.st_nm_base as streetname,
                             b.st_typ_aft as posttype,
                             b.st_nm_suff as postdirectional,
                             b.city as placename,
                             null as placename_usps,
                             b.state as statename,
                             b.country as countrycode,
                             b.postcode as zipcode,
                             sfld.building,
                             sfld.unit,
                             plan,
                             parity,
                             a.score as geocodematchcode,
                             a.x,
                             a.y
                   from (select * from imt_geo_score(sql, sfld, aoffset)) as a, data.streets b where a.id=b.gid order by a.score desc, plan asc loop
        cnt := cnt + 1;
        return next rec;
      end loop;
    end if;
  end if;
  return;
end;
$BODY$
  LANGUAGE plpgsql IMMUTABLE STRICT
  COST 5
  ROWS 100;
ALTER FUNCTION imt_geocoder(stdaddr, integer, double precision, integer)
  OWNER TO postgres;
