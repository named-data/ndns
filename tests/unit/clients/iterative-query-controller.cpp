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
    , consumerFace(io, {false, true})
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
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace tests
} // namespace ndns
} // namespace ndn
