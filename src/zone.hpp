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

#ifndef NDNS_ZONE_HPP
#define NDNS_ZONE_HPP

#include <ndn-cxx/name.hpp>
#include <iostream>

namespace ndn {
namespace ndns {

/**
 * @brief DNS Zone abstraction, which delegates records
 * @see http://en.wikipedia.org/wiki/DNS_zone
 * @see http://irl.cs.ucla.edu/data/files/theses/afanasyev-thesis.pdf
 *
 * The class is copyable, since it may be assigned to another Zone instance
 *  when resolving Response or Query from database
 */
class Zone
{
public:
  Zone();

  /**
   * @brief create a Zone instance
   */
  explicit
  Zone(const Name& name);

  const Name&
  getName() const
  {
    return m_name;
  }

  void
  setName(const Name& name)
  {
    m_name = name;
  }

  uint32_t
  getId() const
  {
    return m_id;
  }

  void
  setId(uint32_t id)
  {
    m_id = id;
  }

  bool
  operator==(const Zone& other) const
  {
    return (m_name == other.getName());
  }

private:
  /**
   * @brief the id when the rr is stored in the database
   *  default value is 0, the database has to guarantee that id is greater than 0.
   */
  uint32_t m_id;

  /**
   * @brief the zone's name, which means all its
   *    delegated subzones or labels are under this namespace
   */
  Name m_name;
};

std::ostream&
operator<<(std::ostream& os, const Zone& zone);

} // namespace ndns
} // namespace ndn

#endif // NDNS_ZONE_HPP
