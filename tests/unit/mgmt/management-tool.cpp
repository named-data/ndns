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

#include "../../src/mgmt/management-tool.hpp"

#include "../../boost-test.hpp"
#include <boost/test/output_test_stream.hpp>
using boost::test_tools::output_test_stream;

#include "ndns-enum.hpp"
#include "ndns-label.hpp"
#include "ndns-tlv.hpp"

#include <vector>
#include <iostream>
#include <fstream>
#include <string>

#include <boost/filesystem.hpp>
#include <boost/algorithm/string/replace.hpp>

#include <ndn-cxx/security/key-chain.hpp>
#include <ndn-cxx/security/validator.hpp>
#include <ndn-cxx/util/io.hpp>
#include <ndn-cxx/util/regex.hpp>

namespace ndn {
namespace ndns {
namespace tests {

BOOST_AUTO_TEST_SUITE(ManagementTool)

static const boost::filesystem::path TEST_DATABASE = TEST_CONFIG_PATH "/management_tool.db";
static const boost::filesystem::path TEST_CERTDIR = TEST_CONFIG_PATH "/management_tool_certs";
static const Name FAKE_ROOT("/fake-root/123456789");

class ManagementToolFixture
{
public:
  ManagementToolFixture()
    : m_tool(TEST_DATABASE.string().c_str())
    , m_keyChain("sqlite3", "file")
    , m_dbMgr(TEST_DATABASE.string().c_str())
  {
    boost::filesystem::create_directory(TEST_CERTDIR);
  }

  ~ManagementToolFixture()
  {
    boost::filesystem::remove_all(TEST_CERTDIR);
    boost::filesystem::remove(TEST_DATABASE);
  }

  void
  getKeyList(const Name& identity, std::vector<Name>& keyList)
  {
    m_keyChain.getAllKeyNamesOfIdentity(identity, keyList, false);
    m_keyChain.getAllKeyNamesOfIdentity(identity, keyList, true);
  }

  void
  getCertList(const Name& identity, std::vector<Name>& certNameList)
  {
    std::vector<Name> keyList;
    getKeyList(identity, keyList);
    for (const Name& name : keyList) {
      m_keyChain.getAllCertificateNamesOfKey(name, certNameList, false);
      m_keyChain.getAllCertificateNamesOfKey(name, certNameList, true);
    }
  }

  bool
  checkIdCert(Zone& zone, const Name& certName)
  {
    Rrset rrset(&zone);
    size_t size = zone.getName().size();
    Name label = certName.getSubName(size+1, certName.size()-size-3);
    rrset.setLabel(label);
    rrset.setType(label::CERT_RR_TYPE);

    if (!m_dbMgr.find(rrset))
      return false;

    Data data(rrset.getData());
    IdentityCertificate cert(data);
    return cert.getName() == certName;
  }

  bool
  checkNs(Zone& zone, const Name& label, NdnsType ndnsType)
  {
    Rrset rrset(&zone);
    rrset.setLabel(label);
    rrset.setType(label::NS_RR_TYPE);

    if (!m_dbMgr.find(rrset))
      return false;

    Data data(rrset.getData());
    MetaInfo info = data.getMetaInfo();

    const Block* block = info.findAppMetaInfo(tlv::NdnsType);
    if (block == 0)
      return false;

    return static_cast<NdnsType>(readNonNegativeInteger(*block)) == ndnsType;
  }

  bool
  checkVersion(Zone& zone, const Name& label, const name::Component& type, uint64_t version)
  {
    Rrset rrset(&zone);
    rrset.setLabel(label);
    rrset.setType(type);

    if (!m_dbMgr.find(rrset))
      return false;

    name::Component tmp = name::Component::fromVersion(version);

    return rrset.getVersion() == tmp;
  }

  bool
  checkTtl(Zone& zone, const Name& label, const name::Component& type, time::seconds ttl)
  {
    Rrset rrset(&zone);
    rrset.setLabel(label);
    rrset.setType(type);

    if (!m_dbMgr.find(rrset))
      return false;

    return rrset.getTtl() == ttl;
  }

