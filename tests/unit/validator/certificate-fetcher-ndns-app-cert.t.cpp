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

#include "validator/certificate-fetcher-ndns-appcert.hpp"

#include "ndns-label.hpp"
#include "daemon/name-server.hpp"
#include "daemon/rrset-factory.hpp"
#include "mgmt/management-tool.hpp"
#include "util/cert-helper.hpp"
#include "validator/validator.hpp"

#include "boost-test.hpp"
#include "unit/database-test-data.hpp"

#include <ndn-cxx/security/validation-policy-simple-hierarchy.hpp>
#include <ndn-cxx/util/dummy-client-face.hpp>

namespace ndn {
namespace ndns {
namespace tests {

BOOST_AUTO_TEST_SUITE(AppCertFetcher)

static unique_ptr<security::Validator>
makeValidatorAppCert(Face& face)
{
  return make_unique<security::Validator>(make_unique<security::ValidationPolicySimpleHierarchy>(),
                                          make_unique<CertificateFetcherAppCert>(face));
}

class AppCertFetcherFixture : public DbTestData
{
public:
  AppCertFetcherFixture()
    : m_validatorFace(m_io, m_keyChain, {true, true})
    , m_validator(makeValidatorAppCert(m_validatorFace))
  {
    // build the data and certificate for this test
    buildAppCertAndData();

    auto serverValidator = NdnsValidatorBuilder::create(m_validatorFace, 10, 0,
                                                        UNIT_TESTS_TMPDIR "/validator.conf");
    // initialize all servers
    auto addServer = [this, &serverValidator] (const Name& zoneName) {
      m_serverFaces.push_back(make_unique<util::DummyClientFace>(m_io, m_keyChain,
                                                                 util::DummyClientFace::Options{true, true}));
      m_serverFaces.back()->linkTo(m_validatorFace);

      // validator is used only for check update signature
      // no updates tested here, so validator will not be used
      // passing m_validator is only for construct server
      Name certName = CertHelper::getDefaultCertificateNameOfIdentity(m_keyChain,
                                                                      Name(zoneName).append("NDNS"));
      auto server = make_shared<NameServer>(zoneName, certName, *m_serverFaces.back(),
                                            m_session, m_keyChain, *serverValidator);
      m_servers.push_back(std::move(server));
    };
    addServer(m_testName);
    addServer(m_netName);
    addServer(m_ndnsimName);
    advanceClocks(time::milliseconds(10), 1);
  }

private:
  void
  buildAppCertAndData()
  {
    // create NDNS-stored certificate and the signed data
    Identity ndnsimIdentity = m_keyChain.createIdentity(m_ndnsimName);
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
    tool.addRrset(appCertRrset, true);

    m_appCertSignedData = Data(Name(m_ndnsimName).append("randomData"));
//added_GM liupenghui
#if 1
	ndn::security::Identity identity;
	ndn::security::pib::Key key;
	
	ndn::Name identityName = ndn::security::extractIdentityFromCertName(ndnsStoredAppCert);
	ndn::Name keyName = ndn::security::extractKeyNameFromCertName(ndnsStoredAppCert);
	identity = m_keyChain.getPib().getIdentity(identityName);
	key = identity.getKey(keyName);
	if (key.getKeyType() == KeyType::SM2)
	  m_keyChain.sign(m_appCertSignedData, signingByCertificate(ndnsStoredAppCert).setDigestAlgorithm(ndn::DigestAlgorithm::SM3));
	else
	  m_keyChain.sign(m_appCertSignedData, signingByCertificate(ndnsStoredAppCert));
#else
	m_keyChain.sign(m_appCertSignedData, signingByCertificate(ndnsStoredAppCert));
#endif

    // load this certificate as the trust anchor
    m_validator->loadAnchor("", std::move(ndnsStoredAppCert));
  }

public:
  util::DummyClientFace m_validatorFace;
  unique_ptr<security::Validator> m_validator;
  std::vector<unique_ptr<util::DummyClientFace>> m_serverFaces;
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
                       [&] (const Data& data, const security::ValidationError& str) {
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
