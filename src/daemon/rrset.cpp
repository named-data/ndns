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

#include "rrset.hpp"

namespace ndn {
namespace ndns {

Rrset::Rrset(Zone* zone)
  : m_id(0)
  , m_zone(zone)
{
}

std::ostream&
operator<<(std::ostream& os, const Rrset& rrset)
{
  os << "Rrset: Id=" << rrset.getId();
  if (rrset.getZone() != nullptr)
    os << " Zone=(" << *rrset.getZone() << ")";

  os << " Label=" << rrset.getLabel()
     << " Type=" << rrset.getType()
     << " Version=" << rrset.getVersion();
  return os;
}

bool
Rrset::operator==(const Rrset& other) const
{
  if (getZone() == nullptr && other.getZone() != nullptr)
    return false;
  else if (getZone() != nullptr && other.getZone() == nullptr)
    return false;
  else if (getZone() == nullptr && other.getZone() == nullptr)
    return (getLabel() == other.getLabel() &&
            getType() == other.getType() && getVersion() == other.getVersion());
  else
    return (*getZone() == *other.getZone() && getLabel() == other.getLabel() &&
            getType() == other.getType() && getVersion() == other.getVersion());

}

bool
Rrset::operator<(const Rrset& other) const
{
  if (getZone() != other.getZone() ||
      (getZone() != nullptr && *getZone() != *other.getZone())) {
    BOOST_THROW_EXCEPTION(std::runtime_error("Cannot compare Rrset that belong to different zones"));
  }

  bool isLess = getLabel() < other.getLabel();
  if (!isLess && getLabel() == other.getLabel()) {
    isLess = getType() < other.getType();

    if (!isLess && getType() == other.getType()) {
      isLess = getVersion() < other.getVersion();
    }
  }
  return isLess;
}


} // namespace ndns
} // namespace ndn
