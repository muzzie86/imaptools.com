    select 
                 a.tlid, 
                 --coalesce(a.fullname,'') as fullname, 
                 --coalesce(b.paflag,'') as paflag, 
                 a.mtfcc, 
                 a.passflg, 
                 a.divroad, 
                 coalesce(c.fromhn,'') as fromhn, 
                 coalesce(c.tohn,'') as tohn, 
                 coalesce(c.side,'') as side, 
                 coalesce(c.zip,'') as zip, 
                 --coalesce(c.plus4,'') as plus4, 
                 a.statefp, 
                 a.countyfp, 
                 a.tfidl, 
                 a.tfidr, 
                 coalesce(b.name,'') as name, 
                 coalesce(b.predirabrv,'') as predirabrv, 
                 coalesce(b.pretypabrv,'') as pretypabrv, 
                 coalesce(b.prequalabr,'') as prequalabr, 
                 coalesce(b.sufdirabrv,'') as sufdirabrv, 
                 coalesce(b.suftypabrv,'') as suftypabrv, 
                 coalesce(b.sufqualabr,'') as sufqualabr,
                 featcat
                 --a.recno 
            from edges as a left outer join addr as c on (a.tlid=c.tlid) 
                 left outer join featnames as b on (a.tlid=b.tlid) 
                 left outer join addrfn as d on 
                     (b.linearid=d.linearid and c.arid=d.arid) 
           where a.roadflg='Y'
           and a.tlid=619853728
           order by a.tlid, a.fullname, c.side
           limit 100;

select
      --a.tlid,
       count(*) as cnt
            from edges as a left outer join addr as c on (a.tlid=c.tlid) 
                 left outer join featnames as b on (a.tlid=b.tlid) 
                 left outer join addrfn as d on 
                     (b.linearid=d.linearid and c.arid=d.arid) 
           where a.roadflg='Y'  and a.fullname is not null
           --group by a.tlid
           --having count(*)>2
           --order by a.tlid
--166405

select count(*) from (
select
      a.tlid,
       count(*) as cnt
            from edges as a left outer join addr as c on (a.tlid=c.tlid) 
                 left outer join featnames as b on (a.tlid=b.tlid) 
                 left outer join addrfn as d on 
                     (b.linearid=d.linearid and c.arid=d.arid) 
           where a.roadflg='Y'  and a.fullname is not null
           group by a.tlid
           having count(*)>2
           order by a.tlid ) as foo

--8031

select
      --a.tlid,
       count(*) as cnt
            from edges as a left outer join addr as c on (a.tlid=c.tlid) 
                 left outer join featnames as b on (a.tlid=b.tlid) 
                 left outer join addrfn as d on 
                     (b.linearid=d.linearid and c.arid=d.arid) 
           where a.featcat='S' and a.fullname is not null
--166419

select count(*) from edges where roadflg='Y'; --102958
select count(*) from edges where featcat='S'; --82508
select featcat, count(*) from edges where roadflg='Y' group by featcat;
--S, 82494
--'', 20464

select roadflg, count(*) from edges where featcat='S' group by roadflg;
select mtfcc, count(*) from edges where featcat is null group by mtfcc order by mtfcc;
select mtfcc, count(*) from edges where featcat is null and fullname is not null group by mtfcc order by mtfcc;

