-- sqlite3 -init usps-actual-init.sql usps-actual.db
create table junk ( zip char(5) constraint zip_pk primary key on conflict ignore, jj text, city char(35), st char(2), typ char(1));
.separator "\t"
.import "usps-20080520-actual.txt" junk
create table zipcode ( zip char(5) constraint zip_pk primary key on conflict ignore, city char(35), st char(2), typ char(1));
insert or ignore into zipcode select zip, city, st, typ from junk;
drop table junk;
vacuum analyze;

