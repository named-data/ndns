/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2018, Regents of the University of California.
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

#include "ndns-enum.hpp"

namespace ndn {
namespace ndns {

std::ostream&
operator<<(std::ostream& os, const NdnsContentType ndnsType)
{
  switch (ndnsType) {
  case NDNS_BLOB:
    os << "BLOB";
    break;
  case NDNS_LINK:
    os << "LINK";
    break;
  case NDNS_NACK:
    os << "NACK";
    break;
  case NDNS_DOE:
    os << "DOE";
    break;
  case NDNS_KEY:
    os << "KEY";
    break;
  case NDNS_AUTH:
    os << "NDNS-Auth";
    break;
  case NDNS_RESP:
    os << "NDNS-Resp";
    break;
  default:
    os << "UNKNOWN";
    break;
  }
  return os;
}

} // namespace ndns
} // namespace ndn
