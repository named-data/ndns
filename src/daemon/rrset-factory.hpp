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

#ifndef NDNS_DAEMON_RRSET_FACTORY_HPP
#define NDNS_DAEMON_RRSET_FACTORY_HPP

#include "common.hpp"
#include "rrset.hpp"
#include "logger.hpp"
#include "daemon/db-mgr.hpp"
#include "ndns-enum.hpp"

#include <ndn-cxx/link.hpp>
#include <ndn-cxx/security/key-chain.hpp>

#include <vector>
#include <string>
#include <boost/filesystem.hpp>

namespace ndn {
namespace ndns {

class RrsetFactory
{
public:
  /** @brief Represents an error might be thrown during runtime
   */
  class Error : public std::runtime_error
  {
  public:
    explicit
    Error(const std::string& what) : std::runtime_error(what)
    {
    }
  };

public:
  RrsetFactory(const boost::filesystem::path& dbFile,
               const Name& zoneName,
               KeyChain& keyChain,
               const Name& inputDskCertName);

  void
  checkZoneKey();

  Rrset
  generateNsRrset(const Name& label,
                  const name::Component& type,
                  const uint64_t version,
                  time::seconds ttl,
                  const ndn::DelegationList& delegations);

  Rrset
  generateTxtRrset(const Name& label,
                   const name::Component& type,
                   const uint64_t version,
                   time::seconds ttl,
                   const std::vector<std::string>& contents);

  Rrset
  generateAuthRrset(const Name& label,
                    const name::Component& type,
                    const uint64_t version,
                    time::seconds ttl);

  Rrset
  generateCertRrset(const Name& label,
                    const name::Component& type,
                    const uint64_t version,
                    time::seconds ttl,
                    const ndn::security::v2::Certificate& cert);

  static std::vector<std::string>
  wireDecodeTxt(const Block& wire);

NDNS_PUBLIC_WITH_TESTS_ELSE_PRIVATE:
  // only used for database-test-data unit test
  void
  onlyCheckZone();

private:
  std::pair<Rrset, Name>
  generateBaseRrset(const Name& label,
                    const name::Component& type,
                    const uint64_t version,
                    const time::seconds& ttl);

  bool
  matchCertificate(const Name& certName, const Name& identity);

  template<encoding::Tag TAG>
  inline size_t
  wireEncode(EncodingImpl<TAG>& block, const std::vector<Block>& rrs) const;

  const Block
  wireEncode(const std::vector<Block>& rrs) const;

  void
  sign(Data& data);

  void
  setContentType(Data& data, NdnsContentType contentType,
                 const time::seconds& ttl);

private:
  KeyChain& m_keyChain;
  std::string m_dbFile;
  Zone m_zone;
  Name m_dskCertName;
  Name m_dskName;
  bool m_checked;
};

} // namespace ndns
} // namespace ndn

#endif // NDNS_DAEMON_RRSET_FACTORY_HPP

