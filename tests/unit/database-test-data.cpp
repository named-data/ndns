/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2022, Regents of the University of California.
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

#include "database-test-data.hpp"
#include "daemon/rrset-factory.hpp"
#include "mgmt/management-tool.hpp"
#include "util/cert-helper.hpp"

#include <ndn-cxx/security/verification-helpers.hpp>

namespace fs = boost::filesystem;

namespace ndn {
namespace ndns {
namespace tests {

const fs::path DbTestData::TEST_DATABASE = fs::path(UNIT_TESTS_TMPDIR) / "test-ndns.db";
const Name DbTestData::TEST_IDENTITY_NAME("/test19");
const fs::path DbTestData::TEST_CERT = fs::path(UNIT_TESTS_TMPDIR) / "anchors" / "root.cert";
const fs::path DbTestData::TEST_DKEY_CERT = fs::path(UNIT_TESTS_TMPDIR) / "dkey.cert";

DbTestData::PreviousStateCleaner::PreviousStateCleaner()
{
  fs::remove(TEST_DATABASE);
  fs::remove(TEST_CERT);
}

DbTestData::DbTestData()
  : m_session(TEST_DATABASE.string())
  , m_testName("/test19")
  , m_netName("/test19/net")
  , m_ndnsimName("/test19/net/ndnsim")
{
  NdnsValidatorBuilder::VALIDATOR_CONF_FILE = (fs::path(UNIT_TESTS_TMPDIR) / "validator.conf").string();

  ManagementTool tool(TEST_DATABASE.string(), m_keyChain);
  // this is how DKEY is added to parent zone in real world.
  auto addDkeyCertToParent = [&tool] (Zone& dkeyFrom, Zone& dkeyTo) {
    Certificate dkeyCert;
    dkeyCert = tool.getZoneDkey(dkeyFrom);
    io::save(dkeyCert, TEST_DKEY_CERT.string());
    tool.addRrsetFromFile(dkeyTo.getName(), TEST_DKEY_CERT.string(),
                          DEFAULT_RR_TTL, DEFAULT_CERT, io::BASE64, true);
  };

  Name testName(m_testName);
  m_test = tool.createZone(testName, ROOT_ZONE);
  // m_test's DKEY is not added to parent zone
  Name netName(m_netName);
  m_net = tool.createZone(netName, testName);
  addDkeyCertToParent(m_net, m_test);
  Name ndnsimName(m_ndnsimName);
  m_ndnsim = tool.createZone(ndnsimName, netName);
  addDkeyCertToParent(m_ndnsim, m_net);

  m_zones.push_back(m_test);
  m_zones.push_back(m_net);
  m_zones.push_back(m_ndnsim);

  Name identityName = Name(testName).append("NDNS");
  m_identity = CertHelper::getIdentity(m_keyChain, identityName);
  m_certName = CertHelper::getDefaultCertificateNameOfIdentity(m_keyChain, identityName);
  m_cert = CertHelper::getCertificate(m_keyChain, identityName, m_certName);
  BOOST_ASSERT(!m_certName.empty());

  io::save(m_cert, TEST_CERT.string());

  int certificateIndex = 0;
  auto addQueryRrset = [this, &certificateIndex] (const Name& label, Zone& zone,
                                                  const name::Component& type) {
    const time::seconds ttl(3000 + 100 * certificateIndex);
    const auto version = name::Component::fromVersion(100 + 1000 * certificateIndex);
    name::Component qType(label::NDNS_ITERATIVE_QUERY);
    NdnsContentType contentType = NDNS_RESP;
    if (type == label::APPCERT_RR_TYPE) {
      contentType = NDNS_KEY;
    }
    else if (type == label::NS_RR_TYPE) {
      contentType = NDNS_LINK;
    }
    else if (type == label::TXT_RR_TYPE) {
      contentType = NDNS_RESP;
    }
    std::ostringstream os;
    os << "a fake content: " << (++certificateIndex) << "th";

    addRrset(zone, label, type, ttl, version, qType, contentType, os.str());
  };

  addQueryRrset("net", m_test, label::NS_RR_TYPE);
  addQueryRrset("ndnsim", m_net, label::NS_RR_TYPE);
  addQueryRrset("www", m_ndnsim, label::TXT_RR_TYPE);
  addQueryRrset("doc/www", m_ndnsim, label::TXT_RR_TYPE);

  addRrset(m_ndnsim, Name("doc"), label::NS_RR_TYPE , time::seconds(2000),
           name::Component::fromVersion(1234), label::NDNS_ITERATIVE_QUERY, NDNS_AUTH,
           std::string(""));

  // last link is the same as former one
  BOOST_ASSERT(!m_links.empty());
  m_links.push_back(m_links.back());
}

void
DbTestData::addRrset(Zone& zone, const Name& label, const name::Component& type,
                     const time::seconds& ttl, const name::Component& version,
                     const name::Component& qType, NdnsContentType contentType, const std::string& msg)
{
  Rrset rrset;
  RrsetFactory rf(TEST_DATABASE.string(), zone.getName(),
                  m_keyChain, m_certName);
  rf.onlyCheckZone();
  if (type == label::NS_RR_TYPE) {
    rrset = rf.generateNsRrset(label, version.toVersion(), ttl, {"/xx"});
    if (contentType != NDNS_AUTH) {
      // do not add AUTH packet to link
      m_links.emplace_back(rrset.getData());
    }
  }
  else if (type == label::TXT_RR_TYPE) {
    rrset = rf.generateTxtRrset(label, version.toVersion(), ttl, {});
  }
  else if (type == label::APPCERT_RR_TYPE) {
    rrset = rf.generateCertRrset(label, version.toVersion(), ttl, m_cert);
  }

  auto data = std::make_shared<Data>(rrset.getData());
  BOOST_VERIFY(security::verifySignature(*data, m_cert));

  ManagementTool tool(TEST_DATABASE.string(), m_keyChain);
  tool.addRrset(rrset);

  m_rrsets.push_back(rrset);
}

DbTestData::~DbTestData()
{
  for (auto& zone : m_zones)
    m_session.remove(zone);

  for (auto& rrset : m_rrsets)
    m_session.remove(rrset);

  m_session.close();
}

} // namespace tests
} // namespace ndns
} // namespace ndn
