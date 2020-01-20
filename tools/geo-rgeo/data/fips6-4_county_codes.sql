-- sqlite3 -init fips6-4_county_codes.sql usps-actual.db
create table fipscodes ( st char(2), fips char(2) not null, fipc char(3) not null, county char(35), primary key (fips,fipc) on conflict abort );
--.separator "\t"
.mode tabs
.import "fips6-4_county_codes.txt" fipscodes
vacuum analyze;
