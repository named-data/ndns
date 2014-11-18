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

#include "management-tool.hpp"
#include "logger.hpp"
#include "ndns-label.hpp"
#include "ndns-tlv.hpp"

#include <string>
#include <iomanip>

#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/algorithm/string/replace.hpp>

#include <ndn-cxx/util/io.hpp>
#include <ndn-cxx/util/regex.hpp>
#include <ndn-cxx/encoding/oid.hpp>
#include <ndn-cxx/security/cryptopp.hpp>

namespace ndn {
namespace ndns {

NDNS_LOG_INIT("ManagementTool");

ManagementTool::ManagementTool(const std::string& dbFile)
  : m_dbMgr(dbFile)
{
}

void
ManagementTool::createZone(const Name &zoneName,
                           const Name& parentZoneName,
                           const time::seconds& cacheTtl,
                           const time::seconds& certTtl,
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

  if (kskCertName != DEFAULT_CERT) {
    if (!matchCertificate(kskCertName, zoneName)) {
      throw Error("Cannot verify KSK certificate");
    }
  }

  if (dskCertName != DEFAULT_CERT) {
    if (!matchCertificate(dskCertName, zoneName)) {
      throw Error("Cannot verify DSK certificate");
    }
  }

  if (kskCertName == DEFAULT_CERT && isRoot) {
    throw Error("Cannot generate KSK for root zone");
  }

  //first generate KSK and DSK to the keyChain system, and add DSK as default
  NDNS_LOG_INFO("Start generating KSK and DSK and their corresponding certificates");
  time::system_clock::TimePoint notBefore = time::system_clock::now();
  time::system_clock::TimePoint notAfter = notBefore + certTtl;
  shared_ptr<IdentityCertificate> kskCert;

  if (kskCertName == DEFAULT_CERT) {
    //create KSK's certificate
    Name kskName = m_keyChain.generateRsaKeyPair(zoneName, true);
    std::vector<CertificateSubjectDescription> kskDesc;
    kskCert = m_keyChain.prepareUnsignedIdentityCertificate(kskName, zoneName, notBefore, notAfter,
                                                            kskDesc);
    //prepare the correct name for the ksk certificate
    Name newScertName = parentZoneName;
    newScertName.append(label::NDNS_CERT_QUERY);
    newScertName.append(zoneName.getSubName(parentZoneName.size()));
    //remove the zone prefix and KEY
    newScertName.append(kskCert->getName().getSubName(zoneName.size()+1));
    kskCert->setName(newScertName);

    m_keyChain.selfSign(*kskCert);
    m_keyChain.addCertificate(*kskCert);
    NDNS_LOG_INFO("Generated KSK: " << kskCert->getName().toUri());
  }
  else {
    kskCert = m_keyChain.getCertificate(kskCertName);
  }

  Name dskName;
  shared_ptr<IdentityCertificate> dskCert;
  if (dskCertName == DEFAULT_CERT) {
    dskName = m_keyChain.generateRsaKeyPairAsDefault(zoneName, false);
    //create DSK's certificate
    std::vector<CertificateSubjectDescription> dskDesc;
    dskCert = m_keyChain.prepareUnsignedIdentityCertificate(dskName, zoneName, notBefore, notAfter,
                                                            dskDesc);
    m_keyChain.sign(*dskCert, kskCert->getName());
    m_keyChain.addCertificateAsKeyDefault(*dskCert);
    NDNS_LOG_INFO("Generated DSK: " << dskCert->getName().toUri());
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

  //third remove identity
  m_keyChain.deleteIdentity(zoneName);
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

  // set rrset
  Rrset rrset(&zone);
  rrset.setLabel(label);
  rrset.setType(type);
  if (ttl == DEFAULT_RR_TTL)
    rrset.setTtl(zone.getTtl());
  else
    rrset.setTtl(ttl);

  // set response
  Response re;
  re.setZone(zoneName);
  re.setQueryType(label::NDNS_ITERATIVE_QUERY);
  re.setRrLabel(label);
  re.setRrType(type);
  re.setNdnsType(ndnsType);

  //set content according to ndns type
  if (ndnsType == NDNS_RAW) {
    Block tmp = ndn::dataBlock(ndn::tlv::Content, contents[0].c_str(), contents[0].length());
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

  shared_ptr<Data> data = re.toData();
  if (version != VERSION_USE_UNIX_TIMESTAMP) {
    name::Component tmp = name::Component::fromVersion(version);
    re.setVersion(tmp);
  }
  m_keyChain.sign(*data, dskCertName);

  rrset.setVersion(re.getVersion());
  rrset.setData(data->wireEncode());

  if (m_dbMgr.find(rrset)) {
    throw Error("Rrset with label=" + label.toUri() + " is already in local NDNS databse");
  }
  NDNS_LOG_INFO("Add rrset with zone-id: " << zone.getId() << " label: " << label << " type: "
                << type);
  m_dbMgr.insert(rrset);
}

void
ManagementTool::addRrSet(const Name& zoneName,
                         const std::string& inFile,
                         const time::seconds& ttl,
                         const Name& inputDskCertName)
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
    data = ndn::io::load<ndn::Data>(std::cin);
  else
    data = ndn::io::load<ndn::Data>(inFile);

  //determine whether the data is a self-signed certificate
  shared_ptr<Regex> regex1 = make_shared<Regex>("(<>*)<KEY>(<>+)<ID-CERT><>");
  if (regex1->match(data->getName())) {
    IdentityCertificate scert(*data);
    Name keyName = scert.getPublicKeyName();
    Name keyLocator = scert.getSignature().getKeyLocator().getName();

    //if it is, extract the content and name from the data, and resign it using the dsk.
    shared_ptr<Regex> regex2 = make_shared<Regex>("(<>*)<KEY>(<>+)<ID-CERT>");
    BOOST_VERIFY(regex2->match(keyLocator) == true);
    if (keyName == regex2->expand("\\1\\2")) {
      shared_ptr<Data> pre = data;
      Name name = pre->getName();
      //check whether the name is legal or not. if not converting it to a legal name
      if (zoneName != regex1->expand("\\1")) {
        Name comp1 = regex1->expand("\\1").getSubName(zoneName.size());
        Name comp2 = regex1->expand("\\2");
        name = zoneName;
        name.append("KEY");
        name.append(comp1);
        name.append(comp2);
        name.append("ID-CERT");
        name.append(pre->getName().get(-1));
      }

      data = make_shared<Data>();
      data->setName(name);
      data->setContent(pre->getContent());

      m_keyChain.sign(*data, dskCertName);
    }
  }

  // create response for the input data
  Response re;
  Name hint;
  re.fromData(hint, zoneName, *data);
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

  if (m_dbMgr.find(rrset)) {
    throw Error("Rrset with label=" + label.toUri() + " is already in local NDNS databse");
  }
  NDNS_LOG_INFO("Add rrset with zone-id: " << zone.getId() << " label: " << label << " type: "
                << type);
  m_dbMgr.insert(rrset);
}

void
ManagementTool::listZone(const Name& zoneName, std::ostream& os, const bool printRaw) {
  Zone zone(zoneName);
  if (!m_dbMgr.find(zone)) {
    os << "No record is found" << std::endl;
    return;
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
    Name hint;
    re.fromData(hint, zoneName, data);

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
    Name hint;
    re.fromData(hint, zoneName, data);
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
        using namespace CryptoPP;
        std::stringstream sstream;
        StringSource ss(re.getAppContent().wire(), re.getAppContent().size(), true,
                        new Base64Encoder(new FileSink(sstream), true, 64));

        std::string content = sstream.str();
        std::string delimiter = "\n";
        size_t pos = 0;
        std::string token;
        while ((pos = content.find(delimiter)) != std::string::npos) {
            token = content.substr(0, pos);
            os << "; " << token << std::endl;
            content.erase(0, pos + delimiter.length());
        }

        os << std::endl;
      }
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

  if (!m_keyChain.doesKeyExistInTpm(keyName, KEY_CLASS_PRIVATE)) {
    NDNS_LOG_WARN("Private key: " << keyName.toUri() << " is not presented in KeyChain");
    return false;
  }

  return true;
}

} // namespace ndns
} // namespace ndn
