/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2020, Regents of the University of California.
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

#include "mgmt/management-tool.hpp"
#include "daemon/rrset-factory.hpp"
#include "util/cert-helper.hpp"
#include "ndns-enum.hpp"
#include "ndns-label.hpp"
#include "ndns-tlv.hpp"

#include "boost-test.hpp"
#include "key-chain-fixture.hpp"

#include <random>

#include <boost/algorithm/string/replace.hpp>
#include <boost/range/adaptors.hpp>
#if BOOST_VERSION >= 105900
#include <boost/test/tools/output_test_stream.hpp>
#else
#include <boost/test/output_test_stream.hpp>
#endif

#include <ndn-cxx/security/transform.hpp>
#include <ndn-cxx/util/io.hpp>
#include <ndn-cxx/util/regex.hpp>

namespace ndn {
namespace ndns {
namespace tests {

using boost::test_tools::output_test_stream;

const auto TEST_DATABASE = boost::filesystem::path(UNIT_TESTS_TMPDIR) / "management_tool.db";
const auto TEST_CERTDIR = boost::filesystem::path(UNIT_TESTS_TMPDIR) / "management_tool_certs";
const Name FAKE_ROOT("/fake-root/123456789");

/**
 * @brief Recursive copy a directory using Boost Filesystem
 *
 * Based on from http://stackoverflow.com/q/8593608/2150331
 */
static void
copyDir(const boost::filesystem::path& source, const boost::filesystem::path& destination)
{
  namespace fs = boost::filesystem;

  fs::create_directory(destination);
  for (fs::directory_iterator file(source); file != fs::directory_iterator(); ++file) {
    fs::path current(file->path());
    if (is_directory(current)) {
      copyDir(current, destination / current.filename());
    }
    else {
      copy_file(current, destination / current.filename());
    }
  }
}

class TestHome : boost::noncopyable
{
public:
  TestHome()
  {
    if (std::getenv("HOME"))
      m_origHome = std::getenv("HOME");

    auto p = boost::filesystem::path(UNIT_TESTS_TMPDIR) / "tests" / "unit" / "mgmt";
    setenv("HOME", p.c_str(), 1);
    boost::filesystem::remove_all(p);
    boost::filesystem::create_directories(p);
    copyDir("tests/unit/mgmt/.ndn", p / ".ndn");
  }

  ~TestHome()
  {
    if (!m_origHome.empty())
      setenv("HOME", m_origHome.data(), 1);
    else
      unsetenv("HOME");
  }

protected:
  std::string m_origHome;
};

class ManagementToolFixture : public TestHome, public KeyChainFixture
{
public:
  class Error : public std::runtime_error
  {
  public:
    using std::runtime_error::runtime_error;
  };

  class PreviousStateCleaner
  {
  public:
    PreviousStateCleaner()
    {
      boost::filesystem::remove(TEST_DATABASE);
      boost::filesystem::remove_all(TEST_CERTDIR);
    }
  };

  ManagementToolFixture()
    : m_tool(TEST_DATABASE.string().c_str(), m_keyChain)
    , m_dbMgr(TEST_DATABASE.string().c_str())
  {
    boost::filesystem::create_directory(TEST_CERTDIR);
    Identity root = m_keyChain.createIdentity("NDNS");
    Key ksk = root.getDefaultKey();
    m_keyChain.deleteCertificate(ksk, ksk.getDefaultCertificate().getName());
    Certificate kskCert = CertHelper::createCertificate(m_keyChain, ksk, ksk, "CERT");
    m_keyChain.addCertificate(ksk, kskCert);
    rootKsk = kskCert.getName();

    Key dsk = m_keyChain.createKey(root);
    // replace rootDsk's default cert with ksk-signing cert
    m_keyChain.deleteCertificate(dsk, dsk.getDefaultCertificate().getName());
    Certificate dskCert = CertHelper::createCertificate(m_keyChain, dsk, ksk, "CERT");
    m_keyChain.addCertificate(dsk, dskCert);
    rootDsk = dskCert.getName();

    Identity other = m_keyChain.createIdentity("/ndns-test/NDNS");
    Key otherKskKey = other.getDefaultKey();
    m_keyChain.deleteCertificate(otherKskKey, otherKskKey.getDefaultCertificate().getName());
    Certificate otherKskCert = CertHelper::createCertificate(m_keyChain, otherKskKey, otherKskKey, "CERT");
    m_keyChain.addCertificate(otherKskKey, otherKskCert);
    otherKsk  = otherKskCert.getName();

    // replace rootDsk's default cert with ksk-signing cert
    Key otherDskKey = m_keyChain.createKey(other);
    m_keyChain.deleteCertificate(otherDskKey, otherDskKey.getDefaultCertificate().getName());
    Certificate otherDskCert = CertHelper::createCertificate(m_keyChain, otherDskKey, otherKskKey, "CERT");
    m_keyChain.addCertificate(otherDskKey, otherDskCert);
    otherDsk = otherDskCert.getName();

    Certificate rootDkeyCert = CertHelper::createCertificate(m_keyChain, otherDskKey, otherKskKey, "CERT");
    m_keyChain.addCertificate(otherDskKey, rootDkeyCert);
    rootDkey = rootDkeyCert.getName();
  }

  ~ManagementToolFixture()
  {
  }

  std::vector<Certificate>
  getCerts(const Name& zoneName)
  {
    Zone zone(zoneName);
    std::vector<Certificate> certs;
    std::map<std::string, Block> zoneInfo = m_dbMgr.getZoneInfo(zone);
    // ksk are always the first key
    certs.push_back(Certificate(zoneInfo["ksk"]));
    certs.push_back(Certificate(zoneInfo["dsk"]));
    return certs;
  }

  Rrset
  findRrSet(Zone& zone, const Name& label, const name::Component& type)
  {
    Rrset rrset(&zone);
    rrset.setLabel(label);
    rrset.setType(type);

    if (!m_dbMgr.find(rrset))
      BOOST_THROW_EXCEPTION(Error("Record not found"));
    else
      return rrset;
  }

  Name
  getLabel(const Zone& zone, const Name& fullName)
  {
    size_t zoneNameSize = zone.getName().size();
    return fullName.getSubName(zoneNameSize + 1, fullName.size() - zoneNameSize - 3);
  }

  Certificate
  findCertFromIdentity(const Name& identityName, const Name& certName)
  {
    Certificate rtn;
    Identity identity = CertHelper::getIdentity(m_keyChain, identityName);
    for (const auto& key : identity.getKeys()) {
      for (const auto& cert : key.getCertificates()) {
        if (cert.getName() == certName) {
          rtn = cert;
          return rtn;
        }
      }
    }
    BOOST_THROW_EXCEPTION(Error("Certificate not found in keyChain"));
    return rtn;
  }

