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

#include "iterative-query-controller.hpp"
#include "logger.hpp"
#include <iostream>

namespace ndn {
namespace ndns {
NDNS_LOG_INIT("IterQueryCtr")

IterativeQueryController::IterativeQueryController(const Name& dstLabel,
                                                   const name::Component& rrType,
                                                   const time::milliseconds& interestLifetime,
                                                   const QuerySucceedCallback& onSucceed,
                                                   const QueryFailCallback& onFail,
                                                   Face& face)
  : QueryController(dstLabel, rrType, interestLifetime, onSucceed, onFail, face)
  , m_step(QUERY_STEP_QUERY_NS)
  , m_nFinishedComps(0)
  , m_nTryComps(1)
{
  if (m_dstLabel.size() == 1) // the first one is to Query RR directly
    m_step = QUERY_STEP_QUERY_RR;
}

void
IterativeQueryController::onTimeout(const Interest& interest)
{
  NDNS_LOG_INFO("[* !! *] timeout happens: " << interest.getName());
  NDNS_LOG_TRACE(*this);
  this->abort();
}

void
IterativeQueryController::abort()
{
  NDNS_LOG_DEBUG("abort iterative query");
  if (m_onFail != nullptr)
    m_onFail(0, "abort");
  else
    NDNS_LOG_TRACE("m_onFail is 0");

}

void
IterativeQueryController::onData(const ndn::Interest& interest, const Data& data)
{
  NdnsType ndnsType = NDNS_RAW;
  const Block* block = data.getMetaInfo().findAppMetaInfo(ndns::tlv::NdnsType);
  if (block != nullptr) {
    ndnsType = static_cast<NdnsType>(readNonNegativeInteger(*block));
  }

  NDNS_LOG_TRACE("[* -> *] get a " << ndnsType
                 << " Response: " << data.getName());

  switch (m_step) {
  case QUERY_STEP_QUERY_NS:
    if (ndnsType == NDNS_NACK) {
      if (m_nFinishedComps + m_nTryComps == m_dstLabel.size() && m_rrType != label::NS_RR_TYPE)
        m_step = QUERY_STEP_QUERY_RR;
      else
        m_step = QUERY_STEP_ANSWER_STUB;
    }
    else if (ndnsType == NDNS_RESP) {
      if (m_nFinishedComps + m_nTryComps == m_dstLabel.size() && m_rrType == label::NS_RR_TYPE) {
        // NS_RR_TYPE is different, since its record is stored at higher level
        m_step = QUERY_STEP_ANSWER_STUB;
      }
      else {
        m_nFinishedComps += m_nTryComps;
        m_nTryComps = 1;
      }
    }
    else if (ndnsType == NDNS_AUTH) {
      m_nTryComps += 1;
    }
    else if (ndnsType == NDNS_RAW) {
      std::ostringstream oss;
      oss << *this;
      NDNS_LOG_WARN("get unexpected Response: NDNS_RAW for QUERY_NS: " << oss.str());
    }
    //
    if (m_nFinishedComps + m_nTryComps > m_dstLabel.size()) {
      if (m_rrType == label::NS_RR_TYPE) {
        m_step = QUERY_STEP_ANSWER_STUB;
      }
      else
        m_step = QUERY_STEP_QUERY_RR;
    }
    break;
  case QUERY_STEP_QUERY_RR:
    m_step = QUERY_STEP_ANSWER_STUB;
    break;
  default:
    NDNS_LOG_WARN("get unexpected Response at State " << *this);
    // throw std::runtime_error("call makeLatestInterest() unexpected: " << *this);
    // do not throw except since it may be duplicated Data
    m_step = QUERY_STEP_ABORT;
    break;
  }

  if (!hasEnded())
    this->express(this->makeLatestInterest()); // express new Expres
  else if (m_step == QUERY_STEP_ANSWER_STUB) {
    NDNS_LOG_TRACE("query ends: " << *this);
    Response re = this->parseFinalResponse(data);
    if (m_onSucceed != nullptr)
      m_onSucceed(data, re);
    else
      NDNS_LOG_TRACE("succeed callback is nullptr");
  }
  else if (m_step == QUERY_STEP_ABORT)
    this->abort();
}

bool
IterativeQueryController::hasEnded()
{
  return (m_step != QUERY_STEP_QUERY_NS && m_step != QUERY_STEP_QUERY_RR);
}

void
IterativeQueryController::start()
{
  Interest interest = this->makeLatestInterest();
  express(interest);
}


void
IterativeQueryController::express(const Interest& interest)
{
  NDNS_LOG_DEBUG("[* <- *] send a Query: " << interest.getName());
  m_face.expressInterest(interest,
                         bind(&IterativeQueryController::onData, this, _1, _2),
                         bind(&IterativeQueryController::onTimeout, this, _1)
                         );
}


const Response
IterativeQueryController::parseFinalResponse(const Data& data)
{
  Response re;
  Name zone = m_dstLabel.getPrefix(m_nFinishedComps);
  re.fromData("", zone, data);
  return re;
}

const Interest
IterativeQueryController::makeLatestInterest()
{
  // NDNS_LOG_TRACE("get latest Interest");
  Query query;
  //const Name& dstLabel = m_query.getRrLabel();

  query.setZone(m_dstLabel.getPrefix(m_nFinishedComps));
  query.setInterestLifetime(m_interestLifetime);
  switch (m_step) {
  case QUERY_STEP_QUERY_NS:
    query.setQueryType(label::NDNS_ITERATIVE_QUERY);
    query.setRrLabel(m_dstLabel.getSubName(m_nFinishedComps, m_nTryComps));
    query.setRrType(label::NS_RR_TYPE);
    break;
  case QUERY_STEP_QUERY_RR:
    if (m_rrType == label::CERT_RR_TYPE) {
      // this only works for dsk, and ksk needs different mechanism
      query.setQueryType(label::NDNS_CERT_QUERY);
    }
    else {
      query.setQueryType(label::NDNS_ITERATIVE_QUERY);
    }
    query.setRrLabel(m_dstLabel.getSubName(m_nFinishedComps));
    query.setRrType(m_rrType);
    break;
  default:
    std::ostringstream oss;
    oss << *this;
    NDNS_LOG_WARN("unexpected state: " << oss.str());
    throw std::runtime_error("call makeLatestInterest() unexpected: " + oss.str());
  }


  Interest interest = query.toInterest();
  return interest;
}

std::ostream&
operator<<(std::ostream& os, const IterativeQueryController::QueryStep step)
{
  switch (step) {
  case IterativeQueryController::QUERY_STEP_QUERY_NS:
    os << "QueryNS";
    break;
  case IterativeQueryController::QUERY_STEP_QUERY_RR:
    os << "QueryRR";
    break;
  case IterativeQueryController::QUERY_STEP_ANSWER_STUB:
    os << "AnswerStub";
    break;
  case IterativeQueryController::QUERY_STEP_ABORT:
    os << "Abort";
    break;
  default:
    os << "UNKNOW";
    break;
  }
  return os;
}

std::ostream&
operator<<(std::ostream& os, const IterativeQueryController& ctr)
{
  os << "InterativeQueryController: dstLabel=" << ctr.getDstLabel()
     << " rrType=" << ctr.getRrType()
     << " currentStep="  << ctr.getStep()
     << " nFinishedComps=" << ctr.getNFinishedComps()
     << " nTryComp=" << ctr.getNTryComps()
    ;

  return os;
}

} // namespace ndns
} // namespace ndn
