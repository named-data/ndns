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

#include "response.hpp"
#include "logger.hpp"

namespace ndn {
namespace ndns {

Response::Response()
  : m_contentType(NDNS_BLOB)
  , m_freshnessPeriod(DEFAULT_RR_FRESHNESS_PERIOD)
  , m_appContent(makeBinaryBlock(ndn::tlv::Content, reinterpret_cast<const uint8_t*>(0), 0))
{
}

Response::Response(const Name& zone, const name::Component& queryType)
  : m_zone(zone)
  , m_queryType(queryType)
  , m_contentType(NDNS_BLOB)
  , m_freshnessPeriod(DEFAULT_RR_FRESHNESS_PERIOD)
  , m_appContent(makeBinaryBlock(ndn::tlv::Content, reinterpret_cast<const uint8_t*>(0), 0))
{
}

template<encoding::Tag T>
inline size_t
Response::wireEncode(EncodingImpl<T>& block) const
{
  if (m_contentType == NDNS_BLOB || m_contentType == NDNS_KEY) {
    // Raw application content
    return block.prependBlock(m_appContent);
  }

  // Content :: = CONTENT-TYPE TLV-LENGTH
  //              Block*

  size_t totalLength = 0;
  for (std::vector<Block>::const_reverse_iterator iter = m_rrs.rbegin();
       iter != m_rrs.rend(); ++iter) {
    totalLength += block.prependBlock(*iter);
  }

  totalLength += block.prependVarNumber(totalLength);
  totalLength += block.prependVarNumber(::ndn::tlv::Content);

  return totalLength;
}

const Block
Response::wireEncode() const
{
  if (m_contentType == NDNS_BLOB || m_contentType == NDNS_KEY) {
    return m_appContent;
  }

  EncodingEstimator estimator;
  size_t estimatedSize = wireEncode(estimator);
  EncodingBuffer buffer(estimatedSize, 0);
  wireEncode(buffer);
  return buffer.block();
}

void
Response::wireDecode(const Block& wire)
{
  if (m_contentType == NDNS_BLOB || m_contentType == NDNS_KEY) {
    m_appContent = wire;
    return;
  }

  wire.parse();

  Block::element_const_iterator iter = wire.elements().begin();
  for (; iter != wire.elements().end(); ++iter) {
    m_rrs.push_back(*iter);
  }
}

bool
Response::fromData(const Name& zone, const Data& data)
{
  label::MatchResult re;
  if (!matchName(data, zone, re))
    return false;

  m_rrLabel = re.rrLabel;
  m_rrType = re.rrType;
  m_version = re.version;

  m_zone = zone;
  size_t len = zone.size();
  m_queryType = data.getName().get(len);

  MetaInfo info = data.getMetaInfo();

  m_freshnessPeriod = time::duration_cast<time::seconds>(info.getFreshnessPeriod());
  m_contentType = NdnsContentType(data.getContentType());

  wireDecode(data.getContent());
  return true;
}


shared_ptr<Data>
Response::toData()
{
  Name name;
  name.append(m_zone)
      .append(m_queryType)
      .append(m_rrLabel)
      .append(m_rrType);

  if (m_version.empty()) {
    name.appendVersion();
    m_version = name.get(-1);
  }
  else {
    name.append(m_version);
  }

  shared_ptr<Data> data = make_shared<Data>(name);

  if (m_contentType != NDNS_BLOB && m_contentType != NDNS_KEY) {
    data->setContent(this->wireEncode());
  }
  else {
    data->setContent(m_appContent);
  }
  data->setFreshnessPeriod(m_freshnessPeriod);
  data->setContentType(m_contentType);

  return data;
}


Response&
Response::addRr(const Block& rr)
{
  this->m_rrs.push_back(rr);
  return *this;
}

Response&
Response::addRr(const std::string& rr)
{
  return this->addRr(makeBinaryBlock(ndns::tlv::RrData, rr.c_str(), rr.size()));
}

bool
Response::removeRr(const Block& rr)
{
  for (std::vector<Block>::iterator iter = m_rrs.begin(); iter != m_rrs.end(); ++iter) {
    if (*iter == rr) {
      m_rrs.erase(iter);
      return true;
    }
  }
  return false;
}

void
Response::setAppContent(const Block& block)
{
  if (block.type() != ndn::tlv::Content) {
    m_appContent = Block(ndn::tlv::Content, block);
  }
  else {
    m_appContent = block;
  }

  m_appContent.encode(); // this is a must
}


bool
Response::operator==(const Response& other) const
{
  bool tmp = (getZone() == other.getZone() &&
              getQueryType() == other.getQueryType() && getRrLabel() == other.getRrLabel() &&
              getRrType() == other.getRrType() && getVersion() == other.getVersion() &&
              getContentType() == other.getContentType());

  if (tmp == false)
    return tmp;

  if (m_contentType == NDNS_BLOB || m_contentType == NDNS_KEY) {
    return tmp && (getAppContent() == other.getAppContent());
  }
  else
    return tmp && getRrs() == other.getRrs();
}

std::ostream&
operator<<(std::ostream& os, const Response& response)
{
  os << "Response: zone=" << response.getZone()
     << " queryType=" << response.getQueryType()
     << " rrLabel=" << response.getRrLabel()
     << " rrType=" << response.getRrType()
     << " version=" << response.getVersion()
     << " freshnessPeriod=" << response.getFreshnessPeriod()
     << " NdnsContentType=" << response.getContentType();
  if (response.getContentType() == NDNS_BLOB
      || response.getContentType() == NDNS_KEY) {
    if (response.getAppContent().empty())
      os << " appContent=NULL";
    else
      os << " appContentSize=" << response.getAppContent().size();
  }
  else {
    os << " rrs.size=" << response.getRrs().size();
  }
  return os;
}
} // namespace ndns
} // namespace ndn
