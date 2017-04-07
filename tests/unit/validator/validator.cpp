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

#include "validator/validator.hpp"

#include "test-common.hpp"
#include "util/cert-helper.hpp"

#include <ndn-cxx/util/io.hpp>

namespace ndn {
namespace ndns {
namespace tests {

NDNS_LOG_INIT("ValidatorTest")

BOOST_AUTO_TEST_SUITE(Validator)

class Fixture : public IdentityManagementFixture
{
public:
  Fixture()
    : m_testId1("/test02")
    , m_testId2("/test02/ndn")
    , m_testId3("/test02/ndn/edu")
    , m_randomId("/test03")
    , m_face(m_keyChain, {false, true})
  {
    m_randomDsk = createRoot(Name(m_randomId).append("NDNS")); // generate a root cert

    m_dsk1 = createRoot(Name(m_testId1).append("NDNS")); // replace to root cert
    m_dsk2 = createIdentity(Name(m_testId2).append("NDNS"), m_dsk1);
    m_dsk3 = createIdentity(Name(m_testId3).append("NDNS"), m_dsk2);

    m_face.onSendInterest.connect(bind(&Fixture::respondInterest, this, _1));
  }

  ~Fixture()
  {
    m_face.getIoService().stop();
    m_face.shutdown();
  }

  const Key
  createIdentity(const Name& id, const Key& parentKey)
  {
    Identity identity = addIdentity(id);
    Key defaultKey = identity.getDefaultKey();
    m_keyChain.deleteKey(identity, defaultKey);

    Key ksk = m_keyChain.createKey(identity);
    Name defaultKskCert = ksk.getDefaultCertificate().getName();
    m_keyChain.deleteCertificate(ksk, defaultKskCert);

    Key dsk = m_keyChain.createKey(identity);
    Name defaultDskCert = dsk.getDefaultCertificate().getName();
    m_keyChain.deleteCertificate(dsk, defaultDskCert);

    auto kskCert = CertHelper::createCertificate(m_keyChain, ksk, parentKey, "CERT", time::days(100));
    auto dskCert = CertHelper::createCertificate(m_keyChain, dsk, ksk, "CERT", time::days(100));

    m_keyChain.addCertificate(ksk, kskCert);
    m_keyChain.addCertificate(dsk, dskCert);

    m_keyChain.setDefaultKey(identity, dsk);
    return dsk;
  }

  const Key
  createRoot(const Name& root)
  {
    Identity rootIdentity = addIdentity(root);
    auto cert = rootIdentity.getDefaultKey().getDefaultCertificate();
    ndn::io::save(cert, TEST_CONFIG_PATH "/anchors/root.cert");
    NDNS_LOG_TRACE("save root cert "<< m_rootCert <<
                  " to: " << TEST_CONFIG_PATH "/anchors/root.cert");
    return rootIdentity.getDefaultKey();
  }

  void
  respondInterest(const Interest& interest)
  {
    Name keyName = interest.getName();
    Name identityName = keyName.getPrefix(-2);
    NDNS_LOG_TRACE("validator needs cert of KEY: " << keyName);
    auto cert = m_keyChain.getPib().getIdentity(identityName)
                                   .getKey(keyName)
                                   .getDefaultCertificate();
    m_face.getIoService().post([this, cert] {
        m_face.receive(cert);
      });
  }

public:
  Name m_testId1;
  Name m_testId2;
  Name m_testId3;
  Name m_randomId;

  Name m_rootCert;

  Key m_dsk1;
  Key m_dsk2;
  Key m_dsk3;

  Key m_randomDsk;

  ndn::util::DummyClientFace m_face;
};


BOOST_FIXTURE_TEST_CASE(Basic, Fixture)
{
  // validator must be created after root key is saved to the target
  auto validator = NdnsValidatorBuilder::create(m_face, TEST_CONFIG_PATH "/" "validator.conf");

  // case1: record of testId3, signed by its dsk, should be successful validated.
  Name dataName;
  dataName
    .append(m_testId3)
    .append("NDNS")
    .append("rrLabel")
    .append("rrType")
    .appendVersion();
  shared_ptr<Data> data = make_shared<Data>(dataName);
  m_keyChain.sign(*data, signingByKey(m_dsk3));

  bool hasValidated = false;
  validator->validate(*data,
                     [&] (const Data& data) {
                       hasValidated = true;
                       BOOST_CHECK(true);
                     },
                     [&] (const Data& data, const security::v2::ValidationError& str) {
                       hasValidated = true;
                       BOOST_CHECK(false);
                     });

  m_face.processEvents(time::milliseconds(-1));

  BOOST_CHECK_EQUAL(hasValidated, true);

  // case2: signing testId2's data by testId3's key, which should failed in validation
  dataName = Name();
  dataName
    .append(m_testId2)
    .append("NDNS")
    .append("rrLabel")
    .append("CERT")
    .appendVersion();
  data = make_shared<Data>(dataName);
  m_keyChain.sign(*data, signingByKey(m_dsk3)); // key's owner's name is longer than data owner's

  hasValidated = false;
  validator->validate(*data,
                     [&] (const Data& data) {
                       hasValidated = true;
                       BOOST_CHECK(false);
                     },
                     [&] (const Data& data, const security::v2::ValidationError& str) {
                       hasValidated = true;
                       BOOST_CHECK(true);
                     });

  m_face.processEvents(time::milliseconds(-1));
  // cannot pass verification due to key's owner's name is longer than data owner's
  BOOST_CHECK_EQUAL(hasValidated, true);

  // case4: totally wrong key to sign
  dataName = Name();
  dataName
    .append(m_testId2)
    .append("NDNS")
    .append("rrLabel")
    .append("CERT")
    .appendVersion();
  data = make_shared<Data>(dataName);
  m_keyChain.sign(*data, signingByKey(m_randomDsk));

  hasValidated = false;
  validator->validate(*data,
                     [&] (const Data& data) {
                       hasValidated = true;
                       BOOST_CHECK(false);
                     },
                     [&] (const Data& data, const security::v2::ValidationError& str) {
                       hasValidated = true;
                       BOOST_CHECK(true);
                     });

  m_face.processEvents(time::milliseconds(-1));
  // cannot pass due to a totally mismatched key
  BOOST_CHECK_EQUAL(hasValidated, true);
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace tests
} // namespace ndns
} // namespace ndn
