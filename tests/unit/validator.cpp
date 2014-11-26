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

#include "validator.hpp"
#include "../boost-test.hpp"
#include <ndn-cxx/util/dummy-client-face.hpp>
#include <ndn-cxx/security/key-chain.hpp>
#include <boost/asio.hpp>

namespace ndn {
namespace ndns {
namespace tests {
NDNS_LOG_INIT("ValidatorTest");

BOOST_AUTO_TEST_SUITE(Validator)

class Fixture
{
public:
  Fixture()
    : m_testId1("/test02")
    , m_testId2("/test02/ndn")
    , m_testId3("/test02/ndn/edu")
    , m_randomId("/test03")
    , m_keyChain("sqlite3", "file")
    , m_version(name::Component::fromVersion(0))
    , m_face(ndn::util::makeDummyClientFace(ndn::util::DummyClientFace::Options { false, true }))
  {
    m_keyChain.deleteIdentity(m_testId1);
    m_keyChain.deleteIdentity(m_testId2);
    m_keyChain.deleteIdentity(m_testId3);
    m_keyChain.deleteIdentity(m_randomId);

    m_randomDsk = createRoot(m_randomId); // generate a root cert

    m_dsk1 = createRoot(m_testId1); // replace to root cert
    m_dsk2 = createIdentity(m_testId2, m_dsk1);
    m_dsk3 = createIdentity(m_testId3, m_dsk2);

    m_selfSignCert = m_keyChain.generateRsaKeyPair(m_testId3, false);
    shared_ptr<IdentityCertificate> cert = m_keyChain.selfSign(m_selfSignCert);
    m_selfSignCert = cert->getName();
    m_keyChain.addCertificate(*cert);
    NDNS_LOG_TRACE("add cert: " << cert->getName() << " to KeyChain");

    m_face->onInterest += bind(&Fixture::respondInterest, this, _1);
  }

  ~Fixture()
  {
    m_face->getIoService().stop();
    m_face->shutdown();
    m_keyChain.deleteIdentity(m_testId1);
    m_keyChain.deleteIdentity(m_testId2);
    m_keyChain.deleteIdentity(m_testId3);
    m_keyChain.deleteIdentity(m_randomId);
  }

  const Name
  createIdentity(const Name& id, const Name& parentCertName)
  {
    Name kskCertName = m_keyChain.createIdentity(id);
    Name kskName = m_keyChain.getDefaultKeyNameForIdentity(id);
    m_keyChain.deleteCertificate(kskCertName);
    auto kskCert = createCertificate(kskName, parentCertName);

    Name dskName = m_keyChain.generateRsaKeyPair(id, false);
    auto dskCert = createCertificate(dskName, kskCert);
    return dskCert;
  }

  const Name
  createRoot(const Name& root)
  {
    m_rootCert = m_keyChain.createIdentity(root);
    ndn::io::save(*(m_keyChain.getCertificate(m_rootCert)), TEST_CONFIG_PATH "/anchors/root.cert");
    NDNS_LOG_TRACE("save root cert "<< m_rootCert <<
                  " to: " << TEST_CONFIG_PATH "/anchors/root.cert");
    Name dsk = m_keyChain.generateRsaKeyPair(root, false);
    auto cert = createCertificate(dsk, m_rootCert);
    return cert;
  }


  const Name
  createCertificate(const Name& keyName, const Name& parentCertName)
  {
    std::vector<CertificateSubjectDescription> desc;
    time::system_clock::TimePoint notBefore = time::system_clock::now();
    time::system_clock::TimePoint notAfter = notBefore + time::days(365);
    desc.push_back(CertificateSubjectDescription(oid::ATTRIBUTE_NAME,
                                                 "Signer: " + parentCertName.toUri()));
    shared_ptr<IdentityCertificate> cert =
      m_keyChain.prepareUnsignedIdentityCertificate(keyName, parentCertName,
                                                    notBefore, notAfter, desc);

    Name tmp = cert->getName().getPrefix(-1).append(m_version);
    cert->setName(tmp);
    m_keyChain.sign(*cert, parentCertName);
    m_keyChain.addCertificateAsKeyDefault(*cert);
    NDNS_LOG_TRACE("add cert: " << cert->getName() << " to KeyChain");
    return cert->getName();
  }


