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

#include "query-controller.hpp"
#include "logger.hpp"
#include <iostream>

namespace ndn {
namespace ndns {

QueryController::QueryController(const Name& dstLabel, const name::Component& rrType,
                                 const time::milliseconds& interestLifetime,
                                 const QuerySucceedCallback& onSucceed, const QueryFailCallback& onFail,
                                 Face& face)
  : m_dstLabel(dstLabel)
  , m_rrType(rrType)
  , m_interestLifetime(interestLifetime)
  , m_onSucceed(onSucceed)
  , m_onFail(onFail)
  , m_face(face)
{
}

std::ostream&
operator<<(std::ostream& os, const QueryController& ctr)
{
  os << "QueryController: dstLabel=" << ctr.getDstLabel()
     << " rrType=" << ctr.getRrType()
    ;

  return os;
}
} // namespace ndns
} // namespace ndn
