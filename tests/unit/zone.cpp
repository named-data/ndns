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

#include "zone.hpp"
#include "../boost-test.hpp"

#include <ndn-cxx/name.hpp>

namespace ndn {
namespace ndns {
namespace tests {

BOOST_AUTO_TEST_SUITE(Zone)

BOOST_AUTO_TEST_CASE(Basic)
{
  Name zoneName("/net/ndnsim");
  ndns::Zone zone1;
  zone1.setName(zoneName);
  zone1.setId(2);
  zone1.setTtl(time::seconds(4000));

  BOOST_CHECK_EQUAL(zone1.getId(), 2);
  BOOST_CHECK_EQUAL(zone1.getName(), zoneName);
  BOOST_CHECK_EQUAL(zone1.getTtl(), time::seconds(4000));

  ndns::Zone zone2(zoneName);
  BOOST_CHECK_EQUAL(zone1, zone2);
  BOOST_CHECK_EQUAL(zone2.getName(), zone1.getName());

  BOOST_CHECK_NE(zone1, ndns::Zone("/net/ndnsim2"));
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace tests
} // namespace ndns
} // namespace ndn
