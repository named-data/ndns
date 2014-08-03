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

#include "query.hpp"
#include "ndns-label.hpp"
namespace ndn {
namespace ndns {

Query::Query()
  : m_forwardingHint("/")
  , m_queryType(QUERY_DNS)
  , m_interestLifetime(time::milliseconds(4000))
  , m_rrType(RR::NS)
{
}

Query::~Query()
{
}

bool Query::fromInterest(const Name &name, const Interest &interest)
{
  return fromInterest(interest);
}

bool Query::fromInterest(const Interest& interest)
{
  Name interestName;
  interestName = interest.getName();

  int qtflag = -1;
  int zoneStart = -1;
  size_t len = interestName.size();

  for (size_t i = 0; i < len; i++) {
    ndn::Name::Component comp = interestName.get(i);
    //if (comp == ndn::ndns::label::ForwardingHintLabel) {
    if (comp == ndn::ndns::label::ForwardingHintComp) {
      zoneStart = i;
    } else if (comp == ndn::ndns::label::QueryDNSComp || comp == ndn::ndns::label::QueryDNSRComp ) {
      qtflag = i;
      break;
    }
  } //for

  if (qtflag == -1) {
    std::cerr << "There is no QueryType in the Interest Name: " << interestName
        << std::endl;
    return false;
  }
  this->m_queryType = toQueryType(interestName.get(qtflag).toUri());
  this->m_rrType = RR::toRRType(interestName.get(len - 1).toUri());
  if (zoneStart == -1) {
    this->m_authorityZone = interestName.getPrefix(qtflag); //the DNS/DNS-R is not included
  } else {
    this->m_forwardingHint = interestName.getPrefix(zoneStart);
    this->m_authorityZone = interestName.getSubName(zoneStart+1, qtflag-zoneStart-1);
  }
  this->m_interestLifetime = interest.getInterestLifetime();
  this->m_rrLabel = interestName.getSubName(qtflag + 1, len - qtflag - 2);
  return true;
}

Interest Query::toInterest() const
{
  Name name;
  if (m_forwardingHint != Name("/")) {
    name = m_forwardingHint;
    name.append(ndn::ndns::label::ForwardingHintLabel);
    //std::cout<<"add forwarding---------------------------:"<<name.toUri()<<std::endl;
  } else {
    //std::cout<<"do not add forwarding---------------------------:"<<std::endl;
  }

  name.append(this->m_authorityZone);
  name.append(toString(this->m_queryType));
  name.append(this->m_rrLabel);
  name.append(RR::toString(this->m_rrType));
  Selectors selector;
  //selector.setMustBeFresh(true);

  Interest interest = Interest(name, selector, -1, this->m_interestLifetime);
  return interest;
}

} // namespace ndns
} // namespace ndn
