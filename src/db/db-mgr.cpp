/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014, Regents of the University of California.
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
#include "../logger.hpp"

#include <boost/algorithm/string/predicate.hpp>

#include <iostream>
#include <fstream>

namespace ndn {
namespace ndns {

NDNS_LOG_INIT("DbMgr");

static const std::string NDNS_SCHEMA = "\
CREATE TABLE IF NOT EXISTS zones (      \n\
  id    INTEGER NOT NULL PRIMARY KEY,   \n\
  name  blob NOT NULL UNIQUE,           \n\
  ttl   integer(10) NOT NULL);          \n\
                                        \n\
CREATE TABLE IF NOT EXISTS rrsets (     \n\
  id      INTEGER NOT NULL PRIMARY KEY, \n\
  zone_id integer(10) NOT NULL,         \n\
  label   blob NOT NULL,                \n\
  type    blob NOT NULL,                \n\
  version blob NOT NULL,                \n\
  ttl     integer(10) NOT NULL,         \n\
  data    blob NOT NULL,                                                \n\
  FOREIGN KEY(zone_id) REFERENCES zones(id) ON UPDATE Cascade ON DELETE Cascade); \n\
                                                                        \n\
CREATE UNIQUE INDEX rrsets_zone_id_label_type_version                   \n\
  ON rrsets (zone_id, label, type, version);                            \n\
";

DbMgr::DbMgr(const std::string& dbFile/* = DEFAULT_CONFIG_PATH "/" "ndns.conf"*/)
  : m_dbFile(dbFile)
  , m_conn(0)
  , m_status(DB_CLOSED)
{
  this->open();
}


DbMgr::~DbMgr()
{
  if (m_status != DB_CLOSED) {
    this->close();
  }
}

void
DbMgr::open()
{
  if (m_status == DB_CONNECTED)
    return;

  int res = sqlite3_open_v2(m_dbFile.c_str(), &m_conn,
                            SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE,
#ifdef DISABLE_SQLITE3_FS_LOCKING
                            "unix-dotfile"
#else
                            0
#endif
                            );

  if (res != SQLITE_OK) {
    m_err = "Cannot open the db file: " + m_dbFile;
    NDNS_LOG_FATAL(m_err);
    m_status = DB_ERROR;
  }

  // ignore any errors from DB creation (command will fail for the existing database, which is ok)
  sqlite3_exec(m_conn, NDNS_SCHEMA.c_str(), 0, 0, 0);

  m_status = DB_CONNECTED;
}

void
DbMgr::close()
{
  if (m_status == DB_CLOSED)
    return;

  int ret = sqlite3_close(m_conn);
  if (ret != SQLITE_OK) {
    m_err = "Cannot close the db: " + m_dbFile;
    NDNS_LOG_FATAL(m_err);
    m_status = DB_ERROR;
  }
  m_status = DB_CLOSED;
}

} // namespace ndns
} // namespace ndn
