Private, public keys, and public key certificates (sqlite3 PIB and file-based TPM files)
are included here to ensure stable security environment to guarantee the exact output for
key-dependent test operations.

If it is necessary to re-generate the environment, remove PIT and TPM files, uncomment and
run ManagementTool/InitPreconfiguredKeys test case and then copy contents of /tmp/.ndn
into this folder.

The following keys are currently pre-configured:

$ HOME=tests/unit/mgmt ndnsec-ls-identity -C
  /
  +->  /ksk-1416974006376
       +->  /KEY/ksk-1416974006376/CERT/%FD%00%00%01I%EA%3Bx%BD
  +->  /dsk-1416974006466
       +->  /KEY/dsk-1416974006466/CERT/%FD%00%00%01I%EA%3By%28

  /ndns-test
  +->  /ndns-test/ksk-1416974006577
       +->  /ndns-test/KEY/ksk-1416974006577/CERT/%FD%00%00%01I%EA%3By%7F
  +->  /ndns-test/dsk-1416974006659
       +->  /ndns-test/KEY/dsk-1416974006659/CERT/%FD%00%00%01I%EA%3Bz%0E


After keys are re-generated, the following actions need to be taken in ManagementTool test suite:

- ManagementToolFixture constructor should initialize new names of certificates
- ExportCertificate, ListAllZones, ListZone, GetRrSet test cases should be fixed to
  reflect the new keys.
