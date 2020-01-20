select std.*
  from as_config cfg,
--       LATERAL as_parse(
       LATERAL as_match(
--       LATERAL as_standardize(
            '123 Windwood Grove South-west Airdrie Alberta',
            grammar,
            clexicon,
            'fr_CA',
            'SPACE,PUNCT,DASH,STOPWORD'
        ) as std
 where cfg.id=26;