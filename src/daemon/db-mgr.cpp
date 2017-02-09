/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2017, Regents of the University of California.
 *
 * This file is part of NDNS (Named Data Networking Domain Name Service).
 * See AUTHORS.md for complete list of NDNS authors and contributors.
 *
 * NDNS is free software: you can redistribute it and/or modify it under the terms
 * of the GNU General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * NDNS is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * NDNS, e.g., in COPYING.md file.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "db-mgr.hpp"
#include "logger.hpp"
#include "clients/response.hpp"

#include <iostream>
#include <fstream>

namespace ndn {
namespace ndns {

NDNS_LOG_INIT("DbMgr")

static const std::string NDNS_SCHEMA = R"VALUE(
CREATE TABLE IF NOT EXISTS zones (
  id    INTEGER NOT NULL PRIMARY KEY,
  name  blob NOT NULL UNIQUE,
  ttl   integer(10) NOT NULL);

CREATE TABLE IF NOT EXISTS zone_info (
  zone_id  INTEGER NOT NULL,
  key      VARCHAR(10) NOT NULL,
  value    blob NOT NULL,
  PRIMARY KEY (zone_id, key),
  FOREIGN KEY(zone_id) REFERENCES zones(id) ON UPDATE Cascade ON DELETE Cascade);

CREATE TABLE IF NOT EXISTS rrsets (
  id      INTEGER NOT NULL PRIMARY KEY,
  zone_id integer(10) NOT NULL,
  label   blob NOT NULL,
  type    blob NOT NULL,
  version blob NOT NULL,
  ttl     integer(10) NOT NULL,
  data    blob NOT NULL,
  FOREIGN KEY(zone_id) REFERENCES zones(id) ON UPDATE Cascade ON DELETE Cascade);

CREATE UNIQUE INDEX rrsets_zone_id_label_type_version
  ON rrsets (zone_id, label, type, version);
)VALUE";

DbMgr::DbMgr(const std::string& dbFile/* = DEFAULT_CONFIG_PATH "/" "ndns.db"*/)
  : m_dbFile(dbFile)
  , m_conn(0)
{
  if (dbFile.empty())
    m_dbFile = DEFAULT_DATABASE_PATH "/" "ndns.db";

  this->open();

  NDNS_LOG_INFO("open database: " << m_dbFile);
}


DbMgr::~DbMgr()
{
  if (m_conn != 0) {
    this->close();
  }
}

void
DbMgr::open()
{
  int res = sqlite3_open_v2(m_dbFile.c_str(), &m_conn,
                            SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE,
#ifdef DISABLE_SQLITE3_FS_LOCKING
                            "unix-dotfile"
#else
                            0
#endif
                            );

  if (res != SQLITE_OK) {
    NDNS_LOG_FATAL("Cannot open the db file: " << m_dbFile);
    BOOST_THROW_EXCEPTION(ConnectError("Cannot open the db file: " + m_dbFile));
  }
  // ignore any errors from DB creation (command will fail for the existing database, which is ok)
  sqlite3_exec(m_conn, NDNS_SCHEMA.c_str(), 0, 0, 0);
}

void
DbMgr::close()
{
  if (m_conn == 0)
    return;

  int ret = sqlite3_close(m_conn);
  if (ret != SQLITE_OK) {
    NDNS_LOG_FATAL("Cannot close the db: " << m_dbFile);
  }
  else {
    m_conn = 0;
    NDNS_LOG_INFO("Close database: " << m_dbFile);
  }
}

void
DbMgr::clearAllData()
{
  const char* sql = "DELETE FROM zones; DELETE FROM rrsets;";

  int rc = sqlite3_exec(m_conn, sql, 0, 0, 0); // sqlite3_step cannot execute multiple SQL statement
  if (rc != SQLITE_OK) {
    BOOST_THROW_EXCEPTION(ExecuteError(sql));
  }

  NDNS_LOG_INFO("clear all the data in the database: " << m_dbFile);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Zone
///////////////////////////////////////////////////////////////////////////////////////////////////

void
DbMgr::insert(Zone& zone)
{
  if (zone.getId() > 0)
    return;

  sqlite3_stmt* stmt;
  const char* sql = "INSERT INTO zones (name, ttl) VALUES (?, ?)";
  int rc = sqlite3_prepare_v2(m_conn, sql, -1, &stmt, 0);
  if (rc != SQLITE_OK) {
    BOOST_THROW_EXCEPTION(PrepareError(sql));
  }

  const Block& zoneName = zone.getName().wireEncode();
  sqlite3_bind_blob(stmt, 1, zoneName.wire(), zoneName.size(), SQLITE_STATIC);
  sqlite3_bind_int(stmt,  2, zone.getTtl().count());

  rc = sqlite3_step(stmt);
  if (rc != SQLITE_DONE) {
    sqlite3_finalize(stmt);
    BOOST_THROW_EXCEPTION(ExecuteError(sql));
  }

  zone.setId(sqlite3_last_insert_rowid(m_conn));
  sqlite3_finalize(stmt);
}

void
DbMgr::setZoneInfo(Zone& zone,
                   const std::string& key,
                   const Block& value)
{
  if (zone.getId() == 0) {
    BOOST_THROW_EXCEPTION(Error("zone has not been initialized"));
  }

  if (key.length() > 10) {
    BOOST_THROW_EXCEPTION(Error("key length should not exceed 10"));
  }

  sqlite3_stmt* stmt;
  const char* sql = "INSERT OR REPLACE INTO zone_info (zone_id, key, value) VALUES (?, ?, ?)";
  int rc = sqlite3_prepare_v2(m_conn, sql, -1, &stmt, 0);
  if (rc != SQLITE_OK) {
    BOOST_THROW_EXCEPTION(PrepareError(sql));
  }

  sqlite3_bind_int(stmt,  1, zone.getId());
  sqlite3_bind_text(stmt, 2, key.c_str(),  key.length(), SQLITE_STATIC);
  sqlite3_bind_blob(stmt, 3, value.wire(), value.size(), SQLITE_STATIC);

  rc = sqlite3_step(stmt);
  if (rc != SQLITE_DONE) {
    sqlite3_finalize(stmt);
    BOOST_THROW_EXCEPTION(ExecuteError(sql));
  }

  sqlite3_finalize(stmt);
}

std::map<std::string, Block>
DbMgr::getZoneInfo(Zone& zone)
{
  using std::string;
  std::map<string, Block> rtn;

  if (zone.getId() == 0) {
    find(zone);
  }

  if (zone.getId() == 0) {
    BOOST_THROW_EXCEPTION(Error("zone has not been initialized"));
  }

  sqlite3_stmt* stmt;
  const char* sql = "SELECT key, value FROM zone_info WHERE zone_id=?";
  int rc = sqlite3_prepare_v2(m_conn, sql, -1, &stmt, 0);
  if (rc != SQLITE_OK) {
    BOOST_THROW_EXCEPTION(PrepareError(sql));
  }

  sqlite3_bind_int(stmt, 1, zone.getId());

  while (sqlite3_step(stmt) == SQLITE_ROW) {
    const char* key = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
    rtn[string(key)] = Block(static_cast<const uint8_t*>(sqlite3_column_blob(stmt, 1)),
                             sqlite3_column_bytes(stmt, 1));
  }

  sqlite3_finalize(stmt);
  return rtn;
}


bool
DbMgr::find(Zone& zone)
{
  sqlite3_stmt* stmt;
  const char* sql = "SELECT id, ttl FROM zones WHERE name=?";
  int rc = sqlite3_prepare_v2(m_conn, sql, -1, &stmt, 0);
  if (rc != SQLITE_OK) {
    BOOST_THROW_EXCEPTION(PrepareError(sql));
  }

  const Block& zoneName = zone.getName().wireEncode();
  sqlite3_bind_blob(stmt, 1, zoneName.wire(), zoneName.size(), SQLITE_STATIC);

  if (sqlite3_step(stmt) == SQLITE_ROW) {
    zone.setId(sqlite3_column_int64(stmt, 0));
    zone.setTtl(time::seconds(sqlite3_column_int(stmt, 1)));
  }
  else {
    zone.setId(0);
  }

  sqlite3_finalize(stmt);

  return zone.getId() != 0;
}

std::vector<Zone>
DbMgr::listZones()
{
  sqlite3_stmt* stmt;
  const char* sql = "SELECT id, name, ttl FROM zones";
  int rc = sqlite3_prepare_v2(m_conn, sql, -1, &stmt, 0);
  if (rc != SQLITE_OK) {
    BOOST_THROW_EXCEPTION(PrepareError(sql));
  }

  std::vector<Zone> vec;

  while (sqlite3_step(stmt) == SQLITE_ROW) {
    vec.emplace_back();
    Zone& zone = vec.back();
    zone.setId(sqlite3_column_int64(stmt, 0));
    zone.setTtl(time::seconds(sqlite3_column_int(stmt, 2)));
    zone.setName(Name(Block(static_cast<const uint8_t*>(sqlite3_column_blob(stmt, 1)),
                            sqlite3_column_bytes(stmt, 1))));
  }
  sqlite3_finalize(stmt);

  return vec;
}

void
DbMgr::remove(Zone& zone)
{
  if (zone.getId() == 0)
    return;

  sqlite3_stmt* stmt;
  const char* sql = "DELETE FROM zones where id=?";
  int rc = sqlite3_prepare_v2(m_conn, sql, -1, &stmt, 0);
  if (rc != SQLITE_OK) {
    BOOST_THROW_EXCEPTION(PrepareError(sql));
  }

  sqlite3_bind_int64(stmt, 1, zone.getId());

  rc = sqlite3_step(stmt);
  if (rc != SQLITE_DONE) {
    sqlite3_finalize(stmt);
    BOOST_THROW_EXCEPTION(ExecuteError(sql));
  }

  sqlite3_finalize(stmt);

  zone = Zone();
}


///////////////////////////////////////////////////////////////////////////////////////////////////
// Rrset
///////////////////////////////////////////////////////////////////////////////////////////////////

void
DbMgr::insert(Rrset& rrset)
{
  if (rrset.getId() != 0)
    return;

  if (rrset.getZone() == 0) {
    BOOST_THROW_EXCEPTION(RrsetError("Rrset has not been assigned to a zone"));
  }

  if (rrset.getZone()->getId() == 0) {
    insert(*rrset.getZone());
  }

  const char* sql =
    "INSERT INTO rrsets (zone_id, label, type, version, ttl, data)"
    "    VALUES (?, ?, ?, ?, ?, ?)";

  sqlite3_stmt* stmt;
  int rc = sqlite3_prepare_v2(m_conn, sql, -1, &stmt, 0);
  if (rc != SQLITE_OK) {
    BOOST_THROW_EXCEPTION(PrepareError(sql));
  }

  sqlite3_bind_int64(stmt, 1, rrset.getZone()->getId());

  const Block& label = rrset.getLabel().wireEncode();
  sqlite3_bind_blob(stmt,  2, label.wire(),              label.size(),              SQLITE_STATIC);
  sqlite3_bind_blob(stmt,  3, rrset.getType().wire(),    rrset.getType().size(),    SQLITE_STATIC);
  sqlite3_bind_blob(stmt,  4, rrset.getVersion().wire(), rrset.getVersion().size(), SQLITE_STATIC);
  sqlite3_bind_int64(stmt, 5, rrset.getTtl().count());
  sqlite3_bind_blob(stmt,  6, rrset.getData().wire(),    rrset.getData().size(),    SQLITE_STATIC);

  rc = sqlite3_step(stmt);
  if (rc != SQLITE_DONE) {
    sqlite3_finalize(stmt);
    BOOST_THROW_EXCEPTION(ExecuteError(sql));
  }

  rrset.setId(sqlite3_last_insert_rowid(m_conn));
  sqlite3_finalize(stmt);
}

bool
DbMgr::find(Rrset& rrset)
{
  if (rrset.getZone() == 0) {
    BOOST_THROW_EXCEPTION(RrsetError("Rrset has not been assigned to a zone"));
  }

  if (rrset.getZone()->getId() == 0) {
    bool isFound = find(*rrset.getZone());
    if (!isFound) {
      return false;
    }
  }

  sqlite3_stmt* stmt;
  const char* sql =
    "SELECT id, ttl, version, data FROM rrsets"
    "    WHERE zone_id=? and label=? and type=?";
  int rc = sqlite3_prepare_v2(m_conn, sql, -1, &stmt, 0);

  if (rc != SQLITE_OK) {
    BOOST_THROW_EXCEPTION(PrepareError(sql));
  }

  sqlite3_bind_int64(stmt, 1, rrset.getZone()->getId());

  const Block& label = rrset.getLabel().wireEncode();
  sqlite3_bind_blob(stmt, 2, label.wire(), label.size(), SQLITE_STATIC);
  sqlite3_bind_blob(stmt, 3, rrset.getType().wire(), rrset.getType().size(), SQLITE_STATIC);

  if (sqlite3_step(stmt) == SQLITE_ROW) {
    rrset.setId(sqlite3_column_int64(stmt, 0));
    rrset.setTtl(time::seconds(sqlite3_column_int64(stmt, 1)));
    rrset.setVersion(Block(static_cast<const uint8_t*>(sqlite3_column_blob(stmt, 2)),
                           sqlite3_column_bytes(stmt, 2)));
    rrset.setData(Block(static_cast<const uint8_t*>(sqlite3_column_blob(stmt, 3)),
                        sqlite3_column_bytes(stmt, 3)));
  }
  else {
    rrset.setId(0);
  }
  sqlite3_finalize(stmt);

  return rrset.getId() != 0;
}

std::vector<Rrset>
DbMgr::findRrsets(Zone& zone)
{
  if (zone.getId() == 0)
    find(zone);

  if (zone.getId() == 0)
    BOOST_THROW_EXCEPTION(RrsetError("Attempting to find all the rrsets with a zone does not in the database"));

  std::vector<Rrset> vec;
  sqlite3_stmt* stmt;
  const char* sql = "SELECT id, ttl, version, data, label, type "
                    "FROM rrsets where zone_id=? ";

  int rc = sqlite3_prepare_v2(m_conn, sql, -1, &stmt, 0);
  if (rc != SQLITE_OK) {
    BOOST_THROW_EXCEPTION(PrepareError(sql));
  }
  sqlite3_bind_int64(stmt, 1, zone.getId());

  while (sqlite3_step(stmt) == SQLITE_ROW) {
    vec.emplace_back(&zone);
    Rrset& rrset = vec.back();

    rrset.setId(sqlite3_column_int64(stmt, 0));
    rrset.setTtl(time::seconds(sqlite3_column_int64(stmt, 1)));
    rrset.setVersion(Block(static_cast<const uint8_t*>(sqlite3_column_blob(stmt, 2)),
                           sqlite3_column_bytes(stmt, 2)));
    rrset.setData(Block(static_cast<const uint8_t*>(sqlite3_column_blob(stmt, 3)),
                        sqlite3_column_bytes(stmt, 3)));
    rrset.setLabel(Name(Block(static_cast<const uint8_t*>(sqlite3_column_blob(stmt, 4)),
                         sqlite3_column_bytes(stmt, 4))));
    rrset.setType(Block(static_cast<const uint8_t*>(sqlite3_column_blob(stmt, 5)),
                  sqlite3_column_bytes(stmt, 5)));
  }
  sqlite3_finalize(stmt);

  return vec;
}


void
DbMgr::remove(Rrset& rrset)
{
  if (rrset.getId() == 0)
    BOOST_THROW_EXCEPTION(RrsetError("Attempting to remove Rrset that has no assigned id"));

  sqlite3_stmt* stmt;
  const char* sql = "DELETE FROM rrsets WHERE id=?";
  int rc = sqlite3_prepare_v2(m_conn, sql, -1, &stmt, 0);

  if (rc != SQLITE_OK) {
    BOOST_THROW_EXCEPTION(PrepareError(sql));
  }

  sqlite3_bind_int64(stmt, 1, rrset.getId());

  rc = sqlite3_step(stmt);
  if (rc != SQLITE_DONE) {
    sqlite3_finalize(stmt);
    BOOST_THROW_EXCEPTION(ExecuteError(sql));
  }

  sqlite3_finalize(stmt);

  rrset = Rrset(rrset.getZone());
}

void
DbMgr::update(Rrset& rrset)
{
  if (rrset.getId() == 0) {
    BOOST_THROW_EXCEPTION(RrsetError("Attempting to replace Rrset that has no assigned id"));
  }

  if (rrset.getZone() == 0) {
    BOOST_THROW_EXCEPTION(RrsetError("Rrset has not been assigned to a zone"));
  }

  sqlite3_stmt* stmt;
  const char* sql = "UPDATE rrsets SET ttl=?, version=?, data=? WHERE id=?";
  int rc = sqlite3_prepare_v2(m_conn, sql, -1, &stmt, 0);

  if (rc != SQLITE_OK) {
    BOOST_THROW_EXCEPTION(PrepareError(sql));
  }

  sqlite3_bind_int64(stmt, 1, rrset.getTtl().count());
  sqlite3_bind_blob(stmt,  2, rrset.getVersion().wire(), rrset.getVersion().size(), SQLITE_STATIC);
  sqlite3_bind_blob(stmt,  3, rrset.getData().wire(),    rrset.getData().size(),    SQLITE_STATIC);
  sqlite3_bind_int64(stmt, 4, rrset.getId());

  sqlite3_step(stmt);
  sqlite3_finalize(stmt);
}

} // namespace ndns
} // namespace ndn
