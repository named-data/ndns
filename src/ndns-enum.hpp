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

#ifndef NDNS_NDNS_ENUM_HPP
#define NDNS_NDNS_ENUM_HPP

#include <ostream>

namespace ndn {
namespace ndns {

/**
 * @brief NdnsType defined in Response.NdnsMetaInfo.NdnsType
 */
enum NdnsType {
  NDNS_RAW = 0, ///< this is not a real type, just mean that MetaInfo does not contain NdnsType
  NDNS_RESP = 1, ///< response type means there are requested RR
  NDNS_NACK = 2, ///< no requested RR
  NDNS_AUTH = 3, ///< only has RR for detailed (longer) label

  NDNS_UNKNOWN = 255
};

std::ostream&
operator<<(std::ostream& os, const NdnsType ndnsType);

/**
 * @brief define Return code of Update's Response
 */
enum UpdateReturnCode {
  UPDATE_OK = 0, ///< Update succeeds
  UPDATE_FAILURE = 1 ///< Update fails
};

} // namespace ndns
} // namespace ndn

#endif // NDNS_NDNS_ENUM_HPP
