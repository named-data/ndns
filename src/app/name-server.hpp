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
#ifndef NDNS_NAME_SERVER_HPP
#define NDNS_NAME_SERVER_HPP

#include <boost/asio.hpp>
#include <boost/noncopyable.hpp>

#include <ndn-cxx/face.hpp>
#include <ndn-cxx/name.hpp>
#include <ndn-cxx/security/key-chain.hpp>

#include "zone.hpp"
#include "db/zone-mgr.hpp"
#include "db/rr-mgr.hpp"
#include "query.hpp"
#include "response.hpp"
#include "rr.hpp"

#include "ndn-app.hpp"

using namespace std;
using namespace ndn;

namespace ndn {
namespace ndns {
class NameServer: public NDNApp
{

public:
  explicit
  NameServer(const char *programName, const char *prefix, const char *nameZone,
      const string dbfile = "src/db/ndns-local.db");

  void
  onInterest(const Name &name, const Interest &interest);

  void
  run();

public:
  /*
   * the name used by the server to provide routeable accessory.

   Name m_name;
   */
  /*
   * the zone the server is in charge of
   */
  Zone m_zone;

  /*
   * to manage the m_zone; m_zoneMgr.getZone() returns the referrence of m_zone
   */
  ZoneMgr m_zoneMgr;

};
//clcass NameServer
}//namespace ndns
} //namespace ndn
#endif /* NAME_SERVER_HPP_ */
