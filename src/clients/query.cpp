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

#include "query.hpp"

namespace ndn {
namespace ndns {

Query::Query()
  : m_interestLifetime(ndn::DEFAULT_INTEREST_LIFETIME)
{
}

Query::Query(const Name& zone, const name::Component& queryType)
  : m_zone(zone)
  , m_queryType(queryType)
  , m_interestLifetime(ndn::DEFAULT_INTEREST_LIFETIME)
{
}


bool
Query::fromInterest(const Name& zone, const Interest& interest)
{
  label::MatchResult re;
  if (!matchName(interest, zone, re))
    return false;

  m_rrLabel = re.rrLabel;
  m_rrType = re.rrType;

  m_zone = zone;

  if (!interest.getForwardingHint().empty()) {
    m_delegationList = interest.getForwardingHint();
  }
  else {
    m_delegationList = DelegationList();
  }


  size_t len = zone.size();
  m_queryType = interest.getName().get(len);

  return true;
}

Interest
Query::toInterest() const
{
  // <zone> [<NDNS>|<NDNS-R>] <rrLabel> <rrType>
  Name name;

  name.append(this->m_zone)
      .append(this->m_queryType)
      .append(this->m_rrLabel)
      .append(this->m_rrType);

  Interest interest;
  interest.setName(name);
  interest.setInterestLifetime(m_interestLifetime);
  if (!m_delegationList.empty()) {
    interest.setForwardingHint(m_delegationList);
  }

  return interest;
}

void
Query::setDelegationListFromLink(const Link& link)
{
  m_delegationList = link.getDelegationList();
}

std::ostream&
operator<<(std::ostream& os, const Query& query)
{
  os << "Query: zone=" << query.getZone()
     << " queryType=" << query.getQueryType()
     << " rrLabel=" << query.getRrLabel()
     << " rrType=" << query.getRrType()
     << " Lifetime=" << query.getInterestLifetime()
    ;
  return os;
}

} // namespace ndns
} // namespace ndn
