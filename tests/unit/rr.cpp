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

#include "rr.hpp"
#include "boost-test.hpp"

//#include <boost/test/test_tools.hpp>
//#include <string>

namespace ndn {
namespace ndns {
namespace tests {

using namespace std;

BOOST_AUTO_TEST_SUITE(rr)

BOOST_AUTO_TEST_CASE(Encode)
{
  string label = "rr:Encode";
  printbegin(label);
  RR rr;
  std::string ex = "www2.ex.com";
  uint32_t id = 201;
  rr.setRrdata(ex);
  rr.setId(id);

  cout<<"before encode"<<endl;
  ndn::Block block = rr.wireEncode();
  cout<<"Encode finishes with totalLenght="<<block.size()<<endl;



  RR rr2;
  rr2.wireDecode(block);

  BOOST_CHECK_EQUAL(rr2.getRrdata(), ex);
  BOOST_CHECK_EQUAL(rr2.getId(), id);
  printend(label);
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace tests
} // namespace ndns
} // namespace ndn
