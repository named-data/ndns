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

#ifndef NDNS_CLIENTS_RESPONSE_HPP
#define NDNS_CLIENTS_RESPONSE_HPP

#include "ndns-tlv.hpp"
#include "ndns-enum.hpp"
#include "ndns-label.hpp"


#include <ndn-cxx/face.hpp>
#include <ndn-cxx/name.hpp>
#include <ndn-cxx/data.hpp>
#include <ndn-cxx/encoding/block-helpers.hpp>
#include <ndn-cxx/encoding/block.hpp>
#include <ndn-cxx/meta-info.hpp>

#include <vector>

namespace ndn {
namespace ndns {

/**
 * @brief Default life time of resource record
 */
const time::seconds DEFAULT_RR_FRESHNESS_PERIOD(3600);


/**
 * @brief NDNS Response abstraction. Response is used on client side,
 * while Rrset is used on server side, and Data packet is used during transmission in network.
 */
class Response
{
public:
  Response();

  Response(const Name& zone, const name::Component& queryType);

  /**
   * @brief fill the attributes from Data packet.
   *
   * @param zone NDNS zone name
   * @param data Data.getName() must the same zone as its prefix,
   *             otherwise it's undefined behavior
   * @return false if Data.getName() does not follow the structure of NDNS Response without
   *         changing any attributes, otherwise return true and fill the attributes
   */
  bool
  fromData(const Name& zone, const Data& data);

  shared_ptr<Data>
  toData();

  Response&
  addRr(const Block& rr);

  /**
   * @brief add Block which contains string information and its tlv type is ndns::tlv::RrData
   *
   * @return Response that is service level information, the encoding level abstraction,
   *         i.e., Block is not very convenient.
   */
  Response&
  addRr(const std::string& rr);

  bool
  removeRr(const Block& rr);

  bool
  operator==(const Response& other) const;

  bool
  operator!=(const Response& other) const
  {
    return !(*this == other);
  }

  /**
   * @brief encode the app-level data
   */
  const Block
  wireEncode() const;

  /**
   * @brief decode the app-level data
   */
  void
  wireDecode(const Block& wire);

  /**
   * @brief encode app-level data
   */
  template<encoding::Tag T>
  size_t
  wireEncode(EncodingImpl<T>& block) const;

public:
  ///////////////////////////////////////////////
  // getter and setter

  const Name&
  getZone() const
  {
    return m_zone;
  }

  void
  setZone(const Name& zone)
  {
    m_zone = zone;
  }

  const name::Component&
  getQueryType() const
  {
    return m_queryType;
  }

  void
  setQueryType(const name::Component& queryType)
  {
    m_queryType = queryType;
  }

  const Name&
  getRrLabel() const
  {
    return m_rrLabel;
  }

  void
  setRrLabel(const Name& rrLabel)
  {
    m_rrLabel = rrLabel;
  }

  void
  setRrType(const name::Component& rrType)
  {
    m_rrType = rrType;
  }

  const name::Component&
  getRrType() const
  {
    return m_rrType;
  }

  void
  setVersion(const name::Component& version)
  {
    m_version = version;
  }

  const name::Component&
  getVersion() const
  {
    return m_version;
  }

  void
  setContentType(NdnsContentType contentType)
  {
    m_contentType = contentType;
  }

  NdnsContentType
  getContentType() const
  {
    return m_contentType;
  }

  const Block&
  getAppContent() const
  {
    return m_appContent;
  }

  void
  setAppContent(const Block& block);

  const std::vector<Block>&
  getRrs() const
  {
    return m_rrs;
  }

  void
  setRrs(const std::vector<Block>& rrs)
  {
    m_rrs = rrs;
  }

  time::seconds
  getFreshnessPeriod() const
  {
    return m_freshnessPeriod;
  }

  void
  setFreshnessPeriod(time::seconds freshnessPeriod)
  {
    m_freshnessPeriod = freshnessPeriod;
  }

private:
  Name m_zone;
  name::Component m_queryType;
  Name m_rrLabel;
  name::Component m_rrType;
  name::Component m_version;

  NdnsContentType m_contentType;
  time::seconds m_freshnessPeriod;

  /**
   * @brief App content. Be valid only for NDNS-NULL Response
   */
  Block m_appContent;

  /**
   * @brief Content of Resource Record. Be valid only when this is not a NDNS-NULL Response
   */
  std::vector<Block> m_rrs;
};

std::ostream&
operator<<(std::ostream& os, const Response& response);

} // namespace ndns
} // namespace ndn

#endif // NDNS_CLIENTS_RESPONSE_HPP
