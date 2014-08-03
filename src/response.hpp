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

#ifndef NDNS_RESPONSE_HPP
#define NDNS_RESPONSE_HPP

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/noncopyable.hpp>

#include <ndn-cxx/face.hpp> // /usr/local/include
#include <ndn-cxx/name.hpp>
#include <ndn-cxx/data.hpp>

#include <ndn-cxx/encoding/block-helpers.hpp>
#include <ndn-cxx/encoding/block.hpp>
#include <ndn-cxx/encoding/tlv-ndnd.hpp>

#include <vector>

#include "ndns-tlv.hpp"
#include "query.hpp"
#include "rr.hpp"

namespace ndn {
namespace ndns {

class Response
{
public:
  enum ResponseType
  {
    NDNS_Resp,
    NDNS_Nack,
    NDNS_Auth,
    UNKNOWN
  };

  static std::string toString(ResponseType responseType)
  {
    std::string label;
    switch (responseType) {
    case NDNS_Resp:
      label = "NDNS Resp";
      break;
    case NDNS_Nack:
      label = "NDNS Nack";
      break;
    case NDNS_Auth:
      label = "NDNS Auth";
      break;
    default:
      label = "UNKNOWN";
      break;
    }
    return label;
  }

  static ResponseType toResponseType(const std::string& str)
  {
    ResponseType atype;
    if (str == "NDNS Resp") {
      atype = NDNS_Resp;
    } else if (str == "NDNS Nack") {
      atype = NDNS_Nack;
    } else if (str == "NDNS Auth") {
      atype = NDNS_Auth;
    } else {
      atype = UNKNOWN;
    }
    return atype;
  }

  Response();
  virtual ~Response();

  inline void addRr(const uint32_t rrId, const std::string rrData)
  {
    RR rr;
    rr.setId(rrId);
    rr.setRrdata(rrData);
    this->m_rrs.push_back(rr);
  }

  const Block&
  wireEncode() const;

  void
  wireDecode(const Block& wire);

  template<bool T>
  size_t
  wireEncode(EncodingImpl<T> & block) const;

  void
  fromData(const Name& name, const Data& data);

  void
  fromData(const Data &data);

  Data
  toData() const;

  const std::string getStringRRs() const
  {
    std::stringstream str;
    str << "[";
    std::vector<RR>::const_iterator iter = m_rrs.begin();
    while (iter != m_rrs.end()) {
      str << " " << *iter;
      iter++;
    }
    str << "]";
    return str.str();
  }

  Query::QueryType getContentType() const
  {
    return m_contentType;
  }

  void setContentType(Query::QueryType contentType)
  {
    m_contentType = contentType;
  }

  time::milliseconds getFreshness() const
  {
    return m_freshness;
  }

  void setFreshness(time::milliseconds freshness)
  {
    m_freshness = freshness;
  }

  const Name& getQueryName() const
  {
    return m_queryName;
  }

  void setQueryName(const Name& queryName)
  {
    m_queryName = queryName;
  }

  ResponseType getResponseType() const
  {
    return m_responseType;
  }

  void setResponseType(ResponseType responseType)
  {
    m_responseType = responseType;
  }

  const std::vector<RR>& getRrs() const
  {
    return m_rrs;
  }

  void setRrs(const std::vector<RR>& rrs)
  {
    m_rrs = rrs;
  }

private:
  time::milliseconds m_freshness;
  Name m_queryName;
  //std::string m_serial;
  Query::QueryType m_contentType;
  ResponseType m_responseType;
  //unsigned int m_numberOfRR;
  std::vector<RR> m_rrs;
  mutable Block m_wire;

};

inline std::ostream&
operator<<(std::ostream& os, const Response& response)
{
  os << "Response: queryName=" << response.getQueryName().toUri()
      << " responseType=" << Response::toString(response.getResponseType())
      << " contentType=" << Query::toString(response.getContentType()) << " [";
  std::vector<RR>::const_iterator iter = response.getRrs().begin();
  while (iter != response.getRrs().end()) {
    os << " " << *iter;
    iter++;
  }
  os << "]";
  return os;
}

} // namespace ndns
} // namespace ndn

#endif // NDNS_RESPONSE_HPP
