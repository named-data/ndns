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
  : m_freshness (time::milliseconds(3600))
  , m_contentType (Query::QUERY_DNS)
  , m_responseType (NDNS_Resp)
{
}

Response::~Response()
{
}

template<bool T>
size_t
Response::wireEncode(EncodingImpl<T> & block) const
{
    size_t totalLength = 0;
    std::vector<RR>::size_type lenOfRR = m_rrs.size();
    RR rr;
    Block btemp;

    //totalLength += m_rrs[0].wireEncode(block);

    for (std::vector<RR>::size_type i=0; i<lenOfRR; i++)
    {
      rr = m_rrs[lenOfRR-i-1];
      totalLength += rr.wireEncode(block);
      //std::cout<<"totalLenght="<<totalLength<<std::endl;
    }

    totalLength += prependNonNegativeIntegerBlock(block,
                        ndn::ndns::tlv::ResponseNumberOfRRData,
                        lenOfRR);

    totalLength += block.prependVarNumber(totalLength);
    totalLength += block.prependVarNumber(ndn::ndns::tlv::ResponseContentBlob);

    totalLength += prependNonNegativeIntegerBlock(block,
                                ndn::ndns::tlv::ResponseFressness,
                                m_freshness.count()
                                  );

    std::string msg = Response::toString(m_responseType);
    totalLength += prependByteArrayBlock(block,
                           ndn::ndns::tlv::ResponseType,
                           reinterpret_cast<const uint8_t*>(msg.c_str()),
                           msg.size()
                           );

    totalLength += m_queryName.wireEncode(block);


    totalLength += block.prependVarNumber(totalLength);
    totalLength += block.prependVarNumber(Tlv::Content);
    return totalLength;
}


const Block&
Response::wireEncode() const
{

  if (m_wire.hasWire())
     return m_wire;
   EncodingEstimator estimator;

   size_t estimatedSize = wireEncode(estimator);
   //std::cout<< typeid( this).name()<<" Instance estimatedsize="<<estimatedSize<<std::endl;
   EncodingBuffer buffer(estimatedSize, 0);
   wireEncode(buffer);
   m_wire = buffer.block();
   return m_wire;
}


void
Response::wireDecode(const Block& wire)
{
  if (!wire.hasWire()) {
    throw Tlv::Error("The supplied block does not contain wire format");
  }

  //if (wire.type() != ndn::ndns::tlv::ResponseContentBlob)
  //  throw Tlv::Error("Unexpected TLV type when decoding Content");

  m_wire = wire;
  m_wire.parse();


  Block::element_const_iterator it = m_wire.elements_begin();

  if (it != m_wire.elements_end() && it->type() == ndn::Tlv::Name)
  {
    m_queryName.wireDecode(*it);
    it ++;
  } else
  {
    throw Tlv::Error("not the ndn::Tlv::Name type");
  }

  if (it != m_wire.elements_end() && it->type() == ndn::ndns::tlv::ResponseType)
  {
    std::string temp = std::string(reinterpret_cast<const char*>(it->value()),
                          it->value_size());
    m_responseType = Response::toResponseType(temp);
      it ++;
  } else
  {
      throw Tlv::Error("not the ndn::ndns::tlv::ReponseType type");
  }

  if (it != m_wire.elements_end() && it->type() == ndn::ndns::tlv::ResponseFressness)
    {
      m_freshness = time::milliseconds(readNonNegativeInteger(*it));
      it ++;
    } else
    {
      throw Tlv::Error("not the ndn::ndns::tlv::ReponseFreshness type");
    }


  if (it != m_wire.elements_end() && it->type() == ndn::ndns::tlv::ResponseContentBlob)
    {
      //Block b2 = it->value();/* to check */
      Block b2 = *it;/* to check */

      b2.parse();
      Block::element_const_iterator it2 = b2.elements_begin();
      size_t rrlen = 0;
      if (it2 != b2.elements_end() && it2->type() == ndn::ndns::tlv::ResponseNumberOfRRData)
      {
         rrlen = readNonNegativeInteger(*it2);
         it2 ++;
      } else
      {
        throw Tlv::Error("not the ndn::ndns::tlv::ResponseNumberOfRRData type");
      }
      for (size_t i=0; i<rrlen; i++)
      {
        if (it2 != b2.elements_end() &&
            it2->type() == ndn::ndns::tlv::RRData)
        {
          RR rr;
          rr.wireDecode(*it2);
          this->m_rrs.push_back(rr);
          it2 ++;
        } else
        {
          throw Tlv::Error("not the ndn::ndns::tlv::RRData type");
        }
      }


      it ++;
    } else
    {
      throw Tlv::Error("not the ndn::ndns::tlv::ResponseContentBlob type");
    }



}

void
Response::fromData(const Data& data)
{

    m_queryName = data.getName();
    m_freshness = data.getFreshnessPeriod();
    m_contentType = Query::QueryType(data.getContentType());
    this->wireDecode(data.getContent());
}


void
Response::fromData(const Name& name, const Data& data)
{
  fromData(data);
}

Data
Response::toData() const
{
  Data data;
  data.setName(m_queryName);
  data.setFreshnessPeriod(this->m_freshness);
  data.setContentType(m_contentType);
  data.setContent(this->wireEncode());

  return data;
}


} // namespace ndns
} // namespace ndn
