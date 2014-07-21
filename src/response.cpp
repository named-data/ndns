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

#include "response.hpp"


namespace ndn {
namespace ndns {

Response::Response()
{
}

Response::~Response()
{
}


Data
Response::toWire() const
{
  Name name = this->m_queryName;
  name.append(this->m_serial);

  Data data(name);

  data.setFreshnessPeriod(this->m_freshness);

  string content = "";

  size_t totalLen = 0;
  Block block = Block();
  block.push_back
    (nonNegativeIntegerBlock
     (tlv::ndns::Type, static_cast<unsigned long>(this->m_responseType))
     );

  block.push_back
    (nonNegativeIntegerBlock
     (tlv::ndns::Fressness, this->m_freshness.count())
     );

  Block block2 = Block(tlv::ndns::ContentBlob);
  block2.push_back
    (nonNegativeIntegerBlock
     (tlv::ndns::NumberOfRRData, this->m_numberOfRR)
     );

  for (int i=0; i<this->m_numberOfRR; i++)
    {
      RR rr = m_rrs[i];
      block2.push_back(rr.toWire());
    }

  block.push_back(block2);

  return data;

}

void
Response::fromWire(const Interest &interest, const Data &data)
{
  Name dataName;
  dataName = data.getName();

  int qtflag = -1;
  size_t len = dataName.size();
  for (size_t i=0; i<len; i++)
    {
      string comp = dataName.get(i).toEscapedString();
      if (comp == ndn::toString(QueryType::DNS) || comp == ndn::toString(QueryType::DNS_R))
        {
          qtflag = i;
          break;
        }
    }//for

  if (qtflag == -1)
    {
      cerr<<"There is no QueryType in the Interest Name: "<<dataName<<endl;
      return;
    }

  this->m_queryName = dataName.getPrefix(-1);

  string last = dataName.get(len-1).toEscapedString();
  if (ndn::toRRType(last) == RRType::UNKNOWN)
    {
      this->m_serial = "";
    } else
    {
      this->m_serial = last;
    }

  Block block = data.getContent();
  this->m_numberOfRR = readNonNegativeInteger(block.get(tlv::ndns::NumberOfRRData));

  Block block2 = block.get(tlv::ndns::ContentBlob);
  for (int i=0; i<this->m_numberOfRR; i++)
    {
      Block block3 = block2.get(tlv::ndns::RRData);
      RR rr;
      rr.fromWire(block3);
      m_rrs.push_back(rr);
    }

}


} // namespace ndns
} // namespace ndn
