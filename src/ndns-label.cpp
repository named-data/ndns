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

#include "ndns-label.hpp"
#include <ndn-cxx/data.hpp>
#include <ndn-cxx/interest.hpp>

namespace ndn {
namespace ndns {
namespace label {

inline size_t
calculateSkip(const Name& name,
              const Name& hint, const Name& zone)
{
  size_t skip = 0;

  if (!hint.empty()) {
    // These are only asserts. The caller should supply the right parameters
    skip = hint.size() + 1 + zone.size();
    BOOST_ASSERT(name.size() > skip);

    BOOST_ASSERT(name.getPrefix(hint.size()) == hint);
    BOOST_ASSERT(name.get(hint.size()) == ForwardingHintLabel);
    BOOST_ASSERT(name.getSubName(hint.size() + 1, zone.size()) == zone);

  }
  else {
    skip = zone.size();
    BOOST_ASSERT(name.size() > skip);
    BOOST_ASSERT(name.getPrefix(zone.size()) == zone);
  }

  BOOST_ASSERT(name.get(skip) == NdnsIterativeQuery ||
               name.get(skip) == NdnsCertQuery);

  ++skip;
  return skip;
}

bool
matchName(const Interest& interest,
          const Name& hint, const Name& zone,
          MatchResult& result)
{
  // [hint / FHLabel] / zoneName / <Update>|rrLabel / UPDATE|rrType

  const Name& name = interest.getName();
  size_t skip = calculateSkip(name, hint, zone);

  if (name.size() - skip < 1)
    return false;

  result.rrType = name.get(-1);
  result.rrLabel = name.getSubName(skip, std::max<size_t>(0, name.size() - skip - 1));
  result.version = name::Component();

  return true;
}

bool
matchName(const Data& data,
          const Name& hint, const Name& zone,
          MatchResult& result)
{
  // [hint / FHLabel] / zoneName / <Update>|rrLabel / UPDATE|rrType

  const Name& name = data.getName();
  size_t skip = calculateSkip(name, hint, zone);

  if (name.size() - skip < 2)
    return false;

  result.version = name.get(-1);
  result.rrType = name.get(-2);
  result.rrLabel = name.getSubName(skip, std::max<size_t>(0, name.size() - skip - 2));

  return true;
}

} // namespace label
} // namespace ndns
} // namespace ndn
