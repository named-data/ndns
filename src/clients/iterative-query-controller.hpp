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

#ifndef NDNS_CLIENTS_ITERATIVE_QUERY_CONTROLLER_HPP
#define NDNS_CLIENTS_ITERATIVE_QUERY_CONTROLLER_HPP

#include "ndns-enum.hpp"
#include "query.hpp"
#include "response.hpp"
#include "query-controller.hpp"
#include "config.hpp"
#include "common.hpp"

#include <ndn-cxx/common.hpp>
#include <ndn-cxx/data.hpp>
#include <ndn-cxx/interest.hpp>
#include <ndn-cxx/name.hpp>

namespace ndn {
namespace ndns {

/**
 * @brief controller which iteratively query a target label
 */
class IterativeQueryController : public QueryController
{
public:
  /**
   * @brief indicates a step in an iterative query process.
   * Iterative Query contains multiple steps which are listed here
   */
  enum QueryStep {
    QUERY_STEP_QUERY_NS = 1, ///< to query the naming server, before querying NS & waiting for Data
    QUERY_STEP_QUERY_RR, ///< to query RR, before querying RR & waiting for it Data
    QUERY_STEP_ANSWER_STUB, ///< to answer to stub resolver, after getting final Response,
                            ///< or NACK or timeout
    QUERY_STEP_ABORT, ///< to abort the resolver process, if unexpected behavior happens
    QUERY_STEP_UNKNOWN = 255
  };

public:
  explicit
  IterativeQueryController(const Name& dstLabel, const name::Component& rrType,
                           const time::milliseconds& interestLifetime,
                           const QuerySucceedCallback& onSucceed, const QueryFailCallback& onFail,
                           Face& face);

  virtual void
  start();

  /**
   * @brief return false if the current status is not QUEYR_STEP_QUERY_NS
   * or QUERY_STEP_QUERY_RR
   */
  virtual bool
  hasEnded();

NDNS_PUBLIC_WITH_TESTS_ELSE_PRIVATE:
  void
  onData(const ndn::Interest& interest, const Data& data);

  /**
   * @brief change the Controller state according to timeout. For current,
   * abort the query when timeout
   */
  void
  onTimeout(const Interest& interest);

  void
  abort();

  /**
   * @brief get the Interest according to current Controller state.
   * Only be valid on State QueryNS & QueryRR, or throw exception
   */
  const Interest
  makeLatestInterest();

  const Response
  parseFinalResponse(const Data& data);

  void
  express(const Interest& interest);

  void
  setNFinishedComps(size_t finished)
  {
    m_nFinishedComps = finished;
  }

public:
  QueryStep
  getStep() const
  {
    return m_step;
  }

  size_t
  getNFinishedComps() const
  {
    return m_nFinishedComps;
  }

  size_t
  getNTryComps() const
  {
    return m_nTryComps;
  }

protected:
  /**
   * @brief current query step
   */
  QueryStep m_step;

  /*
   * the number of label that has been resolved successfully
   * This also define the next AuthZone prefix
   * AuthZone = m_query.getRrLabel().getPrefix(m_nFinishedComps)
   */
  size_t m_nFinishedComps;

  /*
   * used when query the KSK (key signing key), e.g., /net/ndnsim/ksk-1
   */
  size_t m_nTryComps;
};

std::ostream&
operator<<(std::ostream& os, const IterativeQueryController& iq);

std::ostream&
operator<<(std::ostream& os, const IterativeQueryController::QueryStep step);

} // namespace ndns
} // namespace ndn

#endif // NDNS_CLIENTS_ITERATIVE_QUERY_HPP
