create table junk (
    zip char(5) not null primary key,
    jj text,
    city char(35),
    st char(2),
    typ char(1)
);
copy junk from '/home/woodbri/dev/navteq/data/usps-20080520-actual.txt';
create table zipcode (
    zip char(5) not null primary key,
    city varchar(35),
    st char(2),
    typ char(1)
);
insert into zipcode select zip, city, st, typ from junk;
drop table junk;
vacuum analyze zipcode;

