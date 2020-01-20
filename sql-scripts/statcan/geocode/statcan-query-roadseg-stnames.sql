select * from roadseg where l_stname_c not in ('None','Unknown')
                    and coalesce(nullif(nullif(l_hnumf, 0), -1),
                    nullif(nullif(l_hnuml, 0), -1),
                    nullif(nullif(r_hnumf, 0), -1),
                    nullif(nullif(r_hnuml, 0), -1)) is not null
limit 5