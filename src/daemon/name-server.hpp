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

#ifndef NDNS_DAEMON_NAME_SERVER_HPP
#define NDNS_DAEMON_NAME_SERVER_HPP

#include "zone.hpp"
#include "rrset.hpp"
#include "db-mgr.hpp"
#include "ndns-label.hpp"
#include "ndns-tlv.hpp"
#include "validator.hpp"
#include "common.hpp"

#include <ndn-cxx/security/key-chain.hpp>
#include <ndn-cxx/face.hpp>

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/noncopyable.hpp>

#include <sstream>
#include <stdexcept>

namespace ndn {
namespace ndns {

/**
 * @brief provides authoritative name server.
 *
 * The authoritative name server handles NDNS query and update.
 */
class NameServer : noncopyable
{
  DEFINE_ERROR(Error, std::runtime_error);

public:
  explicit
  NameServer(const Name& zoneName, const Name& certName, Face& face, DbMgr& dbMgr,
             KeyChain& keyChain, Validator& validator);

NDNS_PUBLIC_WITH_TESTS_ELSE_PRIVATE:
  void
  onInterest(const Name& prefix, const Interest &interest);

  /**
   * @brief handle NDNS query message
   */
  void
  handleQuery(const Name& prefix, const Interest& interest, const label::MatchResult& re);

  /**
   * @brief handle NDNS update message
   */
  void
  handleUpdate(const Name& prefix, const Interest& interest, const label::MatchResult& re);

  void
  onRegisterFailed(const ndn::Name& prefix, const std::string& reason);

  void
  doUpdate(const shared_ptr<const Interest>& interest, const shared_ptr<const Data>& data);

public:
  const Name&
  getNdnsPrefix()
  {
    return m_ndnsPrefix;
  }

  const Name&
  getKeyPrefix() const
  {
    return m_keyPrefix;
  }

  void
  setKeyPrefix(const Name& keyPrefix)
  {
    m_keyPrefix = keyPrefix;
  }

  const Zone&
  getZone() const
  {
    return m_zone;
  }

  const time::milliseconds&
  getContentFreshness() const
  {
    return m_contentFreshness;
  }

  /**
   * @brief set the Data fresh period
   */
  void
  setContentFreshness(const time::milliseconds& contentFreshness)
  {
    BOOST_ASSERT(contentFreshness > time::milliseconds::zero());
    m_contentFreshness = contentFreshness;
  }

private:
  Zone m_zone;
  DbMgr& m_dbMgr;

  // Name m_hint;

  Name m_ndnsPrefix;
  Name m_keyPrefix;
  Name m_certName;

  time::milliseconds m_contentFreshness;

  Face& m_face;
  KeyChain& m_keyChain;
  Validator& m_validator;
};

} // namespace ndns
} // namespace ndn

#endif // NDNS_DAEMON_NAME_SERVER_HPP
