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

#ifndef NDNS_UTIL_CERT_HELPER_HPP
#define NDNS_UTIL_CERT_HELPER_HPP

#include "common.hpp"
#include <ndn-cxx/security/key-chain.hpp>
#include <ndn-cxx/encoding/tlv.hpp>
#include <ndn-cxx/security/signing-helpers.hpp>

namespace ndn {
namespace ndns {

class CertHelper
{
public:
  static security::Identity
  getIdentity(const KeyChain& keyChain, const Name& identityName);

  static bool
  doesIdentityExist(const KeyChain& keyChain, const Name& identityName);

  static Name
  getIdentityNameFromCert(const Name& certName);

  static security::v2::Certificate
  getCertificate(const KeyChain& keyChain,
                 const Name& identity,
                 const Name& certName);

  static security::v2::Certificate
  getCertificate(const KeyChain& keyChain,
                 const Name& certName);

  static const Name&
  getDefaultKeyNameOfIdentity(const KeyChain& keyChain, const Name& identityName);

  static const Name&
  getDefaultCertificateNameOfIdentity(const KeyChain& keyChain, const Name& identityName);

  static security::v2::Certificate
  createCertificate(KeyChain& keyChain,
                    const security::Key& key,
                    const security::Key& signingKey,
                    const std::string& issuer,
                    const time::seconds& certValidity = time::days(10));
};


} // namespace ndns
} // namespace ndn

#endif // NDNS_UTIL_CERT_HELPER_HPP
