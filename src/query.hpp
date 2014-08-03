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

#ifndef NDNS_QUERY_HPP
#define NDNS_QUERY_HPP

#include "rr.hpp"


#include <ndn-cxx/name.hpp>

namespace ndn {
namespace ndns {

class Query
{
public:

  enum QueryType
  {
    QUERY_DNS = 1,
    QUERY_DNS_R,
    QUERY_KEY,
    QUERY_UNKNOWN
  };

  static std::string toString(const QueryType& qType)
  {
    std::string label;
    switch (qType) {
    case QUERY_DNS:
      label = "DNS";
      break;
    case QUERY_DNS_R:
      label = "DNS-R";
      break;
    case QUERY_KEY:
      label = "KEY";
      break;
    default:
      label = "UNKNOWN";
      break;
    }
    return label;

  }

  static const QueryType toQueryType(const std::string& str)
  {
    QueryType atype;
    if (str == "DNS") {
      atype = QUERY_DNS;
    } else if (str == "DNS-R") {
      atype = QUERY_DNS_R;
    } else if (str == "KEY") {
      atype = QUERY_KEY;
    }
    else {
      atype = QUERY_UNKNOWN;
    }
    return atype;
  }

  Query();

  virtual ~Query();

  const Name& getAuthorityZone() const
  {
    return m_authorityZone;
  }

  void setAuthorityZone(const Name& authorityZone)
  {
    m_authorityZone = authorityZone;
  }

  time::milliseconds getInterestLifetime() const
  {
    return m_interestLifetime;
  }

  void setInterestLifetime(time::milliseconds interestLifetime)
  {
    m_interestLifetime = interestLifetime;
  }

  enum QueryType getQueryType() const
  {
    return m_queryType;
  }

  void setQueryType(enum QueryType queryType)
  {
    m_queryType = queryType;
  }

  const Name& getRrLabel() const
  {
    return m_rrLabel;
  }

  void setRrLabel(const Name& rrLabel)
  {
    m_rrLabel = rrLabel;
  }

  const RR::RRType& getRrType() const
  {
    return m_rrType;
  }

  void setRrType(const RR::RRType& rrType)
  {
    m_rrType = rrType;
  }

private:
  template<bool T>
  size_t
  wireEncode(EncodingImpl<T> & block) const;

public:
  Interest
  toInterest() const;

  bool
  fromInterest(const Name &name, const Interest& interest);

  bool
  fromInterest(const Interest& interest);

  const Name& getFowardingHint() const
  {
    return m_forwardingHint;
  }

  void setFowardingHint(const Name& fowardingHint)
  {
    m_forwardingHint = fowardingHint;
  }

public:

  Name m_authorityZone;
  Name m_forwardingHint;

  enum QueryType m_queryType;
  time::milliseconds m_interestLifetime;
  Name m_rrLabel;
  enum RR::RRType m_rrType;


  mutable Block m_wire;
};

inline std::ostream&
operator<<(std::ostream& os, const Query& query)
{
  os << "Query: authorityZone=" << query.getAuthorityZone().toUri()
      << " queryType=" << Query::toString(query.getQueryType()) << " rrLabel="
      << query.getRrLabel().toUri() << " rrType="
      << RR::toString(query.getRrType());
  return os;
}

} // namespace ndns
} // namespace ndn

#endif // NDNS_QUERY_HPP
