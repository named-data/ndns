
delete from rrs;
delete from rrsets;
delete from zones;

insert into zones (id, name) values 
                  (1, "/"),   
                  (2, "/net"),
                  (3, "/net/ndnsim"),
                  (4, "/dns/google/com"),
                  (5, "/net/ndnsim/git/doc")
                  ;

insert into rrsets (id, zone_id, label, class, type, ndndata) values
                   (1,   3,     "/www", NULL, "TXT", NULL),
                   (2,   3,     "/doc", NULL, "TXT", NULL),
                   (3,   2,     "/ndnsim", NULL, "NS", NULL),
                   (4,   1,     "/net",  NULL,   "NS", NULL),
                   (5,   4,     "/net",  NULL,   "NS", NULL),
                   (6,   3,     "/git/www", NULL, "TXT", NULL),
                   (7,   3,     "/git/doc", NULL, "NS", NULL),
                   (8,   5,     "/www", NULL, "TXT", NULL),
                   (9,   3,     "/repo/www", NULL, "TXT", NULL),
                   (10,  3,     "/norrdata", NULL, "TXT", NULL)
                   ;
                   
                   
insert into rrs (id, rrset_id, ttl, rrdata) values
                (1, 1, 3600, "/ucla"),
                (2, 1, 8000, "/att"),
                (3, 2, 3600, "/ucla"),
                (4, 3, 4000, "/net/ndnsim2"),
                (5, 4, 5000, "/net"),
                (6, 5, 6000, "/net"),
                (7, 3, 4000, "/net/ndnsim"),
                (8, 6, 3000, "/net/ndnsim/git/www"),
                (9, 7, 3000, "/net/ndnsim/git/doc"),
                (10,8, 3000, "/net/ndnsim/git/doc/www"),
                (11,9, 3000, "/net/ndnsim/repo/www")
                ;
