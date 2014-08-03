/*
sqlite3 src/db/ndns-local.db
.read ndns-db-data-demo.sql
*/
delete from rrs;
delete from rrsets;
delete from zones;

insert into zones (id, name) values 
                  (1, "/"),   
                  (2, "/net"),
                  (3, "/net/ndnsim"),
                  (4, "/dns/google/com"),
                  (5, "/net/ndnsim/git/doc"),
                  
                  (6, "/com"),
                  (7, "/com/skype")
                  ;

insert into rrsets (id, zone_id, label, class, type, ndndata) values
                  /* NS of sub-domain */
                   (4,   1,     "/net",  NULL,   "NS", NULL),
                   (3,   2,     "/ndnsim", NULL, "NS", NULL),
                   (7,   3,     "/git/doc", NULL, "NS", NULL),
                   
                    /* FH of NS (name servers) */
                   (21,  1,     "/net/ns1", NULL, "FH", NULL),
                   (18,  2,     "/net/ndnsim/ns1", NULL, "FH", NULL),   
                   (19,  3,     "/net/ndnsim/git/doc/ns1", NULL, "FH", NULL),
              
                   /* TXT of service/app*/
                   (1,   3,     "/www", NULL, "TXT", NULL),
                   (2,   3,     "/doc", NULL, "TXT", NULL),
                   (6,   3,     "/git/www", NULL, "TXT", NULL),
                   (9,   3,     "/repo/www", NULL, "TXT", NULL),
                   (8,   5,     "/www", NULL, "TXT", NULL),
                   (10,  3,     "/norrdata", NULL, "TXT", NULL),
           
                   /* FH of physical hosts, which is not named by hierarchical */
                   (25,  3,     "/net/ndnsim/www/h1",   NULL, "FH", NULL),
                   (26,  3,     "/net/ndnsim/h1",   NULL, "FH", NULL),
                   (27,  3,     "/net/ndnsim/h2",   NULL, "FH", NULL),
                   (28,  5,     "/net/ndnsim/git/doc/h1", NULL, "FH", NULL),
                   
                   /* NDNCERT of domain */
                   (11,  3,     "/www/dsk-1", NULL, "NDNCERT", NULL),
                   (12,  3,     "/www/ksk-1", NULL, "NDNCERT", NULL),
                   (13,  3,     "/dsk-1",     NULL, "NDNCERT", NULL),
                   (14,  2,     "/ndnsim/ksk-1", NULL, "NDNCERT", NULL),
                   (15,  2,     "/dsk-1", NULL, "NDNCERT", NULL),
                   (16,  1,     "/net/ksk-1", NULL, "NDNCERT", NULL),
                   (17,  1,     "/root",     NULL, "NDNCERT", NULL),
                   
                   /* Mobility Support for Skype (NDN rtc in the future) */
                   (100,  1,    "/com",         NULL, "NS", NULL),
                   (101,  6,    "/skype",   NULL, "NS", NULL),
                   (102,  1,    "/com/ns1",     NULL, "FH", NULL),
                   (103,  6,    "/com/skype/ns1", NULL, "FH", NULL),
                   
                   (105,  7,    "/shock",  NULL, "FH",   NULL),
                   (106,  7,    "/alex",   NULL,  "FH",  NULL),
                   (107,  7,    "/lixia",   NULL,  "FH",  NULL),
                   (108,  7,    "/yiding",   NULL,  "FH",  NULL)
                   ;
                   
                   
insert into rrs (id, rrset_id, ttl, rrdata) values
                /* NS */
                (31, 4,  3600, "/net/ns1"),
                (32, 3,  3600, "/net/ndnsim/ns1"),
                (33, 7,  3600, "/net/ndnsim/git/doc/ns1"),
                
                /* FH of NS (name server) */
                (20,  21, 3600, "/sprint"),
                (21,  18, 3600, "/ucla"),
                (22,  19, 3600, "/ucla"), 
                
                /* TXT of service/app */
                (1, 1, 3600, "/net/ndnsim/www/h1"),
                (2, 1, 8000, "/net/ndnsim/h1"),
                (3, 2, 3600, "/net/ndnsim/www/h1"),
                (8, 6, 3000, "/net/ndnsim/h1"),
                (11,9, 3000, "/net/ndnsim/h2"),
                (10,8, 3000, "/net/ndnsim/git/doc/h1"),
                
                /* rrdata of FH of physical hosts, rrdata is FH */
                (25,  25,   3000, "/ucla"),
                (26,  26,   3000, "/ucla"),
                (27,  27,   3000, "/ucla"),
                (28,  28,   3000, "/ucla"),
                
                 /* NDNCERT*/
                (12,11,3000, "/net/ndnsim/DNS/www/dsk-1:010010101101010101"),
                (13,12,3000, "/net/ndnsim/DNS/www/ksk-1:0101111010001111110"),
                (14,13,3000, "/net/ndnsim/DNS/dsk-1:1010110101"),
                (15,14,3000, "/net/DNS/ndnsim/ksk-1:0101010101010111100011010"),
                (16,15,3000, "/net/DNS/dsk-1:01111010101036654646"),
                (17,16,3000, "/DNS/net/ksk-1:43a13f1a3d13a"),
                (18,17,3000, "/KEY/ROOT:44631346541aaaf"),
                
                /* Mobility Support for Skype */
                (100, 100,  3000, "/com/ns1"), 
                (101, 101,  3000, "/com/skype/ns1"),
                (102, 102,  3000, "/bt"),
                (103, 103,  3000, "/verizon"),
                
                (104, 105,  3000, "/ucla"),
                (105, 105,  3000, "/att"),
                (106, 106,  3000, "/ucla"),
                (107, 107,  3000, "/ucla"),
                (108, 108,  3000, "/ucla")
                ;
