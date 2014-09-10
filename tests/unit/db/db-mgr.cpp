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

#include "db/db-mgr.hpp"
#include "../../boost-test.hpp"

namespace ndn {
namespace ndns {
namespace tests {

BOOST_AUTO_TEST_SUITE(DbMgr)

BOOST_AUTO_TEST_CASE(Basic)
{
  ndns::DbMgr mgr(BUILDDIR "/tests/unit/db/dbmgr-ndns.db");
  BOOST_CHECK_EQUAL(mgr.getStatus(), ndns::DbMgr::DB_CONNECTED);

  mgr.close();
  BOOST_CHECK_EQUAL(mgr.getStatus(), ndns::DbMgr::DB_CLOSED);

  // reopen
  mgr.open();
  BOOST_CHECK_EQUAL(mgr.getStatus(), ndns::DbMgr::DB_CONNECTED);
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace tests
} // namespace ndns
} // namespace ndn
