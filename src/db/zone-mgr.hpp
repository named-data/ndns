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

#ifndef NDNS_ZONE_MGR_CPP
#define NDNS_ZONE_MGR_CPP

#include <sqlite3.h>

#include "db-mgr.hpp"
#include "zone.hpp"
#include <ndn-cxx/name.hpp>

namespace ndn {
namespace ndns {

class ZoneMgr: public DBMgr
{

public:
  ZoneMgr(Zone& zone);

public:
  void
  lookupId(const Name& name);

  void
  lookupId();

  const Zone& getZone() const
  {
    return m_zone;
  }

  void setZone(const Zone& zone)
  {
    this->m_zone = zone;
  }

  int
  callback_setId(int argc, char **argv, char **azColName);

  static int static_callback_setId(void *param, int argc, char **argv,
      char **azColName)
  {

    ZoneMgr *mgr = reinterpret_cast<ZoneMgr*>(param);
    return mgr->callback_setId(argc, argv, azColName);
  }

private:
  Zone& m_zone;
};
//class ZoneMgr

}//namespace ndns
} //namespace ndn
#endif
