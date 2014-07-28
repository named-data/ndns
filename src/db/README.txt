#how to create the database
pre-requirements:
1. sqlite3 installed

Steps to install and reset the database:
1. run shell and go to the src/db directory as working directory 
2. run command sqlite3 ndns-local.db on shell and thus, enter the sqlit3 shell program
3. run .read ndns-database-tables.sql, which create the tables
4. run .read ndns-database-data-demo.sql, which insert demo data into database


