  select std.*
    from as_config as cfg,
         LATERAL as_parse( 'Pincher Creek No. 9', clexicon, 'fr_CA', 'SPACE,STOPWORD,DASH,PUNCT') as std
    where cfg.id=26
   
