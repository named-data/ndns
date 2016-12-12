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

#include "rrset-factory.hpp"
#include "mgmt/management-tool.hpp"

#include <boost/algorithm/string/join.hpp>

namespace ndn {
namespace ndns {

NDNS_LOG_INIT("RrsetFactory")

RrsetFactory::RrsetFactory(const std::string& dbFile,
                           const Name& zoneName,
                           KeyChain& keyChain,
                           const Name& inputDskCertName)
  : m_keyChain(keyChain)
  , m_dbFile(dbFile)
  , m_zone(zoneName)
  , m_dskCertName(inputDskCertName)
  , m_checked(false)
{
  if (m_dskCertName == DEFAULT_CERT) {
    m_dskName = m_keyChain.getDefaultKeyNameForIdentity(zoneName);
    m_dskCertName = m_keyChain.getDefaultCertificateNameForKey(m_dskName);
  }
}

void
RrsetFactory::checkZoneKey()
{
  onlyCheckZone();
  if (m_dskCertName != DEFAULT_CERT &&
      !matchCertificate(m_dskCertName, m_zone.getName())) {
    BOOST_THROW_EXCEPTION(Error("Cannot verify certificate"));
  }
}

void
RrsetFactory::onlyCheckZone()
{
  if (m_checked) {
    return ;
  }
  m_checked = true;

  DbMgr dbMgr(m_dbFile);
  const Name& zoneName = m_zone.getName();
  if (!dbMgr.find(m_zone)) {
    BOOST_THROW_EXCEPTION(Error(zoneName.toUri() + " is not presented in the NDNS db"));
  }
}


std::pair<Rrset, Name>
RrsetFactory::generateBaseRrset(const Name& label,
                                const name::Component& type,
                                const uint64_t version,
                                const time::seconds& ttl)
{
  Rrset rrset(&m_zone);

  rrset.setLabel(label);
  rrset.setType(type);
  rrset.setTtl(ttl);

  name::Component qType;
  if (type == label::CERT_RR_TYPE) {
    qType = label::NDNS_CERT_QUERY;
  } else {
    qType = label::NDNS_ITERATIVE_QUERY;
  }

  Name name;
  name.append(m_zone.getName())
      .append(qType)
      .append(label)
      .append(type);

  if (version != VERSION_USE_UNIX_TIMESTAMP) {
    name.append(name::Component::fromVersion(version));
  } else {
    name.appendVersion();
  }

  rrset.setVersion(name.get(-1));

  return std::make_pair(rrset, name);
}

bool
RrsetFactory::matchCertificate(const Name& certName, const Name& identity)
{
  if (!m_keyChain.doesCertificateExist(certName)) {
    NDNS_LOG_WARN(certName.toUri() << " is not presented in KeyChain");
    return false;
  }

  // Check its public key information
  shared_ptr<IdentityCertificate> cert = m_keyChain.getCertificate(certName);
  Name keyName = cert->getPublicKeyName();

  if (!identity.isPrefixOf(keyName) || identity.size() != keyName.size() - 1) {
    NDNS_LOG_WARN(keyName.toUri() << " is not a key of " << identity.toUri());
    return false;
  }

  if (!m_keyChain.doesKeyExistInTpm(keyName, KeyClass::PRIVATE)) {
    NDNS_LOG_WARN("Private key: " << keyName.toUri() << " is not present in KeyChain");
    return false;
  }

  return true;
}

Rrset
RrsetFactory::generateNsRrset(const Name& label,
                              const name::Component& type,
                              const uint64_t version,
                              time::seconds ttl,
                              const ndn::Link::DelegationSet& delegations)
{
  if (!m_checked) {
    BOOST_THROW_EXCEPTION(Error("You have to call checkZoneKey before call generate functions"));
  }

  if (ttl == DEFAULT_RR_TTL)
    ttl = m_zone.getTtl();

  std::pair<Rrset, Name> rrsetAndName = generateBaseRrset(label, type, version, ttl);
  const Name& name = rrsetAndName.second;
  Rrset& rrset = rrsetAndName.first;

  Link link(name);
  for (const auto& i : delegations) {
    link.addDelegation(i.first, i.second);
  }

  setContentType(link, NDNS_LINK, ttl);
  sign(link);
  rrset.setData(link.wireEncode());

  return rrset;
}

Rrset
RrsetFactory::generateTxtRrset(const Name& label,
                               const name::Component& type,
                               const uint64_t version,
                               time::seconds ttl,
                               const std::vector<std::string>& strings)
{
  if (!m_checked) {
    BOOST_THROW_EXCEPTION(Error("You have to call checkZoneKey before call generate functions"));
  }

  if (ttl == DEFAULT_RR_TTL)
    ttl = m_zone.getTtl();

  Name name;
  Rrset rrset;
  std::tie(rrset, name) = generateBaseRrset(label, type, version, ttl);

  std::vector<Block> rrs;
  for (const auto& item : strings) {
    rrs.push_back(makeBinaryBlock(ndns::tlv::RrData,
                                  item.c_str(),
                                  item.size()));
  }

  Data data(name);
  data.setContent(wireEncode(rrs));

  setContentType(data, NDNS_RESP, ttl);
  sign(data);
  rrset.setData(data.wireEncode());

  return rrset;
}

Rrset
RrsetFactory::generateCertRrset(const Name& label,
                                const name::Component& type,
                                const uint64_t version,
                                time::seconds ttl,
                                const IdentityCertificate& cert)
{
  if (!m_checked) {
    BOOST_THROW_EXCEPTION(Error("You have to call checkZoneKey before call generate functions"));
  }

  if (ttl == DEFAULT_RR_TTL)
    ttl = m_zone.getTtl();

  Name name;
  Rrset rrset;
  std::tie(rrset, name) = generateBaseRrset(label, type, version, ttl);

  Data data(name);
  data.setContent(cert.wireEncode());

  setContentType(data, NDNS_KEY, ttl);
  sign(data);
  rrset.setData(data.wireEncode());

  return rrset;
}


void
RrsetFactory::sign(Data& data)
{
  m_keyChain.sign(data, m_dskCertName);
}

void
RrsetFactory::setContentType(Data& data, NdnsContentType contentType,
                             const time::seconds& ttl)
{
  data.setContentType(contentType);
  data.setFreshnessPeriod(ttl);
}

template<encoding::Tag TAG>
inline size_t
RrsetFactory::wireEncode(EncodingImpl<TAG>& block, const std::vector<Block>& rrs) const
{
  // Content :: = CONTENT-TYPE TLV-LENGTH
  //              Block*

  size_t totalLength = 0;
  for (auto iter = rrs.rbegin(); iter != rrs.rend(); ++iter) {
    totalLength += block.prependBlock(*iter);
  }

  totalLength += block.prependVarNumber(totalLength);
  totalLength += block.prependVarNumber(::ndn::tlv::Content);

  return totalLength;
}

const Block
RrsetFactory::wireEncode(const std::vector<Block>& rrs) const
{
  EncodingEstimator estimator;
  size_t estimatedSize = wireEncode(estimator, rrs);
  EncodingBuffer buffer(estimatedSize, 0);
  wireEncode(buffer, rrs);
  return buffer.block();
}

std::vector<std::string>
RrsetFactory::wireDecodeTxt(const Block& wire)
{
  std::vector<std::string> txts;
  wire.parse();

  for (const auto& e : wire.elements()) {
    txts.push_back(std::string(reinterpret_cast<const char*>(e.value()),
                               e.value_size()));
  }

  return txts;
}


} // namespace ndns
} // namespace ndn
