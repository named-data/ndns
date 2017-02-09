/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2017, Regents of the University of California.
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

#include "clients/query.hpp"

#include "test-common.hpp"

#include <boost/lexical_cast.hpp>
#include <string>

namespace ndn {
namespace ndns {
namespace tests {

BOOST_AUTO_TEST_SUITE(Query)

BOOST_FIXTURE_TEST_CASE(TestCase, IdentityManagementFixture)
{
  security::Identity certIdentity = addIdentity("/cert/name");
  Name zone("/net");
  name::Component qType = ndns::label::NDNS_ITERATIVE_QUERY;
  ndns::Query q(zone, qType);

  q.setRrLabel(Name("/ndnsim/www"));
  BOOST_CHECK_EQUAL(q.getRrLabel(), Name("ndnsim/www"));
  BOOST_CHECK_EQUAL(q.getRrLabel(), Name("/ndnsim/www"));
  BOOST_CHECK_EQUAL(q.getZone(), zone);
  BOOST_CHECK_EQUAL(q.getQueryType(), qType);

  q.setRrType(ndns::label::CERT_RR_TYPE);
  BOOST_CHECK_EQUAL(q.getRrType(), label::CERT_RR_TYPE);

  auto link = make_shared<Link>("/ndn/link/NDNS/test/NS");
  for (int i = 1; i <= 5; i++) {
    link->addDelegation(i, std::string("/link/") + to_string(i));
  }
  // link has to be signed first, then wireDecode
  m_keyChain.sign(*link, security::signingByIdentity(certIdentity));

  q.setDelegationListFromLink(*link);
  BOOST_CHECK_EQUAL(q.getDelegationList(), link->getDelegationList());

  Interest interest = q.toInterest();
  BOOST_CHECK_EQUAL(interest.getForwardingHint(), link->getDelegationList());

  ndns::Query q2(zone, qType);
  BOOST_CHECK_EQUAL(q2.fromInterest(zone, interest), true);

  ndns::Query q3;
  BOOST_CHECK_EQUAL(q3.fromInterest(zone, interest), true);

  BOOST_CHECK_EQUAL(q, q3);
  BOOST_CHECK_EQUAL(q, q2);

  ndns::Query q4(zone, qType);
  q4.setRrLabel("/ndnsim/www");
  q4.setRrType(ndns::label::TXT_RR_TYPE);
  interest = q4.toInterest();
  ndns::Query q5;

  BOOST_CHECK_EQUAL(q5.fromInterest(zone, interest), true);
  BOOST_CHECK_EQUAL(q4, q5);
  BOOST_CHECK_NE(q2, q4);
 }

BOOST_AUTO_TEST_SUITE_END()

} // namespace tests
} // namespace ndns
} // namespace ndn
