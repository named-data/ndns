/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2022, Regents of the University of California.
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

#ifndef NDNS_NDNS_ENUM_HPP
#define NDNS_NDNS_ENUM_HPP

#include <ostream>
#include <ndn-cxx/encoding/tlv.hpp>

namespace ndn {
namespace ndns {

/**
 * @brief ContentType of response
 */
enum NdnsContentType {
  NDNS_BLOB = ndn::tlv::ContentType_Blob,
  NDNS_LINK = ndn::tlv::ContentType_Link,
  NDNS_KEY  = ndn::tlv::ContentType_Key,
  NDNS_NACK = ndn::tlv::ContentType_Nack,
  NDNS_DOE  = 1085,
  NDNS_AUTH = 1086, ///< only has RR for detailed (longer) label
  NDNS_RESP = 1087, ///< response type means there are requested RR
  NDNS_UNKNOWN = 1088,  ///< this is not a real type, just mean that contentType is unknown
};

std::ostream&
operator<<(std::ostream& os, NdnsContentType contentType);

/**
 * @brief Return code of Update response
 */
enum UpdateReturnCode {
  UPDATE_OK = 0, ///< Update succeeds
  UPDATE_FAILURE = 1 ///< Update fails
};

} // namespace ndns
} // namespace ndn

#endif // NDNS_NDNS_ENUM_HPP
