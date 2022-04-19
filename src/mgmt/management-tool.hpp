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

#ifndef NDNS_MGMT_MANAGEMENT_TOOL_HPP
#define NDNS_MGMT_MANAGEMENT_TOOL_HPP

#include "ndns-enum.hpp"
#include "clients/response.hpp"
#include "daemon/db-mgr.hpp"
#include "daemon/rrset.hpp"
#include "daemon/rrset-factory.hpp"
#include "daemon/zone.hpp"

#include <ndn-cxx/security/key-chain.hpp>
#include <ndn-cxx/util/io.hpp>

#include <stdexcept>

namespace ndn {
namespace ndns {

inline const Name DEFAULT_CERT;
inline const Name ROOT_ZONE;
inline constexpr time::seconds DEFAULT_CACHE_TTL = time::seconds(3600);
inline constexpr time::seconds DEFAULT_CERT_TTL = time::days(365);
inline const std::vector<std::string> DEFAULT_CONTENTS;
inline const std::string DEFAULT_IO = "-";
inline constexpr time::seconds DEFAULT_RR_TTL = time::seconds(0);
inline constexpr uint64_t VERSION_USE_UNIX_TIMESTAMP = std::numeric_limits<uint64_t>::max();

/**
 * @brief provides management tools to the NDNS system, such as zone creation, zone delegation, DSK
 * generation and root zone creation.
 */
class ManagementTool : boost::noncopyable
{
public:
  /** @brief Represents an error might be thrown during runtime
   */
  class Error : public std::runtime_error
  {
  public:
    using std::runtime_error::runtime_error;
  };

  /**
   * @brief Create instance of the tool
   *
   * @param dbFile   Path to the local database
   * @param keyChain Keychain instance
   */
  ManagementTool(const std::string& dbFile, KeyChain& keyChain);

  /** @brief Create a Zone according to a given name.
   *
   *  Specifically, It will generate a KSK and a DSK (and their certificates) to the following
   *  places:
   *  1. Local NDNS database: a new zone is added.
   *  2. Local NDNS database: an CERT of the DSK is added.
   *  3. KeyChain: an identity named with zone name is added.
   *  4. KeyChain: a KSK and its self-signed certificate is added. The ownership of the KSK is the
   *  parent zone.
   *  5. KeyChain: a DSK and its KSK signed certificate is added.
   *
   *  - SS.cert (self-signed)
   *  - SKS.cert (self's Key signed)
   *  - PKS.cert (parent's Key Signed)
   *
   *  @note To create root zone, supply zoneName and parentZoneName both with ROOT_ZONE
   *
   *  @param zoneName zone's name
   *  @param parentZoneName parent zone's name
   *  @param cacheTtl default TTL for RR sets in the zone
   *  @param certValidity validity for automatically created DSK certificate (@p dskCertName
   *                      should not be empty)
   *  @param kskCertName if given, a zone will be created with this ksk certificate
   *  @param dskCertName if given, a zone will be created with this dsk certificate and provided
   *                     ksk certificate will be ignored
   *  @param dkeyCertName if given, ksk will be signed by this d-key.
   */
  Zone
  createZone(const Name& zoneName,
             const Name& parentZoneName,
             const time::seconds& cacheTtl = DEFAULT_CACHE_TTL,
             const time::seconds& certValidity = DEFAULT_CERT_TTL,
             const Name& kskCertName = DEFAULT_CERT,
             const Name& dskCertName = DEFAULT_CERT,
             const Name& dkeyCertName = DEFAULT_CERT);

  /** @brief Delete a Zone according to a given name.
   *
   *  Specifically, It will do the following things:
   *  1) KeyChain System: delete the Identity with zone name and all its keys/certificates
   *  2) Local NDNS database: delete the zone record
   *  3) Local NDNS database: delete the CERT of the zone's DSK
   */
  void
  deleteZone(const Name& zoneName);

  /** @brief Export the certificate to file system
   *
   *  @param certName the name of the certificate to be exported
   *  @param outFile the path to output to-be exported file, including the file name
   */
  void
  exportCertificate(const Name& certName, const std::string& outFile = DEFAULT_IO);

