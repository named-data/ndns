/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014-2016, Regents of the University of California.
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

#include <string>
#include <iomanip>

#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/lexical_cast.hpp>

#include <ndn-cxx/util/regex.hpp>
#include <ndn-cxx/util/indented-stream.hpp>
#include <ndn-cxx/encoding/oid.hpp>
#include <ndn-cxx/security/cryptopp.hpp>

namespace ndn {
namespace ndns {

NDNS_LOG_INIT("ManagementTool")

ManagementTool::ManagementTool(const std::string& dbFile, KeyChain& keyChain)
  : m_keyChain(keyChain)
  , m_dbMgr(dbFile)
{
}

void
ManagementTool::createZone(const Name &zoneName,
                           const Name& parentZoneName,
                           const time::seconds& cacheTtl,
                           const time::seconds& certValidity,
                           const Name& kskCertName,
                           const Name& dskCertName)
{
  bool isRoot = zoneName == ROOT_ZONE;

  //check preconditions
  Zone zone(zoneName, cacheTtl);
  if (m_dbMgr.find(zone)) {
    throw Error(zoneName.toUri() + " is already presented in the NDNS db");
  }

  if (!isRoot && parentZoneName.equals(zoneName)) {
    throw Error("Parent zone name can not be the zone itself");
  }

  if (!isRoot && !parentZoneName.isPrefixOf(zoneName)) {
    throw Error(parentZoneName.toUri() + " is not a prefix of " + zoneName.toUri());
  }

  // if dsk is provided, there is no need to check ksk
  if (dskCertName != DEFAULT_CERT) {
    if (!matchCertificate(dskCertName, zoneName)) {
      throw Error("Cannot verify DSK certificate");
    }
  }
  else if (kskCertName != DEFAULT_CERT) {
    if (!matchCertificate(kskCertName, zoneName)) {
      throw Error("Cannot verify KSK certificate");
    }
  }

  if (kskCertName == DEFAULT_CERT && isRoot) {
    throw Error("Cannot generate KSK for root zone");
  }

  //first generate KSK and DSK to the keyChain system, and add DSK as default
  NDNS_LOG_INFO("Start generating KSK and DSK and their corresponding certificates");
  Name dskName;
  shared_ptr<IdentityCertificate> dskCert;
  if (dskCertName == DEFAULT_CERT) {
    // if no dsk provided, then generate a dsk either signed by ksk auto generated or user provided
    time::system_clock::TimePoint notBefore = time::system_clock::now();
    time::system_clock::TimePoint notAfter = notBefore + certValidity;
    shared_ptr<IdentityCertificate> kskCert;

    if (kskCertName == DEFAULT_CERT) {
      //create KSK's certificate
      Name kskName = m_keyChain.generateRsaKeyPair(zoneName, true);
      std::vector<CertificateSubjectDescription> kskDesc;
      kskCert = m_keyChain.prepareUnsignedIdentityCertificate(kskName, zoneName, notBefore,
                                                              notAfter, kskDesc, parentZoneName);
      kskCert->setFreshnessPeriod(cacheTtl);

      m_keyChain.selfSign(*kskCert);
      m_keyChain.addCertificate(*kskCert);
      NDNS_LOG_INFO("Generated KSK: " << kskCert->getName());
    }
    else {
      kskCert = m_keyChain.getCertificate(kskCertName);
    }

    dskName = m_keyChain.generateRsaKeyPairAsDefault(zoneName, false);
    //create DSK's certificate
    std::vector<CertificateSubjectDescription> dskDesc;
    dskCert = m_keyChain.prepareUnsignedIdentityCertificate(dskName, zoneName, notBefore, notAfter,
                                                            dskDesc, zoneName);
    dskCert->setFreshnessPeriod(cacheTtl);
    m_keyChain.sign(*dskCert, kskCert->getName());
    m_keyChain.addCertificateAsKeyDefault(*dskCert);
    NDNS_LOG_INFO("Generated DSK: " << dskCert->getName());
  }
  else {
    dskCert = m_keyChain.getCertificate(dskCertName);
    dskName = dskCert->getPublicKeyName();
    m_keyChain.setDefaultKeyNameForIdentity(dskName);
    m_keyChain.setDefaultCertificateNameForKey(dskCert->getName());
  }

  //second add zone to the database
  NDNS_LOG_INFO("Start adding new zone to data base");
  addZone(zone);

  //third create ID-cert
  NDNS_LOG_INFO("Start creating DSK's ID-CERT");
  addIdCert(zone, dskCert, cacheTtl);
}

void
ManagementTool::deleteZone(const Name& zoneName)
{
  //check pre-conditions
  Zone zone(zoneName);
  if (!m_dbMgr.find(zone)) {
    throw Error(zoneName.toUri() + " is not presented in the NDNS db");
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
  //search for the certificate, start from KeyChain then local NDNS database
  shared_ptr<IdentityCertificate> cert;
  if (m_keyChain.doesCertificateExist(certName)) {
    cert = m_keyChain.getCertificate(certName);
  }
  else {
    shared_ptr<Regex> regex = make_shared<Regex>("(<>*)<KEY>(<>+)<ID-CERT><>");
    if (regex->match(certName) != true) {
      throw Error("Certificate name is illegal");
    }
    Name zoneName = regex->expand("\\1");
    Name label = regex->expand("\\2");

    Zone zone(zoneName);
    Rrset rrset(&zone);
    rrset.setLabel(label);
    rrset.setType(label::CERT_RR_TYPE);
    if (m_dbMgr.find(rrset)) {
      Data data(rrset.getData());
      cert = make_shared<IdentityCertificate>(data);
    }
    else {
      throw Error("Cannot find the cert: " + certName.toUri());
    }
  }

  if (outFile == DEFAULT_IO) {
    ndn::io::save(*cert, std::cout);
  }
  else {
    ndn::io::save(*cert, outFile);
    NDNS_LOG_INFO("save cert to file: " << outFile);
  }
}

void
ManagementTool::addRrSet(const Name& zoneName,
                         const Name& label,
                         const name::Component& type,
                         NdnsType ndnsType,
                         const uint64_t version,
                         const std::vector<std::string>& contents,
                         const Name& inputDskCertName,
                         const time::seconds& ttl)
{
  // check pre-condition
  Zone zone(zoneName);
  if (!m_dbMgr.find(zone)) {
    throw Error(zoneName.toUri() + " is not presented in the NDNS db");
  }

  if (ndnsType == NDNS_UNKNOWN) {
    throw Error("The ndns type is unknown");
  }

  if (type == label::CERT_RR_TYPE) {
    throw Error("It cannot handle ID-CERT rrset type");
  }

  // check strange rr type and ndns type combination
  if (type == label::NS_RR_TYPE && ndnsType == NDNS_RAW) {
    throw Error("NS cannot be of the type NDNS_RAW");
  }

  if (type == label::TXT_RR_TYPE && ndnsType != NDNS_RESP) {
    throw Error("TXT cannot be of the type NDNS_RAW or NDNS_AUTH");
  }

  if (ndnsType == NDNS_RAW && contents.size() != 1) {
    throw Error("NDNS_RAW must contain a single content element");
  }

  Name dskName;
  Name dskCertName = inputDskCertName;
  if (dskCertName == DEFAULT_CERT) {
    dskName = m_keyChain.getDefaultKeyNameForIdentity(zoneName);
    dskCertName = m_keyChain.getDefaultCertificateNameForKey(dskName);
  }
  else {
    if (!matchCertificate(dskCertName, zoneName)) {
      throw Error("Cannot verify certificate");
    }
  }

  time::seconds actualTtl = ttl;
  if (ttl == DEFAULT_RR_TTL)
    actualTtl = zone.getTtl();

  // set rrset
  Rrset rrset(&zone);
  rrset.setLabel(label);
  rrset.setType(type);
  rrset.setTtl(actualTtl);

  // set response
  Response re;
  re.setZone(zoneName);
  re.setQueryType(label::NDNS_ITERATIVE_QUERY);
  re.setRrLabel(label);
  re.setRrType(type);
  re.setNdnsType(ndnsType);
  re.setFreshnessPeriod(actualTtl);

  //set content according to ndns type
  if (ndnsType == NDNS_RAW) {
    Block tmp = makeBinaryBlock(ndn::tlv::Content, contents[0].c_str(), contents[0].length());
    re.setAppContent(tmp);
  }
  else if (ndnsType != NDNS_AUTH) {
    if (contents.empty()) {
      re.addRr("");
    }
    else {
      for (const auto& item : contents) {
        re.addRr(item);
      }
    }
  }

  if (version != VERSION_USE_UNIX_TIMESTAMP) {
    name::Component tmp = name::Component::fromVersion(version);
    re.setVersion(tmp);
  }
  shared_ptr<Data> data = re.toData();
  m_keyChain.sign(*data, dskCertName);

  rrset.setVersion(re.getVersion());
  rrset.setData(data->wireEncode());

  checkRrsetVersion(rrset);
  NDNS_LOG_INFO("Added " << rrset);
  m_dbMgr.insert(rrset);
}

void
ManagementTool::addRrSet(const Name& zoneName,
                         const std::string& inFile,
                         const time::seconds& ttl,
                         const Name& inputDskCertName,
                         const ndn::io::IoEncoding encoding)
{
  //check precondition
  Zone zone(zoneName);
  if (!m_dbMgr.find(zone)) {
    throw Error(zoneName.toUri() + " is not presented in the NDNS db");
  }

  Name dskName;
  Name dskCertName = inputDskCertName;
  if (dskCertName == DEFAULT_CERT) {
    dskName = m_keyChain.getDefaultKeyNameForIdentity(zoneName);
    dskCertName = m_keyChain.getDefaultCertificateNameForKey(dskName);
  }
  else {
    if (!matchCertificate(dskCertName, zoneName)) {
      throw Error("Cannot verify certificate");
    }
  }

  if (inFile != DEFAULT_IO) {
    boost::filesystem::path dir = boost::filesystem::path(inFile);
    if (!boost::filesystem::exists(dir) || boost::filesystem::is_directory(dir)) {
      throw Error("Data: " + inFile + " does not exist");
    }
  }

  //first load the data
  shared_ptr<Data> data;
  if (inFile == DEFAULT_IO)
    data = ndn::io::load<ndn::Data>(std::cin, encoding);
  else
    data = ndn::io::load<ndn::Data>(inFile, encoding);

  if (data == nullptr) {
    throw Error("input does not contain a valid Data packet");
  }

  // determine whether the data is a self-signed certificate
  shared_ptr<Regex> regex1 = make_shared<Regex>("(<>*)<KEY>(<>+)<ID-CERT><>");
  if (regex1->match(data->getName())) {
    IdentityCertificate scert(*data);
    Name keyName = scert.getPublicKeyName();
    if (keyName.getPrefix(zoneName.size()) != zoneName) {
      throw Error("the input key does not belong to the zone");
    }

    Name keyLocator = scert.getSignature().getKeyLocator().getName();

    // if it is, extract the content and name from the data, and resign it using the dsk.
    shared_ptr<Regex> regex2 = make_shared<Regex>("(<>*)<KEY>(<>+)<ID-CERT>");
    BOOST_VERIFY(regex2->match(keyLocator) == true);
    if (keyName == regex2->expand("\\1\\2")) {

      Name canonicalName;
      canonicalName
        .append(zoneName)
        .append("KEY")
        .append(keyName.getSubName(zoneName.size(), keyName.size() - zoneName.size()))
        .append("ID-CERT")
        .append(data->getName().get(-1));

      if (data->getName() != canonicalName) {
        // name need to be adjusted
        auto newData = make_shared<Data>();
        newData->setName(canonicalName);
        newData->setMetaInfo(data->getMetaInfo());
        newData->setContent(data->getContent());
        m_keyChain.sign(*newData);

        data = newData;
      }
    }
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
  NDNS_LOG_INFO("Added " << rrset);
  m_dbMgr.insert(rrset);
}

void
ManagementTool::listZone(const Name& zoneName, std::ostream& os, const bool printRaw)
{
  Zone zone(zoneName);
  if (!m_dbMgr.find(zone)) {
    throw Error("Zone " + zoneName.toUri() + " is not found in the database");
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
    int iteration = re.getNdnsType() == NDNS_RAW || re.getNdnsType() == NDNS_AUTH ?
                      1 : re.getRrs().size();
    const std::vector<Block> &rrs = re.getRrs();

    if (re.getNdnsType() != NDNS_RAW && re.getNdnsType() != NDNS_AUTH) {
      os << "; rrset=" << rrset.getLabel().toUri()
         << " type=" << rrset.getType().toUri()
         << " version=" << rrset.getVersion().toUri()
         << " signed-by=" << data.getSignature().getKeyLocator().getName().toUri()
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

      if (re.getNdnsType() != NDNS_RAW && re.getNdnsType() != NDNS_AUTH) {
        using namespace CryptoPP;
        if (rrset.getType() == label::TXT_RR_TYPE) {
          os.write(reinterpret_cast<const char*>(rrs[i].value()), rrs[i].value_size());
          os << std::endl;
        }
        else if (rrset.getType() == label::NS_RR_TYPE) {
          //TODO output the NS data once we have it
          os << std::endl;
        }
        else {
          StringSource ss(rrs[i].wire(), rrs[i].size(), true,
                          new Base64Encoder(new FileSink(os), true, 64));
        }
      }
    }

    if (re.getNdnsType() == NDNS_RAW || re.getNdnsType() == NDNS_AUTH) {
      os.width();
      os << "; content-type=" << re.getNdnsType()
         << " version=" << rrset.getVersion().toUri()
         << " signed-by=" << data.getSignature().getKeyLocator().getName().toUri();
      os << std::endl;

      if (printRaw && re.getNdnsType() == NDNS_RAW) {
        util::IndentedStream istream(os, "; ");

        if (re.getRrType() == label::CERT_RR_TYPE) {
          shared_ptr<Data> data = re.toData();
          IdentityCertificate cert(*data);
          cert.printCertificate(istream);
        }
        else {
          using namespace CryptoPP;
          StringSource ss(re.getAppContent().wire(), re.getAppContent().size(), true,
                          new Base64Encoder(new FileSink(istream), true, 64));
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
ManagementTool::listAllZones(std::ostream& os) {
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

    os << "; default-ttl=" << zone.getTtl().count();
    os << " default-key=" << m_keyChain.getDefaultKeyNameForIdentity(zone.getName());
    os << " default-certificate="
       << m_keyChain.getDefaultCertificateNameForIdentity(zone.getName());
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

  using namespace CryptoPP;
  StringSource ss(rrset.getData().wire(), rrset.getData().size(), true,
                  new Base64Encoder(new FileSink(os), true, 64));
}

void
ManagementTool::addIdCert(Zone& zone, shared_ptr<IdentityCertificate> cert,
                          const time::seconds& ttl)
{
  Rrset rrset(&zone);
  size_t size = zone.getName().size();
  Name label = cert->getName().getSubName(size + 1, cert->getName().size() - size - 3);
  rrset.setLabel(label);
  rrset.setType(label::CERT_RR_TYPE);
  rrset.setTtl(ttl);
  rrset.setVersion(cert->getName().get(-1));
  rrset.setData(cert->wireEncode());

  if (m_dbMgr.find(rrset)) {
    throw Error("ID-CERT with label=" + label.toUri() +
                " is already presented in local NDNS databse");
  }
  NDNS_LOG_INFO("Add rrset with zone-id: " << zone.getId() << " label: " << label << " type: "
                << label::CERT_RR_TYPE);
  m_dbMgr.insert(rrset);
}

void
ManagementTool::addZone(Zone& zone)
{
  if (m_dbMgr.find(zone)) {
    throw Error("Zone with Name=" + zone.getName().toUri() +
                " is already presented in local NDNS databse");
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
  if (!m_keyChain.doesCertificateExist(certName)) {
    NDNS_LOG_WARN(certName.toUri() << " is not presented in KeyChain");
    return false;
  }

  //check its public key information
  shared_ptr<IdentityCertificate> cert = m_keyChain.getCertificate(certName);
  Name keyName = cert->getPublicKeyName();

  if (!identity.isPrefixOf(keyName) || identity.size()!=keyName.size()-1) {
    NDNS_LOG_WARN(keyName.toUri() << " is not a key of " << identity.toUri());
    return false;
  }

  if (!m_keyChain.doesKeyExistInTpm(keyName, KeyClass::PRIVATE)) {
    NDNS_LOG_WARN("Private key: " << keyName.toUri() << " is not present in KeyChain");
    return false;
  }

  return true;
}

void
ManagementTool::checkRrsetVersion(const Rrset& rrset)
{
  Rrset originalRrset(rrset);
  if (m_dbMgr.find(originalRrset)) {
    // update only if rrset has a newer version
    if (originalRrset.getVersion() == rrset.getVersion()) {
      throw Error("Duplicate: " + boost::lexical_cast<std::string>(originalRrset));
    }
    else if (originalRrset.getVersion() > rrset.getVersion()) {
      throw Error("Newer version exists: " + boost::lexical_cast<std::string>(originalRrset));
    }

    m_dbMgr.remove(originalRrset);
  }
}

} // namespace ndns
} // namespace ndn
