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

#include "daemon/db-mgr.hpp"
#include "daemon/name-server.hpp"
#include "clients/response.hpp"
#include "clients/query.hpp"
#include "logger.hpp"

#include "../database-test-data.hpp"
#include "../../boost-test.hpp"

#include <boost/filesystem.hpp>
#include <ndn-cxx/face.hpp>
#include <ndn-cxx/util/dummy-client-face.hpp>

namespace ndn {
namespace ndns {
namespace tests {

NDNS_LOG_INIT("NameServerTest");

class NameServerFixture : public DbTestData
{
public:
  NameServerFixture()
    : face(ndn::util::makeDummyClientFace({ false, true }))
    , zone(m_net.getName())
    , validator(*face)
    , server(zone, m_certName, *face, m_session, m_keyChain, validator)
  {
    // ensure prefix is registered
    run();
  }

  void
  run()
  {
    face->getIoService().poll();
    face->getIoService().reset();
  }

public:
  shared_ptr<ndn::util::DummyClientFace> face;
  Name hint;
  const Name& zone;
  Validator validator;
  ndns::NameServer server;
};

BOOST_FIXTURE_TEST_SUITE(NameServer, NameServerFixture)

BOOST_AUTO_TEST_CASE(NdnsQuery)
{
  Query q(hint, zone, ndns::label::NDNS_ITERATIVE_QUERY);
  q.setRrLabel(Name("ndnsim"));
  q.setRrType(ndns::label::NS_RR_TYPE);

  bool hasDataBack = false;

  face->onData += [&] (const Data& data) {
    hasDataBack = true;
    NDNS_LOG_TRACE("get Data back");
    BOOST_CHECK_EQUAL(data.getName().getPrefix(-1), q.toInterest().getName());

    Response resp;
    BOOST_CHECK_NO_THROW(resp.fromData(hint, zone, data));
    BOOST_CHECK_EQUAL(resp.getNdnsType(), NDNS_RESP);
  };

  face->receive(q.toInterest());

  run();

  BOOST_CHECK_EQUAL(hasDataBack, true);
}

BOOST_AUTO_TEST_CASE(KeyQuery)
{
  Query q(hint, zone, ndns::label::NDNS_ITERATIVE_QUERY);
  q.setQueryType(ndns::label::NDNS_CERT_QUERY);
  q.setRrType(ndns::label::CERT_RR_TYPE);

  size_t nDataBack = 0;

  // will ask for non-existing record
  face->onData += [&] (const Data& data) {
    ++nDataBack;
    NDNS_LOG_TRACE("get Data back");
    BOOST_CHECK_EQUAL(data.getName().getPrefix(-1), q.toInterest().getName());

    Response resp;
    BOOST_CHECK_NO_THROW(resp.fromData(hint, zone, data));
    BOOST_CHECK_EQUAL(resp.getNdnsType(), NDNS_NACK);
  };

  face->receive(q.toInterest());
  run();

  // will ask for the existing record (will have type NDNS_RAW, as it is certificate)
  face->onData.clear();
  face->onData += [&] (const Data& data) {
    ++nDataBack;
    NDNS_LOG_TRACE("get Data back");
    BOOST_CHECK_EQUAL(data.getName().getPrefix(-1), q.toInterest().getName());

    Response resp;
    BOOST_CHECK_NO_THROW(resp.fromData(hint, zone, data));
    BOOST_CHECK_EQUAL(resp.getNdnsType(), NDNS_RAW);
  };

  q.setRrLabel("dsk-3");

  face->receive(q.toInterest());
  run();

  BOOST_CHECK_EQUAL(nDataBack, 2);
}

BOOST_AUTO_TEST_CASE(UpdateReplaceRr)
{
  Response re;
  re.setZone(zone);
  re.setQueryType(label::NDNS_ITERATIVE_QUERY);
  re.setRrLabel(Name("ndnsim"));
  re.setRrType(label::NS_RR_TYPE);
  re.setNdnsType(NDNS_RESP);

  std::string str = "ns1.ndnsim.net";
  re.addRr(dataBlock(ndns::tlv::RrData, str.c_str(), str.size()));
  str = "ns2.ndnsim.net";
  re.addRr(dataBlock(ndns::tlv::RrData, str.c_str(), str.size()));

  shared_ptr<Data> data = re.toData();
  m_keyChain.sign(*data, m_certName);

  Query q(Name(hint), Name(zone), ndns::label::NDNS_ITERATIVE_QUERY);
  const Block& block = data->wireEncode();
  Name name;
  name.append(block);

  q.setRrLabel(name);
  q.setRrType(label::NDNS_UPDATE_LABEL);

  bool hasDataBack = false;

  face->onData += [&] (const Data& data) {
    hasDataBack = true;
    NDNS_LOG_TRACE("get Data back");
    BOOST_CHECK_EQUAL(data.getName().getPrefix(-1), q.toInterest().getName());
    Response resp;

    BOOST_CHECK_NO_THROW(resp.fromData(hint, zone, data));
    std::cout << resp << std::endl;
    BOOST_CHECK_EQUAL(resp.getNdnsType(), NDNS_RESP); // by default NDNS_RAW is enough
    BOOST_CHECK_GT(resp.getRrs().size(), 0);
    Block block = resp.getRrs()[0];
    block.parse();
    int ret = -1;
    BOOST_CHECK_EQUAL(block.type(), ndns::tlv::RrData);
    Block::element_const_iterator val = block.elements_begin();
    BOOST_CHECK_EQUAL(val->type(), ndns::tlv::UpdateReturnCode); // the first must be return code
    ret = readNonNegativeInteger(*val);
    BOOST_CHECK_EQUAL(ret, 0);
  };

  face->receive(q.toInterest());
  run();

  BOOST_CHECK_EQUAL(hasDataBack, true);
}

BOOST_AUTO_TEST_CASE(UpdateInsertNewRr)
{
  Response re;
  re.setZone(zone);
  re.setQueryType(label::NDNS_ITERATIVE_QUERY);
  re.setRrLabel(Name("ndnsim-XYZ")); // insert new records
  re.setRrType(label::NS_RR_TYPE);
  re.setNdnsType(NDNS_RESP);

  std::string str = "ns1.ndnsim.net";
  re.addRr(dataBlock(ndns::tlv::RrData, str.c_str(), str.size()));
  str = "ns2.ndnsim.net";
  re.addRr(dataBlock(ndns::tlv::RrData, str.c_str(), str.size()));

  shared_ptr<Data> data = re.toData();
  m_keyChain.sign(*data, m_certName);

  Query q(Name(hint), Name(zone), ndns::label::NDNS_ITERATIVE_QUERY);
  const Block& block = data->wireEncode();
  Name name;
  name.append(block);

  q.setRrLabel(name);
  q.setRrType(label::NDNS_UPDATE_LABEL);

  bool hasDataBack = false;

  face->onData += [&] (const Data& data) {
    hasDataBack = true;
    NDNS_LOG_TRACE("get Data back");
    BOOST_CHECK_EQUAL(data.getName().getPrefix(-1), q.toInterest().getName());
    Response resp;

    BOOST_CHECK_NO_THROW(resp.fromData(hint, zone, data));
    std::cout << resp << std::endl;
    BOOST_CHECK_EQUAL(resp.getNdnsType(), NDNS_RESP); // by default NDNS_RAW is enough
    BOOST_CHECK_GT(resp.getRrs().size(), 0);
    Block block = resp.getRrs()[0];
    block.parse();
    int ret = -1;
    BOOST_CHECK_EQUAL(block.type(), ndns::tlv::RrData);
    Block::element_const_iterator val = block.elements_begin();
    BOOST_CHECK_EQUAL(val->type(), ndns::tlv::UpdateReturnCode); // the first must be return code
    ret = readNonNegativeInteger(*val);
    BOOST_CHECK_EQUAL(ret, 0);
  };

  face->receive(q.toInterest());
  run();

  BOOST_CHECK_EQUAL(hasDataBack, true);
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace tests
} // namespace ndns
} // namespace ndn
