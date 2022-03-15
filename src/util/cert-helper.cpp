/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2022, Regents of the University of California.
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

#include "cert-helper.hpp"

namespace ndn {
namespace ndns {

security::Identity
CertHelper::getIdentity(const KeyChain& keyChain, const Name& identityName)
{
  return keyChain.getPib().getIdentity(identityName);
}

bool
CertHelper::doesIdentityExist(const KeyChain& keyChain, const Name& identityName)
{
  try {
    keyChain.getPib().getIdentity(identityName);
    return true;
  } catch (const std::exception&) {
    return false;
  }
}

security::Certificate
CertHelper::getCertificate(const KeyChain& keyChain,
                           const Name& identity,
                           const Name& certName)
{
  security::Identity id = keyChain.getPib().getIdentity(identity);
  for (const auto& key : id.getKeys()) {
    for (const auto& cert : key.getCertificates()) {
      if (cert.getName() == certName) {
        return cert;
      }
    }
  }
  NDN_THROW(std::runtime_error(certName.toUri() + " does not exist"));
}

Name
CertHelper::getIdentityNameFromCert(const Name& certName)
{
  static Name::Component keyComp("KEY");
  for (size_t i = 0; i < certName.size(); ++i) {
    if (certName.get(i) == keyComp) {
      return certName.getPrefix(i);
    }
  }
  NDN_THROW(std::runtime_error(certName.toUri() + " is not a legal cert name"));
}

security::Certificate
CertHelper::getCertificate(const KeyChain& keyChain,
                           const Name& certName)
{
  Name identityName = getIdentityNameFromCert(certName);
  return getCertificate(keyChain, identityName, certName);
}

const Name&
CertHelper::getDefaultKeyNameOfIdentity(const KeyChain& keyChain, const Name& identityName)
{
  return getIdentity(keyChain, identityName).getDefaultKey().getName();
}

const Name&
CertHelper::getDefaultCertificateNameOfIdentity(const KeyChain& keyChain, const Name& identityName)
{
  return getIdentity(keyChain, identityName).getDefaultKey()
                                            .getDefaultCertificate()
                                            .getName();
}

security::Certificate
CertHelper::createCertificate(KeyChain& keyChain,
                              const security::Key& key,
                              const security::Key& signingKey,
                              const std::string& issuer,
                              const time::seconds& certValidity)
{
  Name certificateName = key.getName();
  certificateName
    .append(issuer)
    .appendVersion();
  security::Certificate certificate;
  certificate.setName(certificateName);

  // set metainfo
  certificate.setContentType(ndn::tlv::ContentType_Key);
  certificate.setFreshnessPeriod(time::hours(1));

  // set content
  certificate.setContent(key.getPublicKey());

  // set signature-info
  // to overcome the round-up issue in ndn-cxx setPeriod (notBefore is round up to the the next whole second)
  // notBefore = now() - 1 second
  SignatureInfo info;
  info.setValidityPeriod(security::ValidityPeriod(time::system_clock::now() - time::seconds(1),
                                                  time::system_clock::now() + certValidity));

  keyChain.sign(certificate, signingByKey(signingKey).setSignatureInfo(info));
  return certificate;
}

} // namespace ndns
} // namespace ndn