  // function to get a verified public key of a given data
  PublicKey
  getPublicKeyOfData(const Data& data) {
    // first extract certificate name of the signing key
    Signature sig = data.getSignature();
    KeyLocator kl = sig.getKeyLocator();
    Name klName = kl.getName();

    //base case, return root key to verify ROOT DSK certificate
    shared_ptr<Regex> regex1 = make_shared<Regex>(
                                 "<KEY><fake-root><123456789><ksk-[0-9]+><ID-CERT>");
    if (regex1->match(klName)) {
      std::vector<Name> keyList1;
      m_keyChain.getAllKeyNamesOfIdentity(FAKE_ROOT, keyList1, false);
      shared_ptr<PublicKey> rootKey = m_keyChain.getPublicKey(keyList1.front());

      // return root key
      return *rootKey;
    }

    // second extract the certificate from local NDNS database
    // extract zone name and label
    shared_ptr<Regex> regex2 = make_shared<Regex>("(<>*)<KEY>(<>+)<ID-CERT>");
    BOOST_ASSERT(regex2->match(klName) == true);
    Name zoneName = regex2->expand("\\1");
    Name label = regex2->expand("\\2");

    // get ID-CERT from local NDNS database
    Zone zone(zoneName);
    Rrset rrset(&zone);
    rrset.setLabel(label);
    rrset.setType(label::CERT_RR_TYPE);

    BOOST_CHECK_EQUAL(m_dbMgr.find(rrset), true);

    // get public key siging the ID-CERT
    Data tmp(rrset.getData());
    PublicKey pk = getPublicKeyOfData(tmp);

    // verified the ID-CERT
    BOOST_CHECK_EQUAL(Validator::verifySignature(tmp, pk), true);
    IdentityCertificate cert(tmp);

    // third return public key
    return cert.getPublicKeyInfo();
  }

public:
  ndns::ManagementTool m_tool;
  ndn::KeyChain m_keyChain;
  ndns::DbMgr m_dbMgr;
};

BOOST_FIXTURE_TEST_CASE(CreateZone1, ManagementToolFixture)
{
  //create root zone with supplied certificates
  bool isRoot = m_keyChain.doesIdentityExist(ROOT_ZONE);
  size_t iniKey = 0;
  size_t iniCert = 0;
  if (isRoot) {
    std::vector<Name> keyList;
    getKeyList(ROOT_ZONE, keyList);
    iniKey = keyList.size();

    std::vector<Name> certList;
    getCertList(ROOT_ZONE, certList);
    iniCert = certList.size();
  }

  time::system_clock::TimePoint notBefore = time::system_clock::now();
  time::system_clock::TimePoint notAfter = notBefore + DEFAULT_CERT_TTL;

  Name kskName = m_keyChain.generateRsaKeyPair(ROOT_ZONE, true);
  std::vector<CertificateSubjectDescription> kskDesc;
  shared_ptr<IdentityCertificate> kskCert = m_keyChain.prepareUnsignedIdentityCertificate(kskName,
    ROOT_ZONE, notBefore, notAfter, kskDesc);
  m_keyChain.selfSign(*kskCert);
  m_keyChain.addCertificate(*kskCert);

  Name dskName = m_keyChain.generateRsaKeyPair(ROOT_ZONE, false);
  std::vector<CertificateSubjectDescription> dskDesc;
  shared_ptr<IdentityCertificate> dskCert = m_keyChain.prepareUnsignedIdentityCertificate(
    dskName, ROOT_ZONE, notBefore, notAfter, dskDesc);
  m_keyChain.sign(*dskCert, kskCert->getName());
  m_keyChain.addCertificate(*dskCert);

  m_tool.createZone(ROOT_ZONE, ROOT_ZONE, time::seconds(4600), time::seconds(4600),
                    kskCert->getName(), dskCert->getName());

  BOOST_CHECK_EQUAL(m_keyChain.doesIdentityExist(ROOT_ZONE), true);

  std::vector<Name> keyList;
  getKeyList(ROOT_ZONE, keyList);
  BOOST_CHECK_EQUAL(keyList.size(), 2 + iniKey);

  std::vector<Name> certList;
  getCertList(ROOT_ZONE, certList);
  BOOST_CHECK_EQUAL(certList.size(), 2 + iniCert);

  Zone zone(ROOT_ZONE);
  BOOST_CHECK_EQUAL(m_dbMgr.find(zone), true);
  BOOST_CHECK_EQUAL(checkIdCert(zone, dskCert->getName()), true);

  if (isRoot) {
    // will leave the identity in KeyChain
    m_keyChain.deleteCertificate(kskCert->getName());
    m_keyChain.deleteCertificate(dskCert->getName());

    m_keyChain.deleteKey(kskName);
    m_keyChain.deleteKey(dskName);
  }
  else {
    m_tool.deleteZone(ROOT_ZONE);
  }
}

BOOST_FIXTURE_TEST_CASE(CreateZone2, ManagementToolFixture)
{
  //create normal zone
  Name parentZoneName("/ndns-test");
  parentZoneName.appendVersion();
  Name zoneName = parentZoneName;
  zoneName.append("child-zone");


  m_tool.createZone(zoneName, parentZoneName);

  BOOST_CHECK_EQUAL(m_keyChain.doesIdentityExist(zoneName), true);

  std::vector<Name> keyList;
  getKeyList(zoneName, keyList);
  BOOST_CHECK_EQUAL(keyList.size(), 2);

  std::vector<Name> certList;
  getCertList(zoneName, certList);
  BOOST_CHECK_EQUAL(certList.size(), 2);

  Name dskName;
  Name kskName;
  for (std::vector<Name>::iterator it = certList.begin();
      it != certList.end();
      it++) {
    std::string temp = (*it).toUri();
    size_t found = temp.find("dsk");
    if (found != std::string::npos) {
      dskName = *it;
    }
    else {
      kskName = *it;
    }
  }

  Zone zone(zoneName);
  BOOST_CHECK_EQUAL(m_dbMgr.find(zone), true);
  BOOST_CHECK_EQUAL(checkIdCert(zone, dskName), true);

  m_tool.deleteZone(zoneName);
}

BOOST_FIXTURE_TEST_CASE(CreateZone3, ManagementToolFixture)
{
  //create normal zone with TTL
  Name parentZoneName("/ndns-test");
  parentZoneName.appendVersion();
  Name zoneName = parentZoneName;
  zoneName.append("child-zone");

  time::seconds ttl = time::seconds(4200);
  m_tool.createZone(zoneName, parentZoneName, ttl, ttl);

  BOOST_CHECK_EQUAL(m_keyChain.doesIdentityExist(zoneName), true);

  std::vector<Name> keyList;
  getKeyList(zoneName, keyList);
  BOOST_CHECK_EQUAL(keyList.size(), 2);

  std::vector<Name> certList;
  getCertList(zoneName, certList);
  BOOST_CHECK_EQUAL(certList.size(), 2);

  Name dskName;
  for (std::vector<Name>::iterator it = certList.begin();
      it != certList.end();
      it++) {
    std::string temp = (*it).toUri();
    size_t found = temp.find("dsk");
    if (found != std::string::npos) {
      dskName = *it;
    }
  }

  //check zone ttl
  Zone zone(zoneName);
  BOOST_CHECK_EQUAL(m_dbMgr.find(zone), true);
  BOOST_CHECK_EQUAL(zone.getTtl(), ttl);

  //check dsk rrset ttl
  Rrset rrset(&zone);
  size_t size = zone.getName().size();
  Name label = dskName.getSubName(size+1, dskName.size()-size-3);
  rrset.setLabel(label);
  rrset.setType(label::CERT_RR_TYPE);
  BOOST_CHECK_EQUAL(m_dbMgr.find(rrset), true);
  BOOST_CHECK_EQUAL(rrset.getTtl(), ttl);

  //check certificate ttl
  shared_ptr<IdentityCertificate> dskCert = m_keyChain.getCertificate(dskName);
  time::system_clock::Duration tmp = dskCert->getNotAfter() - dskCert->getNotBefore();
  BOOST_CHECK_EQUAL(ttl, tmp);

  m_tool.deleteZone(zoneName);
}

BOOST_FIXTURE_TEST_CASE(CreateZone4, ManagementToolFixture)
{
  //check pre-condition
  Name zoneName("/net/ndnsim");
  Name parentZoneName("/net");

  Zone zone(zoneName);
  m_dbMgr.insert(zone);
  BOOST_CHECK_THROW(m_tool.createZone(zoneName, parentZoneName), ndns::ManagementTool::Error);
  m_dbMgr.remove(zone);

  time::seconds ttl = time::seconds(4200);
  BOOST_CHECK_THROW(m_tool.createZone(zoneName, zoneName), ndns::ManagementTool::Error);

  Name fake1("/com");
  BOOST_CHECK_THROW(m_tool.createZone(zoneName, fake1), ndns::ManagementTool::Error);

  Name fake2("/com/ndnsim");
  BOOST_CHECK_THROW(m_tool.createZone(zoneName, parentZoneName, ttl, ttl, fake2),
                    ndns::ManagementTool::Error);
  Name cert2 = m_keyChain.createIdentity(fake2);
  BOOST_CHECK_THROW(m_tool.createZone(zoneName, parentZoneName, ttl, ttl, cert2),
                    ndns::ManagementTool::Error);
  m_keyChain.deleteIdentity(fake2);

  Name fake3("/net/ndnsim/www");
  Name cert3 = m_keyChain.createIdentity(fake3);
  BOOST_CHECK_THROW(m_tool.createZone(zoneName, parentZoneName, ttl, ttl, cert3),
                    ndns::ManagementTool::Error);
  m_keyChain.deleteIdentity(fake3);

  Name cert4 = m_keyChain.createIdentity(zoneName);
  shared_ptr<IdentityCertificate> cert = m_keyChain.getCertificate(cert4);
  //delete keys in tpm
  m_keyChain.deleteKeyPairInTpm(cert->getPublicKeyName());
  BOOST_CHECK_THROW(m_tool.createZone(zoneName, parentZoneName, ttl, ttl, cert4),
                    ndns::ManagementTool::Error);
  m_keyChain.deleteIdentity(zoneName);

  //for root zone special case
  BOOST_CHECK_THROW(m_tool.createZone(ROOT_ZONE, ROOT_ZONE), ndns::ManagementTool::Error);
}

BOOST_FIXTURE_TEST_CASE(DeleteZone1, ManagementToolFixture)
{
  Name zoneName("/ndns-test");
  zoneName.appendVersion();

  m_tool.createZone(zoneName, ROOT_ZONE);

  std::vector<Name> keyList;
  getKeyList(zoneName, keyList);
  std::vector<Name> certList;
  getCertList(zoneName, certList);

  m_tool.deleteZone(zoneName);

  BOOST_CHECK_EQUAL(m_keyChain.doesIdentityExist(zoneName), false);

  for (std::vector<Name>::iterator it = keyList.begin(); it != keyList.end(); it++) {
    BOOST_CHECK_EQUAL(m_keyChain.doesKeyExistInTpm(*it, KEY_CLASS_PUBLIC), false);
    BOOST_CHECK_EQUAL(m_keyChain.doesKeyExistInTpm(*it, KEY_CLASS_PRIVATE), false);
  }

  Name dskName;
  for (std::vector<Name>::iterator it = certList.begin();
      it != certList.end();
      it++) {
    BOOST_CHECK_EQUAL(m_keyChain.doesCertificateExist(*it), false);
    std::string temp = (*it).toUri();
    size_t found = temp.find("dsk");
    if (found != std::string::npos) {
      dskName = *it;
    }
  }

  Zone zone(zoneName);
  BOOST_CHECK_EQUAL(m_dbMgr.find(zone), false);
  BOOST_CHECK_EQUAL(checkIdCert(zone, dskName), false);
}

BOOST_FIXTURE_TEST_CASE(DeleteZone2, ManagementToolFixture)
{
  Name zoneName("/net/ndnsim");

  BOOST_CHECK_THROW(m_tool.deleteZone(zoneName), ndns::ManagementTool::Error);
}

class OutputTester
{
public:
  OutputTester()
    : savedBuf(std::clog.rdbuf())
  {
    std::cout.rdbuf(buffer.rdbuf());
  }

