

create or replace function mk_statcan_lex()
returns text
as $body$
declare
  lex text;
  lext text;
  inclass text;
  attachment text;

begin

  lex := 'LEXICON:\tlex-statcan.txt\tFRE\tfr_CA\n';

  -- starticle
  inclass := 'STOPWORD';
  attachment := 'DETACH';

  select string_agg('LEXENTRY:\t'||item||'\t'||item||'\t'||inclass||'\t'||attachment, '\n') into lext
    from ( select distinct upper(starticle) as item from strplaname order by upper(starticle) ) as foo;

  lex := lex || lext || '\n';

  -- dirprefix
  -- dirsuffix
  inclass := 'DIRECT';
  attachment := 'DETACH';

  select string_agg('LEXENTRY:\t'||item||'\t'||item||'\t'||inclass||'\t'||attachment, '\n') into lext
    from (
      select distinct item from (
        select distinct upper(dirprefix) as item from strplaname
        union
        select distinct upper(dirsuffix) as item from strplaname
      ) as bar order by item
    ) as foo;

  lex := lex || lext || '\n';

  -- strtypre
  inclass := 'TYPE,WORD';
  attachment := 'DET_PRE';

  select string_agg('LEXENTRY:\t'||item||'\t'||item||'\t'||inclass||'\t'||attachment, '\n') into lext
    from ( select distinct upper(strtypre) as item from strplaname order by upper(strtypre) ) as foo;

  lex := lex || lext || '\n';

  -- strtysuf
  inclass := 'TYPE,WORD';
  attachment := 'DET_SUF';

  select string_agg('LEXENTRY:\t'||item||'\t'||item||'\t'||inclass||'\t'||attachment, '\n') into lext
    from ( select distinct upper(strtysuf) as item from strplaname order by upper(strtysuf) ) as foo;

  lex := lex || lext || '\n';

  -- province
  inclass := 'PROV,WORD';
  attachment := 'DETACH';

  select string_agg('LEXENTRY:\t'||item||'\t'||item||'\t'||inclass||'\t'||attachment, '\n') into lext
    from ( select distinct upper(province) as item from strplaname order by upper(province) ) as foo;

  lex := lex || lext || '\n';

  return lex;
end;
$body$
language 'plpgsql';
