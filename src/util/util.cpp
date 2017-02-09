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

#include "util.hpp"

#include <ndn-cxx/security/transform.hpp>

namespace ndn {
namespace ndns {

using security::transform::base64Encode;
using security::transform::streamSink;
using security::transform::bufferSource;

NdnsContentType
toNdnsContentType(const std::string& str)
{
  if (str == "resp")
    return NDNS_RESP;
  else if (str == "nack")
    return NDNS_NACK;
  else if (str == "auth")
    return NDNS_AUTH;
  else if (str == "blob")
    return NDNS_BLOB;
  else if (str == "link")
    return NDNS_LINK;
  else if (str == "key")
    return NDNS_LINK;
  else
    return NDNS_UNKNOWN;
}

void
output(const Data& data, std::ostream& os, const bool isPretty)
{
  const Block& block = data.wireEncode();
  if (!isPretty) {
    bufferSource(block.wire(), block.size()) >> base64Encode() >> streamSink(os);
  }
  else {
    os << "Name: " << data.getName().toUri() << std::endl;
    os << "KeyLocator: " << data.getSignature().getKeyLocator().getName().toUri() << std::endl;
    bufferSource(block.wire(), block.size()) >> base64Encode() >> streamSink(os);
    os << std::endl;
  }
}

} // namespace ndns
} // namespace ndn
