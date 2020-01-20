die;

drop type if exists rseg cascade;
create type rseg as (
 rsgid integer, 
 name text, 
 hnumf integer, 
 hnuml integer, 
 side char(1), 
 city text, 
 state text, 
 country text
);


-- this function is deprecated for and we should use the large union

create or replace function expand_roadseg() 
  returns setof rseg as
$body$
  declare
    rec record;
    r rseg;
    
  begin
    r.country := 'CANADA';
    for rec in select * from rawdata.roadseg loop
      -- do the left side
      if rec.l_hnumf != 0 and rec.l_hnuml != 0 and (rec.l_hnumf != -1 or rec.l_hnuml != -1) then
        r.rsgid := rec.gid;
        r.hnumf := coalesce(nullif(rec.l_hnumf, -1), rec.l_hnuml);
        r.hnuml := coalesce(nullif(rec.l_hnuml, -1), rec.l_hnumf);
        r.side := 'L';
        r.city := rec.l_placenam;
        r.state := rec.datasetnam;
        if rec.l_stname_c != 'None' then
          r.name := rec.l_stname_c;
          return next r;
        end if;
        if rec.rtnumber1 != 'None' then
          r.name := rec.rtnumber1;
          return next r;
        end if;
        if rec.rtnumber2 != 'None' then
          r.name := rec.rtnumber2;
          return next r;
        end if;
        if rec.rtnumber3 != 'None' then
          r.name := rec.rtnumber3;
          return next r;
        end if;
        if rec.rtnumber4 != 'None' then
          r.name := rec.rtnumber4;
          return next r;
        end if;
        if rec.rtnumber5 != 'None' then
          r.name := rec.rtnumber5;
          return next r;
        end if;
        if rec.rtename1fr != 'None' then
          r.name := rec.rtename1fr;
          return next r;
        end if;
        if rec.rtename2fr != 'None' then
          r.name := rec.rtename2fr;
          return next r;
        end if;
        if rec.rtename3fr != 'None' then
          r.name := rec.rtename3fr;
          return next r;
        end if;
        if rec.rtename4fr != 'None' then
          r.name := rec.rtename4fr;
          return next r;
        end if;
        if rec.rtename1en != 'None' then
          r.name := rec.rtename1en;
          return next r;
        end if;
        if rec.rtename2en != 'None' then
          r.name := rec.rtename2en;
          return next r;
        end if;
        if rec.rtename3en != 'None' then
          r.name := rec.rtename3en;
          return next r;
        end if;
        if rec.rtename4en != 'None' then
          r.name := rec.rtename4en;
          return next r;
        end if;
      end if;
      -- do the right side
      if rec.r_hnumf != 0 and rec.r_hnuml != 0 and (rec.r_hnumf != -1 or rec.r_hnuml != -1) then
        r.rsgid := rec.gid;
        r.hnumf := coalesce(nullif(rec.r_hnumf, -1), rec.r_hnuml);
        r.hnuml := coalesce(nullif(rec.r_hnuml, -1), rec.r_hnumf);
        r.side := 'R';
        r.city := rec.r_placenam;
        r.state := rec.datasetnam;
        if rec.r_stname_c != 'None' then
          r.name := rec.r_stname_c;
          return next r;
        end if;
        if rec.rtnumber1 != 'None' then
          r.name := rec.rtnumber1;
          return next r;
        end if;
        if rec.rtnumber2 != 'None' then
          r.name := rec.rtnumber2;
          return next r;
        end if;
        if rec.rtnumber3 != 'None' then
          r.name := rec.rtnumber3;
          return next r;
        end if;
        if rec.rtnumber4 != 'None' then
          r.name := rec.rtnumber4;
          return next r;
        end if;
        if rec.rtnumber5 != 'None' then
          r.name := rec.rtnumber5;
          return next r;
        end if;
        if rec.rtename1fr != 'None' then
          r.name := rec.rtename1fr;
          return next r;
        end if;
        if rec.rtename2fr != 'None' then
          r.name := rec.rtename2fr;
          return next r;
        end if;
        if rec.rtename3fr != 'None' then
          r.name := rec.rtename3fr;
          return next r;
        end if;
        if rec.rtename4fr != 'None' then
          r.name := rec.rtename4fr;
          return next r;
        end if;
        if rec.rtename1en != 'None' then
          r.name := rec.rtename1en;
          return next r;
        end if;
        if rec.rtename2en != 'None' then
          r.name := rec.rtename2en;
          return next r;
        end if;
        if rec.rtename3en != 'None' then
          r.name := rec.rtename3en;
          return next r;
        end if;
        if rec.rtename4en != 'None' then
          r.name := rec.rtename4en;
          return next r;
        end if;
      end if;
    end loop;
    
    return;
  end;
$body$
  LANGUAGE plpgsql stable strict;

-------------------------------------------------------------------

drop table if exists rawdata.st cascade;

create table rawdata.st (
 gid serial not null primary key,
 rsgid integer, 
 name text, 
 hnumf integer, 
 hnuml integer, 
 side char(1), 
 city text, 
 state text, 
 country text,
 altplace text
);

insert into rawdata.st (rsgid, name, hnumf, hnuml, side, city, state, country, altplace)
select a.*, case when a.side='L' then e.placename else g.placename end as altplace
  from expand_roadseg() as a 
       left outer join rawdata.roadseg b on a.rsgid=b.gid
       left outer join rawdata.addrange c on b.adrangenid=c.nid
       left outer join rawdata.altnamlink d on d.nid=c.l_altnanid
       left outer join rawdata.strplaname e on d.strnamenid=e.nid
       left outer join rawdata.altnamlink f on f.nid=c.r_altnanid
       left outer join rawdata.strplaname g on f.strnamenid=g.nid

-- Query returned successfully: 2486277 rows affected, 376245 ms execution time.

select * from rawdata.st where city != altplace limit 100;

select * from rawdata.roadseg where gid=964373;
