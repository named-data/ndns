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
#include "../boost-test.hpp"

#include <boost/filesystem.hpp>

namespace ndn {
namespace ndns {
namespace tests {

BOOST_AUTO_TEST_SUITE(DbMgr)

static const boost::filesystem::path TEST_DATABASE = BUILDDIR "/tests/unit/db-mgr-ndns.db";

class DbMgrFixture
{
public:
  DbMgrFixture()
    : session(TEST_DATABASE.string().c_str())
  {
  }

  ~DbMgrFixture()
  {
    session.close();
    boost::filesystem::remove(TEST_DATABASE);
  }

public:
  ndns::DbMgr session;
};


BOOST_FIXTURE_TEST_CASE(Basic, DbMgrFixture)
{
  BOOST_CHECK_EQUAL(session.getStatus(), ndns::DbMgr::DB_CONNECTED);

  session.close();
  BOOST_CHECK_EQUAL(session.getStatus(), ndns::DbMgr::DB_CLOSED);

  // reopen
  session.open();
  BOOST_CHECK_EQUAL(session.getStatus(), ndns::DbMgr::DB_CONNECTED);
}

BOOST_FIXTURE_TEST_CASE(Zones, DbMgrFixture)
{
  Zone zone1;
  zone1.setName("/net");
  zone1.setTtl(time::seconds(4600));
  BOOST_CHECK_NO_THROW(session.insert(zone1));
  BOOST_CHECK_GT(zone1.getId(), 0);

  Zone zone2;
  zone2.setName("/net");
  session.lookup(zone2);
  BOOST_CHECK_EQUAL(zone2.getId(), zone1.getId());
  BOOST_CHECK_EQUAL(zone2.getTtl(), zone1.getTtl());

  BOOST_CHECK_NO_THROW(session.insert(zone2)); // zone2 already has id.  Nothing to execute

  zone2.setId(0);
  BOOST_CHECK_THROW(session.insert(zone2), ndns::DbMgr::ExecuteError);

  BOOST_CHECK_NO_THROW(session.remove(zone1));
  BOOST_CHECK_EQUAL(zone1.getId(), 0);

  // record shouldn't exist at this point
  BOOST_CHECK_NO_THROW(session.lookup(zone2));
  BOOST_CHECK_EQUAL(zone2.getId(), 0);
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace tests
} // namespace ndns
} // namespace ndn
