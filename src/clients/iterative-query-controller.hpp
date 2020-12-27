/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2020, Regents of the University of California.
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
#include "query-controller.hpp"
#include "response.hpp"
#include "validator/validator.hpp"

#include <ndn-cxx/ims/in-memory-storage.hpp>
#include <ndn-cxx/link.hpp>

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
                           Face& face, security::Validator* validator = nullptr,
                           ndn::InMemoryStorage* cache = nullptr);

  void
  start() override;

  /**
   * @brief return false if the current status is not QUEYR_STEP_QUERY_NS
   * or QUERY_STEP_QUERY_RR
   */
  bool
  hasEnded() override;

NDNS_PUBLIC_WITH_TESTS_ELSE_PRIVATE:
  void
  onData(const ndn::Interest& interest, const Data& data);

  /**
   * @brief called when any data are validated.
   * It will unwrap the NACK record,
   * so onSucceed callback will be called with only inner DoE
   */
  void
  onDataValidated(const Data& data, NdnsContentType contentType);

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
   * Only be valid on State QueryNS and QueryRR, or throw exception
   */
  const Interest
  makeLatestInterest();

  const Response
  parseFinalResponse(const Data& data);

  void
  express(const Interest& interest);

public:
  void
  setStartComponentIndex(size_t finished) override
  {
    m_nFinishedComps = finished;
  }

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

private:
  bool
  isAbsentByDoe(const Data& data) const;

protected:
  security::Validator* m_validator;
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

private:
  Block m_lastLink;
  Data m_doe;
  Name m_lastLabelType;
  ndn::InMemoryStorage* m_nsCache;
};

std::ostream&
operator<<(std::ostream& os, const IterativeQueryController& iq);

std::ostream&
operator<<(std::ostream& os, const IterativeQueryController::QueryStep step);

// Used if you want the controller's lifetime equals to other object inherited
// from TagHost. For example, in the CertificateFetcher, the queryController's
// lifetime is equal to ValidationState.
using IterativeQueryTag = SimpleTag<shared_ptr<IterativeQueryController>, 1086>;

} // namespace ndns
} // namespace ndn

#endif // NDNS_CLIENTS_ITERATIVE_QUERY_CONTROLLER_HPP