  void
  respondInterest(const Interest& interest)
  {
    Name certName = interest.getName();
    if (certName.isPrefixOf(m_selfSignCert)) {
      // self-sign cert's version number is not m_version
      certName = m_selfSignCert;
    } else {
      certName.append(m_version);
    }
    NDNS_LOG_TRACE("validator needs: " << certName);
    BOOST_CHECK_EQUAL(m_keyChain.doesCertificateExist(certName), true);
    auto cert = m_keyChain.getCertificate(certName);
    m_face->receive<Data>(*cert);
  }

public:
  Name m_testId1;
  Name m_testId2;
  Name m_testId3;
  Name m_randomId;

  Name m_rootCert;

  KeyChain m_keyChain;

  Name m_dsk1;
  Name m_dsk2;
  Name m_dsk3;

  Name m_selfSignCert;

  Name m_randomDsk;

  name::Component m_version;

  shared_ptr<ndn::util::DummyClientFace> m_face;
};


BOOST_FIXTURE_TEST_CASE(Basic, Fixture)
{
  // validator must be created after root key is saved to the target
  ndns::Validator validator(*m_face, TEST_CONFIG_PATH "/" "validator.conf");

  Name dataName(m_testId3);
  dataName.append("NDNS")
    .append("rrLabel")
    .append("rrType")
    .appendVersion();
  shared_ptr<Data> data = make_shared<Data>(dataName);
  m_keyChain.sign(*data, m_dsk3);

  bool hasValidated = false;
  validator.validate(*data,
                     [&] (const shared_ptr<const Data>& data) {
                       hasValidated = true;
                       BOOST_CHECK(true);
                     },
                     [&] (const shared_ptr<const Data>& data, const std::string& str) {
                       hasValidated = true;
                       BOOST_CHECK(false);
                     });

  m_face->processEvents(time::milliseconds(-1));

  BOOST_CHECK_EQUAL(hasValidated, true);


  dataName = m_testId2;
  dataName.append("KEY")
    .append("rrLabel")
    .append("ID-CERT")
    .appendVersion();
  data = make_shared<Data>(dataName);
  m_keyChain.sign(*data, m_dsk3); // key's owner's name is longer than data owner's

  hasValidated = false;
  validator.validate(*data,
                     [&] (const shared_ptr<const Data>& data) {
                       hasValidated = true;
                       BOOST_CHECK(false);
                     },
                     [&] (const shared_ptr<const Data>& data, const std::string& str) {
                       hasValidated = true;
                       BOOST_CHECK(true);
                     });

  m_face->processEvents(time::milliseconds(-1));
  // cannot pass verification due to key's owner's name is longer than data owner's
  BOOST_CHECK_EQUAL(hasValidated, true);


  dataName = m_testId3;
  dataName.append("KEY")
    .append("rrLabel")
    .append("ID-CERT")
    .appendVersion();
  data = make_shared<Data>(dataName);
  m_keyChain.sign(*data, m_selfSignCert);

  hasValidated = false;
  validator.validate(*data,
                     [&] (const shared_ptr<const Data>& data) {
                       hasValidated = true;
                       BOOST_CHECK(false);
                     },
                     [&] (const shared_ptr<const Data>& data, const std::string& str) {
                       hasValidated = true;
                       BOOST_CHECK(true);
                     });

  m_face->processEvents(time::milliseconds(-1));
  // cannot pass due to self-sign cert is used
  BOOST_CHECK_EQUAL(hasValidated, true);

  dataName = m_testId2;
  dataName.append("KEY")
    .append("rrLabel")
    .append("ID-CERT")
    .appendVersion();
  data = make_shared<Data>(dataName);
  m_keyChain.sign(*data, m_randomDsk);

  hasValidated = false;
  validator.validate(*data,
                     [&] (const shared_ptr<const Data>& data) {
                       hasValidated = true;
                       BOOST_CHECK(false);
                     },
                     [&] (const shared_ptr<const Data>& data, const std::string& str) {
                       hasValidated = true;
                       BOOST_CHECK(true);
                     });

  m_face->processEvents(time::milliseconds(-1));
  // cannot pass due to a totally mismatched key
  BOOST_CHECK_EQUAL(hasValidated, true);
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace tests
} // namespace ndns
} // namespace ndn
