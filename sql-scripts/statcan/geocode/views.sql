create or replace view rawdata.streets as
select a.nid, a.rtnumber1, a.rtnumber2, a.rtnumber3, a.rtnumber4,
       a.rtename1en, a.rtename2en, a.rtename3en, a.rtename4en,
       a.rtename1fr, a.rtename2fr, a.rtename3fr, a.rtename4fr,
	   a.l_adddirfg, a.l_hnumf, a.l_hnuml, a.l_stname_c, a.l_placenam,
	   a.r_adddirfg, a.r_hnumf, a.r_hnuml, a.r_stname_c, a.r_placenam,
           c.dirprefix, c.strtypre, c.starticle, c.namebody, c.strtysuf, c.dirsuffix,
           c.muniquad, c.placename, c.province, c.placetype,
	   a.geom
  from rawdata.roadseg a
  join rawdata.addrange b on a.adrangenid=b.nid
  join rawdata.strplaname c on b.l_offnanid=c.nid
union all
select d.nid, d.rtnumber1, d.rtnumber2, d.rtnumber3, d.rtnumber4,
       d.rtename1en, d.rtename2en, d.rtename3en, d.rtename4en,
       d.rtename1fr, d.rtename2fr, d.rtename3fr, d.rtename4fr,
	   d.l_adddirfg, d.l_hnumf, d.l_hnuml, d.l_stname_c, d.l_placenam,
	   d.r_adddirfg, d.r_hnumf, d.r_hnuml, d.r_stname_c, d.r_placenam,
           f.dirprefix, f.strtypre, f.starticle, f.namebody, f.strtysuf, f.dirsuffix,
           f.muniquad, f.placename, f.province, f.placetype,
	   d.geom
  from rawdata.roadseg d
  join rawdata.addrange e on d.adrangenid=e.nid
  join rawdata.strplaname f on e.r_offnanid=f.nid
union all
select g.nid, g.rtnumber1, g.rtnumber2, g.rtnumber3, g.rtnumber4,
       g.rtename1en, g.rtename2en, g.rtename3en, g.rtename4en,
       g.rtename1fr, g.rtename2fr, g.rtename3fr, g.rtename4fr,
	   g.l_adddirfg, g.l_hnumf, g.l_hnuml, g.l_stname_c, g.l_placenam,
	   g.r_adddirfg, g.r_hnumf, g.r_hnuml, g.r_stname_c, g.r_placenam,
           j.dirprefix, j.strtypre, j.starticle, j.namebody, j.strtysuf, j.dirsuffix,
           j.muniquad, j.placename, j.province, j.placetype,
	   g.geom
  from rawdata.roadseg g
  join rawdata.addrange h on g.adrangenid=h.nid
  join rawdata.altnamlink i on h.l_altnanid=i.nid
  join rawdata.strplaname j on i.strnamenid=j.nid
union all
select k.nid, k.rtnumber1, k.rtnumber2, k.rtnumber3, k.rtnumber4,
       k.rtename1en, k.rtename2en, k.rtename3en, k.rtename4en,
       k.rtename1fr, k.rtename2fr, k.rtename3fr, k.rtename4fr,
	   k.l_adddirfg, k.l_hnumf, k.l_hnuml, k.l_stname_c, k.l_placenam,
	   k.r_adddirfg, k.r_hnumf, k.r_hnuml, k.r_stname_c, k.r_placenam,
           n.dirprefix, n.strtypre, n.starticle, n.namebody, n.strtysuf, n.dirsuffix,
           n.muniquad, n.placename, n.province, n.placetype,
	   k.geom
  from rawdata.roadseg k
  join rawdata.addrange l on k.adrangenid=l.nid
  join rawdata.altnamlink m on l.r_altnanid=m.nid
  join rawdata.strplaname n on m.strnamenid=n.nid
;
