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
#include "iterative-query.hpp"

namespace ndn {
namespace ndns {

IterativeQuery::IterativeQuery(const Query& query)
  : m_step(IterativeQuery::NSQuery)
  , m_tryNum(0)
  , m_tryMax(2)
  , m_query(query)
  , m_finishedLabelNum(0)
  , m_lastFinishedLabelNum(0)
  , m_rrLabelLen(1)
  , m_authZoneIndex(0)
{
}

bool IterativeQuery::doTimeout()
{
  abort();
  return false;
}

void IterativeQuery::abort()
 {
  std::cout<<"Abort the Resolving"<<std::endl;
   std::cout << (*this);
   std::cout << std::endl;
 }

void IterativeQuery::doData(Data& data)
{
  std::cout << "[* -> *] resolve Data: " << data.getName().toUri() << std::endl;
  Response re;
  re.fromData(data);
  std::cout << re << std::endl;

  if (re.getResponseType() == Response::UNKNOWN) {
    std::cout << "[* !! *] unknown content type and exit";
    m_step = Abort;
    abort();
    return;
  } else if (re.getResponseType() == Response::NDNS_Nack) {
    if (m_step == NSQuery) {
      //In fact, there are two different situations
      //1st: /net/ndnsim/DNS/www/NS is nacked
      //2st: /net/DNS/ndnsim/www/NS is nacked
      m_step = RRQuery;

      if (m_query.getRrType() == RR::NDNCERT && m_rrLabelLen == 1) {
        //here working for KSK and NDNCERT when get a Nack
        //e.g., /net/ndnsim/ksk-1, ksk-1 returns nack, but it should query /net

        Name dstLabel = m_query.getRrLabel();
        Name label = dstLabel.getSubName(m_finishedLabelNum, m_rrLabelLen);
        if (boost::starts_with(label.toUri(), "/ksk") || boost::starts_with(label.toUri(), "/KSK")) {
          m_finishedLabelNum = m_lastFinishedLabelNum;
        }

      }
    } else if (m_step == RRQuery) {
      m_step = AnswerStub;
    }

    m_lastResponse = re;
  } else if (re.getResponseType() == Response::NDNS_Auth) { // need more specific info
    m_rrLabelLen += 1;
  } else if (re.getResponseType() == Response::NDNS_Resp) { // get the intermediate answer
    if (m_step == NSQuery) {
      //do nothing, step NSQuery
      m_lastFinishedLabelNum = m_finishedLabelNum;
      m_finishedLabelNum += m_rrLabelLen;
      m_rrLabelLen = 1;
      m_authZoneIndex = 0;
      m_lastResponse = re;

    } else if (m_step == RRQuery) { // final resolver gets result back
      m_step = AnswerStub;
      m_lastResponse = re;
    }

    std::cout << "get RRs: " << m_lastResponse.getStringRRs() << std::endl;
  }

}

const Interest IterativeQuery::toLatestInterest()
{
  Query query = Query();
  Name dstLabel = m_query.getRrLabel();

  Name authZone = dstLabel.getPrefix(m_finishedLabelNum);

  Name label;
  if (m_step == RRQuery) {
    label = dstLabel.getSubName(m_finishedLabelNum);
  } else {
    label = dstLabel.getSubName(m_finishedLabelNum, m_rrLabelLen);
  }
  query.setAuthorityZone(authZone);
  query.setRrLabel(label);

  if (m_step == NSQuery) {
    query.setRrType(RR::NS);
    query.setQueryType(Query::QUERY_DNS);
  } else if (m_step == RRQuery) {
    query.setRrType(m_query.getRrType());
    if (m_query.getRrType() == RR::NDNCERT) {
      query.setQueryType(Query::QUERY_KEY);
      query.setQueryType(Query::QUERY_DNS);
    } else {
      query.setQueryType(Query::QUERY_DNS);
    }

  } else if (m_step == AnswerStub) {
    query.setRrType(m_query.getRrType());
    query.setQueryType(Query::QUERY_DNS);
  }
  Interest interest = query.toInterest();
  //m_lastInterest = interest;

  return interest;
}

} /* namespace ndns */
} /* namespace ndn */
