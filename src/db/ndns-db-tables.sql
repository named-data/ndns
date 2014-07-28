/*
some difference with Alex's sql file:
1. class field in table rrsets could be NULL
2. type field in table rrsets is changed to text from integer(10)
*/
DROP TRIGGER IF EXISTS rrs_update;
DROP TABLE IF EXISTS zones;
DROP TABLE IF EXISTS rrsets;
DROP TABLE IF EXISTS rrs;
CREATE TABLE zones (
  id    INTEGER NOT NULL PRIMARY KEY, 
  name blob NOT NULL UNIQUE);
CREATE TABLE rrsets (
  id       INTEGER NOT NULL PRIMARY KEY, 
  zone_id integer(10) NOT NULL, 
  label   text NOT NULL, 
  class   integer(10), 
  type    text NOT NULL, 
  ndndata blob, 
  FOREIGN KEY(zone_id) REFERENCES zones(id) ON UPDATE Cascade ON DELETE Cascade);
CREATE TABLE rrs (
  id        INTEGER NOT NULL PRIMARY KEY, 
  rrset_id integer(10) NOT NULL, 
  ttl      integer(10) NOT NULL, 
  rrdata   blob NOT NULL, 
  FOREIGN KEY(rrset_id) REFERENCES rrsets(id) ON UPDATE Cascade ON DELETE Cascade);
CREATE UNIQUE INDEX rrsets_zone_id_label_class_type 
  ON rrsets (zone_id, label, class, type);
CREATE INDEX rrs_rrset_id 
  ON rrs (rrset_id);
CREATE INDEX rrs_rrset_id_rrdata 
  ON rrs (rrset_id, rrdata);
CREATE TRIGGER rrs_update
BEFORE INSERT ON rrs
FOR EACH ROW
BEGIN
    DELETE FROM rrs WHERE rrset_id = NEW.rrset_id AND rrdata = NEW.rrdata;
END;

