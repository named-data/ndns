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

#include "management-tool.hpp"
#include "logger.hpp"
#include "ndns-label.hpp"
#include "ndns-tlv.hpp"
#include "util/cert-helper.hpp"

#include <iomanip>
#include <iostream>

#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/lexical_cast.hpp>

#include <ndn-cxx/util/regex.hpp>
#include <ndn-cxx/util/indented-stream.hpp>
#include <ndn-cxx/util/io.hpp>
#include <ndn-cxx/link.hpp>
#include <ndn-cxx/security/signing-helpers.hpp>
#include <ndn-cxx/security/transform.hpp>

namespace ndn {
namespace ndns {

NDNS_LOG_INIT(ManagementTool);

using security::transform::base64Encode;
using security::transform::streamSink;
using security::transform::bufferSource;
using security::Certificate;

ManagementTool::ManagementTool(const std::string& dbFile, KeyChain& keyChain)
  : m_keyChain(keyChain)
  , m_dbMgr(dbFile)
{
}

Zone
ManagementTool::createZone(const Name& zoneName,
                           const Name& parentZoneName,
                           const time::seconds& cacheTtl,
                           const time::seconds& certValidity,
                           const Name& kskCertName,
                           const Name& dskCertName,
                           const Name& dkeyCertName)
{
  bool isRoot = zoneName == ROOT_ZONE;
  Name zoneIdentityName = Name(zoneName).append(label::NDNS_ITERATIVE_QUERY);

  //check preconditions
  Zone zone(zoneName, cacheTtl);
  if (m_dbMgr.find(zone)) {
    NDN_THROW(Error(zoneName.toUri() + " is already present in the NDNS db"));
  }

  if (!isRoot && parentZoneName.equals(zoneName)) {
    NDN_THROW(Error("Parent zone name can not be the zone itself"));
  }

  if (!isRoot && !parentZoneName.isPrefixOf(zoneName)) {
    NDN_THROW(Error(parentZoneName.toUri() + " is not a prefix of " + zoneName.toUri()));
  }

  // if dsk is provided, there is no need to check ksk
  if (dskCertName != DEFAULT_CERT) {
    if (!matchCertificate(dskCertName, zoneIdentityName)) {
      NDN_THROW(Error("Cannot verify DSK certificate"));
    }
  }
  else if (kskCertName != DEFAULT_CERT) {
    if (!matchCertificate(kskCertName, zoneIdentityName)) {
      NDN_THROW(Error("Cannot verify KSK certificate"));
    }
  }

  if (dkeyCertName == DEFAULT_CERT && isRoot) {
    NDN_THROW(Error("Cannot generate dkey for root zone"));
  }

  // Generate a parentZone's identity to generate a D-Key.
  // This D-key will be passed to parent zone and resigned.
  Name dkeyIdentityName;
  if (dkeyCertName == DEFAULT_CERT) {
    dkeyIdentityName = Name(parentZoneName).append(label::NDNS_ITERATIVE_QUERY)
      .append(zoneName.getSubName(parentZoneName.size()));
  }
  else {
    dkeyIdentityName = CertHelper::getIdentityNameFromCert(dkeyCertName);
  }
  NDNS_LOG_INFO("Generated D-Key's identityName: " + dkeyIdentityName.toUri());

  Name dskName;
  security::Key ksk;
  security::Key dsk;
  security::Key dkey;
  Certificate dskCert;
  Certificate kskCert;
  Certificate dkeyCert;
  auto zoneIdentity = m_keyChain.createIdentity(zoneIdentityName);
  auto dkeyIdentity = m_keyChain.createIdentity(dkeyIdentityName);

  if (dkeyCertName == DEFAULT_CERT) {
    dkey = m_keyChain.createKey(dkeyIdentity);
    m_keyChain.deleteCertificate(dkey, dkey.getDefaultCertificate().getName());

    dkeyCert = CertHelper::createCertificate(m_keyChain, dkey, dkey, label::CERT_RR_TYPE.toUri(), certValidity);
    dkeyCert.setFreshnessPeriod(cacheTtl);
    m_keyChain.addCertificate(dkey, dkeyCert);
    NDNS_LOG_INFO("Generated DKEY: " << dkeyCert.getName());

  }
  else {
    dkeyCert = CertHelper::getCertificate(m_keyChain, dkeyIdentityName, dkeyCertName);
    dkey = dkeyIdentity.getKey(dkeyCert.getKeyName());
  }

  if (kskCertName == DEFAULT_CERT) {
    ksk = m_keyChain.createKey(zoneIdentity);
    // delete automatically generated certificates,
    // because its issue is 'self' instead of CERT_RR_TYPE
    m_keyChain.deleteCertificate(ksk, ksk.getDefaultCertificate().getName());
    kskCert = CertHelper::createCertificate(m_keyChain, ksk, dkey, label::CERT_RR_TYPE.toUri(), certValidity);
    kskCert.setFreshnessPeriod(cacheTtl);
    m_keyChain.addCertificate(ksk, kskCert);
    NDNS_LOG_INFO("Generated KSK: " << kskCert.getName());
  }
  else {
    // ksk usually might not be the default key of a zone
    kskCert = CertHelper::getCertificate(m_keyChain, zoneIdentityName, kskCertName);
    ksk = zoneIdentity.getKey(kskCert.getKeyName());
  }

  if (dskCertName == DEFAULT_CERT) {
    // if no dsk provided, then generate a dsk either signed by ksk auto generated or user provided
    dsk = m_keyChain.createKey(zoneIdentity);
    m_keyChain.deleteCertificate(dsk, dsk.getDefaultCertificate().getName());
    dskCert = CertHelper::createCertificate(m_keyChain, dsk, ksk, label::CERT_RR_TYPE.toUri(), certValidity);
    dskCert.setFreshnessPeriod(cacheTtl);
    // dskCert will become the default certificate, since the default cert has been deleted.
    m_keyChain.addCertificate(dsk, dskCert);
    m_keyChain.setDefaultKey(zoneIdentity, dsk);
    NDNS_LOG_INFO("Generated DSK: " << dskCert.getName());
  }
  else {
    dskCert = CertHelper::getCertificate(m_keyChain, zoneIdentityName, dskCertName);
    dsk = zoneIdentity.getKey(dskCert.getKeyName());
    m_keyChain.setDefaultKey(zoneIdentity, dsk);
    m_keyChain.setDefaultCertificate(dsk, dskCert);
  }

  //second add zone to the database
  NDNS_LOG_INFO("Start adding new zone to data base");
  addZone(zone);

  //third create ID-cert
  NDNS_LOG_INFO("Start adding Certificates to NDNS database");
  addIdCert(zone, kskCert, cacheTtl, dskCert);
  addIdCert(zone, dskCert, cacheTtl, dskCert);

  NDNS_LOG_INFO("Start saving KSK and DSK's id to ZoneInfo");
  m_dbMgr.setZoneInfo(zone, "ksk", kskCert.wireEncode());
  m_dbMgr.setZoneInfo(zone, "dsk", dskCert.wireEncode());

  NDNS_LOG_INFO("Start saving DKEY certificate id to ZoneInfo");
  m_dbMgr.setZoneInfo(zone, "dkey", dkeyCert.wireEncode());

  generateDoe(zone);
  return zone;
}

void
ManagementTool::deleteZone(const Name& zoneName)
{
  //check pre-conditions
  Zone zone(zoneName);
  if (!m_dbMgr.find(zone)) {
    NDN_THROW(Error(zoneName.toUri() + " is not present in the NDNS db"));
  }

  //first remove all rrsets of this zone from local ndns database
  std::vector<Rrset> rrsets = m_dbMgr.findRrsets(zone);
  for (Rrset& rrset : rrsets) {
    m_dbMgr.remove(rrset);
  }

  //second remove zone from local ndns database
  removeZone(zone);
}

void
ManagementTool::exportCertificate(const Name& certName, const std::string& outFile)
{
  // only search in local NDNS database
  security::Certificate cert;
  shared_ptr<Regex> regex = make_shared<Regex>("(<>*)<NDNS>(<>+)<CERT><>");
  if (!regex->match(certName)) {
    NDN_THROW(Error("Certificate name is illegal"));
  }

  Name zoneName = regex->expand("\\1");
  Name identityName = Name(zoneName).append(label::NDNS_ITERATIVE_QUERY);
  Name label = regex->expand("\\2");

  Zone zone(zoneName);
  Rrset rrset(&zone);
  rrset.setLabel(label);
  rrset.setType(label::CERT_RR_TYPE);
  if (m_dbMgr.find(rrset)) {
    cert = security::Certificate(rrset.getData());
  }
  else {
    NDN_THROW(Error("Cannot find the cert: " + certName.toUri()));
  }

  if (outFile == DEFAULT_IO) {
    ndn::io::save(cert, std::cout);
  }
  else {
    ndn::io::save(cert, outFile);
    NDNS_LOG_INFO("save cert to file: " << outFile);
  }
}

void
ManagementTool::addMultiLevelLabelRrset(Rrset& rrset,
                                        RrsetFactory& zoneRrFactory,
                                        const time::seconds& authTtl)
{
  const Name& label = rrset.getLabel();

  // Check whether it is legal to insert the rrset
  for (size_t i = 1; i <= label.size() - 1; i++) {
    Name prefix = label.getPrefix(i);
    Rrset prefixNsRr(rrset.getZone());
    prefixNsRr.setLabel(prefix);
    prefixNsRr.setType(label::NS_RR_TYPE);
    if (m_dbMgr.find(prefixNsRr)) {
      Data data(prefixNsRr.getData());
      if (data.getContentType() == NDNS_LINK) {
        NDN_THROW(Error("Cannot override " + boost::lexical_cast<std::string>(prefixNsRr) + " (NDNS_LINK)"));
      }
    }
  }

  // check that it does not override existing AUTH
  if (rrset.getType() == label::NS_RR_TYPE) {
    Rrset rrsetCopy = rrset;
    if (m_dbMgr.find(rrsetCopy)) {
      if (Data(rrsetCopy.getData()).getContentType() == NDNS_AUTH) {
        NDN_THROW(Error("Cannot override " + boost::lexical_cast<std::string>(rrsetCopy) + " (NDNS_AUTH)"));
      }
    }
  }

  for (size_t i = 1; i <= label.size() - 1; i++) {
    Name prefix = label.getPrefix(i);
    Rrset prefixNsRr(rrset.getZone());
    prefixNsRr.setLabel(prefix);
    prefixNsRr.setType(label::NS_RR_TYPE);
    if (m_dbMgr.find(prefixNsRr)) {
      NDNS_LOG_INFO("NDNS_AUTH Rrset Label=" << prefix << " is already existed, insertion skipped");
      continue;
    }

    Rrset authRr = zoneRrFactory.generateAuthRrset(prefix,
                                                   VERSION_USE_UNIX_TIMESTAMP, authTtl);
    NDNS_LOG_INFO("Adding NDNS_AUTH " << authRr);
    m_dbMgr.insert(authRr);
  }

  checkRrsetVersion(rrset);
  NDNS_LOG_INFO("Adding " << rrset);
  m_dbMgr.insert(rrset);
  generateDoe(*rrset.getZone());
}

void
ManagementTool::addRrset(Rrset& rrset)
{
  // check that it does not override existing AUTH
  Rrset rrsetCopy = rrset;
  rrsetCopy.setType(label::NS_RR_TYPE);
  if (m_dbMgr.find(rrsetCopy)) {
    if (Data(rrsetCopy.getData()).getContentType() == NDNS_AUTH) {
      NDN_THROW(Error("Can not add this Rrset: it overrides a NDNS_AUTH record"));
    }
  }

  checkRrsetVersion(rrset);
  NDNS_LOG_INFO("Added " << rrset);
  m_dbMgr.insert(rrset);
  generateDoe(*rrset.getZone());
}

void
ManagementTool::addRrsetFromFile(const Name& zoneName,
                                 const std::string& inFile,
                                 const time::seconds& ttl,
                                 const Name& inputDskCertName,
                                 ndn::io::IoEncoding encoding,
                                 bool needResign)
{
  //check precondition
  Zone zone(zoneName);
  Name zoneIdentityName = Name(zoneName).append(label::NDNS_ITERATIVE_QUERY);
  if (!m_dbMgr.find(zone)) {
    NDN_THROW(Error(zoneName.toUri() + " is not present in the NDNS db"));
  }

  Name dskName;
  Name dskCertName = inputDskCertName;
  if (dskCertName == DEFAULT_CERT) {
    dskName = CertHelper::getDefaultKeyNameOfIdentity(m_keyChain, zoneIdentityName);
    dskCertName = CertHelper::getDefaultCertificateNameOfIdentity(m_keyChain, zoneIdentityName);
  }
  else {
    if (!matchCertificate(dskCertName, zoneIdentityName)) {
      NDN_THROW(Error("Cannot verify certificate"));
    }
  }

  if (inFile != DEFAULT_IO) {
    boost::filesystem::path dir = boost::filesystem::path(inFile);
    if (!boost::filesystem::exists(dir) || boost::filesystem::is_directory(dir)) {
      NDN_THROW(Error("Data: " + inFile + " does not exist"));
    }
  }

  // load data
  shared_ptr<Data> data;
  if (inFile == DEFAULT_IO)
    data = ndn::io::load<ndn::Data>(std::cin, encoding);
  else
    data = ndn::io::load<ndn::Data>(inFile, encoding);

  if (data == nullptr) {
    NDN_THROW(Error("input does not contain a valid Data packet"));
  }

  if (needResign) {
    // TODO validityPeriod should be able to be configured
    SignatureInfo info;
    info.setValidityPeriod(security::ValidityPeriod(time::system_clock::now(),
                                                    time::system_clock::now() + DEFAULT_CERT_TTL));
    m_keyChain.sign(*data, signingByCertificate(dskCertName).setSignatureInfo(info));
  }

  // create response for the input data
  Response re;
  re.fromData(zoneName, *data);
  Name label = re.getRrLabel();
  name::Component type = re.getRrType();

  Rrset rrset(&zone);
  rrset.setLabel(label);
  rrset.setType(type);
  if (ttl == DEFAULT_RR_TTL)
    rrset.setTtl(zone.getTtl());
  else
    rrset.setTtl(ttl);
  rrset.setVersion(re.getVersion());
  rrset.setData(data->wireEncode());

  checkRrsetVersion(rrset);
  NDNS_LOG_INFO("Adding rrset from file " << rrset);
  m_dbMgr.insert(rrset);
  generateDoe(*rrset.getZone());
}

security::Certificate
ManagementTool::getZoneDkey(Zone& zone)
{
  std::map<std::string, Block> zoneInfo = m_dbMgr.getZoneInfo(zone);
  return security::Certificate(zoneInfo["dkey"]);
}

void
ManagementTool::listZone(const Name& zoneName, std::ostream& os, bool printRaw)
{
  Zone zone(zoneName);
  if (!m_dbMgr.find(zone)) {
    NDN_THROW(Error("Zone " + zoneName.toUri() + " is not found in the database"));
  }

  //first output the zone name
  os << "; Zone " << zoneName.toUri() << std::endl << std::endl;

  //second output all rrsets
  std::vector<Rrset> rrsets = m_dbMgr.findRrsets(zone);

  //set width for different columns
  size_t labelWidth = 0;
  size_t ttlWidth = 0;
  size_t typeWidth = 0;
  for (Rrset& rrset : rrsets) {
    Data data(rrset.getData());
    Response re;
    re.fromData(zoneName, data);

    if (rrset.getLabel().toUri().size() > labelWidth)
      labelWidth = rrset.getLabel().toUri().size();

    std::stringstream seconds;
    seconds << rrset.getTtl().count();
    if (seconds.str().size() > ttlWidth)
      ttlWidth = seconds.str().size();

    if (rrset.getType().toUri().size() > typeWidth)
      typeWidth = rrset.getType().toUri().size();
  }

  //output
  for (Rrset& rrset : rrsets) {
    Data data(rrset.getData());
    Response re;
    re.fromData(zoneName, data);
    int iteration = re.getContentType() == NDNS_BLOB
                    || re.getContentType() == NDNS_KEY
                    || re.getContentType() == NDNS_AUTH ? 1 : re.getRrs().size();

    const auto& rrs = re.getRrs();

    if (re.getContentType() != NDNS_BLOB && re.getContentType() != NDNS_KEY) {
      os << "; rrset=" << rrset.getLabel().toUri()
         << " type=" << rrset.getType().toUri()
         << " version=" << rrset.getVersion().toUri()
         << " signed-by=" << data.getKeyLocator()->getName().toUri()
         << std::endl;
    }

    for (int i = 0; i < iteration; i++) {
      os.setf(os.left);
      os.width(labelWidth + 2);
      os << rrset.getLabel().toUri();

      os.width(ttlWidth + 2);
      os << rrset.getTtl().count();

      os.width(typeWidth + 2);
      os << rrset.getType().toUri();

      if (re.getContentType() != NDNS_BLOB && re.getContentType() != NDNS_KEY) {
        if (rrset.getType() == label::TXT_RR_TYPE) {
          os.write(reinterpret_cast<const char*>(rrs[i].value()), rrs[i].value_size());
          os << std::endl;
        }
        else if (rrset.getType() == label::NS_RR_TYPE) {
          BOOST_ASSERT(iteration == 1);
          if (re.getContentType() == NDNS_AUTH) {
            const std::string authStr = "NDNS-Auth";
            os << authStr;
          }
          else {
            Link link(rrset.getData());
            for (const auto& delegation : link.getDelegationList()) {
              os << delegation << ";";
            }
          }
          os << std::endl;
        }
        else {
          bufferSource(rrs[i]) >> base64Encode() >> streamSink(os);
        }
      }
    }

    if (re.getContentType() == NDNS_BLOB || re.getContentType() == NDNS_KEY) {
      os.width();
      os << "; content-type=" << re.getContentType()
         << " version=" << rrset.getVersion().toUri()
         << " signed-by=" << data.getKeyLocator()->getName().toUri();
      os << std::endl;

      if (printRaw && (re.getContentType() == NDNS_BLOB
                       || re.getContentType() == NDNS_KEY)) {
        ndn::util::IndentedStream istream(os, "; ");

        if (re.getRrType() == label::CERT_RR_TYPE) {
          security::Certificate cert(rrset.getData());
          os << cert;
          // cert.printCertificate(istream);
        }
        else {
          bufferSource(re.getAppContent()) >> base64Encode() >> streamSink(os);
        }
      }
      os << std::endl;
    }
    else {
      os << std::endl;
    }
  }
}

void
ManagementTool::listAllZones(std::ostream& os)
{
  std::vector<Zone> zones = m_dbMgr.listZones();

  size_t nameWidth = 0;
  for (const Zone& zone : zones) {
    if (zone.getName().toUri().size() > nameWidth)
      nameWidth = zone.getName().toUri().size();
  }

  for (const Zone& zone : zones) {
    os.setf(os.left);
    os.width(nameWidth + 2);
    os << zone.getName().toUri();
    Name zoneIdentity = Name(zone.getName()).append(label::NDNS_ITERATIVE_QUERY);

    os << "; default-ttl=" << zone.getTtl().count();
    os << " default-key=" << CertHelper::getDefaultKeyNameOfIdentity(m_keyChain, zoneIdentity);
    os << " default-certificate="
       << CertHelper::getDefaultCertificateNameOfIdentity(m_keyChain, zoneIdentity);
    os << std::endl;
  }
}

void
ManagementTool::removeRrSet(const Name& zoneName, const Name& label, const name::Component& type)
{
  Zone zone(zoneName);
  Rrset rrset(&zone);
  rrset.setLabel(label);
  rrset.setType(type);

  if (!m_dbMgr.find(rrset)) {
    return;
  }
  NDNS_LOG_INFO("Remove rrset with zone-id: " << zone.getId() << " label: " << label << " type: "
                << type);
  m_dbMgr.remove(rrset);
  generateDoe(zone);
}

void
ManagementTool::getRrSet(const Name& zoneName,
                         const Name& label,
                         const name::Component& type,
                         std::ostream& os)
{
  Zone zone(zoneName);
  Rrset rrset(&zone);
  rrset.setLabel(label);
  rrset.setType(type);

  if (!m_dbMgr.find(rrset)) {
    os << "No record is found" << std::endl;
    return;
  }

  bufferSource(rrset.getData()) >> base64Encode() >> streamSink(os);
}

void
ManagementTool::addIdCert(Zone& zone, const Certificate& cert,
                          const time::seconds& ttl,
                          const Certificate& dskCert)
{
  Rrset rrsetKey(&zone);
  size_t size = zone.getName().size();
  Name label = cert.getName().getSubName(size + 1, cert.getName().size() - size - 3);
  rrsetKey.setLabel(label);
  rrsetKey.setType(label::CERT_RR_TYPE);
  rrsetKey.setTtl(ttl);
  rrsetKey.setVersion(cert.getName().get(-1));
  rrsetKey.setData(cert.wireEncode());

  if (m_dbMgr.find(rrsetKey)) {
    NDN_THROW(Error("CERT with label=" + label.toUri() +
                    " is already present in local NDNS database"));
  }

  m_dbMgr.insert(rrsetKey);
  NDNS_LOG_INFO("Add rrset with zone-id: " << zone.getId() << " label: " << label << " type: "
                << label::CERT_RR_TYPE);
}

void
ManagementTool::addZone(Zone& zone)
{
  if (m_dbMgr.find(zone)) {
    NDN_THROW(Error("Zone with Name=" + zone.getName().toUri() +
                    " is already present in local NDNS database"));
  }
  NDNS_LOG_INFO("Add zone with Name: " << zone.getName().toUri());
  m_dbMgr.insert(zone);
}

void
ManagementTool::removeZone(Zone& zone)
{
  if (!m_dbMgr.find(zone)) {
    return;
  }
  NDNS_LOG_INFO("Remove zone with Name: " << zone.getName().toUri());
  m_dbMgr.remove(zone);
}

bool
ManagementTool::matchCertificate(const Name& certName, const Name& identity)
{
  auto id = m_keyChain.getPib().getIdentity(identity);
  for (const auto& key : id.getKeys()) {
    try {
      key.getCertificate(certName);
      return true;
    }
    catch (const std::exception&) {
    }
  }
  return false;
}

void
ManagementTool::checkRrsetVersion(const Rrset& rrset)
{
  Rrset originalRrset(rrset);
  if (m_dbMgr.find(originalRrset)) {
    // update only if rrset has a newer version
    if (originalRrset.getVersion() == rrset.getVersion()) {
      NDN_THROW(Error("Duplicate: " + boost::lexical_cast<std::string>(originalRrset)));
    }
    else if (originalRrset.getVersion() > rrset.getVersion()) {
      NDN_THROW(Error("Newer version exists: " + boost::lexical_cast<std::string>(originalRrset)));
    }

    m_dbMgr.remove(originalRrset);
  }
}

void
ManagementTool::generateDoe(Zone& zone)
{
  // check zone existence
  if (!m_dbMgr.find(zone)) {
    NDN_THROW(Error(zone.getName().toUri() + " is not present in the NDNS db"));
  }

  // remove all the Doe records
  m_dbMgr.removeRrsetsOfZoneByType(zone, label::DOE_RR_TYPE);

  // get the records out
  std::vector<Rrset> allRecords = m_dbMgr.findRrsets(zone);

  // sort them by DoE label name (same as in the database)
  std::sort(allRecords.begin(), allRecords.end());

  RrsetFactory factory(m_dbMgr.getDbFile(), zone.getName(), m_keyChain, DEFAULT_CERT);
  factory.checkZoneKey();

  for (size_t i = 0; i < allRecords.size() - 1; i++) {
    Name lowerLabel = Name(allRecords[i].getLabel()).append(allRecords[i].getType());
    Name upperLabel = Name(allRecords[i + 1].getLabel()).append(allRecords[i + 1].getType());
    Rrset doe = factory.generateDoeRrset(lowerLabel,
                                         VERSION_USE_UNIX_TIMESTAMP,
                                         DEFAULT_CACHE_TTL, lowerLabel, upperLabel);
    m_dbMgr.insert(doe);
  }

  Name lastLabel = Name(allRecords.back().getLabel()).append(allRecords.back().getType());
  Name firstLabel = Name(allRecords.front().getLabel()).append(allRecords.front().getType());
  Rrset lastRange = factory.generateDoeRrset(lastLabel,
                                             VERSION_USE_UNIX_TIMESTAMP,
                                             DEFAULT_CACHE_TTL, lastLabel, firstLabel);
  m_dbMgr.insert(lastRange);

  // This guard will be the lowest label-ranked record
  // so if requested label+type is less than the lowest label except for this one, it will be choosed
  // by findLowerBound. This small trick avoids complicated SQL query in findLowerBound
  Rrset guardRange = factory.generateDoeRrset(Name(""),
                                              VERSION_USE_UNIX_TIMESTAMP,
                                              DEFAULT_CACHE_TTL, lastLabel, firstLabel);
  m_dbMgr.insert(guardRange);
  NDNS_LOG_INFO("DoE record updated");
}

} // namespace ndns
} // namespace ndn
