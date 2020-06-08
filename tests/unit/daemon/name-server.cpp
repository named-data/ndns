/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2020, Regents of the University of California.
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

#include "daemon/name-server.hpp"
#include "daemon/db-mgr.hpp"
#include "clients/response.hpp"
#include "clients/query.hpp"

#include "test-common.hpp"
#include "unit/database-test-data.hpp"

#include <ndn-cxx/util/regex.hpp>

namespace ndn {
namespace ndns {
namespace tests {

NDNS_LOG_INIT(NameServerTest);

class NameServerFixture : public DbTestData
{
public:
  NameServerFixture()
    : face({false, true})
    , zone(m_test.getName())
    , validator(NdnsValidatorBuilder::create(face))
    , server(zone, m_certName, face, m_session, m_keyChain, *validator)
  {
    // ensure prefix is registered
    run();
    advanceClocks(time::milliseconds(10), 1);
  }

  void
  run()
  {
    face.getIoService().poll();
    face.getIoService().reset();
  }

public:
  ndn::util::DummyClientFace face;
  const Name& zone;
  unique_ptr<security::v2::Validator> validator;
  ndns::NameServer server;
};

BOOST_FIXTURE_TEST_SUITE(NameServer, NameServerFixture)

BOOST_AUTO_TEST_CASE(NdnsQuery)
{
  Query q(zone, ndns::label::NDNS_ITERATIVE_QUERY);
  q.setRrLabel(Name("net"));
  q.setRrType(ndns::label::NS_RR_TYPE);

  bool hasDataBack = false;

  face.onSendData.connectSingleShot([&] (const Data& data) {
    hasDataBack = true;
    NDNS_LOG_TRACE("get Data back");
    BOOST_CHECK_EQUAL(data.getName().getPrefix(-1), q.toInterest().getName());

    Response resp;
    BOOST_CHECK_NO_THROW(resp.fromData(zone, data));
    BOOST_CHECK_EQUAL(resp.getContentType(), NDNS_LINK);
  });

  face.receive(q.toInterest());

  run();

  BOOST_CHECK_EQUAL(hasDataBack, true);
}

BOOST_AUTO_TEST_CASE(KeyQuery)
{
  Query q(zone, ndns::label::NDNS_ITERATIVE_QUERY);
  q.setRrType(ndns::label::CERT_RR_TYPE);

  size_t nDataBack = 0;

  // will ask for non-existing record
  face.onSendData.connectSingleShot([&] (const Data& data) {
    ++nDataBack;
    NDNS_LOG_TRACE("get Data back");
    BOOST_CHECK_EQUAL(data.getName().getPrefix(-1), q.toInterest().getName());

    Response resp;
    BOOST_CHECK_NO_THROW(resp.fromData(zone, data));
    BOOST_CHECK_EQUAL(resp.getContentType(), NDNS_NACK);
  });

  face.receive(q.toInterest());
  run();

  // will ask for the existing record (will have type NDNS_KEY, as it is certificate)
  face.onSendData.connectSingleShot([&] (const Data& data) {
    ++nDataBack;
    NDNS_LOG_TRACE("get Data back");
    BOOST_CHECK_EQUAL(data.getName().getPrefix(-1), q.toInterest().getName());

    Response resp;
    BOOST_CHECK_NO_THROW(resp.fromData(zone, data));
    BOOST_CHECK_EQUAL(resp.getContentType(), NDNS_KEY);
  });

  Response certResp;
  certResp.fromData(zone, m_cert);
  q.setRrLabel(certResp.getRrLabel());

  face.receive(q.toInterest());
  run();

  BOOST_CHECK_EQUAL(nDataBack, 2);

  // explicit interest with correct version
  face.receive(Interest(m_cert.getName()));

  face.onSendData.connectSingleShot([&] (const Data& data) {
    ++nDataBack;

    Response resp;
    BOOST_CHECK_NO_THROW(resp.fromData(zone, data));
    BOOST_CHECK_EQUAL(resp.getContentType(), NDNS_KEY);
  });

  run();
  BOOST_CHECK_EQUAL(nDataBack, 3);

  // explicit interest with wrong version
  Name wrongName = m_cert.getName().getPrefix(-1);
  wrongName.appendVersion();
  face.receive(Interest(wrongName));

  face.onSendData.connectSingleShot([&] (const Data& data) {
    ++nDataBack;

    Response resp;
    BOOST_CHECK_NO_THROW(resp.fromData(zone, data));
    BOOST_CHECK_EQUAL(resp.getContentType(), NDNS_NACK);
  });

  run();
  BOOST_CHECK_EQUAL(nDataBack, 4);
}

BOOST_AUTO_TEST_CASE(UpdateReplaceRr)
{
  Response re;
  re.setZone(zone);
  re.setQueryType(label::NDNS_ITERATIVE_QUERY);
  re.setRrLabel(Name("net"));
  re.setRrType(label::NS_RR_TYPE);
  re.setContentType(NDNS_RESP);

  std::string str = "ns1.ndnsim.net";
  re.addRr(makeBinaryBlock(ndns::tlv::RrData, str.c_str(), str.size()));
  str = "ns2.ndnsim.net";
  re.addRr(makeBinaryBlock(ndns::tlv::RrData, str.c_str(), str.size()));

  shared_ptr<Data> data = re.toData();
  m_keyChain.sign(*data, security::signingByCertificate(m_cert));

  Query q(Name(zone), ndns::label::NDNS_ITERATIVE_QUERY);
  const Block& block = data->wireEncode();
  Name name;
  name.append(block);

  q.setRrLabel(name);
  q.setRrType(label::NDNS_UPDATE_LABEL);

  bool hasDataBack = false;

  face.onSendData.connectSingleShot([&] (const Data& data) {
    hasDataBack = true;
    NDNS_LOG_TRACE("get Data back");
    BOOST_CHECK_EQUAL(data.getName().getPrefix(-1), q.toInterest().getName());
    Response resp;

    BOOST_CHECK_NO_THROW(resp.fromData(zone, data));
    BOOST_CHECK_EQUAL(resp.getContentType(), NDNS_RESP); // by default NDNS_BLOB is enough
    BOOST_CHECK_GT(resp.getRrs().size(), 0);
    Block block = resp.getRrs()[0];
    block.parse();
    int ret = -1;
    BOOST_CHECK_EQUAL(block.type(), ndns::tlv::RrData);
    Block::element_const_iterator val = block.elements_begin();
    BOOST_CHECK_EQUAL(val->type(), ndns::tlv::UpdateReturnCode); // the first must be return code
    ret = readNonNegativeInteger(*val);
    BOOST_CHECK_EQUAL(ret, 0);
  });
  face.receive(q.toInterest());
  run();

  BOOST_CHECK_EQUAL(hasDataBack, true);
}

BOOST_AUTO_TEST_CASE(UpdateInsertNewRr)
{
  Response re;
  re.setZone(zone);
  re.setQueryType(label::NDNS_ITERATIVE_QUERY);
  re.setRrLabel(Name("net-XYZ")); // insert new records
  re.setRrType(label::NS_RR_TYPE);
  re.setContentType(NDNS_RESP);

  std::string str = "ns1.ndnsim.net";
  re.addRr(makeBinaryBlock(ndns::tlv::RrData, str.c_str(), str.size()));
  str = "ns2.ndnsim.net";
  re.addRr(makeBinaryBlock(ndns::tlv::RrData, str.c_str(), str.size()));

  shared_ptr<Data> data = re.toData();
  m_keyChain.sign(*data, security::signingByCertificate(m_cert));

  Query q(Name(zone), ndns::label::NDNS_ITERATIVE_QUERY);
  const Block& block = data->wireEncode();
  Name name;
  name.append(block);

  q.setRrLabel(name);
  q.setRrType(label::NDNS_UPDATE_LABEL);

  bool hasDataBack = false;

  face.onSendData.connectSingleShot([&] (const Data& data) {
    hasDataBack = true;
    NDNS_LOG_TRACE("get Data back");
    BOOST_CHECK_EQUAL(data.getName().getPrefix(-1), q.toInterest().getName());
    Response resp;

    BOOST_CHECK_NO_THROW(resp.fromData(zone, data));
    BOOST_CHECK_EQUAL(resp.getContentType(), NDNS_RESP); // by default NDNS_BLOB is enough
    BOOST_CHECK_GT(resp.getRrs().size(), 0);
    Block block = resp.getRrs()[0];
    block.parse();
    int ret = -1;
    BOOST_CHECK_EQUAL(block.type(), ndns::tlv::RrData);
    Block::element_const_iterator val = block.elements_begin();
    BOOST_CHECK_EQUAL(val->type(), ndns::tlv::UpdateReturnCode); // the first must be return code
    ret = readNonNegativeInteger(*val);
    BOOST_CHECK_EQUAL(ret, 0);
  });

  face.receive(q.toInterest());
  run();

  BOOST_CHECK_EQUAL(hasDataBack, true);
}

BOOST_AUTO_TEST_CASE(UpdateValidatorCannotFetchCert)
{
  Identity zoneIdentity = m_keyChain.createIdentity(TEST_IDENTITY_NAME);
  Key dsk = m_keyChain.createKey(zoneIdentity);

  Name dskCertName = dsk.getName();
  dskCertName
    .append("CERT")
    .appendVersion();
  Certificate dskCert;
  dskCert.setName(dskCertName);
  dskCert.setContentType(ndn::tlv::ContentType_Key);
  dskCert.setFreshnessPeriod(time::hours(1));
  dskCert.setContent(dsk.getPublicKey().data(), dsk.getPublicKey().size());
  SignatureInfo info;
  info.setValidityPeriod(security::ValidityPeriod(time::system_clock::now(),
                                                  time::system_clock::now() + time::days(365)));

  m_keyChain.sign(dskCert, security::signingByCertificate(m_cert));
  m_keyChain.setDefaultCertificate(dsk, dskCert);

  NDNS_LOG_TRACE("KeyChain: add cert: " << dskCert.getName() << ". KeyLocator: "
                 << dskCert.getKeyLocator()->getName());

  Rrset rrset(&m_test);
  Name label = dskCert.getName().getPrefix(-2).getSubName(m_test.getName().size() + 1);
  rrset.setLabel(label);
  rrset.setType(label::CERT_RR_TYPE);
  rrset.setVersion(dskCert.getName().get(-1));
  rrset.setTtl(m_test.getTtl());
  rrset.setData(dskCert.wireEncode());
  m_session.insert(rrset);
  NDNS_LOG_TRACE("DB: zone " << m_test << " add a CERT RR with name="
                 << dskCert.getName() << " rrLabel=" << label);

  Response re;
  re.setZone(zone);
  re.setQueryType(label::NDNS_ITERATIVE_QUERY);
  re.setRrLabel(Name("ndnsim-XYZ")); // insert new records
  re.setRrType(label::NS_RR_TYPE);
  re.setContentType(NDNS_RESP);

  std::string str = "ns1.ndnsim.net";
  re.addRr(makeBinaryBlock(ndns::tlv::RrData, str.c_str(), str.size()));
  str = "ns2.ndnsim.net";
  re.addRr(makeBinaryBlock(ndns::tlv::RrData, str.c_str(), str.size()));

  shared_ptr<Data> data = re.toData();
  m_keyChain.sign(*data, security::signingByCertificate(dskCert));

  Query q(Name(zone), ndns::label::NDNS_ITERATIVE_QUERY);
  const Block& block = data->wireEncode();
  Name name;
  name.append(block);

  q.setRrLabel(name);
  q.setRrType(label::NDNS_UPDATE_LABEL);

  bool hasDataBack = false;

  // no data back, since the Update cannot pass verification
  face.onSendData.connectSingleShot([&] (const Data&) {
    hasDataBack = true;
    BOOST_FAIL("UNEXPECTED");
  });

  face.receive(q.toInterest());
  run();

  BOOST_CHECK_EQUAL(hasDataBack, false);
}

class NameServerFixture2 : public DbTestData
{
public:
  NameServerFixture2()
    : face(io, m_keyChain, {false, true})
    , validatorFace(io, m_keyChain, {false, true})
    , zone(m_test.getName())
    , validator(NdnsValidatorBuilder::create(validatorFace)) // different face for validator
    , server(zone, m_certName, face, m_session, m_keyChain, *validator)
  {
    // ensure prefix is registered
    run();

    validatorFace.onSendInterest.connect([this] (const Interest& interest) {
      NDNS_LOG_TRACE("validatorFace get Interest: " << interest.getName());

      shared_ptr<const Interest> i = interest.shared_from_this();
      io.post([i, this] {
          face.receive(*i);
        });
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
  ndn::util::DummyClientFace face;
  ndn::util::DummyClientFace validatorFace;
  const Name& zone;
  unique_ptr<security::v2::Validator> validator;
  ndns::NameServer server;
};

BOOST_FIXTURE_TEST_CASE(UpdateValidatorFetchCert, NameServerFixture2)
{
  Response re;
  re.setZone(zone);
  re.setQueryType(label::NDNS_ITERATIVE_QUERY);
  re.setRrLabel(Name("ndnsim-XYZ")); // insert new records
  re.setRrType(label::NS_RR_TYPE);
  re.setContentType(NDNS_RESP);

  std::string str = "ns1.ndnsim.net";
  re.addRr(makeBinaryBlock(ndns::tlv::RrData, str.c_str(), str.size()));
  str = "ns2.ndnsim.net";
  re.addRr(makeBinaryBlock(ndns::tlv::RrData, str.c_str(), str.size()));

  shared_ptr<Data> data = re.toData();
  m_keyChain.sign(*data, security::signingByCertificate(m_cert));

  Query q(Name(zone), ndns::label::NDNS_ITERATIVE_QUERY);
  const Block& block = data->wireEncode();
  Name name;
  name.append(block);

  q.setRrLabel(name);
  q.setRrType(label::NDNS_UPDATE_LABEL);

  bool hasDataBack = false;

  shared_ptr<Regex> regex = make_shared<Regex>("(<>*)<NDNS><KEY>(<>+)<CERT><>");
  face.onSendData.connect([&] (const Data& data) {
    if (regex->match(data.getName())) {
      shared_ptr<const Data> d = data.shared_from_this();
      io.post([d, this] {
          validatorFace.receive(*d); // It's data requested by validator
        });
    }
    else {
      // cert is requested by validator
      hasDataBack = true;
      NDNS_LOG_TRACE("get Data back");
      BOOST_CHECK_EQUAL(data.getName().getPrefix(-1), q.toInterest().getName());
      Response resp;

      BOOST_CHECK_NO_THROW(resp.fromData(zone, data));
      BOOST_CHECK_EQUAL(resp.getContentType(), NDNS_RESP); // by default NDNS_BLOB is enough
      BOOST_CHECK_GT(resp.getRrs().size(), 0);
      Block block = resp.getRrs()[0];
      block.parse();
      int ret = -1;
      BOOST_CHECK_EQUAL(block.type(), ndns::tlv::RrData);
      Block::element_const_iterator val = block.elements_begin();
      BOOST_CHECK_EQUAL(val->type(), ndns::tlv::UpdateReturnCode); // the first must be return code
      ret = readNonNegativeInteger(*val);
      BOOST_CHECK_EQUAL(ret, 0);
    }
  });

  face.receive(q.toInterest());
  run();

  BOOST_CHECK_EQUAL(hasDataBack, true);
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace tests
} // namespace ndns
} // namespace ndn
