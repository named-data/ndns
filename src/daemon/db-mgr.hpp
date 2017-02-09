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

#ifndef NDNS_DAEMON_DB_MGR_HPP
#define NDNS_DAEMON_DB_MGR_HPP

#include "config.hpp"
#include "zone.hpp"
#include "rrset.hpp"

#include <ndn-cxx/common.hpp>
#include <sqlite3.h>

namespace ndn {
namespace ndns {

#define DEFINE_ERROR(ErrorName, Base)           \
class ErrorName : public Base                   \
{                                               \
public:                                         \
  explicit                                      \
  ErrorName(const std::string& what)            \
    : Base(what)                                \
  {                                             \
  }                                             \
}



/**
 * @brief Database Manager, provides CRUD operations on stored entities
 *
 * @note Method names follow MongoDB convention: insert/remove/find/update
 */
class DbMgr : noncopyable
{
public:

  /**
   * @brief The Database Status
   */
  enum DbStatus {
    DB_CONNECTED,
    DB_CLOSED,
    DB_ERROR
  };

  DEFINE_ERROR(Error, std::runtime_error);
  DEFINE_ERROR(PrepareError, Error);
  DEFINE_ERROR(ExecuteError, Error);
  DEFINE_ERROR(ConnectError, Error);

public:
  explicit
  DbMgr(const std::string& dbFile = DEFAULT_DATABASE_PATH "/" "ndns.db");

  ~DbMgr();

  /**
   * @brief connect to the database. If it's already opened, do nothing.
   */
  void
  open();

  /**
   * @brief close the database connection. Do nothing if it's already closed.
   * Destructor would automatically close the database connection as well.
   */
  void
  close();

  /**
  * @brief clear all the data in the database
  */
  void
  clearAllData();

public: // Zone manipulation
  DEFINE_ERROR(ZoneError, Error);

  /**
   * @brief insert the m_zone to the database, and set the zone's id.
   * If the zone is already in the db, handle the exception without leaving it to upper level,
   * meanwhile, set the zone's id too.
   * @pre m_zone.getId() == 0
   * @post m_zone.getId() > 0
   */
  void
  insert(Zone& zone);

  /**
   * @brief set zoneInfo by key-value
   */
  void
  setZoneInfo(Zone& zone,
              const std::string& key,
              const Block& value);

  /**
   * @brief get zoneInfo
   */
  std::map<std::string, Block>
  getZoneInfo(Zone& zone);

  /**
   * @brief lookup the zone by name, fill the m_id and m_ttl
   * @post whatever the previous id is
   * @return true if the record exist
   */
  bool
  find(Zone& zone);

  /**
   * @brief get all zones in the database
   */
  std::vector<Zone>
  listZones();

  /**
   * @brief remove the zone
   * @pre m_zone.getId() > 0
   * @post m_zone.getId() == 0
   */
  void
  remove(Zone& zone);

public: // Rrset manipulation
  DEFINE_ERROR(RrsetError, Error);

  /**
   * @brief add the rrset
   * @pre m_rrset.getId() == 0
   * @post m_rrset.getId() > 0
   */
  void
  insert(Rrset& rrset);

  /**
   * @brief get the data from db according to `m_zone`, `m_label`, `m_type`.
   *
   * If record exists, `m_ttl`, `m_version` and `m_data` is set
   *
   * @pre m_rrset.getZone().getId() > 0
   * @post whatever the previous id is,
   *       m_rrset.getId() > 0 if record exists, otherwise m_rrset.getId() == 0
   * @return true if the record exist
   */
  bool
  find(Rrset& rrset);

  /**
   * @brief get all the rrsets which is stored at given zone
   * @throw RrsetError() if zone does not exist in the database
   * @note if zone.getId() == 0, the function setId for the zone automatically
   * @note all returned rrsets' m_zone point to the memory of the param[in] zone
   */
  std::vector<Rrset>
  findRrsets(Zone& zone);

  /**
   * @brief remove the rrset
   * @pre m_rrset.getId() > 0
   * @post m_rrset.getId() == 0
   */
  void
  remove(Rrset& rrset);

  /**
   * @brief replace ttl, version, and Data with new values
   * @pre m_rrset.getId() > 0
   */
  void
  update(Rrset& rrset);

  ////////////////////////////////
  ////////getter and setter
public:
  const std::string&
  getDbFile() const
  {
    return m_dbFile;
  }

private:
  std::string m_dbFile;
  sqlite3* m_conn;
};

} // namespace ndns
} // namespace ndn

#endif // NDNS_DAEMON_DB_MGR_HPP
