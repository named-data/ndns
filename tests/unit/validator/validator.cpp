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

#include "validator/validator.hpp"
#include "ndns-label.hpp"
#include "util/cert-helper.hpp"
#include "daemon/name-server.hpp"

#include "test-common.hpp"
#include "dummy-forwarder.hpp"
#include "unit/database-test-data.hpp"

#include <ndn-cxx/util/io.hpp>

namespace ndn {
namespace ndns {
namespace tests {

NDNS_LOG_INIT("ValidatorTest")

BOOST_AUTO_TEST_SUITE(Validator)

class ValidatorTestFixture : public DbTestData
{
public:
  ValidatorTestFixture()
    : m_forwarder(m_io, m_keyChain)
    , m_face(m_forwarder.addFace())
    , m_validator(NdnsValidatorBuilder::create(m_face, 500, 0, TEST_CONFIG_PATH "/" "validator.conf"))
  {
    // generate a random cert
    // check how does name-server test do
    // initlize all servers
    auto addServer = [&] (const Name& zoneName) {
      Face& face = m_forwarder.addFace();
      // validator is used only for check update signature
      // no updates tested here, so validator will not be used
      // passing m_validator is only for construct server
      Name certName = CertHelper::getDefaultCertificateNameOfIdentity(m_keyChain,
                                                                      Name(zoneName).append("NDNS"));
      auto server = make_shared<NameServer>(zoneName, certName, face,
                                            m_session, m_keyChain, *m_validator);
      m_servers.push_back(server);
    };
    addServer(m_testName);
    addServer(m_netName);
    addServer(m_ndnsimName);
    m_ndnsimCert = CertHelper::getDefaultCertificateNameOfIdentity(m_keyChain,
                                                        Name(m_ndnsimName).append("NDNS"));
    m_randomCert = m_keyChain.createIdentity("/random/identity").getDefaultKey()
    .getDefaultCertificate().getName();
    advanceClocks(time::milliseconds(10), 1);
  }

  ~ValidatorTestFixture()
  {
    m_face.getIoService().stop();
    m_face.shutdown();
  }

public:
  DummyForwarder m_forwarder;
  ndn::Face& m_face;
  unique_ptr<security::v2::Validator> m_validator;
  std::vector<shared_ptr<ndns::NameServer>> m_servers;
  Name m_ndnsimCert;
  Name m_randomCert;
};


BOOST_FIXTURE_TEST_CASE(Basic, ValidatorTestFixture)
{
  SignatureInfo info;
  info.setValidityPeriod(security::ValidityPeriod(time::system_clock::TimePoint::min(),
                                                  time::system_clock::now() + time::days(10)));

  // case1: record of testId3, signed by its dsk, should be successful validated.
  Name dataName;
  dataName
    .append(m_ndnsimName)
    .append("NDNS")
    .append("rrLabel")
    .append("rrType")
    .appendVersion();
  shared_ptr<Data> data = make_shared<Data>(dataName);
  m_keyChain.sign(*data, signingByCertificate(m_ndnsimCert).setSignatureInfo(info));

  bool hasValidated = false;
  m_validator->validate(*data,
                        [&] (const Data& data) {
                          hasValidated = true;
                          BOOST_CHECK(true);
                        },
                        [&] (const Data& data, const security::v2::ValidationError& str) {
                          hasValidated = true;
                          BOOST_CHECK(false);
                        });

  advanceClocks(time::seconds(3), 100);
  // m_io.run();
  BOOST_CHECK_EQUAL(hasValidated, true);

  // case2: signing testId2's data by testId3's key, which should failed in validation
  dataName = Name();
  dataName
    .append(m_netName)
    .append("NDNS")
    .append("rrLabel")
    .append("CERT")
    .appendVersion();
  data = make_shared<Data>(dataName);
  m_keyChain.sign(*data, signingByCertificate(m_ndnsimCert)); // key's owner's name is longer than data owner's

  hasValidated = false;
  m_validator->validate(*data,
                        [&] (const Data& data) {
                          hasValidated = true;
                          BOOST_CHECK(false);
                        },
                        [&] (const Data& data, const security::v2::ValidationError& str) {
                          hasValidated = true;
                          BOOST_CHECK(true);
                        });

  advanceClocks(time::seconds(3), 100);
  // cannot pass verification due to key's owner's name is longer than data owner's
  BOOST_CHECK_EQUAL(hasValidated, true);

  // case3: totally wrong key to sign
  dataName = Name();
  dataName
    .append(m_ndnsimName)
    .append("NDNS")
    .append("rrLabel")
    .append("CERT")
    .appendVersion();
  data = make_shared<Data>(dataName);
  m_keyChain.sign(*data, signingByCertificate(m_randomCert));

  hasValidated = false;
  m_validator->validate(*data,
                        [&] (const Data& data) {
                          hasValidated = true;
                          BOOST_CHECK(false);
                        },
                        [&] (const Data& data, const security::v2::ValidationError& str) {
                          hasValidated = true;
                          BOOST_CHECK(true);
                        });

  advanceClocks(time::seconds(3), 100);
  // cannot pass due to a totally mismatched key
  BOOST_CHECK_EQUAL(hasValidated, true);
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace tests
} // namespace ndns
} // namespace ndn