  Certificate
  findCertFromDb(Zone& zone, const Name& fullName)
  {
    Rrset rrset = findRrSet(zone, getLabel(zone, fullName), label::CERT_RR_TYPE);
    Certificate cert;
    cert.wireDecode(rrset.getData());
    return cert;
  }

  Certificate
  findDkeyFromDb(const Name& zoneName)
  {
    Zone zone(zoneName);
    std::map<std::string, Block> zoneInfo = m_dbMgr.getZoneInfo(zone);
    return Certificate(zoneInfo["dkey"]);
  }

  Response
  findResponse(Zone& zone, const Name& label, const name::Component& type)
  {
    Rrset rrset = findRrSet(zone, label, type);
    Data data(rrset.getData());
    Response resp;
    resp.fromData(zone.getName(), data);
    return resp;
  }

public:
  PreviousStateCleaner cleaner; // must be first variable
  ndns::ManagementTool m_tool;
  ndns::DbMgr m_dbMgr;

  // Names of pre-created certificates
  // Uncomment and run InitPreconfiguredKeys test case and then update names in the
  // constructor.
  Name rootKsk;
  Name rootDsk;
  Name otherKsk;
  Name otherDsk;
  Name rootDkey;
};

BOOST_FIXTURE_TEST_SUITE(ManagementTool, ManagementToolFixture)

// BOOST_FIXTURE_TEST_CASE(InitPreconfiguredKeys, ManagementToolFixture)
// {
//   using time::seconds;

//   auto generateCerts = [this] (const Name& zone, const Name& parentCert = Name()) -> Name {
//     // to re-generate certificates, uncomment and then update rootKsk/rootDsk names
//     Name kskName = m_keyChain.generateRsaKeyPair(zone, true);
//     auto kskCert = m_keyChain
//       .prepareUnsignedIdentityCertificate(kskName, zone, time::fromUnixTimestamp(seconds(0)),
//                                           time::fromUnixTimestamp(seconds(2147483648)), {});
//     if (parentCert.empty()) {
//       m_keyChain.selfSign(*kskCert);
//     }
//     else {
//       m_keyChain.sign(*kskCert, parentCert);
//     }
//     m_keyChain.addCertificate(*kskCert);

//     Name dskName = m_keyChain.generateRsaKeyPair(zone, false);
//     auto dskCert = m_keyChain
//       .prepareUnsignedIdentityCertificate(dskName, zone, time::fromUnixTimestamp(seconds(0)),
//                                           time::fromUnixTimestamp(seconds(2147483648)), {});
//     m_keyChain.sign(*dskCert, kskCert->getName());
//     m_keyChain.addCertificate(*dskCert);

//     return dskCert->getName();
//   };

//   Name rootDsk = generateCerts(ROOT_ZONE);
//   generateCerts("/ndns-test", rootDsk);

//   copyDir(UNIT_TESTS_TMPDIR "/tests/unit/mgmt/.ndn", "/tmp/.ndn");
//   std::cout << "Manually copy contents of /tmp/.ndn into tests/unit/mgmt/.ndn" << std::endl;
// }

BOOST_AUTO_TEST_CASE(SqliteLabelOrder)
{
  // the correctness of our DoE design rely on the ordering of SQLite
  // this unit test make sure that our label::isSmallerInLabelOrder
  // is the same as the ordering of BLOB in SQLite

  std::random_device seed;
  std::mt19937 gen(seed());

  auto genRandomString = [&] (int length) -> std::string {
    std::string charset =
    "0123456789"
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz";
    std::uniform_int_distribution<size_t> dist(0, charset.size() - 1);

    std::string str(length, 0);
    std::generate_n(str.begin(), length, [&] { return charset[dist(gen)];} );
    return str;
  };

  auto genRandomLabel = [&]() -> Name {
    std::uniform_int_distribution<size_t> numberOfLabelsDist(1, 5);
    std::uniform_int_distribution<size_t> labelSizeDist(1, 10);
    Name nm;
    size_t length = numberOfLabelsDist(gen);
    for (size_t i = 0; i < length; i++) {
      nm.append(genRandomString(labelSizeDist(gen)));
    }
    return nm;
  };

  const size_t NGENERATED_LABELS = 10;

  Zone zone = m_tool.createZone("/net/ndnsim", "/net");
  RrsetFactory rrsetFactory(TEST_DATABASE.string(), zone.getName(),
                            m_keyChain, DEFAULT_CERT);
  rrsetFactory.checkZoneKey();
  std::vector<Rrset> rrsets;
  std::vector<Name> names;
  for (size_t i = 0; i < NGENERATED_LABELS; i++) {
    Name randomLabel = genRandomLabel();
    Rrset randomTxt = rrsetFactory.generateTxtRrset(randomLabel,
                                                    VERSION_USE_UNIX_TIMESTAMP,
                                                    DEFAULT_CACHE_TTL,
                                                    {});

    rrsets.push_back(randomTxt);
    m_dbMgr.insert(randomTxt);
    names.push_back(randomLabel);
  }

  std::sort(rrsets.begin(), rrsets.end());

  using boost::adaptors::filtered;
  using boost::adaptors::transformed;

  std::vector<Rrset> rrsetsFromDb = m_dbMgr.findRrsets(zone);
  auto fromDbFiltered = rrsetsFromDb | filtered([] (const Rrset& rrset) {
      return rrset.getType() == label::TXT_RR_TYPE;
    });

  auto extractLabel = [] (const Rrset& rr) {
    return rr.getLabel();
  };
  auto expected = rrsets | transformed(extractLabel);
  auto actual = fromDbFiltered | transformed(extractLabel);

  BOOST_CHECK_EQUAL_COLLECTIONS(expected.begin(), expected.end(),
                                actual.begin(), actual.end());
}

BOOST_AUTO_TEST_CASE(CreateDeleteRootFixture)
{
  // creating root_zone need a rootDkey
  BOOST_CHECK_THROW(m_tool.createZone(ROOT_ZONE, ROOT_ZONE,
                                      time::seconds(4600),
                                      time::seconds(4600),
                                      rootKsk, rootDsk), ndns::ManagementTool::Error);

  m_tool.createZone(ROOT_ZONE, ROOT_ZONE,
                    time::seconds(4600),
                    time::seconds(4600),
                    rootKsk, rootDsk, rootDkey);

  Zone zone(ROOT_ZONE);
  Name zoneIdentityName = Name(ROOT_ZONE).append("NDNS");
  BOOST_REQUIRE_EQUAL(m_dbMgr.find(zone), true);
  BOOST_REQUIRE_NO_THROW(findCertFromDb(zone, rootDsk));
  BOOST_CHECK_EQUAL(findCertFromDb(zone, rootDsk).getName(), rootDsk);
  BOOST_CHECK_EQUAL(findCertFromDb(zone, rootKsk).getName(), rootKsk);
  BOOST_CHECK_EQUAL(findDkeyFromDb(ROOT_ZONE).getName(), rootDkey);

  BOOST_CHECK_EQUAL(findCertFromIdentity(zoneIdentityName, rootDsk).getName(), rootDsk);
  BOOST_CHECK_EQUAL(findCertFromIdentity(zoneIdentityName, rootKsk).getName(), rootKsk);

  BOOST_CHECK_NO_THROW(m_tool.deleteZone(ROOT_ZONE));
  BOOST_CHECK_EQUAL(m_dbMgr.find(zone), false);
}

BOOST_AUTO_TEST_CASE(CreateDeleteChildFixture)
{
  Name parentZoneName("/ndns-test");
  Name zoneName = Name(parentZoneName).append("child-zone");

  Zone zone1(zoneName);
  Name zoneIdentityName = Name(zoneName).append(label::NDNS_ITERATIVE_QUERY);
  BOOST_REQUIRE_EQUAL(m_dbMgr.find(zone1), false);

  // will generate keys automatically
  m_tool.createZone(zoneName, parentZoneName);
  BOOST_CHECK_EQUAL(CertHelper::doesIdentityExist(m_keyChain, zoneIdentityName), true);

  std::vector<Certificate>&& certs = getCerts(zoneName);
  BOOST_REQUIRE_EQUAL(certs.size(), 2);

  const Name& ksk = certs[0].getName();
  const Name& dsk = certs[1].getName();

  Zone zone(zoneName);
  BOOST_REQUIRE_EQUAL(m_dbMgr.find(zone), true);
  BOOST_REQUIRE_NO_THROW(findCertFromDb(zone, dsk));
  BOOST_CHECK_EQUAL(findCertFromDb(zone, dsk).getName(), dsk);
  BOOST_CHECK_EQUAL(findCertFromDb(zone, ksk).getName(), ksk);

  BOOST_CHECK_EQUAL(findCertFromIdentity(zoneIdentityName, dsk), findCertFromDb(zone, dsk));
  BOOST_CHECK_EQUAL(findCertFromIdentity(zoneIdentityName, ksk), findCertFromDb(zone, ksk));

  BOOST_CHECK_NO_THROW(m_tool.deleteZone(zoneName));

  BOOST_CHECK_THROW(m_tool.deleteZone(zoneName), ndns::ManagementTool::Error);
  BOOST_CHECK_THROW(m_tool.deleteZone("/non/existing/zone"), ndns::ManagementTool::Error);
}

BOOST_AUTO_TEST_CASE(CreateZoneWithFixture)
{
  Name parentZoneName("/ndns-test");
  Name zoneName = Name(parentZoneName).append("child-zone");
  Name zoneIdentityName = Name(zoneName).append(label::NDNS_ITERATIVE_QUERY);

  m_tool.createZone(zoneName, parentZoneName, time::seconds(4200), time::days(30));
  BOOST_CHECK_EQUAL(CertHelper::doesIdentityExist(m_keyChain, zoneIdentityName), true);

  std::vector<Certificate>&& certs = getCerts(zoneName);
  BOOST_REQUIRE_EQUAL(certs.size(), 2);

  const Name& dsk = certs[1].getName();

  // Check zone ttl
  Zone zone(zoneName);
  BOOST_REQUIRE_EQUAL(m_dbMgr.find(zone), true);
  BOOST_CHECK_EQUAL(zone.getTtl(), time::seconds(4200));

  // check dkey name
  Name dkeyName = Name(parentZoneName).append("NDNS").append(zoneName.getSubName(parentZoneName.size()));
  Certificate dkey = findDkeyFromDb(zoneName);
  BOOST_CHECK(dkeyName.isPrefixOf(dkey.getName()));

  // TODO: check signing hierarchy

  // Check dsk rrset ttl
  Rrset rrset;
  BOOST_REQUIRE_NO_THROW(rrset = findRrSet(zone, getLabel(zone, dsk), label::CERT_RR_TYPE));
  BOOST_CHECK_EQUAL(rrset.getTtl(), time::seconds(4200));

  // Check certificate freshnessPeriod and validity
  Certificate cert = CertHelper::getCertificate(m_keyChain, zoneIdentityName, dsk);
  time::system_clock::TimePoint beg,end;
  std::tie(beg, end) = cert.getValidityPeriod().getPeriod();

  BOOST_REQUIRE_NO_THROW(cert = findCertFromDb(zone, dsk));
  BOOST_CHECK_EQUAL(cert.getFreshnessPeriod(), time::seconds(4200));
  BOOST_CHECK_EQUAL(end - beg, time::days(30));
  m_tool.deleteZone(zoneName);
}

BOOST_AUTO_TEST_CASE(ZoneCreatePreconditions)
{
  BOOST_CHECK_NO_THROW(m_tool.createZone("/net/ndnsim", "/net"));
  BOOST_CHECK_THROW(m_tool.createZone("/net/ndnsim", "/net"), ndns::ManagementTool::Error);

  std::vector<Certificate>&& certs = getCerts("/net/ndnsim");
  BOOST_REQUIRE_EQUAL(certs.size(), 2);

  const Name& ksk = certs[0].getName();
  const Name& dsk = certs[1].getName();

  m_tool.deleteZone("/net/ndnsim");
  // identity will still exist after the zone is deleted

  BOOST_CHECK_THROW(m_tool.createZone("/net/ndnsim", "/net/ndnsim"), ndns::ManagementTool::Error);

  BOOST_CHECK_THROW(m_tool.createZone("/net/ndnsim", "/com"), ndns::ManagementTool::Error);

  BOOST_CHECK_NO_THROW(m_tool.createZone("/net/ndnsim", "/",
                                         time::seconds(1), time::days(1), ksk, dsk));
  BOOST_CHECK_EQUAL(getCerts("/net/ndnsim").size(), 2);
  m_tool.deleteZone("/net/ndnsim");

  BOOST_CHECK_NO_THROW(m_tool.createZone("/net/ndnsim", "/",
                                         time::seconds(1), time::days(1), Name(), dsk));

  m_tool.deleteZone("/net/ndnsim");

  BOOST_CHECK_NO_THROW(m_tool.createZone("/net/ndnsim", "/",
                                         time::seconds(1), time::days(1), ksk, Name()));
  m_tool.deleteZone("/net/ndnsim");

  BOOST_CHECK_THROW(m_tool.createZone("/net/ndnsim", "/net",
                                      time::seconds(1), time::days(1), "/com/ndnsim"),
                                      ndns::ManagementTool::Error);

  Identity id = m_keyChain.createIdentity("/net/ndnsim/NDNS");
  Certificate cert = id.getDefaultKey().getDefaultCertificate();
  BOOST_CHECK_NO_THROW(m_tool.createZone("/net/ndnsim", "/net",
                                         time::seconds(1), time::days(1), cert.getName()));

  id = m_keyChain.createIdentity("/com/ndnsim/NDNS");
  cert = id.getDefaultKey().getDefaultCertificate();

  BOOST_CHECK_THROW(m_tool.createZone("/net/ndnsim", "/net",
                                      time::seconds(1), time::days(1), cert.getName()),
                    ndns::ManagementTool::Error);

  id = m_keyChain.createIdentity("/net/ndnsim/www/NDNS");
  cert = id.getDefaultKey().getDefaultCertificate();
  BOOST_CHECK_THROW(m_tool.createZone("/net/ndnsim", "/net",
                                      time::seconds(1), time::days(1), cert.getName()),
                    ndns::ManagementTool::Error);

  id = m_keyChain.createIdentity("/net/ndnsim/NDNS");
  cert = id.getDefaultKey().getDefaultCertificate();
  m_keyChain.deleteCertificate(id.getDefaultKey(), cert.getName());
  BOOST_CHECK_THROW(m_tool.createZone("/net/ndnsim", "/net",
                                      time::seconds(1), time::days(1), cert.getName()),
                    ndns::ManagementTool::Error);

  // for root zone special case (requires a valid DKEY to be specified)
  BOOST_CHECK_THROW(m_tool.createZone("/", "/"), ndns::ManagementTool::Error);

  BOOST_CHECK_NO_THROW(m_tool.createZone("/", "/", time::seconds(1), time::days(1),
                                         DEFAULT_CERT, DEFAULT_CERT, rootDkey));
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

// BOOST_AUTO_TEST_CASE(ExportCertificate)
// {
//   std::string outputFile = TEST_CERTDIR.string() + "/ss.cert";

//   BOOST_REQUIRE_THROW(m_tool.exportCertificate("/random/name", outputFile),
//                       ndns::ManagementTool::Error);

//   BOOST_REQUIRE_EQUAL(boost::filesystem::exists(outputFile), false);
//   // doesn't check the zone, export from KeyChain directly
//   BOOST_CHECK_NO_THROW(m_tool.exportCertificate(otherDsk, outputFile));
//   BOOST_REQUIRE_EQUAL(boost::filesystem::exists(outputFile), true);

//   std::string dskValue =
//     "Bv0C3Ac3CAluZG5zLXRlc3QIA0tFWQgRZHNrLTE0MTY5NzQwMDY2NTkIB0lELUNF\n"
//     "UlQICf0AAAFJ6jt6DhQDGAECFf0BYTCCAV0wIhgPMTk3MDAxMDEwMDAwMDBaGA8y\n"
//     "MDM4MDExOTAzMTQwOFowEzARBgNVBCkTCi9uZG5zLXRlc3QwggEgMA0GCSqGSIb3\n"
//     "DQEBAQUAA4IBDQAwggEIAoIBAQDIFUL7Fz8mmxxIT8l3FtWm+CuH9+iQ0Uj/a30P\n"
//     "mKe4gWvtxzhb4vIngYbXGv2iUzHswdqYlTVeDdW6eOFKMvyY5p5eVtLqDFZ7EEK0\n"
//     "0rpTh648HjCSz+Awgp2nbiYAAVvhP6YF+NxGBH412uPI7kLY6ozypsNmYP+K4SYT\n"
//     "oY9ee4xLSjqzXfLMyP1h8OHcN/aNmccRJlyYblCmCDbZPnzu3ttHHwdrYQLeFvb0\n"
//     "B5grCAQoPHwkfxkEnzQBA/fbUdvKNdayEkuibPLlIlmj2cBtk5iVk8JCSibP3Zlz\n"
//     "36Sks1DAO+1EvCRnjoH5vYmkpMUBFue+6A40IQG4brM2CiIRAgERFjMbAQEcLgcs\n"
//     "CAluZG5zLXRlc3QIA0tFWQgRa3NrLTE0MTY5NzQwMDY1NzcIB0lELUNFUlQX/QEA\n"
//     "GP2bQqp/7rfb8tShwDbXihWrPojwEFqlfwLibK9aM1RxwpHVqbtRsPYmuWc87LaU\n"
//     "OztPOZinHGL80ypFC+wYadVGnE8MPdTkUYUik7mbHDEsYWADoyGMVhoZv+OTJ/5m\n"
//     "MUh/kR1FMiqtZcIQtLB3cdCeGlZBl9wm2SvhMKVUym3RsQO46RpnmsEQcCfWMBZg\n"
//     "u5U6mhYIpiQPZ/sYyZ9zXstwsIfaF1p0V+1dW5y99PZJXIegVKhkGGU0ibjYoJy7\n"
//     "6uUjqBBDX8KMdt6n/Zy1/pGG1eOchMyV0JZ8+MJxWuiTEh5PJeYMFHTV/BVp8aPy\n"
//     "8UNqhMpjAZwW6pdvOZADVg==\n";

//   {
//     std::ifstream ifs(outputFile.c_str());
//     std::string actualValue((std::istreambuf_iterator<char>(ifs)),
//                             std::istreambuf_iterator<char>());
//     BOOST_CHECK_EQUAL(actualValue, dskValue);
//   }
//   boost::filesystem::remove(outputFile);

//   // doesn't check the zone, export from KeyChain directly
//   BOOST_CHECK_NO_THROW(m_tool.exportCertificate(otherKsk, outputFile));
//   boost::filesystem::remove(outputFile);

//   Name zoneName("/ndns-test");
//   m_tool.createZone(zoneName, ROOT_ZONE, time::seconds(4200), time::days(30),
//                     otherKsk, otherDsk);

//   m_keyChain.deleteCertificate(otherKsk);
//   m_keyChain.deleteCertificate(otherDsk);

//   // retrieve cert from the zone
//   BOOST_CHECK_NO_THROW(m_tool.exportCertificate(otherDsk, outputFile));
//   {
//     std::ifstream ifs(outputFile.c_str());
//     std::string actualValue((std::istreambuf_iterator<char>(ifs)),
//                             std::istreambuf_iterator<char>());
//     BOOST_CHECK_EQUAL(actualValue, dskValue);
//   }
//   boost::filesystem::remove(outputFile);

//   BOOST_REQUIRE_THROW(m_tool.exportCertificate(otherKsk, outputFile),
//                       ndns::ManagementTool::Error);

//   // output to std::cout
//   std::string acutalOutput;
//   {
//     OutputTester tester;
//     m_tool.exportCertificate(otherDsk, "-");
//     acutalOutput = tester.buffer.str();
//   }
//   BOOST_CHECK_EQUAL(acutalOutput, dskValue);
// }

BOOST_AUTO_TEST_CASE(AddRrset)
{
  Name zoneName("/ndns-test");
  Zone zone(zoneName);

  time::seconds ttl1(4200);
  time::seconds ttl2(4500);
  m_tool.createZone(zoneName, ROOT_ZONE, ttl1);

  RrsetFactory rf(TEST_DATABASE, zoneName, m_keyChain, DEFAULT_CERT);
  rf.checkZoneKey();
  Rrset rrset1 = rf.generateNsRrset("/l1", 7654, ttl2, DelegationList());

  BOOST_CHECK_NO_THROW(m_tool.addRrset(rrset1));
  Rrset rrset2 = findRrSet(zone, "/l1", label::NS_RR_TYPE);
  BOOST_CHECK_EQUAL(rrset1, rrset2);
}

BOOST_AUTO_TEST_CASE(AddMultiLevelLabelRrset)
{
  Name zoneName("/ndns-test");
  Zone zone(zoneName);

  time::seconds ttl(4200);
  m_tool.createZone(zoneName, ROOT_ZONE, ttl);

  RrsetFactory rf(TEST_DATABASE, zoneName, m_keyChain, DEFAULT_CERT);
  rf.checkZoneKey();

  auto checkRrset = [&zone, &zoneName, this](Name label,
                                             name::Component type,
                                             NdnsContentType contentType) -> void {
    Rrset rr1 = findRrSet(zone, label, type);
    BOOST_CHECK_EQUAL(Data(rr1.getData()).getContentType(), contentType);
    Response response1;
    response1.fromData(zoneName, Data(rr1.getData()));
    BOOST_CHECK_EQUAL(response1.getRrLabel(), label);
  };

  Name labelName("/l1/l2/l3");

  Rrset rrset1 = rf.generateNsRrset(labelName, 7654, ttl, DelegationList());

  //add NS NDNS_AUTH and check user-defined ttl
  BOOST_CHECK_NO_THROW(m_tool.addMultiLevelLabelRrset(rrset1, rf, ttl));
  Rrset rrset2 = findRrSet(zone, labelName, label::NS_RR_TYPE);
  BOOST_CHECK_EQUAL(rrset1, rrset2);

  checkRrset("/l1", label::NS_RR_TYPE, ndns::NDNS_AUTH);
  checkRrset("/l1/l2", label::NS_RR_TYPE, ndns::NDNS_AUTH);

  // insert a same-name rrset with TXT type
  Rrset txtRr = rf.generateTxtRrset("/l1/l2/l3", 7654, ttl, std::vector<std::string>());
  BOOST_CHECK_NO_THROW(m_tool.addMultiLevelLabelRrset(txtRr, rf, ttl));

  checkRrset("/l1", label::NS_RR_TYPE, ndns::NDNS_AUTH);
  checkRrset("/l1/l2", label::NS_RR_TYPE, ndns::NDNS_AUTH);
  checkRrset("/l1/l2/l3", label::TXT_RR_TYPE, ndns::NDNS_RESP);
  // check that there is no confliction
  checkRrset("/l1/l2/l3", label::NS_RR_TYPE, ndns::NDNS_LINK);

  // insert a shorter NS, when there are longer NS or TXT
  Rrset shorterNs = rf.generateNsRrset("/l1/l2", 7654, ttl, DelegationList());
  BOOST_CHECK_THROW(m_tool.addMultiLevelLabelRrset(shorterNs, rf, ttl),
                    ndns::ManagementTool::Error);

  // insert a longer NS, when there is already a shorter NS
  Rrset longerNs = rf.generateNsRrset("/l1/l2/l3/l4", 7654, ttl, DelegationList());
  BOOST_CHECK_THROW(m_tool.addMultiLevelLabelRrset(longerNs, rf, ttl),
                    ndns::ManagementTool::Error);

  // insert a smaller TXT, when there are longer NS and TXT
  Rrset shorterTxt = rf.generateTxtRrset("/l1/l2", 7654, ttl, std::vector<std::string>());
  BOOST_CHECK_NO_THROW(m_tool.addMultiLevelLabelRrset(shorterTxt, rf, ttl));

  // insert a smaller NS, when there is long TXT
  Rrset longTxt = rf.generateTxtRrset("/k1/k2/k3", 7654, ttl, std::vector<std::string>());
  Rrset smallerNs = rf.generateNsRrset("/k1/k2", 7654, ttl, DelegationList());
  BOOST_CHECK_NO_THROW(m_tool.addMultiLevelLabelRrset(longTxt, rf, ttl));
  BOOST_CHECK_THROW(m_tool.addMultiLevelLabelRrset(smallerNs, rf, ttl),
                    ndns::ManagementTool::Error);

  // inserting a longer TXT, when there is shoter TXT
  Rrset longerTxt = rf.generateTxtRrset("/k1/k2/k3/k4", 7654, ttl, std::vector<std::string>());
  BOOST_CHECK_NO_THROW(m_tool.addMultiLevelLabelRrset(longerTxt, rf, ttl));
}

BOOST_AUTO_TEST_CASE(AddRrSetDskCertPreConditon)
{
  // check pre-condition
  Name zoneName("/ndns-test");

  // Check: throw if zone not exist
  std::string certPath = TEST_CERTDIR.string();
  BOOST_CHECK_THROW(m_tool.addRrsetFromFile(zoneName, certPath), ndns::ManagementTool::Error);

  m_tool.createZone(zoneName, ROOT_ZONE);

  // Check: throw if certificate does not match
  BOOST_CHECK_THROW(m_tool.addRrsetFromFile(zoneName, certPath), ndns::ManagementTool::Error);

  std::string rightCertPath = TEST_CERTDIR.string() + "/ss.cert";
  std::vector<Certificate>&& certs = getCerts(zoneName);
  const Name& ksk = certs[0].getName();
  m_tool.exportCertificate(ksk, rightCertPath);

  // Check: throw if it's a duplicated certificate
  BOOST_CHECK_THROW(m_tool.addRrsetFromFile(zoneName, rightCertPath), ndns::ManagementTool::Error);
}

BOOST_AUTO_TEST_CASE(AddRrSetDskCert)
{
  Name parentZoneName("/ndns-test");
  Name zoneName("/ndns-test/child-zone");
  Name zoneIdentityName = Name(zoneName).append(label::NDNS_ITERATIVE_QUERY);

  m_tool.createZone(parentZoneName, ROOT_ZONE, time::seconds(1), time::days(1), otherKsk, otherDsk);
  m_tool.createZone(zoneName, parentZoneName);

  Zone zone(zoneName);
  Zone parentZone(parentZoneName);

  Certificate dkey(findDkeyFromDb(zone.getName()));
  std::string output = TEST_CERTDIR.string() + "/ss.cert";
  ndn::io::save(dkey, output);

  BOOST_CHECK_NO_THROW(m_tool.addRrsetFromFile(parentZoneName, output));
  // Check if child zone's d-key could be inserted correctly
  BOOST_CHECK_NO_THROW(findRrSet(parentZone, getLabel(parentZone, dkey.getName()), label::CERT_RR_TYPE));
}

BOOST_AUTO_TEST_CASE(AddRrSetDskCertUserProvidedCert)
{
  //check using user provided certificate
  Name parentZoneName("/ndns-test");
  Name parentZoneIdentityName = Name(parentZoneName).append(label::NDNS_ITERATIVE_QUERY);
  Name zoneName("/ndns-test/child-zone");
  Name zoneIdentityName = Name(zoneName).append(label::NDNS_ITERATIVE_QUERY);

  // Name dskName = m_keyChain.generateRsaKeyPair(parentZoneName, false);
  Identity id = CertHelper::getIdentity(m_keyChain, parentZoneIdentityName);
  Key dsk = m_keyChain.createKey(id);
  Certificate dskCert = dsk.getDefaultCertificate();

  // check addRrsetFromFile1
  m_tool.createZone(parentZoneName, ROOT_ZONE, time::seconds(1), time::days(1), otherKsk, otherDsk);
  m_tool.createZone(zoneName, parentZoneName);

  Certificate dkey(findDkeyFromDb(zoneName));
  std::string output = TEST_CERTDIR.string() + "/ss.cert";
  ndn::io::save(dkey, output);

  BOOST_CHECK_NO_THROW(m_tool.addRrsetFromFile(parentZoneName, output, time::seconds(4600),
                                               dskCert.getName()));
}

BOOST_AUTO_TEST_CASE(AddRrSetDskCertInvalidOutput)
{
  //check invalid output
  Name parentZoneName("/ndns-test");
  Name zoneName = Name(parentZoneName).append("child-zone");
  m_tool.createZone(zoneName, parentZoneName);

  Name content = "invalid data packet";
  std::string output = TEST_CERTDIR.string() + "/ss.cert";
  ndn::io::save(content, output);

  BOOST_CHECK_THROW(m_tool.addRrsetFromFile(zoneName, output), ndns::ManagementTool::Error);
}

BOOST_AUTO_TEST_CASE(AddRrSetVersionControl)
{
  //check version control
  time::seconds ttl(4200);
  Name parentZoneName("/ndns-test");
  Name zoneName = Name(parentZoneName).append("child-zone");
  m_tool.createZone(zoneName, parentZoneName);

  Name label("/label");
  uint64_t version = 110;

  RrsetFactory rf(TEST_DATABASE, zoneName, m_keyChain, DEFAULT_CERT);
  rf.checkZoneKey();

  Rrset rrset1 = rf.generateTxtRrset(label, version, ttl, {});

  m_tool.addRrset(rrset1);
  // throw error when adding duplicated rrset with the same version
  BOOST_CHECK_THROW(m_tool.addRrset(rrset1),
                    ndns::ManagementTool::Error);
  version--;
  Rrset rrset2 = rf.generateTxtRrset(label, version, ttl, {});
  // throw error when adding duplicated rrset with older version
  BOOST_CHECK_THROW(m_tool.addRrset(rrset2),
                    ndns::ManagementTool::Error);

  version++;
  version++;
  Rrset rrset3 = rf.generateTxtRrset(label, version, ttl, {});
  BOOST_CHECK_NO_THROW(m_tool.addRrset(rrset3));

  Zone zone(zoneName);
  m_dbMgr.find(zone);
  Rrset rrset;
  rrset.setZone(&zone);
  rrset.setLabel(label);
  rrset.setType(label::TXT_RR_TYPE);
  m_dbMgr.find(rrset);

  BOOST_CHECK_EQUAL(rrset.getVersion(), name::Component::fromVersion(version));
}

BOOST_AUTO_TEST_CASE(AddRrSetDskCertFormat)
{
  //check input with different formats
  Name parentZoneName("/ndns-test");
  Name zoneName = Name(parentZoneName).append("child-zone");
  Zone parentZone(parentZoneName);

  m_tool.createZone(parentZoneName, ROOT_ZONE, time::seconds(1), time::days(1), otherKsk, otherDsk);
  m_tool.createZone(zoneName, parentZoneName);

  Certificate cert(findDkeyFromDb(zoneName));
  std::string output = TEST_CERTDIR.string() + "/a.cert";

  Name parentZoneIdentityName = Name(parentZoneName).append(label::NDNS_ITERATIVE_QUERY);

  // base64
  ndn::io::save(cert, output, ndn::io::BASE64);
  BOOST_CHECK_NO_THROW(
    m_tool.addRrsetFromFile(parentZoneName, output, DEFAULT_CACHE_TTL, DEFAULT_CERT, ndn::io::BASE64));
  m_tool.removeRrSet(parentZoneName, getLabel(parentZone, cert.getName()), label::CERT_RR_TYPE);

  // raw
  ndn::io::save(cert, output, ndn::io::NO_ENCODING);
  BOOST_CHECK_NO_THROW(
    m_tool.addRrsetFromFile(parentZoneName, output, DEFAULT_CACHE_TTL, DEFAULT_CERT, ndn::io::NO_ENCODING));
  m_tool.removeRrSet(parentZoneName, getLabel(parentZone, cert.getName()), label::CERT_RR_TYPE);

  // hex
  ndn::io::save(cert, output, ndn::io::HEX);
  BOOST_CHECK_NO_THROW(
    m_tool.addRrsetFromFile(parentZoneName, output, DEFAULT_CACHE_TTL, DEFAULT_CERT, ndn::io::HEX));
  m_tool.removeRrSet(parentZoneName, getLabel(parentZone, cert.getName()), label::CERT_RR_TYPE);

  // incorrect encoding input
  ndn::io::save(cert, output, ndn::io::HEX);
  BOOST_CHECK_THROW(
    m_tool.addRrsetFromFile(parentZoneName, output, DEFAULT_CACHE_TTL, DEFAULT_CERT,
                            static_cast<ndn::io::IoEncoding>(127)),
    ndns::ManagementTool::Error);
}

BOOST_AUTO_TEST_CASE(ListAllZones)
{
  m_tool.createZone(ROOT_ZONE, ROOT_ZONE, time::seconds(1), time::days(1), rootKsk, rootDsk, rootDkey);
  m_tool.createZone("/ndns-test", ROOT_ZONE, time::seconds(10), time::days(1), otherKsk, otherDsk);

  Name rootDskName = CertHelper::getCertificate(m_keyChain, "/NDNS/", rootDsk).getKeyName();
  Name otherDskName = CertHelper::getCertificate(m_keyChain, "/ndns-test/NDNS/", otherDsk).getKeyName();

  std::string expectedValue =
  "/           ; default-ttl=1 default-key=" + rootDskName.toUri() +  " "
  "default-certificate=" + rootDsk.toUri() + "\n"
  "/ndns-test  ; default-ttl=10 default-key=" + otherDskName.toUri() +  " "
  "default-certificate=" + otherDsk.toUri() + "\n";

  output_test_stream testOutput;
  m_tool.listAllZones(testOutput);
  BOOST_CHECK(testOutput.is_equal(expectedValue));
}

// Test need to fix values of keys, otherwise it produces different values every time

// BOOST_AUTO_TEST_CASE(ListZone)
// {
//   m_tool.createZone("/ndns-test", ROOT_ZONE, time::seconds(10), time::days(1), otherKsk, otherDsk);

//   RrsetFactory rf(TEST_DATABASE, "/ndns-test", m_keyChain, DEFAULT_CERT);
//   rf.checkZoneKey();

//   // Add NS with NDNS_RESP
//   Delegation del;
//   del.preference = 10;
//   del.name = Name("/get/link");
//   DelegationList ds = {del};
//   Rrset rrset1 = rf.generateNsRrset("/label1", 100, DEFAULT_RR_TTL, ds);
//   m_tool.addRrset(rrset1);

//   // Add NS with NDNS_AUTH
//   Rrset rrset2 = rf.generateAuthRrset("/label2", 100000, DEFAULT_RR_TTL);
//   m_tool.addRrset(rrset2);

//   // Add TXT from file
//   std::string output = TEST_CERTDIR.string() + "/a.rrset";
//   Response re1;
//   re1.setZone("/ndns-test");
//   re1.setQueryType(label::NDNS_ITERATIVE_QUERY);
//   re1.setRrLabel("/label2");
//   re1.setRrType(label::TXT_RR_TYPE);
//   re1.setContentType(NDNS_RESP);
//   re1.setVersion(name::Component::fromVersion(654321));
//   re1.addRr("First RR");
//   re1.addRr("Second RR");
//   re1.addRr("Last RR");
//   shared_ptr<Data> data1 = re1.toData();
//   m_keyChain.sign(*data1, security::signingByCertificate(otherDsk));
//   ndn::io::save(*data1, output);
//   m_tool.addRrsetFromFile("/ndns-test", output);

//   // Add TXT in normal way
//   Rrset rrset3 = rf.generateTxtRrset("/label3", 3333, DEFAULT_RR_TTL, {"Hello", "World"});
//   m_tool.addRrset(rrset3);

//   m_tool.listZone("/ndns-test", std::cout, true);

//   output_test_stream testOutput;
//   m_tool.listZone("/ndns-test", testOutput, true);

//   std::string expectedValue =
//     R"VALUE(; Zone /ndns-test

// ; rrset=/ type=DOE version=%FD%00%00%01b%26e%AE%99 signed-by=/ndns-test/NDNS/KEY/%12%05t%90%3AjvA
// /                                 3600  DOE   Bw0IBmxhYmVsMwgDVFhU
// /                                 3600  DOE   BxUIA0tFWQgIEgV0kDpqdkEIBENFUlQ=

// /KEY/%12%05t%90%3AjvA             10    CERT  ; content-type=KEY version=%FD%00%00%01b%26e%AEY signed-by=/ndns-test/NDNS/KEY/%8D%1Dj%1E%BE%B0%2A%E4
// Certificate name:
//   /ndns-test/NDNS/KEY/%12%05t%90%3AjvA/CERT/%FD%00%00%01b%26e%AEY
// Validity:
//   NotBefore: 20180314T212340
//   NotAfter: 20180324T212340
// Public key bits:
//   MIIBSzCCAQMGByqGSM49AgEwgfcCAQEwLAYHKoZIzj0BAQIhAP////8AAAABAAAA
//   AAAAAAAAAAAA////////////////MFsEIP////8AAAABAAAAAAAAAAAAAAAA////
//   ///////////8BCBaxjXYqjqT57PrvVV2mIa8ZR0GsMxTsPY7zjw+J9JgSwMVAMSd
//   NgiG5wSTamZ44ROdJreBn36QBEEEaxfR8uEsQkf4vOblY6RA8ncDfYEt6zOg9KE5
//   RdiYwpZP40Li/hp/m47n60p8D54WK84zV2sxXs7LtkBoN79R9QIhAP////8AAAAA
//   //////////+85vqtpxeehPO5ysL8YyVRAgEBA0IABHU62fbCa6KR7G1iyMr6/NtF
//   5oHrAdzttIgh5pk1VS1YcFO1zhpUnpJS43FlduYHVBLrXwYS6tZ15Ge/D3uy1f4=
// Signature Information:
//   Signature Type: SignatureSha256WithEcdsa
//   Key Locator: Name=/ndns-test/NDNS/KEY/%8D%1Dj%1E%BE%B0%2A%E4

// ; rrset=/KEY/%12%05t%90%3AjvA/CERT type=DOE version=%FD%00%00%01b%26e%AE%91 signed-by=/ndns-test/NDNS/KEY/%12%05t%90%3AjvA
// /KEY/%12%05t%90%3AjvA/CERT        3600  DOE   BxUIA0tFWQgIEgV0kDpqdkEIBENFUlQ=
// /KEY/%12%05t%90%3AjvA/CERT        3600  DOE   BxUIA0tFWQgIjR1qHr6wKuQIBENFUlQ=

// /KEY/%8D%1Dj%1E%BE%B0%2A%E4       10    CERT  ; content-type=KEY version=%FD%00%00%01b%26e%AEX signed-by=/ndns-test/NDNS/KEY/%8D%1Dj%1E%BE%B0%2A%E4
// Certificate name:
//   /ndns-test/NDNS/KEY/%8D%1Dj%1E%BE%B0%2A%E4/CERT/%FD%00%00%01b%26e%AEX
// Validity:
//   NotBefore: 20180314T212340
//   NotAfter: 20180324T212340
// Public key bits:
//   MIIBSzCCAQMGByqGSM49AgEwgfcCAQEwLAYHKoZIzj0BAQIhAP////8AAAABAAAA
//   AAAAAAAAAAAA////////////////MFsEIP////8AAAABAAAAAAAAAAAAAAAA////
//   ///////////8BCBaxjXYqjqT57PrvVV2mIa8ZR0GsMxTsPY7zjw+J9JgSwMVAMSd
//   NgiG5wSTamZ44ROdJreBn36QBEEEaxfR8uEsQkf4vOblY6RA8ncDfYEt6zOg9KE5
//   RdiYwpZP40Li/hp/m47n60p8D54WK84zV2sxXs7LtkBoN79R9QIhAP////8AAAAA
//   //////////+85vqtpxeehPO5ysL8YyVRAgEBA0IABMRVD/FUfCQVvjcwQLe9k1aS
//   5pZ/xmFndOHn1+a0OYVzxCV1JcxL1eojcij42tCP5mtocrj9DjYyFBv4Atg1RZE=
// Signature Information:
//   Signature Type: SignatureSha256WithEcdsa
//   Key Locator: Self-Signed Name=/ndns-test/NDNS/KEY/%8D%1Dj%1E%BE%B0%2A%E4

// ; rrset=/KEY/%8D%1Dj%1E%BE%B0%2A%E4/CERT type=DOE version=%FD%00%00%01b%26e%AE%93 signed-by=/ndns-test/NDNS/KEY/%12%05t%90%3AjvA
// /KEY/%8D%1Dj%1E%BE%B0%2A%E4/CERT  3600  DOE   BxUIA0tFWQgIjR1qHr6wKuQIBENFUlQ=
// /KEY/%8D%1Dj%1E%BE%B0%2A%E4/CERT  3600  DOE   BwwIBmxhYmVsMQgCTlM=

// ; rrset=/label1 type=NS version=%FDd signed-by=/ndns-test/NDNS/KEY/%12%05t%90%3AjvA
// /label1                           10    NS    10

// ; rrset=/label1/NS type=DOE version=%FD%00%00%01b%26e%AE%94 signed-by=/ndns-test/NDNS/KEY/%12%05t%90%3AjvA
// /label1/NS                        3600  DOE   BwwIBmxhYmVsMQgCTlM=
// /label1/NS                        3600  DOE   BwwIBmxhYmVsMggCTlM=

// ; rrset=/label2 type=NS version=%FD%00%01%86%A0 signed-by=/ndns-test/NDNS/KEY/%12%05t%90%3AjvA
// /label2                           10    NS    NDNS-Auth

// ; rrset=/label2 type=TXT version=%FD%00%09%FB%F1 signed-by=/ndns-test/NDNS/KEY/%12%05t%90%3AjvA
// /label2                           10    TXT   First RR
// /label2                           10    TXT   Second RR
// /label2                           10    TXT   Last RR

// ; rrset=/label2/NS type=DOE version=%FD%00%00%01b%26e%AE%96 signed-by=/ndns-test/NDNS/KEY/%12%05t%90%3AjvA
// /label2/NS                        3600  DOE   BwwIBmxhYmVsMggCTlM=
// /label2/NS                        3600  DOE   Bw0IBmxhYmVsMggDVFhU

// ; rrset=/label2/TXT type=DOE version=%FD%00%00%01b%26e%AE%97 signed-by=/ndns-test/NDNS/KEY/%12%05t%90%3AjvA
// /label2/TXT                       3600  DOE   Bw0IBmxhYmVsMggDVFhU
// /label2/TXT                       3600  DOE   Bw0IBmxhYmVsMwgDVFhU

// ; rrset=/label3 type=TXT version=%FD%0D%05 signed-by=/ndns-test/NDNS/KEY/%12%05t%90%3AjvA
// /label3                           10    TXT   Hello
// /label3                           10    TXT   World

// ; rrset=/label3/TXT type=DOE version=%FD%00%00%01b%26e%AE%98 signed-by=/ndns-test/NDNS/KEY/%12%05t%90%3AjvA
// /label3/TXT                       3600  DOE   Bw0IBmxhYmVsMwgDVFhU
// /label3/TXT                       3600  DOE   BxUIA0tFWQgIEgV0kDpqdkEIBENFUlQ=

// )VALUE";

//   BOOST_CHECK(testOutput.is_equal(expectedValue));
// }

BOOST_FIXTURE_TEST_CASE(GetRrSet, ManagementToolFixture)
{
  Name zoneName("/ndns-test");
  m_tool.createZone(zoneName, ROOT_ZONE, time::seconds(1), time::days(1), otherKsk, otherDsk);
  RrsetFactory rf(TEST_DATABASE, zoneName, m_keyChain, DEFAULT_CERT);
  rf.checkZoneKey();
  Rrset rrset1 = rf.generateTxtRrset("/label", 100, DEFAULT_RR_TTL, {"Value1", "Value2"});

  m_tool.addRrset(rrset1);

  std::stringstream os;

  using security::transform::base64Encode;
  using security::transform::streamSink;
  using security::transform::bufferSource;

  bufferSource(rrset1.getData().wire(), rrset1.getData().size()) >> base64Encode() >> streamSink(os);

  std::string expectedValue = os.str();

  output_test_stream testOutput;
  m_tool.getRrSet(zoneName, "/label",label::TXT_RR_TYPE, testOutput);
  BOOST_CHECK(testOutput.is_equal(expectedValue));
}

BOOST_FIXTURE_TEST_CASE(RemoveRrSet, ManagementToolFixture)
{
  Name zoneName("/ndns-test");

  m_tool.createZone(zoneName, ROOT_ZONE);
  RrsetFactory rf(TEST_DATABASE, zoneName, m_keyChain, DEFAULT_CERT);
  rf.checkZoneKey();

  Rrset rrset1 = rf.generateTxtRrset("/label", 100, DEFAULT_RR_TTL, {});

  BOOST_CHECK_NO_THROW(m_tool.addRrset(rrset1));

  Zone zone(zoneName);
  BOOST_CHECK_NO_THROW(findRrSet(zone, "/label", label::TXT_RR_TYPE));

  BOOST_CHECK_NO_THROW(m_tool.removeRrSet(zoneName, "/label", label::NS_RR_TYPE));

  BOOST_CHECK_THROW(findRrSet(zone, "/label", label::NS_RR_TYPE), Error);
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace tests
} // namespace ndns
} // namespace ndn
