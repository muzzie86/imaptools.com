select * from failures limit 10;
select * from standardize_address('lex', 'gaz', 'rules', '123 AR-9', '')

select count(*) from failures; -- 5,754
select sum(cnt) from failures where name != '';  -- 204,666

with b as ( select *, (standardize_address('lex', 'gaz', 'rules', '123 '||name, '')) as s from failures )
select * from b
 where coalesce((s).house_num,(s).predir,(s).qual,(s).pretype,(s).name,(s).suftype,(s).sufdir) is null and name != ''

-- rows     =  578

with b as ( select *, (standardize_address('lex', 'gaz', 'rules', '123 '||name, '')) as s from failures )
select sum(cnt) from b
 where coalesce((s).house_num,(s).predir,(s).qual,(s).pretype,(s).name,(s).suftype,(s).sufdir) is null and name != ''

-- sum(cnt) = 3309
