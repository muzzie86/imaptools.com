-- check the column widths and adjust the table definition below

select max(length(name)) as name,
max(length(type)) as type,
max(length(afl_val)) as afl_val,
max(length(atl_val)) as atl_val,
max(length(afr_val)) as afr_val,
max(length(atr_val)) as atr_val,
max(length(csdname_l)) as csdname_l,
max(length(csdname_r)) as csdname_r,
max(length(prname_l)) as prname_l,
max(length(prname_r)) as prname_r
from roadseg;

/*
 name | type | afl_val | atl_val | afr_val | atr_val | csdname_l | csdname_r | prname_l | prname_r
 ------+------+---------+---------+---------+---------+-----------+-----------+----------+----------
      47 |    6 |       6 |       6 |       6 |       6 |        64 |        64 |     51 |       51

streets.name needs to be wider than (name + type + 1)

statcan2019=# select type, count(*) from roadseg group by type order by type;
  type  | count
--------+--------
 ABBEY  |     10
 ACCESS |    210
 ACRES  |    126
 ALLÉE  |    936
 ALLEY  |    195
 AUT    |  10473
 AV     |  41280
 AVE    | 205541
 BAY    |   3546
 BEACH  |     53
 BEND   |    220
 BLVD   |  27497
 BOUL   |  37895
 BRGE   |      5
 BROOK  |      9
 BYPASS |    311
 BYWAY  |      1
 CAPE   |     36
 CAR    |    394
 CARREF |      3
 CDS    |     11
 CERCLE |     80
 CH     |  66972
 CHASE  |     75
 CIR    |   4883
 CIRCT  |     72
 CLOSE  |   4429
 COMMON |    701
 CONC   |    637
 CÔTE   |   1036
 COUR   |     26
 COURS  |     30
 COVE   |   1104
 CRES   |  41813
 CREST  |     45
 CRNRS  |     21
 CROFT  |      1
 CROIS  |   1213
 CROSS  |    266
 CRSSRD |     22
 CRT    |  21954
 CTR    |     72
 DALE   |      7
 DELL   |      4
 DESSTE |    349
 DIVERS |     74
 DOWNS  |     10
 DR     | 137951
 DRPASS |      1
 END    |     37
 ESPL   |     25
 ESTATE |    544
 EXPY   |    414
 EXTEN  |    421
 FARM   |      3
 FIELD  |      5
 FOREST |     13
 FRONT  |     49
 FSR    |   3043
 FWY    |    173
 GATE   |   3016
 GDNS   |   1068
 GLADE  |      4
 GLEN   |    117
 GREEN  |   1144
 GRNDS  |     10
 GROVE  |    857
 HARBR  |     17
 HAVEN  |     19
 HEATH  |    180
 HGHLDS |     19
 HILL   |   1257
 HOLLOW |     86
 HTS    |   1310
 HWY    | 115272
 ÎLE    |     14
 IMP    |    717
 INLET  |      1
 ISLAND |    548
 KEY    |     28
 KNOLL  |     10
 LANDNG |    653
 LANE   |  34135
 LANEWY |      6
 LINE   |  11254
 LINK   |    724
 LKOUT  |     26
 LMTS   |      1
 LOOP   |    868
 MALL   |    168
 MANOR  |    810
 MAZE   |      4
 MEADOW |     61
 MEWS   |    694
 MONTÉE |   4157
 MOUNT  |     59
 MTN    |     21
 ORCH   |      5
 PARADE |     86
 PARC   |     13
 PASS   |    145
 PATH   |    469
 PEAK   |      3
 PINES  |      4
 PK     |   1470
 PKY    |   4418
 PL     |  23169
 PLACE  |   5275
 PLAT   |      7
 PLAZA  |     62
 POINTE |     23
 PORT   |      4
 PROM   |    573
 PT     |   1108
 PTWAY  |     61
 PVT    |   1262
 QUAI   |     13
 QUAY   |     97
 RAMP   |     19
 RANG   |  14404
 RD     | 342300
 REACH  |     27
 RG     |     16
 RIDGE  |    549
 RISE   |    716
 RLE    |    289
 ROUTE  |  26008
 ROW    |    491
 RTE    |   2647
 RTOFWY |      1
 RUE    | 219634
 RUN    |    351
 SECTN  |      1
 SENT   |    197
 SIDERD |   3718
 SQ     |   1197
 ST     | 286522
 STROLL |     15
 SUBDIV |    208
 TERR   |   3189
 TLINE  |    558
 TRACE  |      2
 TRAIL  |  10314
 TRNABT |      6
 TRUNK  |      2
 TSSE   |    738
 VALE   |     10
 VIA    |     12
 VIEW   |    793
 VILLAS |    306
 VILLGE |    215
 VISTA  |     36
 VOIE   |     45
 WALK   |    535
 WAY    |  18893
 WHARF  |     19
 WOOD   |     44
 WYND   |    309
        | 462875
(159 rows)

statcan2019=# select class, count(*) from roadseg group by class order by class;
 class |  count
-------+---------
 10    |   12081
 11    |    9475
 12    |   81171
 13    |    2221
 20    |   23756
 21    |  110787
 22    |  280315
 23    | 1567064
 24    |    1161
 25    |   33306
 26    |   55706
 27    |     196
 29    |   23661
 80    |    1519
 87    |     598
 90    |    9822
 95    |     798
       |   19503
(18 rows)



*/


