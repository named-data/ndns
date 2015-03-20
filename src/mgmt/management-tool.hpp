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

#ifndef NDNS_MGMT_MANAGEMENT_TOOL_HPP
#define NDNS_MGMT_MANAGEMENT_TOOL_HPP

#include "config.hpp"
#include "ndns-enum.hpp"
#include "./daemon/zone.hpp"
#include "./daemon/db-mgr.hpp"
#include "./daemon/rrset.hpp"
#include "./clients/response.hpp"

#include <stdexcept>
#include <ndn-cxx/common.hpp>
#include <ndn-cxx/security/identity-certificate.hpp>
#include <ndn-cxx/security/key-chain.hpp>

namespace ndn {
namespace ndns {

static const Name DEFAULT_CERT;
static const Name ROOT_ZONE;
static const time::seconds DEFAULT_CACHE_TTL = time::seconds(3600);
static const time::seconds DEFAULT_CERT_TTL = time::days(365);
static const std::vector<std::string> DEFAULT_CONTENTS;
static const std::string DEFAULT_IO = "-";
static const time::seconds DEFAULT_RR_TTL = time::seconds(0);
static constexpr uint64_t VERSION_USE_UNIX_TIMESTAMP = std::numeric_limits<uint64_t>::max();

/**
 * @brief provides management tools to the NDNS system, such as zone creation, zone delegation, DSK
 * generation and root zone creation.
 */
class ManagementTool : noncopyable
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

  /** @param certDir Path to the directory to store certificates
   *  @param dbFile Path to the local database
   */
  ManagementTool(const std::string& dbFile, KeyChain& keyChain);

  /** @brief Create a Zone according to a given name.
   *
   *  Specifically, It will generate a KSK and a DSK (and their certificates) to the following
   *  places:
   *  1) Local NDNS database: a new zone is added.
   *  2) Local NDNS database: an ID-CERT of the DSK is added.
   *  3) KeyChain: an identity named with zone name is added.
   *  4) KeyChain: a KSK and its self-signed certificate is added. The ownership of the KSK is the
   *  parent zone.
   *  5) KeyChain: a DSK and its KSK signed certificate is added.
   *
   *  -SS.cert (self-signed)
   *  -SKS.cert (self's Key signed)
   *  -PKS.cert (parent's Key Signed)
   *
   *  @attention
   *  1) to create root zone, supply zoneName and parentZoneName both with ROOT_ZONE
   *
   *  @param zoneName zone's name
   *  @param parentZoneName parent zone's name
   *  @param cacheTtl default TTL for RR sets in the zone
   *  @param certValidity validity for automatically created DSK certificate (@p dskCertName
   *                      should not be empty)
   *  @param kskCertName if given, a zone will be created with this ksk certificate and its key
   *  @param dskCertName if given, a zone will be created with this dsk certificate and its key
   */
  void
  createZone(const Name& zoneName,
             const Name& parentZoneName,
             const time::seconds& cacheTtl = DEFAULT_CACHE_TTL,
             const time::seconds& certValidity = DEFAULT_CERT_TTL,
             const Name& kskCertName = DEFAULT_CERT,
             const Name& dskCertName = DEFAULT_CERT);

  /** @brief Delete a Zone according to a given name.
   *
   *  Specifically, It will do the following things:
   *  1) KeyChain System: delete the Identity with zone name and all its keys/certificates
   *  2) Local NDNS database: delete the zone record
   *  3) Local NDNS database: delete the ID-CERT of the zone's DSK
   */
  void
  deleteZone(const Name& zoneName);

  /** @brief Export the certificate to file system
   *
   *  @param certName the name of the certificate to be exported
   *  @param output the path to output to-be exported file, including the file name
   */
  void
  exportCertificate(const Name& certName, const std::string& outFile = DEFAULT_IO);

