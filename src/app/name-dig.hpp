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
#ifndef NAME_DIG_HPP_
#define NAME_DIG_HPP_

#include "ndn-app.hpp"
#include "rr.hpp"
#include "response.hpp"
#include "query.hpp"

namespace ndn {
namespace ndns {

class NameDig: public NDNApp {
public:
  NameDig(const char *programName, const char *prefix);
  virtual ~NameDig();

  void
  onData(const ndn::Interest& interest, Data& data);


  void
  onTimeout(const ndn::Interest& interest);

  void
  sendQuery();

  void
  run();

  const vector<RR>& getRrs() const {
    return m_rrs;
  }

  void setRrs(const vector<RR>& rrs) {
    m_rrs = rrs;
  }

  const Name& getResolverName() const {
    return m_resolverName;
  }

  void setResolverName(const Name& resolverName) {
    m_resolverName = resolverName;
  }



  const Name& getDstLabel() const {
    return m_dstLabel;
  }

  void setDstLabel(const Name& dstLabel) {
    m_dstLabel = dstLabel;
  }

  RR::RRType getRrType() const {
    return m_rrType;
  }

  void setRrType(RR::RRType rrType) {
    m_rrType = rrType;
  }

private:
  Name m_resolverName;
  Name m_dstLabel;
  RR::RRType m_rrType;
  vector<RR> m_rrs;
};

} /* namespace ndns */
} /* namespace ndn */

#endif /* NAME_DIG_HPP_ */
