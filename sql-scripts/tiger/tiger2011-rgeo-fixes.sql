select count(*) from streets where l_ac4='Immokalee' or r_ac4='Immokalee'; -- 5396
select count(*) from streets where l_postcode='34110' or r_postcode='34110'; -- 837
select * from streets where l_postcode='34110' or r_postcode='34110'; -- 837

update streets set l_ac5='NAPLES', l_postcode='34110' where l_postcode is null and r_postcode='34110'; -- 114
update streets set r_ac5='NAPLES', r_postcode='34110' where r_postcode is null and l_postcode='34110'; -- 127

select * from imt_reverse_geocode_flds(-81.746440000000007,26.281540000000000)