  /** @brief add rrset to the NDNS local database
   *
   *  This one is only capable of adding NS type including NDNS_RESP and NDNS_AUTH and user defined
   *  type with string content.
   *  Other complicated situations can be handled by the other addRrSet() overload function.
   *
   *  @param zoneName the name of the zone to hold the rrset
   *  @param label the rrset label
   *  @param type the rrset type
   *  @param ndnsType the ndnsType of the response, for user-defined type, just set it NDNS_RAW
   *  @param version the version of the response and rrset, default is Unix Timestamp
   *  @param content the content of the response
   *  @param dskName the DSK to signed the response, default is the zone's DSK
   *  @param ttl the ttl of the rrset
    */
  void
  addRrSet(const Name& zoneName,
           const Name& label,
           const name::Component& type,
           NdnsType ndnsType,
           const uint64_t version = VERSION_USE_UNIX_TIMESTAMP,
           const std::vector<std::string>& contents = DEFAULT_CONTENTS,
           const Name& dskCertName = DEFAULT_CERT,
           const time::seconds& ttl = DEFAULT_RR_TTL);

  /** @brief add rrset to the NDNS local database
   *
   *  This overload is capable of adding any data to the rrset as long as the supplied data is
   *  valid.
   *  A special case is to add the ID-CERT of KSK to the parent zone. At this case, the SS cert
   *  should be supplied, and therefore it will use the parent zone's DSK to resign the certificate.
   *  For other cases, the data will be added directly without any modification.
   *
   *  @param zoneName the name of the zone to hold the rrset
   *  @param dataPath the path to the supplied data
   *  @param ttl the ttl of the rrset
   *  @param dskName the DSK to signed the special case, default is the zone's DSK
    */
  void
  addRrSet(const Name& zoneName,
           const std::string& inFile = DEFAULT_IO,
           const time::seconds& ttl = DEFAULT_RR_TTL,
           const Name& dskCertName = DEFAULT_CERT);

  /** @brief remove rrset from the NDNS local database
   *
   *  @param zonName the name of the zone holding the rrset
   *  @param label rrset's label
   *  @param type rrset's type
    */
  void
  removeRrSet(const Name& zoneName, const Name& label, const name::Component& type);

  /** @brief output the raw data of the selected rrset
   *
   *  @param zonName the name of the zone holding the rrset
   *  @param label rrset's label
   *  @param type rrset's type
   *  @param os the ostream to print information to
   *  @param isPP indicate pretty print
   */
  void
  getRrSet(const Name& zoneName,
           const Name& label,
           const name::Component& type,
           std::ostream& os);

  /** @brief generates an output like DNS zone file. Reference:
   *  http://en.wikipedia.org/wiki/Zone_file
   *
   *  @param zoneName the name of the zone to investigate
   *  @param os the ostream to print information to
   *  @param printRaw set to print content of ndns-raw rrset
   *  @throw Error if zoneName does not exist in the database
   */
  void
  listZone(const Name& zoneName, std::ostream& os, const bool printRaw = false);

  /** @brief lists all existing zones within this name server.
   *
   *  @param os the ostream to print information to
    */
  void
  listAllZones(std::ostream& os);

private:
  /** @brief add ID-CERT to the NDNS local database
    */
  void
  addIdCert(Zone& zone, shared_ptr<IdentityCertificate> cert, const time::seconds& ttl);

  /** @brief add zone to the NDNS local database
    */
  void
  addZone(Zone& zone);

  /** @brief remove zone from the NDNS local database
    */
  void
  removeZone(Zone& zone);

  /** @brief determine whether a certificate matches with both the identity and key type
   */
  bool
  matchCertificate(const Name& certName, const Name& identity);

  /** @brief determine whether an older version of the rrset exists
   */
  void
  checkRrsetVersion(const Rrset& rrset);

private:
  KeyChain& m_keyChain;
  DbMgr m_dbMgr;
};

} // namespace ndns
} // namespace ndn

#endif // NDNS_MGMT_MANAGEMENT_TOOL_HPP
