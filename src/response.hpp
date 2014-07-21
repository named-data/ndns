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

namespace ndn {
namespace ndns {

enum ResponseType
  {
    NDNS_Resp,
    NDNS_Nack,
    NDNS_Auth
  };


static std::string
toString(ResponseType responseType) const
{
  string label;
  switch (responseType)
    {
    case ResponseType::NDNS_Resp:
      label = "NDNS Resp";
      break;
    case ResponseType::NDNS_Nack:
      label = "NDNS Nack";
      break;
    case ResponseType::NDNS_Auth:
      label = "NDNS Auth";
      break;
    default:
      label = "Default";
      break;
    }
  return label;
}

class Response {
public:
  Response();
  virtual ~Response();

  Data
  toWire() const;

  void
  fromWire(const Interest &interest, const Data &data);

private:
  Name m_queryName;
  std::string m_serial;
  ResponseType m_responseType;

  time::milliseconds m_freshness;

  unsigned int m_numberOfRR;
  vector<RR>  m_rrs;

};

} // namespace ndns
} // namespace ndn

#endif // NDNS_RESPONSE_HPP
