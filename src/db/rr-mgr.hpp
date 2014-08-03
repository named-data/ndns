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

#ifndef RR_MGR_HPP
#define RR_MGR_HPP

#include "db-mgr.hpp"
#include "query.hpp"
#include "response.hpp"
#include "zone.hpp"

namespace ndn {
namespace ndns {
class RRMgr: public DBMgr
{
public:
  RRMgr(Zone& zone, Query& query, Response& response);
  virtual ~RRMgr();

public:
  int lookup();

  int callback_getRr(int argc, char **argv, char **azColName);

  static int static_callback_getRr(void *param, int argc, char **argv,
      char **azColName)
  {
    RRMgr *mgr = reinterpret_cast<RRMgr*>(param);
    return mgr->callback_getRr(argc, argv, azColName);
  }

  int count();
  int callback_countRr(int argc, char **argv, char **azColName);

  static int static_callback_countRr(void *param, int argc, char **argv,
      char **azColName)
  {
    RRMgr *mgr = reinterpret_cast<RRMgr*>(param);
    return mgr->callback_countRr(argc, argv, azColName);
  }

  const Query& getQuery() const
  {
    return m_query;
  }

  void setQuery(const Query& query)
  {
    m_query = query;
  }

  const Response& getResponse() const
  {
    return m_response;
  }

  void setResponse(const Response& response)
  {
    m_response = response;
  }

  const Zone& getZone() const
  {
    return m_zone;
  }

  void setZone(const Zone& zone)
  {
    m_zone = zone;
  }

private:
  unsigned int m_count;
  Zone& m_zone;
  Query& m_query;
  Response& m_response;
};

} //namespace ndns
} /* namespace ndn */

#endif /* RR_MGR_HPP_ */
