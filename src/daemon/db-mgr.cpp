/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2022, Regents of the University of California.
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
#include "util/util.hpp"

namespace ndn {
namespace ndns {

NDNS_LOG_INIT(DbMgr);

const std::string NDNS_SCHEMA = R"SQL(
CREATE TABLE IF NOT EXISTS zones (
  id    INTEGER NOT NULL PRIMARY KEY,
  name  BLOB NOT NULL UNIQUE,
  ttl   INTEGER NOT NULL
);

CREATE TABLE IF NOT EXISTS zone_info (
  zone_id INTEGER NOT NULL,
  key     TEXT NOT NULL,
  value   BLOB NOT NULL,
  PRIMARY KEY(zone_id, key),
  FOREIGN KEY(zone_id) REFERENCES zones(id) ON UPDATE CASCADE ON DELETE CASCADE
);

CREATE TABLE IF NOT EXISTS rrsets (
  id      INTEGER NOT NULL PRIMARY KEY,
  zone_id INTEGER NOT NULL,
  label   BLOB NOT NULL,
  type    BLOB NOT NULL,
  version BLOB NOT NULL,
  ttl     INTEGER NOT NULL,
  data    BLOB NOT NULL,
  FOREIGN KEY(zone_id) REFERENCES zones(id) ON UPDATE CASCADE ON DELETE CASCADE
);

CREATE UNIQUE INDEX rrsets_zone_id_label_type_version
  ON rrsets(zone_id, label, type, version);
)SQL";

DbMgr::DbMgr(const std::string& dbFile)
  : m_dbFile(dbFile)
  , m_conn(nullptr)
{
  if (m_dbFile.empty())
    m_dbFile = getDefaultDatabaseFile();

  open();

  NDNS_LOG_INFO("open database: " << m_dbFile);
}

DbMgr::~DbMgr()
{
  close();
}

void
DbMgr::open()
{
  int res = sqlite3_open_v2(m_dbFile.data(), &m_conn,
                            SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE,
#ifdef DISABLE_SQLITE3_FS_LOCKING
                            "unix-dotfile"
#else
                            nullptr
#endif
                            );

  if (res != SQLITE_OK) {
    NDNS_LOG_FATAL("Cannot open the db file: " << m_dbFile);
    NDN_THROW(ConnectError("Cannot open the db file: " + m_dbFile));
  }

  // ignore any errors from DB creation (command will fail for the existing database, which is ok)
  sqlite3_exec(m_conn, NDNS_SCHEMA.data(), nullptr, nullptr, nullptr);
}

void
DbMgr::close()
{
  if (m_conn == nullptr)
    return;

  int ret = sqlite3_close(m_conn);
  if (ret != SQLITE_OK) {
    NDNS_LOG_FATAL("Cannot close the db: " << m_dbFile);
  }
  else {
    m_conn = nullptr;
    NDNS_LOG_INFO("Close database: " << m_dbFile);
  }
}

void
DbMgr::clearAllData()
{
  const char* sql = "DELETE FROM zones; DELETE FROM rrsets;";

  // sqlite3_step cannot execute multiple SQL statements
  int rc = sqlite3_exec(m_conn, sql, nullptr, nullptr, nullptr);
  if (rc != SQLITE_OK) {
    NDN_THROW(ExecuteError(sql));
  }

  NDNS_LOG_INFO("clear all the data in the database: " << m_dbFile);
}

void
DbMgr::saveName(const Name& name, sqlite3_stmt* stmt, int iCol, bool isStatic)
{
  static const uint8_t dummy = 0;
  const auto& wire = name.wireEncode();
  const auto* ptr = wire.value();
  if (ptr == nullptr) {
    // if value() returns nullptr (i.e., value_size() == 0), pass a non-null dummy
    // pointer instead; we cannot bind nullptr because the column may be "NOT NULL"
    ptr = &dummy;
  }
  sqlite3_bind_blob(stmt, iCol, ptr, wire.value_size(), isStatic ? SQLITE_STATIC : SQLITE_TRANSIENT);
}

Name
DbMgr::restoreName(sqlite3_stmt* stmt, int iCol)
{
  Name name;
  span buffer(static_cast<const uint8_t*>(sqlite3_column_blob(stmt, iCol)),
              sqlite3_column_bytes(stmt, iCol));

  while (!buffer.empty()) {
    name::Component component;
    try {
      component.wireDecode(Block(buffer));
    }
    catch (const ndn::tlv::Error&) {
      NDN_THROW_NESTED(Error("Error while decoding name from the database"));
    }
    name.append(component);
    buffer = buffer.subspan(component.size());
  }

  return name;
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
  int rc = sqlite3_prepare_v2(m_conn, sql, -1, &stmt, nullptr);
  if (rc != SQLITE_OK) {
    NDN_THROW(PrepareError(sql));
  }

  saveName(zone.getName(), stmt, 1);
  sqlite3_bind_int(stmt,  2, zone.getTtl().count());

  rc = sqlite3_step(stmt);
  if (rc != SQLITE_DONE) {
    sqlite3_finalize(stmt);
    NDN_THROW(ExecuteError(sql));
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
    NDN_THROW(Error("zone has not been initialized"));
  }

  if (key.length() > 10) {
    NDN_THROW(Error("key length should not exceed 10"));
  }

  sqlite3_stmt* stmt;
  const char* sql = "INSERT OR REPLACE INTO zone_info (zone_id, key, value) VALUES (?, ?, ?)";
  int rc = sqlite3_prepare_v2(m_conn, sql, -1, &stmt, nullptr);
  if (rc != SQLITE_OK) {
    NDN_THROW(PrepareError(sql));
  }

  sqlite3_bind_int(stmt,  1, zone.getId());
  sqlite3_bind_text(stmt, 2, key.data(),   key.length(), SQLITE_STATIC);
  sqlite3_bind_blob(stmt, 3, value.data(), value.size(), SQLITE_STATIC);

  rc = sqlite3_step(stmt);
  if (rc != SQLITE_DONE) {
    sqlite3_finalize(stmt);
    NDN_THROW(ExecuteError(sql));
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
    NDN_THROW(Error("zone has not been initialized"));
  }

  sqlite3_stmt* stmt;
  const char* sql = "SELECT key, value FROM zone_info WHERE zone_id=?";
  int rc = sqlite3_prepare_v2(m_conn, sql, -1, &stmt, nullptr);
  if (rc != SQLITE_OK) {
    NDN_THROW(PrepareError(sql));
  }

  sqlite3_bind_int(stmt, 1, zone.getId());

  while (sqlite3_step(stmt) == SQLITE_ROW) {
    const char* key = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
    rtn[string(key)] = Block(span(static_cast<const uint8_t*>(sqlite3_column_blob(stmt, 1)),
                                  sqlite3_column_bytes(stmt, 1)));
  }

  sqlite3_finalize(stmt);
  return rtn;
}

bool
DbMgr::find(Zone& zone)
{
  sqlite3_stmt* stmt;
  const char* sql = "SELECT id, ttl FROM zones WHERE name=?";
  int rc = sqlite3_prepare_v2(m_conn, sql, -1, &stmt, nullptr);
  if (rc != SQLITE_OK) {
    NDN_THROW(PrepareError(sql));
  }

  saveName(zone.getName(), stmt, 1);

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
  int rc = sqlite3_prepare_v2(m_conn, sql, -1, &stmt, nullptr);
  if (rc != SQLITE_OK) {
    NDN_THROW(PrepareError(sql));
  }

  std::vector<Zone> vec;

  while (sqlite3_step(stmt) == SQLITE_ROW) {
    vec.emplace_back();
    Zone& zone = vec.back();
    zone.setId(sqlite3_column_int64(stmt, 0));
    zone.setTtl(time::seconds(sqlite3_column_int(stmt, 2)));
    zone.setName(restoreName(stmt, 1));
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
  int rc = sqlite3_prepare_v2(m_conn, sql, -1, &stmt, nullptr);
  if (rc != SQLITE_OK) {
    NDN_THROW(PrepareError(sql));
  }

  sqlite3_bind_int64(stmt, 1, zone.getId());

  rc = sqlite3_step(stmt);
  if (rc != SQLITE_DONE) {
    sqlite3_finalize(stmt);
    NDN_THROW(ExecuteError(sql));
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

  if (rrset.getZone() == nullptr) {
    NDN_THROW(RrsetError("Rrset has not been assigned to a zone"));
  }

  if (rrset.getZone()->getId() == 0) {
    insert(*rrset.getZone());
  }

  const char* sql =
    "INSERT INTO rrsets (zone_id, label, type, version, ttl, data)"
    "    VALUES (?, ?, ?, ?, ?, ?)";

  sqlite3_stmt* stmt;
  int rc = sqlite3_prepare_v2(m_conn, sql, -1, &stmt, nullptr);
  if (rc != SQLITE_OK) {
    NDN_THROW(PrepareError(sql));
  }

  sqlite3_bind_int64(stmt, 1, rrset.getZone()->getId());

  saveName(rrset.getLabel(), stmt, 2);
  sqlite3_bind_blob(stmt,  3, rrset.getType().data(),    rrset.getType().size(),    SQLITE_STATIC);
  sqlite3_bind_blob(stmt,  4, rrset.getVersion().data(), rrset.getVersion().size(), SQLITE_STATIC);
  sqlite3_bind_int64(stmt, 5, rrset.getTtl().count());
  sqlite3_bind_blob(stmt,  6, rrset.getData().data(),    rrset.getData().size(),    SQLITE_STATIC);

  rc = sqlite3_step(stmt);
  if (rc != SQLITE_DONE) {
    sqlite3_finalize(stmt);
    NDN_THROW(ExecuteError(sql));
  }

  rrset.setId(sqlite3_last_insert_rowid(m_conn));
  sqlite3_finalize(stmt);
}

bool
DbMgr::find(Rrset& rrset)
{
  if (rrset.getZone() == nullptr) {
    NDN_THROW(RrsetError("Rrset has not been assigned to a zone"));
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
  int rc = sqlite3_prepare_v2(m_conn, sql, -1, &stmt, nullptr);
  if (rc != SQLITE_OK) {
    NDN_THROW(PrepareError(sql));
  }

  sqlite3_bind_int64(stmt, 1, rrset.getZone()->getId());

  saveName(rrset.getLabel(), stmt, 2);
  sqlite3_bind_blob(stmt, 3, rrset.getType().data(), rrset.getType().size(), SQLITE_STATIC);

  if (sqlite3_step(stmt) == SQLITE_ROW) {
    rrset.setId(sqlite3_column_int64(stmt, 0));
    rrset.setTtl(time::seconds(sqlite3_column_int64(stmt, 1)));
    rrset.setVersion(name::Component(Block(span(static_cast<const uint8_t*>(sqlite3_column_blob(stmt, 2)),
                                                sqlite3_column_bytes(stmt, 2)))));
    rrset.setData(Block(span(static_cast<const uint8_t*>(sqlite3_column_blob(stmt, 3)),
                             sqlite3_column_bytes(stmt, 3))));
  }
  else {
    rrset.setId(0);
  }
  sqlite3_finalize(stmt);

  return rrset.getId() != 0;
}

bool
DbMgr::findLowerBound(Rrset& rrset)
{
  if (rrset.getZone() == nullptr) {
    NDN_THROW(RrsetError("Rrset has not been assigned to a zone"));
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
    "    WHERE zone_id=? and label<? and type=? ORDER BY label DESC";
  int rc = sqlite3_prepare_v2(m_conn, sql, -1, &stmt, nullptr);
  if (rc != SQLITE_OK) {
    NDN_THROW(PrepareError(sql));
  }

  sqlite3_bind_int64(stmt, 1, rrset.getZone()->getId());

  saveName(rrset.getLabel(), stmt, 2);
  sqlite3_bind_blob(stmt, 3, rrset.getType().data(), rrset.getType().size(), SQLITE_STATIC);

  if (sqlite3_step(stmt) == SQLITE_ROW) {
    rrset.setId(sqlite3_column_int64(stmt, 0));
    rrset.setTtl(time::seconds(sqlite3_column_int64(stmt, 1)));
    rrset.setVersion(name::Component(Block(span(static_cast<const uint8_t*>(sqlite3_column_blob(stmt, 2)),
                                                sqlite3_column_bytes(stmt, 2)))));
    rrset.setData(Block(span(static_cast<const uint8_t*>(sqlite3_column_blob(stmt, 3)),
                             sqlite3_column_bytes(stmt, 3))));
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
    NDN_THROW(RrsetError("Attempting to find all the rrsets with a zone does not in the database"));

  std::vector<Rrset> vec;
  sqlite3_stmt* stmt;
  const char* sql = "SELECT id, ttl, version, data, label, type "
                    "FROM rrsets where zone_id=? ORDER BY label";

  int rc = sqlite3_prepare_v2(m_conn, sql, -1, &stmt, nullptr);
  if (rc != SQLITE_OK) {
    NDN_THROW(PrepareError(sql));
  }
  sqlite3_bind_int64(stmt, 1, zone.getId());

  while (sqlite3_step(stmt) == SQLITE_ROW) {
    vec.emplace_back(&zone);
    Rrset& rrset = vec.back();

    rrset.setId(sqlite3_column_int64(stmt, 0));
    rrset.setTtl(time::seconds(sqlite3_column_int64(stmt, 1)));
    rrset.setVersion(name::Component(Block(span(static_cast<const uint8_t*>(sqlite3_column_blob(stmt, 2)),
                                                sqlite3_column_bytes(stmt, 2)))));
    rrset.setData(Block(span(static_cast<const uint8_t*>(sqlite3_column_blob(stmt, 3)),
                             sqlite3_column_bytes(stmt, 3))));
    rrset.setLabel(restoreName(stmt, 4));
    rrset.setType(name::Component(Block(span(static_cast<const uint8_t*>(sqlite3_column_blob(stmt, 5)),
                                             sqlite3_column_bytes(stmt, 5)))));
  }
  sqlite3_finalize(stmt);

  return vec;
}

void
DbMgr::removeRrsetsOfZoneByType(Zone& zone, const name::Component& type)
{
  if (zone.getId() == 0)
    find(zone);

  if (zone.getId() == 0)
    NDN_THROW(RrsetError("Attempting to find all the rrsets with a zone does not in the database"));

  sqlite3_stmt* stmt;
  const char* sql = "DELETE FROM rrsets WHERE zone_id = ? AND type = ?";
  int rc = sqlite3_prepare_v2(m_conn, sql, -1, &stmt, nullptr);
  if (rc != SQLITE_OK) {
    NDN_THROW(PrepareError(sql));
  }

  sqlite3_bind_int64(stmt, 1, zone.getId());
  sqlite3_bind_blob(stmt,  2, type.data(), type.size(), SQLITE_STATIC);

  rc = sqlite3_step(stmt);
  if (rc != SQLITE_DONE) {
    sqlite3_finalize(stmt);
    NDN_THROW(ExecuteError(sql));
  }
  sqlite3_finalize(stmt);
}

void
DbMgr::remove(Rrset& rrset)
{
  if (rrset.getId() == 0)
    NDN_THROW(RrsetError("Attempting to remove Rrset that has no assigned id"));

  sqlite3_stmt* stmt;
  const char* sql = "DELETE FROM rrsets WHERE id=?";
  int rc = sqlite3_prepare_v2(m_conn, sql, -1, &stmt, nullptr);
  if (rc != SQLITE_OK) {
    NDN_THROW(PrepareError(sql));
  }

  sqlite3_bind_int64(stmt, 1, rrset.getId());

  rc = sqlite3_step(stmt);
  if (rc != SQLITE_DONE) {
    sqlite3_finalize(stmt);
    NDN_THROW(ExecuteError(sql));
  }

  sqlite3_finalize(stmt);

  rrset = Rrset(rrset.getZone());
}

void
DbMgr::update(Rrset& rrset)
{
  if (rrset.getId() == 0) {
    NDN_THROW(RrsetError("Attempting to replace Rrset that has no assigned id"));
  }

  if (rrset.getZone() == nullptr) {
    NDN_THROW(RrsetError("Rrset has not been assigned to a zone"));
  }

  sqlite3_stmt* stmt;
  const char* sql = "UPDATE rrsets SET ttl=?, version=?, data=? WHERE id=?";
  int rc = sqlite3_prepare_v2(m_conn, sql, -1, &stmt, nullptr);
  if (rc != SQLITE_OK) {
    NDN_THROW(PrepareError(sql));
  }

  sqlite3_bind_int64(stmt, 1, rrset.getTtl().count());
  sqlite3_bind_blob(stmt,  2, rrset.getVersion().data(), rrset.getVersion().size(), SQLITE_STATIC);
  sqlite3_bind_blob(stmt,  3, rrset.getData().data(),    rrset.getData().size(),    SQLITE_STATIC);
  sqlite3_bind_int64(stmt, 4, rrset.getId());

  sqlite3_step(stmt);
  sqlite3_finalize(stmt);
}

} // namespace ndns
} // namespace ndn
