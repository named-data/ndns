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

#include "response.hpp"
#include "rr.hpp"
#include "query.hpp"
#include "rr.hpp"

#include "boost-test.hpp"
#include <string>


namespace ndn {
namespace ndns {
namespace tests {


using namespace std;

BOOST_AUTO_TEST_SUITE(response)

BOOST_AUTO_TEST_CASE(Encode)
{

  printbegin("response:Encode");

  Response re, re2;
  cout<<"construct a Response Instance"<<endl;
  vector<RR> vec;

  RR rr;
  std::string ex3 = "www3.ex.com";
  uint32_t id = 203;
  rr.setRrdata(ex3);
  rr.setId(id);
  vec.push_back(rr);


  RR rr2;
  std::string ex4 = "www4.ex.com";
  id = 204;
  rr2.setRrdata(ex4);
  rr2.setId(id);
  vec.push_back(rr2);

  ndns::Query q;
  Name n1("/dns.google.com");
  Name n2("/www.baidu.com");
  q.setAuthorityZone(n1);
  q.setRrLabel(n2);
  q.setQueryType(ndns::Query::QUERY_DNS_R);
  Interest interest = q.toInterest();
  Name n3(q.toInterest().getName());
  n3.appendNumber((uint64_t)1313344);

  re.setQueryName(n3);
  re.setFreshness(time::milliseconds(4444));
  re.setRrs(vec);



  cout<<"before encode"<<endl;
  //ndn::Block block = re.wireEncode();
  //re.wireEncode();
  Data data = re.toData();
  cout<<re.toData()<<endl;

  cout<<"Encode finishes"<<endl;


  re2.fromData(n2, data);
  BOOST_CHECK_EQUAL(re.getQueryName(), re2.getQueryName());
  BOOST_CHECK_EQUAL(re.getContentType(), re2.getContentType());
  BOOST_CHECK_EQUAL(re.getResponseType(), re2.getResponseType());
  BOOST_CHECK_EQUAL(re.getRrs().size(), re2.getRrs().size());
  BOOST_CHECK_EQUAL(re.getRrs()[0], re2.getRrs()[0]);
  BOOST_CHECK_EQUAL(re.getRrs()[1], re2.getRrs()[1]);


  printend("response:Encode");
}

BOOST_AUTO_TEST_SUITE_END()


} // namespace tests
} // namespace ndns
} // namespace ndn
