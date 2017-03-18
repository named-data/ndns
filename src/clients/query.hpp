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

#ifndef NDNS_CLIENTS_QUERY_HPP
#define NDNS_CLIENTS_QUERY_HPP

#include "ndns-label.hpp"
#include "ndns-enum.hpp"

#include <ndn-cxx/name.hpp>
#include <ndn-cxx/link.hpp>

namespace ndn {
namespace ndns {

/**
 * @brief NDNS Query abstraction
 *
 * Query is an Interest whose name follows the format:
 *
 *      <zone> [<NDNS>|<NDNS-R>] <rrLabel> <rrType>
 */
class Query : noncopyable
{
public:
  Query();

  Query(const Name& zone, const name::Component& queryType);

  /**
   * @brief construct an Interest according to the query abstraction
   */
  Interest
  toInterest() const;

  /**
   * @brief extract the query information (rrLabel, rrType) from a Interest
   *
   * @param zone     NDNS zone
   * @param interest The Interest to parse; the Interest must have correct zone,
   *                 otherwise it's undefined behavior
   */
  bool
  fromInterest(const Name& zone, const Interest& interest);

  void
  setDelegationListFromLink(const Link& link);

  bool
  operator==(const Query& other) const
  {
    return (getZone() == other.getZone() &&
      getQueryType() == other.getQueryType() && getRrLabel() == other.getRrLabel() &&
      getRrType() == other.getRrType());
  }

  bool
  operator!=(const Query& other) const
  {
    return !(*this == other);
  }

public:

  /**
   * @brief get name of authoritative zone
   */
  const Name&
  getZone() const
  {
    return m_zone;
  }

  /**
   * @brief set name of authoritative zone
   */
  void
  setZone(const Name& zone)
  {
    m_zone = zone;
  }

  /**
   * @brief get lifetime of the Interest
   */
  const time::milliseconds&
  getInterestLifetime() const
  {
    return m_interestLifetime;
  }

  /**
   * @brief set lifetime of the Interest
   */
  void
  setInterestLifetime(const time::milliseconds& interestLifetime)
  {
    m_interestLifetime = interestLifetime;
  }


  /**
   * @brief get query type
   */
  const name::Component&
  getQueryType() const
  {
    return m_queryType;
  }

  /**
   * @brief get query type
   */
  void
  setQueryType(const name::Component& queryType)
  {
    m_queryType = queryType;
  }

  /**
   * @brief get label of resource record
   */
  const Name&
  getRrLabel() const
  {
    return m_rrLabel;
  }

  /**
   * @brief set label of resource record
   */
  void
  setRrLabel(const Name& rrLabel)
  {
    m_rrLabel = rrLabel;
  }

  /**
   * @brief get type resource record
   */
  const name::Component&
  getRrType() const
  {
    return m_rrType;
  }

  /**
   * @brief set type resource record
   */
  void
  setRrType(const name::Component& rrType)
  {
    m_rrType = rrType;
  }

  /**
   * @brief set link object
   */
  void
  setDelegationList(const DelegationList& delegations)
  {
    m_delegationList = delegations;
  }

  /**
   * @brief get Link object
   */
  const DelegationList&
  getDelegationList() const
  {
    return m_delegationList;
  }

private:
  Name m_zone;
  name::Component m_queryType;
  Name m_rrLabel;
  name::Component m_rrType;
  time::milliseconds m_interestLifetime;
  DelegationList m_delegationList;
};

std::ostream&
operator<<(std::ostream& os, const Query& query);

} // namespace ndns
} // namespace ndn

#endif // NDNS_CLIENTS_QUERY_HPP
