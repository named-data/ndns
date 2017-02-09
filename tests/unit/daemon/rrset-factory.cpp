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

#include "daemon/rrset-factory.hpp"

#include "test-common.hpp"
#include "mgmt/management-tool.hpp"

#include <boost/lexical_cast.hpp>
#include <ndn-cxx/security/verification-helpers.hpp>

namespace ndn {
namespace ndns {
namespace tests {

NDNS_LOG_INIT("RrsetFactoryTest")

class RrsetFactoryFixture : public IdentityManagementFixture
{
public:
  RrsetFactoryFixture()
    : TEST_DATABASE2(TEST_CONFIG_PATH "/" "test-ndns.db"),
      TEST_IDENTITY_NAME("/rrest/factory"),
      TEST_CERT(TEST_CONFIG_PATH "/" "anchors/root.cert"),
      m_session(TEST_DATABASE2.string()),
      m_zoneName(TEST_IDENTITY_NAME)
  {
    Zone zone1;
    zone1.setName(m_zoneName);
    zone1.setTtl(time::seconds(4600));
    BOOST_CHECK_NO_THROW(m_session.insert(zone1));

    Name identityName = Name(TEST_IDENTITY_NAME).append("NDNS");

    m_identity = this->addIdentity(identityName);
    m_cert = m_identity.getDefaultKey().getDefaultCertificate();
    m_certName = m_cert.getName();
    saveIdentityCertificate(m_identity, TEST_CERT.string());

    NDNS_LOG_INFO("save test root cert " << m_certName << " to: " << TEST_CERT.string());
    BOOST_CHECK_GT(m_certName.size(), 0);
    NDNS_LOG_TRACE("test certName: " << m_certName);
  }

  ~RrsetFactoryFixture()
  {
    m_session.close();
    boost::filesystem::remove(TEST_DATABASE2);
    NDNS_LOG_INFO("remove database " << TEST_DATABASE2);
    boost::filesystem::remove(TEST_CERT);
  }

public:
  const boost::filesystem::path TEST_DATABASE2;
  const Name TEST_IDENTITY_NAME;
  const boost::filesystem::path TEST_CERT;
  ndns::DbMgr m_session;
  Name m_zoneName;
  Name m_certName;
  Identity m_identity;
  Certificate m_cert;
};

BOOST_FIXTURE_TEST_SUITE(RrsetFactoryTest,  RrsetFactoryFixture)

BOOST_AUTO_TEST_CASE(CheckZoneKey)
{
  // zone throws check: zone not exists
  RrsetFactory rf1(TEST_DATABASE2, "/not/exist/zone", m_keyChain, m_certName);
  BOOST_CHECK_THROW(rf1.checkZoneKey(), ndns::RrsetFactory::Error);

  // cert throws check: !matchCertificate
  RrsetFactory rf2(TEST_DATABASE2, m_zoneName, m_keyChain, "wrongCert");
  BOOST_CHECK_THROW(rf2.checkZoneKey(), std::runtime_error);

  RrsetFactory rf3(TEST_DATABASE2, m_zoneName, m_keyChain, m_certName);
  BOOST_CHECK_NO_THROW(rf3.checkZoneKey());
}

BOOST_AUTO_TEST_CASE(GenerateNsRrset)
{
  Name label("/nstest");
  name::Component type = label::NS_RR_TYPE;
  uint64_t version = 1234;
  time::seconds ttl(2000);
  Zone zone(m_zoneName);
  m_session.find(zone);

  RrsetFactory rf(TEST_DATABASE2, m_zoneName, m_keyChain, m_certName);

  // rf without checkZoneKey: throw.
  ndn::DelegationList delegations;
  BOOST_CHECK_THROW(rf.generateNsRrset(label, type, version, ttl, delegations),
                    ndns::RrsetFactory::Error);
  rf.checkZoneKey();

  for (int i = 1; i <= 4; i++) {
    Name name("/delegation/" + std::to_string(i));
    delegations.insert(i, name);
  }

  Rrset rrset = rf.generateNsRrset(label, type, version, ttl, delegations);

  BOOST_CHECK_EQUAL(rrset.getId(), 0);
  BOOST_REQUIRE(rrset.getZone() != nullptr);
  BOOST_CHECK_EQUAL(*rrset.getZone(), zone);
  BOOST_CHECK_EQUAL(rrset.getLabel(), label);
  BOOST_CHECK_EQUAL(rrset.getType(), type);
  BOOST_CHECK_EQUAL(rrset.getVersion().toVersion(), version);
  BOOST_CHECK_EQUAL(rrset.getTtl(), ttl);

  const Name linkName("/rrest/factory/NDNS/nstest/NS/%FD%04%D2");
  Link link;
  BOOST_CHECK_NO_THROW(link.wireDecode(rrset.getData()));

  BOOST_CHECK_EQUAL(link.getName(), linkName);
  BOOST_CHECK_EQUAL(link.getContentType(), NDNS_LINK);
  BOOST_CHECK(link.getDelegationList() == delegations);

  // BOOST_CHECK_EQUAL(Validator::verifySignature(link, m_cert.getPublicKeyInfo()), true);
  security::verifySignature(link, m_cert);
}

BOOST_AUTO_TEST_CASE(GenerateTxtRrset)
{
  Name label("/txttest");
  name::Component type = label::TXT_RR_TYPE;
  uint64_t version = 1234;
  time::seconds ttl(2000);
  std::vector<std::string> txts;
  Zone zone(m_zoneName);
  m_session.find(zone);

  RrsetFactory rf(TEST_DATABASE2, m_zoneName, m_keyChain, m_certName);

  // rf without checkZoneKey: throw.
  BOOST_CHECK_THROW(rf.generateTxtRrset(label, type, version, ttl, txts),
                    ndns::RrsetFactory::Error);
  rf.checkZoneKey();
  BOOST_CHECK_NO_THROW(rf.generateTxtRrset(label, type, version, ttl, txts));
  rf.checkZoneKey();

  for (int i = 1; i <= 4; i++) {
    txts.push_back(std::to_string(i));
  }

  Rrset rrset = rf.generateTxtRrset(label, type, version, ttl, txts);

  BOOST_CHECK_EQUAL(rrset.getId(), 0);
  BOOST_CHECK_EQUAL(*rrset.getZone(), zone);
  BOOST_CHECK_EQUAL(rrset.getLabel(), label);
  BOOST_CHECK_EQUAL(rrset.getType(), type);
  BOOST_CHECK_EQUAL(rrset.getVersion().toVersion(), version);
  BOOST_CHECK_EQUAL(rrset.getTtl(), ttl);

  Name dataName = m_zoneName.append(label::NDNS_ITERATIVE_QUERY)
                            .append(label)
                            .append(type)
                            .append(rrset.getVersion());

  Data data;
  BOOST_CHECK_NO_THROW(data.wireDecode(rrset.getData()));

  BOOST_CHECK_EQUAL(data.getName(), dataName);
  BOOST_CHECK_EQUAL(data.getContentType(), NDNS_RESP);

  BOOST_CHECK(txts == RrsetFactory::wireDecodeTxt(data.getContent()));

  // shared_ptr<IdentityCertificate> cert = m_keyChain.getCertificate(m_certName);
  // BOOST_CHECK(Validator::verifySignature(data, cert->getPublicKeyInfo()));
  security::verifySignature(data, m_cert);
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace tests
} // namespace ndns
} // namespace ndn