DROP TABLE IF EXISTS streets CASCADE;

CREATE TABLE streets
(
  gid serial NOT NULL,
  link_id numeric(10,0),
  name character varying(80),       -- name || ' ' || type
  l_refaddr numeric(10,0),          -- afl_val::numeric
  l_nrefaddr numeric(10,0),         -- atl_val::numeric
  l_sqlnumf character varying(12),  -- 
  l_sqlfmt character varying(16),   -- 
  l_cformat character varying(16),  -- 
  r_refaddr numeric(10,0),          -- afr_val::numeric
  r_nrefaddr numeric(10,0),         -- atr_val::numeric
  r_sqlnumf character varying(12),  -- 
  r_sqlfmt character varying(16),   -- 
  r_cformat character varying(16),  -- 
  l_ac5 character varying(65),      -- city|town|village   -- csdname_l
  l_ac4 character varying(50),      -- county subdivision
  l_ac3 character varying(50),      -- county
  l_ac2 character varying(55),      -- province             -- prname_l
  l_ac1 character varying(35),      -- country code         -- 'CA'
  r_ac5 character varying(65),      -- city|town|village   -- csdname_r
  r_ac4 character varying(50),      -- county subdivision
  r_ac3 character varying(50),      -- county
  r_ac2 character varying(55),      -- province             -- prname_r
  r_ac1 character varying(35),      -- country code         -- 'CA'
  l_postcode character varying(11), -- not provided with statcan
  r_postcode character varying(11), -- not provided with statcan
  mtfcc integer,                    -- road class (as an integer)
  passflg character varying(1),     -- ignore
  divroad character varying(1),     -- ignore
  the_geom geometry NOT NULL,
  CONSTRAINT streets_pkey PRIMARY KEY (gid),
  CONSTRAINT enforce_dims_the_geom CHECK (st_ndims(the_geom) = 2),
  CONSTRAINT enforce_geotype_the_geom CHECK (geometrytype(the_geom) = 'MULTILINESTRING'::text OR the_geom IS NULL),
  CONSTRAINT enforce_srid_the_geom CHECK (st_srid(the_geom) = 4326)
)
WITH (
  OIDS=FALSE
);

insert into streets (
    link_id,
    name,
    l_refaddr,
    l_nrefaddr,
    r_refaddr,
    r_nrefaddr,
    l_ac5,
    l_ac2,
    l_ac1,
    r_ac5,
    r_ac2,
    r_ac1,
    mtfcc,
    the_geom
)
select 
    ngd_uid::numeric,
    name || ' ' || type,
    afl_val::numeric,
    atl_val::numeric,
    afr_val::numeric,
    atr_val::numeric,
    csdname_l,
    prname_l,
    'CA'::text,
    csdname_r,
    prname_r,
    'CA'::text,
    (1000 + class::integer)::integer,
    st_multi(geom)
  from roadseg;
