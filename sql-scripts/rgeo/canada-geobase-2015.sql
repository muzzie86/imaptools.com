select * from rawdata.roadseg limit 500;

update rawdata.roadseg set
   rtnumber1 = nullif(rtnumber1, 'None'),
   rtnumber2 = nullif(rtnumber2, 'None'),
   rtnumber3 = nullif(rtnumber3, 'None'),
   rtnumber4 = nullif(rtnumber4, 'None'),
   rtnumber5 = nullif(rtnumber5, 'None'),
   rtename1fr = nullif(rtename1fr, 'None'),
   rtename2fr = nullif(rtename2fr, 'None'),
   rtename3fr = nullif(rtename3fr, 'None'),
   rtename4fr = nullif(rtename4fr, 'None'),
   rtename1en = nullif(rtename1en, 'None'),
   rtename2en = nullif(rtename2en, 'None'),
   rtename3en = nullif(rtename3en, 'None'),
   rtename4en = nullif(rtename4en, 'None'),
   exitnbr = nullif(exitnbr, 'None'),
   structid = nullif(structid, 'None'),
   structtype = nullif(structtype, 'None'),
   strunameen = nullif(strunameen, 'None'),
   strunamefr = nullif(strunamefr, 'None'),
   l_hnumf = nullif(nullif(l_hnumf, 0), -1),
   l_hnuml = nullif(nullif(l_hnuml, 0), -1),
   r_hnumf = nullif(nullif(r_hnumf, 0), -1),
   r_hnuml = nullif(nullif(r_hnuml, 0), -1),
   l_stname_c = nullif(l_stname_c, 'None'),
   r_stname_c = nullif(r_stname_c, 'None'),
   closing = nullif(closing, 'Unknown'),
   speed = nullif(speed, -1);
-- Query returned successfully: 2412079 rows affected, 93036 ms execution time.

select * from rawdata.roadseg where gid=113;
select * from rawdata.addrange where nid='af526816ff294d8fb189cf463757675a';
select * from rawdata.strplaname where nid='faadfa23e63b4b8ebf3fbebf5233f590';
