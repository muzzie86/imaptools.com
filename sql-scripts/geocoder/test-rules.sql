drop table if exists rules cascade;
create table rules (
    id serial not null primary key,
    rule text
);
copy rules (rule) from stdin;
0 6 0 13 0 -1 1 4 5 5 5 -1 1 10
0 6 0 13 0 22 -1 1 4 5 5 5 7 -1 1 10
0 6 0 18 13 0 18 -1 1 4 5 5 5 5 5 -1 1 15
0 6 0 18 13 0 18 22 -1 1 4 5 5 5 5 5 7 -1 1 15
0 6 0 22 13 0 22 -1 1 4 5 5 5 5 5 -1 1 15
0 6 0 22 13 0 22 22 -1 1 4 5 5 5 5 5 7 -1 1 15
0 6 23 13 23 -1 1 4 5 5 5 -1 1 16
0 6 23 13 23 22 -1 1 4 5 5 5 7 -1 1 16
-1
\.
