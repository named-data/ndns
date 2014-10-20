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

#include "ndns-enum.hpp"
#include "../boost-test.hpp"

namespace ndn {
namespace ndns {
namespace tests {

BOOST_AUTO_TEST_SUITE(NdnsEnum)

BOOST_AUTO_TEST_CASE(NdnsTypeToString)
{
  BOOST_CHECK_EQUAL(toString(NDNS_RESP), "NDNS-Resp");
  BOOST_CHECK_EQUAL(toString(NDNS_AUTH), "NDNS-Auth");
  BOOST_CHECK_EQUAL(toString(NDNS_NACK), "NDNS-Nack");
  BOOST_CHECK_EQUAL(toString(NDNS_RAW), "NDNS-Raw");

  BOOST_CHECK_EQUAL(toString(static_cast<NdnsType>(254)), "UNKNOWN");
  BOOST_CHECK_EQUAL(toString(static_cast<NdnsType>(255)), "UNKNOWN");
  BOOST_CHECK_EQUAL(toString(static_cast<NdnsType>(256)), "UNKNOWN");
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace tests
} // namespace ndns
} // namespace ndn
