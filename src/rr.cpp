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

#include "rr.hpp"

namespace ndn {
namespace ndns {

RR::RR()
  : m_id(0Lu)
  , m_rrData("ex.com")
{
}

RR::~RR()
{
}

template<bool T>
size_t
RR::wireEncode(EncodingImpl<T> & block) const
{
  size_t totalLength = 0;
  const std::string& msg = this->getRrdata();
  totalLength += prependByteArrayBlock(block,
                                       tlv::ndns::RRData,
                                       reinterpret_cast<const uint8_t*>(msg.c_str()),
                                       msg.size()
                                       );

  return totalLength;
}

const Block&
RR::wireEncode() const
{
  if (m_wire.hasWire())
    return m_wire;
  EncodingEstimator estimator;
  size_t estimatedSize = wireEncode(estimator);
  EncodingBuffer buffer(estimatedSize, 0);
  wireEncode(buffer);
  m_wire = buffer.block();
  return m_wire;
}

void
RR::wireDecode(const Block& wire)
{
  if (!wire.hasWire()) {
    throw Tlv::Error("The supplied block does not contain wire format");
  }

  if (wire.type() != tlv::ndns::RRData)
    throw Tlv::Error("Unexpected TLV type when decoding Content");

  m_wire = wire;
  m_wire.parse();

  Block::element_const_iterator it = m_wire.elements_begin();

  if (it != m_wire.elements_end() && it->type() == tlv::ndns::RRData)
    {
      m_rrData = std::string(reinterpret_cast<const char*>(it->value()), it->value_size());
      it ++;
    } else {
    throw Tlv::Error("not the RRData Type");
  }

}

} // namespace ndns
} // namespace ndn