  ~OutputTester()
  {
    std::cout.rdbuf(savedBuf);
  }

public:
  std::stringstream buffer;
  std::streambuf* savedBuf;
};

BOOST_FIXTURE_TEST_CASE(ExportCertificate, ManagementToolFixture)
{
  Name zoneName("/ndns-test");
  zoneName.appendVersion();
  std::string output = TEST_CERTDIR.string() + "/ss.cert";

  /// @todo Do not generate anything in root zone. Use some prefix for testing, e.g. /test

  //check precondition
  Name certName("/net/KEY/ksk-123/ID-CERT/123");
  BOOST_CHECK_THROW(m_tool.exportCertificate(certName, output), ndns::ManagementTool::Error);

  //get certificate from KeyChain
  m_tool.createZone(zoneName, ROOT_ZONE);
  std::vector<Name> certList;
  getCertList(zoneName, certList);
  Name kskCertName;
  Name dskCertName;
  for (const auto& cert : certList) {
    std::string temp = cert.toUri();
    size_t found = temp.find("ksk");
    if (found != std::string::npos) {
      kskCertName = cert;
    }
    else {
      dskCertName = cert;
    }
  }
  m_tool.exportCertificate(kskCertName, output);
  BOOST_CHECK_EQUAL(boost::filesystem::exists(output), true);

  //get certificate from ndns database
  boost::filesystem::remove(output);
  m_keyChain.deleteCertificate(dskCertName);
  m_tool.exportCertificate(dskCertName, output);
  BOOST_CHECK_EQUAL(boost::filesystem::exists(output), true);

  //output to std::cout
  std::string acutalOutput;
  {
    OutputTester tester;
    m_tool.exportCertificate(dskCertName, "-");
    acutalOutput = tester.buffer.str();
  }
  // BOOST_CHECK_EQUAL(acutalOutput,
  //                   "Bv0DDAdCCAluZG5zLXRlc3QICf0AAAFJ6RF8OggDS0VZCBFkc2stMTQxNjk1NDQ3\n"
  //                   "NzcxNQgHSUQtQ0VSVAgJ/QAAAUnpEXzcFAMYAQIV/QF7MIIBdzAiGA8yMDE0MTEy\n"
  //                   "NTIyMjc1N1oYDzIwMTUxMTI1MjIyNzU3WjAtMCsGA1UEKRMkL25kbnMtdGVzdC8l\n"
  //                   "RkQlMDAlMDAlMDFJJUU5JTExJTdDJTNBMIIBIDANBgkqhkiG9w0BAQEFAAOCAQ0A\n"
  //                   "MIIBCAKCAQEAvRAQ0iLGmoUCNFQjtD6ClA6BzmtViWywDRU7eDXHHqz1BmXNGS28\n"
  //                   "dmVbk6C8ZiQxVh5IoN2psxU1avC2lLM7HXHnyUtmfu9/zkxdBSFq8B+8FchFpPWe\n"
  //                   "9M/TSFmEemcFLdMgYEJOurF3osASZ70/kJJ7f1yiFRm1dtO2pl5eVUuiZPcgVwMU\n"
  //                   "nCUDT/wZ6l2LDr4K7wo6p5F8PYiIddS6zT38gZsRNwCeR5JDwCPY3u5x21aVSSY7\n"
  //                   "8PS/wdSG0vmjI/kQEWhjaV7ibyTc6ygrbq5RiLD+18CkEF+ECLKjvLnyIO8DkbbM\n"
  //                   "iMVyc6QSiKnq7bLCyqbBVpZqNpBjdMvDuQIBERY+GwEBHDkHNwgDS0VZCAluZG5z\n"
  //                   "LXRlc3QICf0AAAFJ6RF8OggRa3NrLTE0MTY5NTQ0Nzc2MjcIB0lELUNFUlQX/QEA\n"
  //                   "BKchd4m2kpI7N376Z8yRAKA3epcUlGI5E1Xs7pKTFwA16ZQ+dTThBQSzzVaJcOC2\n"
  //                   "UIc2WVRG5wCxcCLou4bxnbuPaQORyeRnmoodTPoIYsDrWVPxeVY1xJSKCbGoY8dh\n"
  //                   "BuXmo9nl4X2wk/gZx6//afATgzQCliQK/J9PwigqirJqH6aD+3Ji209FbP57tv0g\n"
  //                   "aL0Dy5IA6LrJI1Dxq0MXCRv3bNtdGNQZT0AfHIgCrhPe6MMLGn+C9UTDP2nzMqbX\n"
  //                   "4MaAdshgUFj0mI2iPL3Bep9pYI6zxbU3hJ25UQegjJdC3qoFjz2sp/D6J8wAlCPX\n"
  //                   "gGH3iewd94xFpPcdrhxtHQ==\n");

  m_tool.deleteZone(zoneName);
}

BOOST_FIXTURE_TEST_CASE(AddRrSet1, ManagementToolFixture)
{
  // check pre-condition
  BOOST_CHECK_THROW(m_tool.addRrSet(ROOT_ZONE, ROOT_ZONE, label::NS_RR_TYPE, NDNS_RESP),
                    ndns::ManagementTool::Error);

  Name zoneName("/ndns-test");
  zoneName.appendVersion();
  Zone zone(zoneName);
  m_dbMgr.insert(zone);

  BOOST_CHECK_THROW(m_tool.addRrSet(ROOT_ZONE, ROOT_ZONE, label::NS_RR_TYPE, NDNS_UNKNOWN),
                    ndns::ManagementTool::Error);
  BOOST_CHECK_THROW(m_tool.addRrSet(zoneName, zoneName, label::CERT_RR_TYPE, NDNS_RAW),
                    ndns::ManagementTool::Error);
  BOOST_CHECK_THROW(m_tool.addRrSet(ROOT_ZONE, ROOT_ZONE, label::NS_RR_TYPE, NDNS_RAW),
                    ndns::ManagementTool::Error);
  BOOST_CHECK_THROW(m_tool.addRrSet(ROOT_ZONE, ROOT_ZONE, label::TXT_RR_TYPE, NDNS_RAW),
                    ndns::ManagementTool::Error);

  m_dbMgr.remove(zone);
}

BOOST_FIXTURE_TEST_CASE(AddRrSet2, ManagementToolFixture)
{
  Name zoneName("/ndns-test");
  zoneName.appendVersion();
  Zone zone(zoneName);
  uint64_t version = 1234;
  time::seconds ttl1(4200);
  time::seconds ttl2(4500);
  m_tool.createZone(zoneName, ROOT_ZONE, ttl1);

  //add NS NDNS_AUTH and check user-defined ttl
  Name label1("/net/ndnsim1");
  BOOST_CHECK_NO_THROW(m_tool.addRrSet(zoneName, label1, label::NS_RR_TYPE, NDNS_AUTH, version,
                                       {}, DEFAULT_CERT, ttl2));
  BOOST_CHECK_EQUAL(checkNs(zone, label1, NDNS_AUTH), true);
  BOOST_CHECK_EQUAL(checkVersion(zone, label1, label::NS_RR_TYPE, version), true);
  BOOST_CHECK_EQUAL(checkTtl(zone, label1, label::NS_RR_TYPE, ttl2), true);

  //add NS NDNS_RESP and check default ttl
  Name label2("/net/ndnsim2");
  BOOST_CHECK_NO_THROW(m_tool.addRrSet(zoneName, label2, label::NS_RR_TYPE, NDNS_RESP, version));
  BOOST_CHECK_EQUAL(checkNs(zone, label2, NDNS_RESP), true);
  BOOST_CHECK_EQUAL(checkVersion(zone, label2, label::NS_RR_TYPE, version), true);
  BOOST_CHECK_EQUAL(checkTtl(zone, label2, label::NS_RR_TYPE, ttl1), true);

  //add TXT NDNS_RESP and check rr
  std::string test = "oops";
  BOOST_CHECK_NO_THROW(m_tool.addRrSet(zoneName, label2, label::TXT_RR_TYPE, NDNS_RESP, version,
                                       {test}));
  Rrset rrset1(&zone);
  rrset1.setLabel(label2);
  rrset1.setType(label::TXT_RR_TYPE);
  BOOST_CHECK_EQUAL(m_dbMgr.find(rrset1), true);

  Data data1(rrset1.getData());
  Response re1;
  Name hint1;
  BOOST_CHECK_EQUAL(re1.fromData(hint1, zoneName, data1), true);
  const Block &block = re1.getRrs().front();
  std::string someString(reinterpret_cast<const char*>(block.value()), block.value_size());
  BOOST_CHECK_EQUAL(test, someString);

  //add user defined type
  Name label3("/net/ndnsim3");
  name::Component type("A");
  std::string content = "10.10.0.1";
  m_tool.addRrSet(zoneName, label3, type, NDNS_RAW, version, {content});
  BOOST_CHECK_EQUAL(checkVersion(zone, label3, type, version), true);

  Rrset rrset2(&zone);
  rrset2.setLabel(label3);
  rrset2.setType(type);
  BOOST_CHECK_EQUAL(m_dbMgr.find(rrset2), true);

  Data data2(rrset2.getData());
  Response re2;
  Name hint2;
  BOOST_CHECK_EQUAL(re2.fromData(hint2, zoneName, data2), true);
  Block tmp = ndn::dataBlock(ndn::tlv::Content, content.c_str(), content.length());
  tmp.encode();
  BOOST_REQUIRE(tmp == re2.getAppContent());

  m_tool.deleteZone(zoneName);
}

BOOST_FIXTURE_TEST_CASE(AddRrSet3, ManagementToolFixture)
{
  // check pre-condition
  Name zoneName("/ndns-test");
  zoneName.appendVersion();
  std::string certPath = TEST_CERTDIR.string();
  BOOST_CHECK_THROW(m_tool.addRrSet(zoneName, certPath), ndns::ManagementTool::Error);

  m_tool.createZone(zoneName, ROOT_ZONE);
  BOOST_CHECK_THROW(m_tool.addRrSet(zoneName, certPath), ndns::ManagementTool::Error);
  m_tool.deleteZone(zoneName);
}

BOOST_FIXTURE_TEST_CASE(AddRrSet4, ManagementToolFixture)
{
  Name parentZoneName("/ndns-test");
  parentZoneName.appendVersion();
  Name zoneName = parentZoneName;
  zoneName.append("/child-zone");
  Zone zone(parentZoneName);
  time::seconds ttl1(4200);
  time::seconds ttl2(4500);
  m_tool.createZone(zoneName, parentZoneName);
  m_tool.createZone(parentZoneName, ROOT_ZONE, ttl1);

  //add KSK ID-CERT
  std::string output = TEST_CERTDIR.string() + "/ss.cert";
  std::vector<Name> certList;
  getCertList(zoneName, certList);
  Name kskCertName;
  for (std::vector<Name>::iterator it = certList.begin();
       it != certList.end();
       it++) {
    std::string temp = (*it).toUri();
    size_t found = temp.find("ksk");
    if (found != std::string::npos) {
      kskCertName = *it;
      break;
    }
  }
  m_tool.exportCertificate(kskCertName, output);

  BOOST_CHECK_NO_THROW(m_tool.addRrSet(parentZoneName, output, ttl2));
  BOOST_CHECK_EQUAL(checkIdCert(zone, kskCertName), true);

  //add data
  Response re;
  re.setZone(parentZoneName);
  re.setQueryType(label::NDNS_ITERATIVE_QUERY);
  re.setRrLabel(zoneName);
  re.setRrType(label::NS_RR_TYPE);
  re.setNdnsType(NDNS_RESP);
  shared_ptr<Data> data1 = re.toData();
  m_keyChain.sign(*data1, kskCertName);
  ndn::io::save(*data1, output);

  //check user-defined ttl and default ttl
  BOOST_CHECK_NO_THROW(m_tool.addRrSet(parentZoneName, output));
  BOOST_CHECK_EQUAL(checkTtl(zone, zoneName, label::NS_RR_TYPE, ttl1), true);
  m_tool.removeRrSet(parentZoneName, zoneName, label::NS_RR_TYPE);
  BOOST_CHECK_NO_THROW(m_tool.addRrSet(parentZoneName, output, ttl2));
  BOOST_CHECK_EQUAL(checkTtl(zone, zoneName, label::NS_RR_TYPE, ttl2), true);

  Rrset rrset(&zone);
  rrset.setLabel(zoneName);
  rrset.setType(label::NS_RR_TYPE);
  BOOST_CHECK_EQUAL(m_dbMgr.find(rrset), true);

  Data data2(rrset.getData());
  BOOST_REQUIRE(*data1 == data2);

  //add KSK ID-CERT with illegal name and convert it
  Name iZoneName = parentZoneName;
  iZoneName.append("illegal");
  Name illegalCertName = m_keyChain.createIdentity(iZoneName);
  m_tool.exportCertificate(illegalCertName, output);
  BOOST_CHECK_NO_THROW(m_tool.addRrSet(parentZoneName, output, ttl2));

  Name legalCertName = parentZoneName;
  legalCertName.append("KEY");
  legalCertName.append("illegal");
  legalCertName.append(illegalCertName.getSubName(4));
  BOOST_CHECK_EQUAL(checkIdCert(zone, legalCertName), true);
  m_keyChain.deleteIdentity(iZoneName);

  m_tool.deleteZone(zoneName);
  m_tool.deleteZone(parentZoneName);
}

BOOST_FIXTURE_TEST_CASE(AddRrSet5, ManagementToolFixture)
{
  //check using user provided certificate
  Name parentZoneName("/ndns-test");
  parentZoneName.appendVersion();
  Name zoneName = parentZoneName;
  zoneName.append("child-zone");

  Name dskName = m_keyChain.generateRsaKeyPair(parentZoneName, false);
  shared_ptr<IdentityCertificate> dskCert = m_keyChain.selfSign(dskName);
  m_keyChain.addCertificateAsKeyDefault(*dskCert);

  //check addRrSet1
  m_tool.createZone(zoneName, parentZoneName);
  m_tool.createZone(parentZoneName, ROOT_ZONE);

  //add KSK ID-CERT
  std::string output =  TEST_CERTDIR.string() +  "/ss.cert";
  std::vector<Name> certList;
  getCertList(zoneName, certList);
  Name kskCertName;
  for (std::vector<Name>::iterator it = certList.begin();
       it != certList.end();
       it++) {
    std::string temp = (*it).toUri();
    size_t found = temp.find("ksk");
    if (found != std::string::npos) {
      kskCertName = *it;
      break;
    }
  }
  m_tool.exportCertificate(kskCertName, output);

  BOOST_CHECK_NO_THROW(m_tool.addRrSet(parentZoneName, output, time::seconds(4600),
                                       dskCert->getName()));

  //check addRrSet2
  Name label1("/net/ndnsim1");
  BOOST_CHECK_NO_THROW(m_tool.addRrSet(parentZoneName, label1, label::NS_RR_TYPE, NDNS_AUTH, -1, {},
                                       dskCert->getName()));

  m_tool.deleteZone(zoneName);
  m_tool.deleteZone(parentZoneName);
}

BOOST_FIXTURE_TEST_CASE(ListZone, ManagementToolFixture)
{
  Name zoneName("/ndns-test/mgmt/123456789");

  m_tool.createZone(zoneName, ROOT_ZONE);
  Name certName = m_keyChain.getDefaultCertificateNameForIdentity(zoneName);
  shared_ptr<IdentityCertificate> cert = m_keyChain.getCertificate(certName);

  //add NS with NDNS_RESP
  Name label2("/ndnsim");
  uint64_t version = 1234;
  m_tool.addRrSet(zoneName, label2, label::NS_RR_TYPE, NDNS_RESP, version);

  //add NS with NDNS_AUTH
  Name label3("/ndnsim/oops");
  m_tool.addRrSet(zoneName, label3, label::NS_RR_TYPE, NDNS_AUTH, version);

  //add TXT
  std::string output = TEST_CERTDIR.string() + "/a.rrset";
  Response re1;
  re1.setZone(zoneName);
  re1.setQueryType(label::NDNS_ITERATIVE_QUERY);
  re1.setRrLabel(label2);
  re1.setRrType(label::TXT_RR_TYPE);
  re1.setNdnsType(NDNS_RESP);
  re1.setVersion(name::Component::fromVersion(version));
  re1.addRr("First RR");
  re1.addRr("Second RR");
  re1.addRr("Last RR");
  shared_ptr<Data> data1= re1.toData();
  m_keyChain.sign(*data1, certName);
  ndn::io::save(*data1, output);
  m_tool.addRrSet(zoneName, output);

  //add User-Defined
  name::Component type("A");
  Response re2;
  re2.setZone(zoneName);
  re2.setQueryType(label::NDNS_ITERATIVE_QUERY);
  re2.setRrLabel(label2);
  re2.setRrType(type);
  re2.setNdnsType(NDNS_RESP);
  re2.setVersion(name::Component::fromVersion(version));
  re2.addRr("First RR");
  re2.addRr("Second RR");
  re2.addRr("Last RR");
  shared_ptr<Data> data2= re2.toData();
  m_keyChain.sign(*data2, certName);
  ndn::io::save(*data2, output);
  m_tool.addRrSet(zoneName, output);

  output_test_stream testOutput;
  m_tool.listZone(zoneName, testOutput, true);

  /// @todo check the output

  // BOOST_CHECK(testOutput.
  //   is_equal(
  //     "; Zone /ndns-test/mgmt/123456789\n"
  //     "\n"
  //     "; rrset=/ndnsim type=A version=%FD%04%D2 signed-by=*\n"
  //     "/ndnsim             3600  A        vwhGaXJzdCBSUg==\n"
  //     "/ndnsim             3600  A        vwlTZWNvbmQgUlI=\n"
  //     "/ndnsim             3600  A        vwdMYXN0IFJS\n"
  //     "\n"
  //     "; rrset=/ndnsim type=NS version=%FD%04%D2 signed-by=*\n"
  //     "/ndnsim             3600  NS\n"
  //     "\n"
  //     "; rrset=/ndnsim type=TXT version=%FD%04%D2 signed-by=*\n"
  //     "/ndnsim             3600  TXT      First RR\n"
  //     "/ndnsim             3600  TXT      Second RR\n"
  //     "/ndnsim             3600  TXT      Last RR\n"
  //     ""
  //     "/ndnsim/oops        3600  NS       ; content-type=NDNS-Auth version=* signed-by=*\n"
  //     "/dsk-1416888156106  3600  ID-CERT  ; content-type=NDNS-Raw version=* signed-by=*\n"
  //     "; Ff0BcjCCAW4wIhgPMjAxNDExMjUwNDAyMzVaGA8yMDE1MTEyNTA0MDIzNVowIjAg\n"
  //     "; BgNVBCkTGS9uZG5zLXRlc3QvbWdtdC8xMjM0NTY3ODkwggEiMA0GCSqGSIb3DQEB\n"
  //     "; AQUAA4IBDwAwggEKAoIBAQD8Lsd/kKdcdXTKEhcJpy7M0cnZgpZ3lpQX2Nz8InXN\n"
  //     "; BkF3b6TsuMSokmDQ78xXNTx0jGUM1g47kQNsk2WTjYgDZUOfpBWz5PZcaO35dUz8\n"
  //     "; wNGc1lED9A/PbFB4t5NPbZsrThCPw+IvzTYN6A/Dxg9h69zLEEsLWD2PzCkyHQ+F\n"
  //     "; JjRgzhJPsrl1CoVFSP/bu70xMJVqOwj3GMej6mm0gXXmMv4aEI22jot7+fO07jIy\n"
  //     "; WUl0fhY33Cgo50YvDeeRsdo+At29OMN8Dd/s2z/6olH0P7IK1/cn++DWcO5zMd1x\n"
  //     "; zWm+6IoZA6mV1J2O5U0AkbllRVZ+vZPI/lWGzCtXK6jfAgMBAAE=\n"
  //     ));

  m_tool.deleteZone(zoneName);
}

BOOST_FIXTURE_TEST_CASE(ListAllZones, ManagementToolFixture)
{
  Name zoneName1("/ndns-test1/mgmt/123456789");
  m_tool.createZone(zoneName1, ROOT_ZONE);

  Name zoneName2("/ndns-test2/mgmt/123456789");
  m_tool.createZone(zoneName2, ROOT_ZONE);

  Name zoneName3("/ndns-test3/mgmt/123456789");
  m_tool.createZone(zoneName3, ROOT_ZONE);

  //TODO Check the output
  // a sample output is like this
  //  /ndns-test1/mgmt/123456789  ; default-ttl=* default-key=* default-certificate=*
  //  /ndns-test2/mgmt/123456789  ; default-ttl=* default-key=* default-certificate=*
  //  /ndns-test3/mgmt/123456789  ; default-ttl=* default-key=* default-certificate=*
  output_test_stream testOutput;
  m_tool.listAllZones(testOutput);

  m_tool.deleteZone(zoneName1);
  m_tool.deleteZone(zoneName2);
  m_tool.deleteZone(zoneName3);
}

BOOST_FIXTURE_TEST_CASE(GetRrSet, ManagementToolFixture)
{
  Name zoneName("/ndns-test");
  zoneName.appendVersion();
  m_tool.createZone(zoneName, ROOT_ZONE);

  Name label("/net/ndnsim2");

  m_tool.addRrSet(zoneName, label, label::NS_RR_TYPE, NDNS_RESP);

  // a sample output is like this
//  Bv0Bjgc5CAluZG5zLXRlc3QICf0AAAFJ5Sn8HwgETkROUwgDbmV0CAduZG5zaW0y
//  CAJOUwgJ/QAAAUnlKgXYFAkZBAA27oC0AQEVAr8AFj4bAQEcOQc3CAluZG5zLXRl
//  c3QICf0AAAFJ5Sn8HwgDS0VZCBFkc2stMTQxNjg4ODk3NTY0NAgHSUQtQ0VSVBf9
//  AQBj8QOr3vPo1wzuTAwwdF6UEfHmHYxkMor5bw/5Lc6Wpt6SkTb7Ku92/MHedtRD
//  2UsIhQV+JR7jUGzLVLUuktmrBTT7G9tSioTOVpbtJ7EEYYhcDCV0h1Kefz01AS1O
//  bUuZbxAWboZCzW6OluTq9Db2c2xEmzE8EzmMQ5zflIL7vQxEigH1kBsvGoMZThOb
//  aKrfcACfKU3kZQBzyPVbur5PER3dgBjVcdu5aIEXhO3Nf7b22OrPSP8AztZGvkz0
//  5TSBf7wwxtB6V/aMlDPaKFPgI2mXan729e1JGYZ0OmKhvuPnhT9ApOh9UNSJuhO8
//  puPJKCOSQHspQHOhnEy9Ee9U
  output_test_stream testOutput;
  m_tool.getRrSet(zoneName, label, label::NS_RR_TYPE, testOutput);

  m_tool.deleteZone(zoneName);
}

BOOST_FIXTURE_TEST_CASE(RemoveRrSet, ManagementToolFixture)
{
  Name zoneName("/ndns-test");
  zoneName.appendVersion();
  m_tool.createZone(zoneName, ROOT_ZONE);
  Name label2("/net/ndnsim2");
  m_tool.addRrSet(zoneName, label2, label::NS_RR_TYPE, NDNS_RESP);

  m_tool.removeRrSet(zoneName, label2, label::NS_RR_TYPE);
  Zone zone(zoneName);
  BOOST_CHECK_EQUAL(checkNs(zone, label2, NDNS_RESP), false);

  m_tool.deleteZone(zoneName);
}

BOOST_FIXTURE_TEST_CASE(DataValidation, ManagementToolFixture)
{
  Name subZoneName1 = FAKE_ROOT;
  subZoneName1.append("net");
  Name subZoneName2 = subZoneName1;
  subZoneName2.append("ndnsim");

  // create fake-root zone
  m_tool.createZone(FAKE_ROOT, ROOT_ZONE);

  // create subZone1 by creating zone, delegating it to ROOT zone
  m_tool.createZone(subZoneName1, FAKE_ROOT);

  std::string output1 = TEST_CERTDIR.string() + "/ss1.cert";
  std::vector<Name> certList1;
  getCertList(subZoneName1, certList1);
  Name kskCertName1;
  for (std::vector<Name>::iterator it = certList1.begin();
       it != certList1.end();
       it++) {
    std::string temp = (*it).toUri();
    size_t found = temp.find("ksk");
    if (found != std::string::npos) {
      kskCertName1 = *it;
      break;
    }
  }
  m_tool.exportCertificate(kskCertName1, output1);
  m_tool.addRrSet(FAKE_ROOT, output1, time::seconds(4600));

  // create subZone2 by creating zone, delegating it to subZone1
  m_tool.createZone(subZoneName2, subZoneName1);

  std::string output2 = TEST_CERTDIR.string() + "/ss2.cert";
  std::vector<Name> certList2;
  getCertList(subZoneName2, certList2);
  Name kskCertName2;
  for (std::vector<Name>::iterator it = certList2.begin();
       it != certList2.end();
       it++) {
    std::string temp = (*it).toUri();
    size_t found = temp.find("ksk");
    if (found != std::string::npos) {
      kskCertName2 = *it;
      break;
    }
  }
  m_tool.exportCertificate(kskCertName2, output2);
  m_tool.addRrSet(subZoneName1, output2, time::seconds(4600));

  //simulate the trust chain
  Data data("test");
  Name certName = m_keyChain.getDefaultCertificateNameForIdentity(subZoneName2);
  m_keyChain.sign(data, certName);

  //details of the verification please see the getPublicKeyOfData() function
  PublicKey pk = getPublicKeyOfData(data);
  BOOST_CHECK_EQUAL(Validator::verifySignature(data, pk), true);

  m_tool.deleteZone(subZoneName2);
  m_tool.deleteZone(subZoneName1);
  m_tool.deleteZone(FAKE_ROOT);
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace tests
} // namespace ndns
} // namespace ndn
