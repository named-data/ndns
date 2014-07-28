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

#include "../boost-test.hpp"

#include "db/rr-mgr.hpp"

//#include <ndn-cxx/name.hpp>
namespace ndn {
namespace ndns {
namespace tests {

using namespace std;

BOOST_AUTO_TEST_SUITE(rrMgr)

BOOST_AUTO_TEST_CASE(db)
{

  string label = "rrMgr::db";
  printbegin(label);

  ndns::Query q;
  Name n1("/net");
  Name n2("/ndnsim");
  q.setAuthorityZone(n1);
  q.setRrLabel(n2);
  q.setQueryType(ndns::Query::QUERY_DNS_R);


  Zone zone;
  zone.setAuthorizedName(Name("/net"));
  zone.setId(2);

  Response re;

  RRMgr mgr(zone, q, re);
  BOOST_CHECK_EQUAL(mgr.lookup(), 0);


  zone.setId(23);
  RRMgr mgr2(zone, q, re);
  BOOST_CHECK_EQUAL(mgr2.lookup(), 0);



  printend(label);
}
BOOST_AUTO_TEST_SUITE_END()

} // namespace tests
} // namespace ndns
} // namespace ndn
