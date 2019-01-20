/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2019, Regents of the University of California.
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

#include "config.hpp"
#include "logger.hpp"
#include "daemon/config-file.hpp"
#include "daemon/name-server.hpp"
#include "util/cert-helper.hpp"

#include <ndn-cxx/face.hpp>
#include <ndn-cxx/security/key-chain.hpp>

#include <boost/asio/io_service.hpp>
#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>

NDNS_LOG_INIT(NdnsDaemon);

namespace ndn {
namespace ndns {

/**
 * @brief Name Server Daemon
 * @note NdnsDaemon allows multiple name servers hosted by the same daemon, and they
 * share same KeyChain, DbMgr, Validator and Face
 */
class NdnsDaemon : noncopyable
{
public:
  DEFINE_ERROR(Error, std::runtime_error);

  NdnsDaemon(const std::string& configFile, Face& face, Face& validatorFace)
    : m_face(face)
    , m_validatorFace(validatorFace)
  {
    NDNS_LOG_INFO("ConfigFile = " << configFile);
    ConfigFile config;
    config.addSectionHandler("zones", bind(&NdnsDaemon::processZonesSection, this, _1));
    config.parse(configFile, false);
  }

  void
  processZonesSection(const ndn::ndns::ConfigSection& section)
  {
    if (section.begin() == section.end()) {
      BOOST_THROW_EXCEPTION(Error("zones section is empty"));
    }

    std::string dbFile = DEFAULT_DATABASE_PATH "/" "ndns.db";
    auto item = section.find("dbFile");
    if (item != section.not_found()) {
      dbFile = item->second.get_value<std::string>();
    }
    NDNS_LOG_INFO("DbFile = " << dbFile);
    m_dbMgr = make_unique<DbMgr>(dbFile);

    std::string validatorConfigFile = DEFAULT_CONFIG_PATH "/" "validator.conf";
    item = section.find("validatorConfigFile");
    if (item != section.not_found()) {
      validatorConfigFile = item->second.get_value<std::string>();
    }
    NDNS_LOG_INFO("ValidatorConfigFile = " << validatorConfigFile);
    m_validator = NdnsValidatorBuilder::create(m_validatorFace, 500, 0, validatorConfigFile);

    for (const auto& option : section) {
      Name name;
      Name cert;
      if (option.first == "zone") {
        try {
          name = option.second.get<Name>("name"); // exception leads to exit
        }
        catch (const std::exception&) {
          NDNS_LOG_ERROR("Required `name' attribute missing in `zone' section");
          BOOST_THROW_EXCEPTION(Error("Required `name' attribute missing in `zone' section"));
        }
        try {
          cert = option.second.get<Name>("cert");
        }
        catch (const std::exception&) {
          // ignore
        }

        if (cert.empty()) {
          try {
            cert = CertHelper::getDefaultCertificateNameOfIdentity(m_keyChain,
                                                                   Name(name)
                                                                   .append(label::NDNS_ITERATIVE_QUERY));
          }
          catch (const std::exception& e) {
            NDNS_LOG_ERROR("Identity " << name << " does not have a default certificate: " << e.what());
            BOOST_THROW_EXCEPTION(Error("identity does not have default certificate"));
          }
        }
        else {
          try {
            CertHelper::getCertificate(m_keyChain, name, cert);
          } catch (const std::exception&) {
            BOOST_THROW_EXCEPTION(Error("Certificate `" + cert.toUri() + "` does not exist in the KeyChain"));
          }
        }
        NDNS_LOG_TRACE("name = " << name << " cert = " << cert);
        m_servers.push_back(make_shared<NameServer>(name, cert, m_face, *m_dbMgr,
                                                    m_keyChain, *m_validator));
      }
    } // for
  }

private:
  Face& m_face;
  Face& m_validatorFace;
  unique_ptr<security::v2::Validator> m_validator;
  unique_ptr<DbMgr> m_dbMgr;
  std::vector<shared_ptr<NameServer>> m_servers;
  KeyChain m_keyChain;
};

} // namespace ndns
} // namespace ndn

int
main(int argc, char* argv[])
{
  std::string configFile(DEFAULT_CONFIG_PATH "/ndns.conf");

  namespace po = boost::program_options;
  po::options_description optsDesc("Options");
  optsDesc.add_options()
    ("help,h",        "print this help message and exit")
    ("config-file,c", po::value<std::string>(&configFile)->default_value(configFile),
                      "path to configuration file")
    ;

  po::variables_map vm;
  try {
    po::store(po::parse_command_line(argc, argv, optsDesc), vm);
    po::notify(vm);
  }
  catch (const po::error& e) {
    std::cerr << "ERROR: " << e.what() << std::endl;
    return 2;
  }
  catch (const boost::bad_any_cast& e) {
    std::cerr << "ERROR: " << e.what() << std::endl;
    return 2;
  }

  if (vm.count("help") != 0) {
    std::cout << "Usage: " << argv[0] << " [options]\n"
              << "\n"
              << optsDesc;
    return 0;
  }

  boost::asio::io_service io;
  ndn::Face face(io);
  ndn::Face validatorFace(io);

  try {
    // NFD does not to forward Interests to the face it was received from.
    // If the name server and its validator share the same face,
    // the validator cannot be forwarded to the name server itself
    // For now, two faces are used here.

    // refs: https://redmine.named-data.net/issues/2206
    // @TODO enhance validator to get the certificate from the local db if present

    ndn::ndns::NdnsDaemon daemon(configFile, face, validatorFace);
    face.processEvents();
  }
  catch (const std::exception& e) {
    NDNS_LOG_FATAL(e.what());
    return 1;
  }

  return 0;
}
