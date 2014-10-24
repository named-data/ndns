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

#ifndef NDNS_UTIL_UTIL_HPP
#define NDNS_UTIL_UTIL_HPP

namespace ndn {
namespace ndns {

NdnsType
toNdnsType(const std::string& str)
{
  if (str == "resp")
    return NDNS_RESP;
  else if (str == "nack")
    return NDNS_NACK;
  else if (str == "auth")
    return NDNS_AUTH;
  else if (str == "raw")
    return NDNS_RAW;
  else
    return NDNS_UNKNOWN;
}

} // namespace ndns
} // namespace ndn

#endif // NDNS_UTIL_UTIL_HPP
