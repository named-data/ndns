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

namespace ndn {
namespace ndns {

enum RRType
  {
    NS,
    TXT,
    UNKNOWN
  };

class RR {
public:
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

private:
  template<bool T>
  size_t
  wireEncode(EncodingImpl<T> & block) const;

public:

  const Block&
  wireEncode() const;

  void
  wireDecode(const Block& wire);


  Interest
  toWire() const;


private:
  uint32_t m_id;
  std::string m_rrData;

  mutable Block m_wire;
};

} // namespace ndns
} // namespace ndn

#endif // NDNS_RR_HPP