  /** @brief Add rrset to the NDNS local database from a file
   *
   *  The function Loads data from file and then adds it to the rrset without modification
   *  Loaded data is assummed to be valid
   *  Data will be resigned by zone's DSK, if needResign is true.
   *
   *  @param zoneName the name of the zone to hold the rrset
   *  @param inFile the path to the supplied data
   *  @param ttl the ttl of the rrset
   *  @param dskCertName the DSK to signed the special case, default is the zone's DSK
   *  @param encoding the encoding of the input file
   *  @param needResign whether data should be resigned by DSK
   */
  void
  addRrsetFromFile(const Name& zoneName,
                   const std::string& inFile = DEFAULT_IO,
                   const time::seconds& ttl = DEFAULT_RR_TTL,
                   const Name& dskCertName = DEFAULT_CERT,
                   ndn::io::IoEncoding encoding = ndn::io::BASE64,
                   bool needResign = false);

  /** @brief Add rrset to the NDNS local database
   *
   *  @param rrset rrset
   */
  void
  addRrset(Rrset& rrset);

  /** @brief Add rrset with multi-level label to the NDNS local database
   *
   *  The appropriate AUTH records will be created automatically if they do not yet exist. The
   *  existing records are kept intact.
   *
   *  @throw Error If one of the levels has been delegated to another zone. For example, if
   *               there is an NS record with label `/foo`, then inserting @p rrset having a
   *               multi-level label that use `/foo` as prefix will cause an error.
   *
   *  @throw Error If @p rrset will override an AUTH record.  For example, if there is already
   *               an AUTH record with label `/foo/bar`, then inserting NS-type @p rrset that
   *               has the the same label will cause an error.
   *
   *  For example, inserting a rrset with `/foo/bar/test` label and TXT type into zone `/zone/NDNS`
   *  will create:
   *  - `/zone/NDNS/foo/NS` (.ContentType AUTH)
   *  - `/zone/NDNS/foo/bar/NS` (.ContentType AUTH)
   *  - `/zone/NDNS/foo/bar/test/TXT` (.ContentType NDNS-Resp)
   *
   *  @param rrset rrset
   *  @param zoneRrFactory that is used for generate AUTH packet
   *  @param authTtl
   */
  void
  addMultiLevelLabelRrset(Rrset& rrset,
                          RrsetFactory& zoneRrFactory,
                          const time::seconds& authTtl);

  /** @brief remove rrset from the NDNS local database
   *
   *  @param zoneName the name of the zone holding the rrset
   *  @param label rrset's label
   *  @param type rrset's type
   */
  void
  removeRrSet(const Name& zoneName, const Name& label, const name::Component& type);

  /** @brief output the raw data of the selected rrset
   *
   *  @param zoneName the name of the zone holding the rrset
   *  @param label rrset's label
   *  @param type rrset's type
   *  @param os the ostream to print information to
   */
  void
  getRrSet(const Name& zoneName,
           const Name& label,
           const name::Component& type,
           std::ostream& os);

  security::Certificate
  getZoneDkey(Zone& zone);

  /** @brief generates an output like DNS zone file. Reference:
   *  http://en.wikipedia.org/wiki/Zone_file
   *
   *  @param zoneName the name of the zone to investigate
   *  @param os the ostream to print information to
   *  @param printRaw set to print content of ndns-raw rrset
   *  @throw Error if zoneName does not exist in the database
   */
  void
  listZone(const Name& zoneName, std::ostream& os, bool printRaw = false);

  /** @brief lists all existing zones within this name server.
   *
   *  @param os the ostream to print information to
   */
  void
  listAllZones(std::ostream& os);

private:
  /** @brief add CERT to the NDNS local database
   */
  void
  addIdCert(Zone& zone, const ndn::security::Certificate& cert,
            const time::seconds& ttl,
            const ndn::security::Certificate& dskCertName);

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

  /**
     @brief generate all Doe records
   */
  void generateDoe(Zone& zone);

private:
  KeyChain& m_keyChain;
  DbMgr m_dbMgr;
};

} // namespace ndns
} // namespace ndn

#endif // NDNS_MGMT_MANAGEMENT_TOOL_HPP
