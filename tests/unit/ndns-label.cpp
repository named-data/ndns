/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014-2016, Regents of the University of California.
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

#include "ndns-label.hpp"

#include "test-common.hpp"

namespace ndn {
namespace ndns {
namespace tests {

BOOST_AUTO_TEST_SUITE(NdnsLabel)

BOOST_AUTO_TEST_CASE(MatchInterest)
{
  using namespace label;
  Name zone("/net/ndnsim");

  Interest interest1("/net/ndnsim/NDNS/www/dsk-111/NS");
  Interest interest2("/net/ndnsim/NDNS/www/dsk-111/NS/%FD%00");

  MatchResult re;
  BOOST_CHECK_EQUAL(matchName(interest1, zone, re), true);
  BOOST_CHECK_EQUAL(re.rrLabel, Name("/www/dsk-111"));
  BOOST_CHECK_EQUAL(re.rrType, name::Component("NS"));
  BOOST_CHECK_EQUAL(re.version, name::Component());

  BOOST_CHECK_EQUAL(matchName(interest2, zone, re), true);
  BOOST_CHECK_EQUAL(re.rrLabel, Name("/www/dsk-111"));
  BOOST_CHECK_EQUAL(re.rrType, name::Component("NS"));
  BOOST_CHECK_EQUAL(re.version, name::Component::fromEscapedString("%FD%00"));
}

BOOST_AUTO_TEST_CASE(MatchData)
{
  using namespace label;
  Name zone("/net/ndnsim");

  Data data1("/net/ndnsim/NDNS/www/dsk-111/NS/%FD%00");

  MatchResult re;
  BOOST_CHECK_EQUAL(matchName(data1, zone, re), true);
  BOOST_CHECK_EQUAL(re.rrLabel, Name("/www/dsk-111"));
  BOOST_CHECK_EQUAL(re.rrType, name::Component("NS"));
  BOOST_REQUIRE_NO_THROW(re.version.toVersion());
  BOOST_CHECK_EQUAL(re.version.toVersion(), 0);
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace tests
} // namespace ndns
} // namespace ndn
