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

#ifndef NDNS_DAEMON_RRSET_HPP
#define NDNS_DAEMON_RRSET_HPP

#include "zone.hpp"

namespace ndn {
namespace ndns {

/**
 * @brief Resource Record Set (rrset) define the the entry's attributes in NDNS
 *        this class also reflects the table (rrset) schema in the database
 *
 * The class is copyable, since it may be assigned to another Rrset instance
 * when resolving Response or Query from database
 */
class Rrset
{
public:
  explicit
  Rrset(Zone* zone = nullptr);

  /**
   * @brief get the id
   *
   * Default value is 0, the database has to guarantee that id is greater than 0.
   */
  uint64_t
  getId() const
  {
    return m_id;
  }

  /**
   * @brief set the id
   *
   * Default value is 0, the database has to guarantee that id is greater than 0.
   */
  void
  setId(uint64_t id)
  {
    m_id = id;
  }

  /**
   * @brief get the zone where the record is stored
   */
  Zone*
  getZone() const
  {
    return m_zone;
  }

  /**
   * @brief set the zone where the record is stored
   */
  void
  setZone(Zone* zone)
  {
    m_zone = zone;
  }

  /**
   * @brief get the label of rrset
   */
  const Name&
  getLabel() const
  {
    return m_label;
  }

  /**
   * @brief set the label of rrset
   */
  void
  setLabel(const Name& label)
  {
    m_label = label;
  }

  /**
   * @brief get the type of this rrset
   */
  const name::Component&
  getType() const
  {
    return m_type;
  }

  /**
   * @brief set the type of this rrset
   */
  void
  setType(const name::Component& type)
  {
    m_type = type;
  }

  /**
   * @brief get version of the data
   */
  const name::Component&
  getVersion() const
  {
    return m_version;
  }

  /**
   * @brief set version of the rrset
   */
  void
  setVersion(const name::Component& version)
  {
    m_version = version;
  }

  /**
   * @brief get ttl of the rrset
   */
  const time::seconds&
  getTtl() const
  {
    return m_ttl;
  }

  /**
   * @brief set ttl of the rrset
   */
  void
  setTtl(const time::seconds& ttl)
  {
    m_ttl = ttl;
  }

  /**
   * @brief get wire formatted block of the Response
   */
  const Block&
  getData() const
  {
    return m_data;
  }

  /**
   * @brief set wire formatted block of the Response
   */
  void
  setData(const Block& data)
  {
    m_data = data;
  }

  /**
   * @brief compare two rrset instance
   *
   * Note that comparison ignores id, TTL, and Data when comparing RR sets
   */
  bool
  operator==(const Rrset& other) const;

  /**
   * @brief compare two rrset instance
   *
   * Note that comparison ignores id, TTL, and Data when comparing RR sets
   */
  bool
  operator!=(const Rrset& other) const
  {
    return !(*this == other);
  }

  bool
  operator<(const Rrset& other) const;

private:
  uint64_t m_id;
  Zone* m_zone;
  Name m_label;
  name::Component m_type;
  name::Component m_version;
  time::seconds m_ttl;
  Block m_data;
};

std::ostream&
operator<<(std::ostream& os, const Rrset& Rrset);

} // namespace ndns
} // namespace ndn

#endif // NDNS_DAEMON_RRSET_HPP
