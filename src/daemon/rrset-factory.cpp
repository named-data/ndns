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

#include "rrset-factory.hpp"
#include "mgmt/management-tool.hpp"
#include "util/cert-helper.hpp"

#include <ndn-cxx/security/signing-helpers.hpp>

#include <boost/algorithm/string/join.hpp>

namespace ndn {
namespace ndns {

NDNS_LOG_INIT("RrsetFactory")

RrsetFactory::RrsetFactory(const boost::filesystem::path& dbFile,
                           const Name& zoneName,
                           KeyChain& keyChain,
                           const Name& inputDskCertName)
  : m_keyChain(keyChain)
  , m_dbFile(dbFile.string())
  , m_zone(zoneName)
  , m_dskCertName(inputDskCertName)
  , m_checked(false)
{
  Name identityName = Name(zoneName).append(label::NDNS_ITERATIVE_QUERY);
  if (m_dskCertName == DEFAULT_CERT) {
    m_dskName = CertHelper::getDefaultKeyNameOfIdentity(m_keyChain, identityName);
    m_dskCertName = CertHelper::getDefaultCertificateNameOfIdentity(m_keyChain, identityName);
  }
}

void
RrsetFactory::checkZoneKey()
{
  onlyCheckZone();
  Name zoneIdentityName = Name(m_zone.getName()).append(label::NDNS_ITERATIVE_QUERY);
  if (m_dskCertName != DEFAULT_CERT &&
      !matchCertificate(m_dskCertName, zoneIdentityName)) {
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

  Name name;
  name.append(m_zone.getName())
      .append(label::NDNS_ITERATIVE_QUERY)
      .append(label)
      .append(type);

  if (version != VERSION_USE_UNIX_TIMESTAMP) {
    name.append(name::Component::fromVersion(version));
  }
  else {
    name.appendVersion();
  }

  rrset.setVersion(name.get(-1));

  return std::make_pair(rrset, name);
}

bool
RrsetFactory::matchCertificate(const Name& certName, const Name& identity)
{
  try {
    CertHelper::getCertificate(m_keyChain, identity, certName);
    return true;
  } catch (ndn::security::Pib::Error) {
    return false;
  }
}

Rrset
RrsetFactory::generateNsRrset(const Name& label,
                              const name::Component& type,
                              const uint64_t version,
                              time::seconds ttl,
                              const ndn::DelegationList& delegations)
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
  link.setDelegationList(delegations);

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
                                const ndn::security::v2::Certificate& cert)
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

Rrset
RrsetFactory::generateAuthRrset(const Name& label,
                                const name::Component& type,
                                const uint64_t version,
                                time::seconds ttl)
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

  setContentType(data, NDNS_AUTH, ttl);
  sign(data);
  rrset.setData(data.wireEncode());

  return rrset;
}

void
RrsetFactory::sign(Data& data)
{
  m_keyChain.sign(data, signingByCertificate(m_dskCertName));
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
