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

#include "database-test-data.hpp"
#include "logger.hpp"

#include <boost/filesystem.hpp>

namespace ndn {
namespace ndns {
namespace tests {
NDNS_LOG_INIT("TestFakeData")

const boost::filesystem::path DbTestData::TEST_DATABASE = TEST_CONFIG_PATH "/" "test-ndns.db";
const Name DbTestData::TEST_IDENTITY_NAME("/test19");
const boost::filesystem::path DbTestData::TEST_CERT =
  TEST_CONFIG_PATH "/" "anchors/root.cert";

DbTestData::DbTestData()
  : m_session(TEST_DATABASE.string())
{
  NDNS_LOG_TRACE("start creating test data");

  ndns::Validator::VALIDATOR_CONF_FILE = TEST_CONFIG_PATH "/" "validator.conf";

  m_certName = m_keyChain.createIdentity(TEST_IDENTITY_NAME);

  ndn::io::save(*(m_keyChain.getCertificate(m_certName)), TEST_CERT.string());
  NDNS_LOG_INFO("save test root cert " << m_certName << " to: " << TEST_CERT.string());

  BOOST_CHECK_GT(m_certName.size(), 0);
  NDNS_LOG_TRACE("test certName: " << m_certName);

  m_root = Zone(TEST_IDENTITY_NAME);
  Name name(TEST_IDENTITY_NAME);
    name.append("net");
  m_net = Zone(name);
  name.append("ndnsim");
  m_ndnsim =Zone(name);

  m_session.insert(m_root);
  BOOST_CHECK_GT(m_root.getId(), 0);
  m_session.insert(m_net);
  BOOST_CHECK_GT(m_net.getId(), 0);
  m_session.insert(m_ndnsim);
  BOOST_CHECK_GT(m_ndnsim.getId(), 0);

  m_zones.push_back(m_root);
  m_zones.push_back(m_net);
  m_zones.push_back(m_ndnsim);

  int certificateIndex = 0;
  function<void(const Name&,Zone&,const name::Component&)> addQueryRrset =
    [this, &certificateIndex] (const Name& label, Zone& zone,
                               const name::Component& type) {
    const time::seconds ttl(3000 + 100 * certificateIndex);
    const name::Component version = name::Component::fromVersion(100 + 1000 * certificateIndex);
    name::Component qType(label::NDNS_ITERATIVE_QUERY);
    NdnsType ndnsType = NDNS_RESP;
    if (type == label::CERT_RR_TYPE) {
      ndnsType = NDNS_RAW;
      qType = label::NDNS_CERT_QUERY;
    }
    std::ostringstream os;
    os << "a fake content: " << (++certificateIndex) << "th";

    addRrset(zone, label, type, ttl, version, qType, ndnsType, os.str());
  };

  addQueryRrset("/dsk-1", m_root, label::CERT_RR_TYPE);
  addQueryRrset("/net/ksk-2", m_root, label::CERT_RR_TYPE);
  addQueryRrset("/dsk-3", m_net, label::CERT_RR_TYPE);
  addQueryRrset("/ndnsim/ksk-4", m_net, label::CERT_RR_TYPE);
  addQueryRrset("/dsk-5", m_ndnsim, label::CERT_RR_TYPE);

  addQueryRrset("net", m_root, label::NS_RR_TYPE);
  addQueryRrset("ndnsim", m_net, label::NS_RR_TYPE);
  addQueryRrset("www", m_ndnsim, label::TXT_RR_TYPE);
  addQueryRrset("doc/www", m_ndnsim, label::TXT_RR_TYPE);


  addRrset(m_ndnsim, Name("doc"), label::NS_RR_TYPE , time::seconds(2000),
           name::Component::fromVersion(1234), label::NDNS_ITERATIVE_QUERY, NDNS_AUTH,
           std::string(""));

  NDNS_LOG_INFO("insert testing data: OK");
}

void
DbTestData::addRrset(Zone& zone, const Name& label, const name::Component& type,
                     const time::seconds& ttl, const name::Component& version,
                     const name::Component& qType, NdnsType ndnsType, const std::string& msg)
{
  Rrset rrset(&zone);
  rrset.setLabel(label);
  rrset.setType(type);
  rrset.setTtl(ttl);
  rrset.setVersion(version);

  Response re;
  re.setZone(zone.getName());
  re.setQueryType(qType);
  re.setRrLabel(label);
  re.setRrType(type);
  re.setVersion(version);
  re.setNdnsType(ndnsType);
  re.setFreshnessPeriod(ttl);

  if (msg.size() > 0) {
    if (type == label::CERT_RR_TYPE)
      re.setAppContent(dataBlock(ndn::tlv::Content, msg.c_str(), msg.size()));
    else
      re.addRr(msg);
  }
  shared_ptr<Data> data = re.toData();
  m_keyChain.sign(*data, m_certName); // now we ignore the certificate to sign the data
  shared_ptr<IdentityCertificate> cert = m_keyChain.getCertificate(m_certName);
  BOOST_CHECK_EQUAL(Validator::verifySignature(*data, cert->getPublicKeyInfo()), true);
  rrset.setData(data->wireEncode());

  m_session.insert(rrset);

  m_rrsets.push_back(rrset);
}

DbTestData::~DbTestData()
{
  for (auto& zone : m_zones)
    m_session.remove(zone);

  for (auto& rrset : m_rrsets)
    m_session.remove(rrset);

  m_session.close();

  boost::filesystem::remove(TEST_DATABASE);
  boost::filesystem::remove(TEST_CERT);

  // m_keyChain.deleteIdentity(TEST_IDENTITY_NAME);

  NDNS_LOG_INFO("remove database: " << TEST_DATABASE);
}

} // namespace tests
} // namespace ndns
} // namespace ndn
