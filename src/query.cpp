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

namespace ndn {
namespace ndns {

Query::Query()
  : m_interestLifetime(time::milliseconds(4000))
{
}

Query::~Query() {
}


void
Query::fromWire(const Name &name, const Interest &interest)
{
  Name interestName;
  interestName = interest.getName();

  int qtflag = -1;
  size_t len = interestName.size();
  for (size_t i=0; i<len; i++)
    {
      std::string comp = interestName.get(i).toUri();
      if (comp == toString(QUERY_DNS) || comp == toString(QUERY_DNS_R))
        {
          qtflag = i;
          break;
        }
    }//for

  if (qtflag == -1)
    {
      std::cerr << "There is no QueryType in the Interest Name: " << interestName << std::endl;
      return;
    }
  this->m_queryType = toQueryType(interestName.get(qtflag).toUri());
  this->m_rrType = toRRType(interestName.get(len-1).toUri());
  this->m_authorityZone = interestName.getPrefix(qtflag); //the DNS/DNS-R is not included
  this->m_interestLifetime = interest.getInterestLifetime();

}

template<bool T>
size_t
Query::wireEncode(EncodingImpl<T>& block) const
{
  size_t totalLength = 0;
  totalLength += 0;


  size_t totalLength = prependByteArrayBlock(block, tlv::Bytes,
                                             m_key.get().buf(), m_key.get().size());
  totalLength += m_keyName.wireEncode(block);
  totalLength += block.prependVarNumber(totalLength);
  totalLength += block.prependVarNumber(tlv::PublicKey);

  return totalLength;
}

Interest
Query::toWire() const
{
  Name name = this->m_authorityZone;
  name.append(toString(this->m_queryType));
  name.append(this->m_rrLabel);
  name.append(toString(this->m_rrType));
  Selectors selector;
  //selector.setMustBeFresh(true);

  Interest interest = Interest(name, selector, -1, this->m_interestLifetime);
  return interest;
}

} // namespace ndns
} // namespace ndn
