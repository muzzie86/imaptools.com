
-- placename patterns

select sum(cnt) as sum, inclass from (
    select name, cnt, string_agg(word, '|' order by seq) as words, string_agg(inclass, '|' order by seq) as inclass
    from (
            select clean_placenames(l_placenam) as name, count(*) as cnt
            from rawdata.roadseg where l_placenam not in ('None','Unknown') group by name
        ) as a,
        as_config as cfg,
        LATERAL as_parse( name, clexicon, 'fr_CA', 'SPACE,STOPWORD,DASH,PUNCT') as std
    where cfg.id=26
    group by name, cnt
) as foo
group by inclass
order by sum desc;


-- street name patterns

select sum(cnt) as sum, inclass from (
    select name, cnt, string_agg(word, '|' order by seq) as words, string_agg(inclass, '|' order by seq) as inclass
    from (
            select l_stname_c as name, count(*) as cnt
            from rawdata.roadseg where l_stname_c not in ('None','Unknown') group by name
        ) as a,
        as_config as cfg,
        LATERAL as_parse( name, clexicon, 'fr_CA', 'SPACE,STOPWORD,DASH,PUNCT') as std
    where cfg.id=26
    group by name, cnt
) as foo
group by inclass
order by sum desc
