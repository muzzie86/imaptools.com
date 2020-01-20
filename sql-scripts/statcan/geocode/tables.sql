-- Table: rawdata.roadseg

-- DROP TABLE rawdata.roadseg;

CREATE TABLE rawdata.roadseg
(
    gid integer NOT NULL,
    nid character varying(32) COLLATE pg_catalog."default",
    roadsegid integer,
    adrangenid character varying(32) COLLATE pg_catalog."default",
    datasetnam character varying(25) COLLATE pg_catalog."default",
    specvers character varying(100) COLLATE pg_catalog."default",
    accuracy smallint,
    acqtech character varying(23) COLLATE pg_catalog."default",
    provider character varying(24) COLLATE pg_catalog."default",
    credate character varying(8) COLLATE pg_catalog."default",
    revdate character varying(8) COLLATE pg_catalog."default",
    metacover character varying(8) COLLATE pg_catalog."default",
    roadclass character varying(21) COLLATE pg_catalog."default",
    rtnumber1 character varying(10) COLLATE pg_catalog."default",
    rtnumber2 character varying(10) COLLATE pg_catalog."default",
    rtnumber3 character varying(10) COLLATE pg_catalog."default",
    rtnumber4 character varying(10) COLLATE pg_catalog."default",
    rtnumber5 character varying(10) COLLATE pg_catalog."default",
    rtename1fr character varying(100) COLLATE pg_catalog."default",
    rtename2fr character varying(100) COLLATE pg_catalog."default",
    rtename3fr character varying(100) COLLATE pg_catalog."default",
    rtename4fr character varying(100) COLLATE pg_catalog."default",
    rtename1en character varying(100) COLLATE pg_catalog."default",
    rtename2en character varying(100) COLLATE pg_catalog."default",
    rtename3en character varying(100) COLLATE pg_catalog."default",
    rtename4en character varying(100) COLLATE pg_catalog."default",
    exitnbr character varying(10) COLLATE pg_catalog."default",
    nbrlanes smallint,
    pavstatus character varying(7) COLLATE pg_catalog."default",
    pavsurf character varying(8) COLLATE pg_catalog."default",
    unpavsurf character varying(7) COLLATE pg_catalog."default",
    structid character varying(32) COLLATE pg_catalog."default",
    structtype character varying(15) COLLATE pg_catalog."default",
    strunameen character varying(100) COLLATE pg_catalog."default",
    strunamefr character varying(100) COLLATE pg_catalog."default",
    l_adddirfg character varying(18) COLLATE pg_catalog."default",
    l_hnumf integer,
    l_hnuml integer,
    l_stname_c character varying(100) COLLATE pg_catalog."default",
    l_placenam character varying(100) COLLATE pg_catalog."default",
    r_adddirfg character varying(18) COLLATE pg_catalog."default",
    r_hnumf integer,
    r_hnuml integer,
    r_stname_c character varying(100) COLLATE pg_catalog."default",
    r_placenam character varying(100) COLLATE pg_catalog."default",
    closing character varying(7) COLLATE pg_catalog."default",
    roadjuris character varying(100) COLLATE pg_catalog."default",
    speed smallint,
    trafficdir character varying(18) COLLATE pg_catalog."default",
    geom geometry(MultiLineString,4617)
)
WITH (
    OIDS = FALSE
)
TABLESPACE pg_default;


-- Table: rawdata.altnamlink

-- DROP TABLE rawdata.altnamlink;

CREATE TABLE rawdata.altnamlink
(
    gid integer NOT NULL,
    datasetnam character varying(25) COLLATE pg_catalog."default",
    specvers character varying(100) COLLATE pg_catalog."default",
    credate character varying(8) COLLATE pg_catalog."default",
    revdate character varying(8) COLLATE pg_catalog."default",
    nid character varying(32) COLLATE pg_catalog."default",
    strnamenid character varying(32) COLLATE pg_catalog."default",
    CONSTRAINT altnamlink_pkey PRIMARY KEY (nid)
)
WITH (
    OIDS = FALSE
)
TABLESPACE pg_default;


