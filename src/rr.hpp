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

#ifndef NDNS_RR_HPP
#define NDNS_RR_HPP

#include "ndns-tlv.hpp"

#include <ndn-cxx/encoding/block.hpp>
#include <ndn-cxx/interest.hpp>

#include <ndn-cxx/encoding/encoding-buffer.hpp>

namespace ndn {
namespace ndns {

class RR
{
public:

  enum RRType
  {
    NS,
    TXT,
    FH,
    A,
    AAAA,
    NDNCERT,
    UNKNOWN
  };
  static std::string toString(const RRType& type)
  {
    std::string str;

    switch (type) {
    case NS:
      str = "NS";
      break;
    case TXT:
      str = "TXT";
      break;
    case FH:
      str = "FH";
      break;
    case A:
      str = "A";
      break;
    case AAAA:
      str = "AAAA";
      break;
    case NDNCERT:
      str = "NDNCERT";
      break;
    default:
      str = "UNKNOWN";
      break;
    }
    return str;
  }

  static RRType toRRType(const std::string& str)
  {
    RRType atype;
    if (str == "NS") {
      atype = NS;
    } else if (str == "TXT") {
      atype = TXT;
    } else if (str == "FH") {
      atype = FH;
    } else if (str == "A") {
      atype = A;
    } else if (str == "AAAA") {
      atype = AAAA;
    } else if (str == "NDNCERT") {
      atype = NDNCERT;
    }
    else {
      atype = UNKNOWN;
    }
    return atype;
  }

  RR();
  virtual ~RR();

  const std::string&
  getRrdata() const
  {

    return m_rrData;
  }

  void setRrdata(const std::string& rrdata)
  {
    this->m_rrData = rrdata;
  }

public:

  uint32_t getId() const
  {
    return m_id;
  }

  void setId(uint32_t id)
  {
    m_id = id;
  }

  const Block& getWire() const
  {
    return m_wire;
  }

  void setWire(const Block& wire)
  {
    m_wire = wire;
  }

  inline bool operator==(const RR& rr) const
  {
    if (this->getRrdata() == rr.getRrdata())
      return true;

    return false;
  }

  template<bool T>
  inline size_t wireEncode(EncodingImpl<T> & block) const
  {
    size_t totalLength = 0;
    const std::string& msg = this->getRrdata();
    totalLength += prependByteArrayBlock(block, ndn::ndns::tlv::RRDataSub2,
        reinterpret_cast<const uint8_t*>(msg.c_str()), msg.size());

    totalLength += prependNonNegativeIntegerBlock(block,
        ndn::ndns::tlv::RRDataSub1, this->getId());

    totalLength += block.prependVarNumber(totalLength);
    totalLength += block.prependVarNumber(ndn::ndns::tlv::RRData);
    //std::cout<<"call rr.h wireEncode"<<std::endl;
    return totalLength;
  }

  const Block&
  wireEncode() const;

  void
  wireDecode(const Block& wire);

  //inline std::ostream& operator<<(std::ostream& os, const RR& rr);

private:
  uint32_t m_id;
  //unsigned long m_id;
  std::string m_rrData;

  mutable Block m_wire;
};
//class RR

inline std::ostream&
operator<<(std::ostream& os, const RR& rr)
{
  os << "RR: Id=" << rr.getId() << " Data=" << rr.getRrdata();
  return os;
}

} // namespace ndns
} // namespace ndn

#endif // NDNS_RR_HPP
