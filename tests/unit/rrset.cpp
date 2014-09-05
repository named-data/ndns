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

#include "rrset.hpp"
#include "../boost-test.hpp"

#include <ndn-cxx/name.hpp>

namespace ndn {
namespace ndns {
namespace tests {

BOOST_AUTO_TEST_SUITE(Rrset)

BOOST_AUTO_TEST_CASE(Basic)
{
  ndns::Rrset rrset1;
  rrset1.setId(1);
  rrset1.setZone(0);
  rrset1.setLabel("/www/1");
  rrset1.setType(name::Component("NS"));
  rrset1.setVersion(name::Component::fromVersion(1));
  rrset1.setTtl(time::seconds(10));
  rrset1.setData(Name("/test/1").wireEncode());

  BOOST_CHECK_EQUAL(rrset1.getId(), 1);
  BOOST_CHECK_EQUAL(rrset1.getZone(), static_cast<Zone*>(0));
  BOOST_CHECK_EQUAL(rrset1.getLabel(), Name("/www/1"));
  BOOST_CHECK_EQUAL(rrset1.getType(), name::Component("NS"));
  BOOST_CHECK_EQUAL(rrset1.getVersion(), name::Component::fromVersion(1));
  BOOST_CHECK_EQUAL(rrset1.getTtl(), time::seconds(10));
  BOOST_CHECK(rrset1.getData() == Name("/test/1").wireEncode());

  Name zoneName("/net/ndnsim");

  ndns::Rrset rrset2(rrset1);
  BOOST_CHECK_EQUAL(rrset1, rrset2);

  rrset2.setId(2);
  BOOST_CHECK_EQUAL(rrset1, rrset2);
  rrset2 = rrset1;

  Zone zone;
  rrset2.setZone(&zone);
  BOOST_CHECK_NE(rrset1, rrset2);

  rrset2 = rrset1;
  BOOST_CHECK_EQUAL(rrset1, rrset2);

  rrset2.setLabel(Name("/www/2"));
  BOOST_CHECK_NE(rrset1, rrset2);
  rrset2 = rrset1;

  rrset2.setType(name::Component("TXT"));
  BOOST_CHECK_NE(rrset1, rrset2);
  rrset2 = rrset1;

  rrset2.setVersion(name::Component::fromVersion(2));
  BOOST_CHECK_NE(rrset1, rrset2);
  rrset2 = rrset1;

  rrset2.setTtl(time::seconds(1));
  BOOST_CHECK_EQUAL(rrset1, rrset2);
  rrset2 = rrset1;

  rrset2.setData(Name("/test/2").wireEncode());
  BOOST_CHECK_EQUAL(rrset1, rrset2);
  rrset2 = rrset1;
}

BOOST_AUTO_TEST_CASE(Ostream)
{
  ndns::Zone zone("/test");
  ndns::Rrset rrset(&zone);

  rrset.setId(1);
  rrset.setLabel("/www/1");
  rrset.setType(name::Component("NS"));
  rrset.setVersion(name::Component::fromVersion(1));
  rrset.setTtl(time::seconds(10));

  boost::test_tools::output_test_stream os;
  os << rrset;
  BOOST_CHECK(os.is_equal("Rrset: Id=1 Zone=(Zone: Id=0 Name=/test)"
                          " Label=/www/1 Type=NS Version=%FD%01"));
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace tests
} // namespace ndns
} // namespace ndn
