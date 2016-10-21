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

#include "clients/iterative-query-controller.hpp"
#include "daemon/name-server.hpp"

#include "test-common.hpp"
#include "unit/database-test-data.hpp"

namespace ndn {
namespace ndns {
namespace tests {

NDNS_LOG_INIT("IterativeQueryControllerTest")

class QueryControllerFixture : public DbTestData
{
public:
  QueryControllerFixture()
    : producerFace(io, {false, true})
    , consumerFace(io, {true, true})
    , validator(producerFace)
    , top(m_root.getName(), m_certName, producerFace, m_session, m_keyChain, validator)
    , net(m_net.getName(), m_certName, producerFace, m_session, m_keyChain, validator)
    , ndnsim(m_ndnsim.getName(), m_certName, producerFace, m_session, m_keyChain, validator)
  {
    run();
    producerFace.onSendInterest.connect([this] (const Interest& interest) {
      io.post([=] { consumerFace.receive(interest); });
    });
    consumerFace.onSendInterest.connect([this] (const Interest& interest) {
      io.post([=] { producerFace.receive(interest); });
    });
    producerFace.onSendData.connect([this] (const Data& data) {
      io.post([=] { consumerFace.receive(data); });
    });
    consumerFace.onSendData.connect([this] (const Data& data) {
      io.post([=] { producerFace.receive(data); });
    });
  }

  void
  run()
  {
    io.poll();
    io.reset();
  }

public:
  boost::asio::io_service io;
  ndn::util::DummyClientFace producerFace;
  ndn::util::DummyClientFace consumerFace;

  Validator validator;
  ndns::NameServer top;
  ndns::NameServer net;
  ndns::NameServer ndnsim;
};


BOOST_AUTO_TEST_SUITE(IterativeQueryController)

BOOST_AUTO_TEST_CASE_EXPECTED_FAILURES(Basic, 3)

BOOST_FIXTURE_TEST_CASE(Basic, QueryControllerFixture)
{
  using std::string;
  using ndns::NameServer;

  Name name = m_ndnsim.getName();
  Name dstLabel(name.append("www"));
  name::Component rrType("TXT");
  time::milliseconds lifetime(4000);

  bool hasDataBack = false;
  auto ctr = make_shared<ndns::IterativeQueryController>(
    dstLabel, rrType, lifetime,
    [&hasDataBack] (const Data&, const Response&) {
      hasDataBack = true;
      BOOST_CHECK(true);
    },
    [&hasDataBack] (uint32_t errCode, const std::string& errMsg) {
      BOOST_CHECK(false);
    },
    consumerFace);

  // IterativeQueryController is a whole process
  // the tester should not send Interest one by one
  // instead of starting it and letting it handle Interest/Data automatically
  ctr->setStartComponentIndex(1);

  ctr->start();

  run();

  BOOST_CHECK_EQUAL(hasDataBack, true);

  const std::vector<Interest>& interestRx = consumerFace.sentInterests;
  BOOST_CHECK_EQUAL(interestRx.size(), 4);

  std::vector<std::string> interestNames =
    {
      "/test19/NDNS/net/NS",
      "/test19/net/NDNS/ndnsim/NS",
      "/test19/net/ndnsim/NDNS/www/NS",
      "/test19/net/ndnsim/NDNS/www/TXT"
    };
  for (int i = 0; i < 4; i++) {
    // check if NDNS do iterative-query with right names
    BOOST_CHECK_EQUAL(interestRx[i].getName(), Name(interestNames[i]));
    // except for the first one, interest sent should has a Link object
    if (i > 0) {
      BOOST_CHECK_EQUAL(interestRx[i].hasLink(), true);
      if (interestRx[i].hasLink()) {
        BOOST_CHECK_EQUAL(interestRx[i].getLink(), m_links[i - 1]);
      }
    }
  }

}

BOOST_AUTO_TEST_SUITE_END()

} // namespace tests
} // namespace ndns
} // namespace ndn
