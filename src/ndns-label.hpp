/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2017, Regents of the University of California.
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

#ifndef NDNS_NDNS_LABEL_HPP
#define NDNS_NDNS_LABEL_HPP

#include "ndns-enum.hpp"

#include <ndn-cxx/name.hpp>
#include <ndn-cxx/interest.hpp>

#include <string>

namespace ndn {
namespace ndns {
namespace label {


/**
 * @brief NDNS iterative query type
 */

const name::Component NDNS_ITERATIVE_QUERY("NDNS");

/**
 * @brief NDNS recursive query type
 */
// it is not supported now
// const name::Component NDNS_RECURSIVE_QUERY("NDNS-R");

/////////////////////////////////////////////

/**
 * @brief label of update message, located at the last component in Interest name
 */
const name::Component NDNS_UPDATE_LABEL("UPDATE");


//////////////////////////////////////////////

/**
 * @brief NS resource record type
 */
const name::Component NS_RR_TYPE("NS");

/**
 * @brief ndns related certificate resource record type
 */
const name::Component CERT_RR_TYPE("CERT");

/**
 * @brief application stored certificate resource record type
 */
const name::Component APPCERT_RR_TYPE("APPCERT");

/**
 * @brief TXT resource record type
 */
const name::Component TXT_RR_TYPE("TXT");


//////////////////////////////////////////

/**
 * @brief result of Matching. version only works when matching a Interest Name
 */
struct MatchResult
{
  Name rrLabel;
  name::Component rrType;
  name::Component version;
};

/**
 * @brief match the Interest (NDNS query, NDNS update) name
 *
 * @param[in] interest Interest to parse
 * @param[in] zone Zone that the Interest is related to
 *            (only the length will be taken into account)
 * @param[out] result The matching result
 * @return true if match succeeds, or false
 */
bool
matchName(const Interest& interest,
          const Name& zone,
          MatchResult& result);

/**
 * @brief match the Data (NDNS query response, NDNS update response) name
 *
 * @param[in] data Data to parse
 * @param[in] zone Zone that the Data is related to
 *            (only the length will be taken into account)
 * @param[out] result The matching result
 * @return true if match succeeds, or false
 */
bool
matchName(const Data& data,
          const Name& zone,
          MatchResult& result);


} // namespace label
} // namespace ndns
} // namespace ndn

#endif // NDNS_NDNS_LABEL_HPP
