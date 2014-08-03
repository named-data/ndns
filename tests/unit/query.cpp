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

#include "query.hpp"

#include "boost-test.hpp"

namespace ndn {
namespace ndns {
namespace tests {

using namespace std;

BOOST_AUTO_TEST_SUITE(query)

BOOST_AUTO_TEST_CASE(UnderstandInterest)
{
  printbegin("query::UnderstandInterst");
  ndns::Query q;
  q.setFowardingHint(Name("/ucla"));

  Name n1("/dns.google.com");
  Name n2("/www.baidu.com");
  q.setAuthorityZone(n1);
  q.setRrLabel(n2);
  q.setQueryType(ndns::Query::QUERY_DNS_R);

  Interest interest = q.toInterest();
  cout<<"InterestName="<<interest.getName()<<endl;


  ndns::Query q2;
  Name name;
  q2.fromInterest(name, interest);
  cout<<"AuthZone="<<q2.getAuthorityZone()<<" RRLable="<<q2.getRrLabel()<<endl;
  BOOST_CHECK_EQUAL(q.getAuthorityZone(), q2.getAuthorityZone());
  BOOST_CHECK_EQUAL(q.getRrLabel(), q2.getRrLabel());
  BOOST_CHECK_EQUAL(q.getQueryType(), q2.getQueryType());
  BOOST_CHECK_EQUAL(q.getFowardingHint(), q2.getFowardingHint());
  printend("query:UnderstandInterst");
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace tests
} // namespace ndns
} // namespace ndn
