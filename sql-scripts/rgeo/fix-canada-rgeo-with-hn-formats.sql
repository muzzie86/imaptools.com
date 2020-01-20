set search_path = data, public;
begin;
update streets set l_refaddr=l_nrefaddr where l_refaddr is null and l_nrefaddr is not null;
update streets set l_nrefaddr=l_refaddr where l_nrefaddr is null and l_refaddr is not null;
update streets set r_refaddr=r_nrefaddr where r_refaddr is null and r_nrefaddr is not null;
update streets set r_nrefaddr=r_refaddr where r_nrefaddr is null and r_refaddr is not null;


update streets set l_sqlnumf='FM'||repeat('9',case when length(l_refaddr::text)>length(l_nrefaddr::text) then length(l_refaddr::text) else length(l_nrefaddr::text) end),
                   l_sqlfmt='{}',
                   l_cformat='%'||case when length(l_refaddr::text)>length(l_nrefaddr::text) then length(l_refaddr::text) else length(l_nrefaddr::text) end||'d',
                   r_sqlnumf='FM'||repeat('9',case when length(r_refaddr::text)>length(r_nrefaddr::text) then length(r_refaddr::text) else length(r_nrefaddr::text) end),
                   r_sqlfmt='{}',
                   r_cformat='%'||case when length(r_refaddr::text)>length(r_nrefaddr::text) then length(r_refaddr::text) else length(r_nrefaddr::text) end||'d'
 where l_refaddr is not null or r_refaddr is not null or l_nrefaddr is not null or r_nrefaddr is not null;

commit;