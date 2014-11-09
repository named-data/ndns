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

#ifndef NDNS_CLIENTS_QUERY_CONTROLLER_HPP
#define NDNS_CLIENTS_QUERY_CONTROLLER_HPP

#include "query.hpp"
#include "response.hpp"

#include <ndn-cxx/data.hpp>
#include <ndn-cxx/interest.hpp>
#include <ndn-cxx/name.hpp>
#include <ndn-cxx/face.hpp>


namespace ndn {
namespace ndns {

/**
 * @brief callback function when succeeding getting the final Response
 * @param[in] Data the Data packet which contains the Response, client should verify the packet
 * @param[in] Response the Final Response converted from Data
 */
typedef function<void(const Data&, const Response&)> QuerySucceedCallback;

/**
 * @brief callback function when failing to get the final Response
 */
typedef function<void(uint32_t errCode, const std::string& errMsg)> QueryFailCallback;

/**
 * @brief a Query Controller interface
 *
 */
class QueryController : noncopyable
{
public:
  QueryController(const Name& dstLabel, const name::Component& rrType,
                  const time::milliseconds& interestLifetime,
                  const QuerySucceedCallback& onSucceed, const QueryFailCallback& onFail,
                  Face& face);

  /**
   * @brief start query process.
   */
  virtual void
  start() = 0;

  /**
   * @brief should keep sending query, or has ended yet,
   */
  virtual bool
  hasEnded() = 0;

public:
  ////////////////
  // getter

  const Name&
  getDstLabel() const
  {
    return m_dstLabel;
  }

  const time::milliseconds&
  getInterestLifetime() const
  {
    return m_interestLifetime;
  }

  const name::Component&
  getRrType() const
  {
    return m_rrType;
  }

protected:
  const Name m_dstLabel;
  const name::Component m_rrType;
  const time::milliseconds m_interestLifetime;

  const QuerySucceedCallback m_onSucceed;
  const QueryFailCallback m_onFail;

  Face& m_face;

};

std::ostream&
operator<<(std::ostream& os, const QueryController& ctr);

} // namespace ndns
} // namespace ndn

#endif // NDNS_CLIENTS_QUERY_CONTROLLER_HPP
