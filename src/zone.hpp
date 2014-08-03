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

#include "rr.hpp"
#include "query.hpp"
#include "response.hpp"

namespace ndn {
namespace ndns {

class Zone
{

public:

  Zone(const Name& name);
  Zone();
  virtual ~Zone();

  const Name& getAuthorizedName() const
  {
    return m_authorizedName;
  }

  void setAuthorizedName(const Name& authorizedName)
  {
    m_authorizedName = authorizedName;
  }

  uint32_t getId() const
  {
    return m_id;
  }

  void setId(uint32_t id)
  {
    m_id = id;
  }

private:
  uint32_t m_id;
  Name m_authorizedName;
};
//class Zone

}// namespace ndns
} // namespace ndn

#endif // NDNS_ZONE_HPP