-- Table: rawdata.addrange

-- DROP TABLE rawdata.addrange;

CREATE TABLE rawdata.addrange
(
    gid integer NOT NULL,
    nid character varying(32) COLLATE pg_catalog."default",
    datasetnam character varying(25) COLLATE pg_catalog."default",
    specvers character varying(100) COLLATE pg_catalog."default",
    accuracy smallint,
    acqtech character varying(23) COLLATE pg_catalog."default",
    provider character varying(24) COLLATE pg_catalog."default",
    credate character varying(8) COLLATE pg_catalog."default",
    revdate character varying(8) COLLATE pg_catalog."default",
    metacover character varying(8) COLLATE pg_catalog."default",
    l_digdirfg character varying(18) COLLATE pg_catalog."default",
    l_hnumstr character varying(9) COLLATE pg_catalog."default",
    l_rfsysind character varying(18) COLLATE pg_catalog."default",
    l_hnumtypf character varying(16) COLLATE pg_catalog."default",
    l_hnumf integer,
    l_hnumsuff character varying(20) COLLATE pg_catalog."default",
    l_hnumtypl character varying(16) COLLATE pg_catalog."default",
    l_hnuml integer,
    l_hnumsufl character varying(20) COLLATE pg_catalog."default",
    l_offnanid character varying(32) COLLATE pg_catalog."default",
    l_altnanid character varying(32) COLLATE pg_catalog."default",
    r_digdirfg character varying(18) COLLATE pg_catalog."default",
    r_hnumstr character varying(9) COLLATE pg_catalog."default",
    r_rfsysind character varying(18) COLLATE pg_catalog."default",
    r_hnumtypf character varying(16) COLLATE pg_catalog."default",
    r_hnumf integer,
    r_hnumsuff character varying(20) COLLATE pg_catalog."default",
    r_hnumtypl character varying(16) COLLATE pg_catalog."default",
    r_hnuml integer,
    r_hnumsufl character varying(20) COLLATE pg_catalog."default",
    r_offnanid character varying(32) COLLATE pg_catalog."default",
    r_altnanid character varying(32) COLLATE pg_catalog."default",
    CONSTRAINT addrange_pkey PRIMARY KEY (nid)
)
WITH (
    OIDS = FALSE
)
TABLESPACE pg_default;

-- Table: rawdata.strplaname

-- DROP TABLE rawdata.strplaname;

CREATE TABLE rawdata.strplaname
(
    gid integer NOT NULL,
    nid character varying(32) COLLATE pg_catalog."default",
    datasetnam character varying(25) COLLATE pg_catalog."default",
    specvers character varying(100) COLLATE pg_catalog."default",
    accuracy smallint,
    acqtech character varying(23) COLLATE pg_catalog."default",
    provider character varying(24) COLLATE pg_catalog."default",
    credate character varying(8) COLLATE pg_catalog."default",
    revdate character varying(8) COLLATE pg_catalog."default",
    metacover character varying(8) COLLATE pg_catalog."default",
    dirprefix character varying(10) COLLATE pg_catalog."default",
    strtypre character varying(30) COLLATE pg_catalog."default",
    starticle character varying(20) COLLATE pg_catalog."default",
    namebody character varying(50) COLLATE pg_catalog."default",
    strtysuf character varying(30) COLLATE pg_catalog."default",
    dirsuffix character varying(10) COLLATE pg_catalog."default",
    muniquad character varying(10) COLLATE pg_catalog."default",
    placename character varying(100) COLLATE pg_catalog."default",
    province character varying(25) COLLATE pg_catalog."default",
    placetype character varying(100) COLLATE pg_catalog."default",
    CONSTRAINT strplaname_pkey PRIMARY KEY (nid)
)
WITH (
    OIDS = FALSE
)
TABLESPACE pg_default;


