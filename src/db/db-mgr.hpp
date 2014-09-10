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

#ifndef NDNS_DB_DB_MGR_HPP
#define NDNS_DB_DB_MGR_HPP

#include "config.hpp"

#include <ndn-cxx/common.hpp>
#include <sqlite3.h>

namespace ndn {
namespace ndns {

/**
 * @brief Database Manager, which provides some common DB functionalities
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

public:
  /**
   * @brief constructor
   */
  explicit
  DbMgr(const std::string& dbFile = DEFAULT_CONFIG_PATH "/" "ndns.conf");

  /**
   * @brief destructor
   */
  ~DbMgr();

  /**
   * @brief connect to the database
   */
  void
  open();

  /**
   * @brief close the database connection
   */
  void
  close();

  /**
   * @brief get error message
   *
   * only valid when some db-related error happens,
   * such as db file does not exist, wrong SQL, etc
   */
  const std::string&
  getErr() const
  {
    return m_err;
  }

  /**
   * @brief get Database connection status
   */
  DbStatus
  getStatus() const
  {
    return m_status;
  }

private:
  /**
   * @brief set error message
   *
   * only valid when some db-related error happens,
   * such as db file does not exist, wrong SQL, etc
   */
  void
  setErr(const std::string& err)
  {
    this->m_err = err;
  }

  /**
   * @brief set Database connection status
   */
  void
  setStatus(DbStatus status)
  {
    this->m_status = status;
  }

private:
  std::string m_dbFile;
  sqlite3* m_conn;

  std::string m_err;
  DbStatus m_status;
};

} // namespace ndns
} // namespace ndn

#endif // NDNS_DB_DB_MGR_HPP
