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

#ifndef NDNS_DAEMON_ZONE_HPP
#define NDNS_DAEMON_ZONE_HPP

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
  Zone(const Name& name, const time::seconds& ttl = time::seconds(3600));

  /**
   * @brief get name of the zone
   */
  const Name&
  getName() const
  {
    return m_name;
  }

  /**
   * @brief set name of the zone
   */
  void
  setName(const Name& name)
  {
    m_name = name;
  }

  /**
   * @brief get the id when the rr is stored in the database
   *  default value is 0, the database has to guarantee that id is greater than 0.
   */
  uint64_t
  getId() const
  {
    return m_id;
  }

  /**
   * @brief set the id when the rr is stored in the database
   *  default value is 0, the database has to guarantee that id is greater than 0.
   */
  void
  setId(uint64_t id)
  {
    m_id = id;
  }

  /**
   * @brief get default ttl of resource record delegated in this zone, measured by seconds.
   * default 3600 (seconds)
   */
  const time::seconds&
  getTtl() const
  {
    return m_ttl;
  }

  /**
   * @brief set default ttl of resource record delegated in this zone, measured by seconds.
   * default 3600 (seconds)
   */
  void
  setTtl(const time::seconds& ttl)
  {
    m_ttl = ttl;
  }

  /**
   * @brief two zones are equal if they have the same name. Zone's name is unique
   */
  bool
  operator==(const Zone& other) const
  {
    return (m_name == other.getName());
  }

  bool
  operator!=(const Zone& other) const
  {
    return (m_name != other.getName());
  }

private:
  uint64_t m_id;

  Name m_name;

  time::seconds m_ttl;
};

std::ostream&
operator<<(std::ostream& os, const Zone& zone);

} // namespace ndns
} // namespace ndn

#endif // NDNS_DAEMON_ZONE_HPP
