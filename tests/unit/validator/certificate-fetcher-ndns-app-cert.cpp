/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
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
#include "validator/certificate-fetcher-ndns-appcert.hpp"
#include "ndns-label.hpp"
#include "util/cert-helper.hpp"
#include "daemon/name-server.hpp"
#include "daemon/rrset-factory.hpp"
#include "mgmt/management-tool.hpp"

#include "test-common.hpp"
#include "dummy-forwarder.hpp"
#include "unit/database-test-data.hpp"

#include <ndn-cxx/util/io.hpp>
#include <ndn-cxx/security/v2/validation-policy-simple-hierarchy.hpp>

namespace ndn {
namespace ndns {
namespace tests {

NDNS_LOG_INIT("AppCertFetcher")

BOOST_AUTO_TEST_SUITE(AppCertFetcher)

unique_ptr<security::v2::Validator>
CreateValidatorAppCert(Face& face)
{
  return make_unique<security::v2::Validator>(make_unique<::ndn::security::v2::ValidationPolicySimpleHierarchy>(),
                                              make_unique<CertificateFetcherAppCert>(face));
}

class AppCertFetcherFixture : public DbTestData
{
public:
  AppCertFetcherFixture()
    : m_forwarder(m_io, m_keyChain)
    , m_face(m_forwarder.addFace())
    , m_validator(CreateValidatorAppCert(m_face))
  {
    // build the data and certificate for this test
    buildAppCertAndData();

    auto validatorOnlyForConstructServer = NdnsValidatorBuilder::create(m_face, 10, 0, TEST_CONFIG_PATH "/" "validator.conf");
    // initlize all servers
    auto addServer = [&] (const Name& zoneName) {
      Face& face = m_forwarder.addFace();
      // validator is used only for check update signature
      // no updates tested here, so validator will not be used
      // passing m_validator is only for construct server
      Name certName = CertHelper::getDefaultCertificateNameOfIdentity(m_keyChain,
                                                           Name(zoneName).append("NDNS"));
      auto server = make_shared<NameServer>(zoneName, certName, face,
                                            m_session, m_keyChain, *validatorOnlyForConstructServer);
      m_servers.push_back(server);
    };
    addServer(m_testName);
    addServer(m_netName);
    addServer(m_ndnsimName);
    advanceClocks(time::milliseconds(10), 1);
  }

  ~AppCertFetcherFixture()
  {
    m_face.getIoService().stop();
    m_face.shutdown();
  }

private:
  void
  buildAppCertAndData()
  {
    // create NDNS-stored certificate and the signed data
    Identity ndnsimIdentity = addIdentity(m_ndnsimName);
    Key randomKey = m_keyChain.createKey(ndnsimIdentity);
    Certificate ndnsStoredAppCert = randomKey.getDefaultCertificate();
    RrsetFactory rf(TEST_DATABASE.string(), m_ndnsimName, m_keyChain,
                    CertHelper::getIdentity(m_keyChain, Name(m_ndnsimName).append(label::NDNS_ITERATIVE_QUERY))
                                                                          .getDefaultKey()
                                                                          .getDefaultCertificate()
                                                                          .getName());
    rf.onlyCheckZone();
    Rrset appCertRrset = rf.generateCertRrset(randomKey.getName().getSubName(-2),
                                              VERSION_USE_UNIX_TIMESTAMP, DEFAULT_RR_TTL,
                                              ndnsStoredAppCert);
    ManagementTool tool(TEST_DATABASE.string(), m_keyChain);
    tool.addRrset(appCertRrset);

    m_appCertSignedData = Data(Name(m_ndnsimName).append("randomData"));
    m_keyChain.sign(m_appCertSignedData, signingByCertificate(ndnsStoredAppCert));

    // load this certificate as the trust anchor
    m_validator->loadAnchor("", std::move(ndnsStoredAppCert));
  }

public:
  DummyForwarder m_forwarder;
  ndn::Face& m_face;
  unique_ptr<security::v2::Validator> m_validator;
  std::vector<shared_ptr<ndns::NameServer>> m_servers;
  Data m_appCertSignedData;
};


BOOST_FIXTURE_TEST_CASE(Basic, AppCertFetcherFixture)
{
  bool hasValidated = false;
  m_validator->validate(m_appCertSignedData,
                       [&] (const Data& data) {
                         hasValidated = true;
                         BOOST_CHECK(true);
                       },
                       [&] (const Data& data, const security::v2::ValidationError& str) {
                         hasValidated = true;
                         BOOST_CHECK(false);
                       });
  advanceClocks(time::milliseconds(10), 1000);
  BOOST_CHECK_EQUAL(hasValidated, true);
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace tests
} // namespace ndns
} // namespace ndn
