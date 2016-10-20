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

#include "daemon/name-server.hpp"
#include "logger.hpp"
#include "config.hpp"
#include "daemon/config-file.hpp"
#include "ndn-cxx/security/key-chain.hpp"
#include <boost/program_options.hpp>

namespace ndn {
namespace ndns {

NDNS_LOG_INIT("NdnsDaemon")

/**
 * @brief Name Server Daemon
 * @note NdnsDaemon allows multiple name servers hosted by the same daemon, and they
 * share same KeyChain, DbMgr, Validator and Face
 */
class NdnsDaemon : noncopyable
{
public:
  DEFINE_ERROR(Error, std::runtime_error);

  explicit
  NdnsDaemon(const std::string& configFile, Face& face, Face& validatorFace)
    : m_face(face)
    , m_validatorFace(validatorFace)
  {
    try {
      ConfigFile config;
      NDNS_LOG_INFO("NnsnDaemon ConfigFile = " << configFile);

      config.addSectionHandler("zones",
                               bind(&NdnsDaemon::processZonesSection, this, _1, _3));

      config.parse(configFile, false);

    }
    catch (boost::filesystem::filesystem_error& e) {
      if (e.code() == boost::system::errc::permission_denied) {
        NDNS_LOG_FATAL("Permissions denied for " << e.path1());
      }
      else {
        NDNS_LOG_FATAL(e.what());
      }
    }
    catch (const std::exception& e) {
      NDNS_LOG_FATAL(e.what());
    }
  }

  void
  processZonesSection(const ndn::ndns::ConfigSection& section, const std::string& filename)
  {
    using namespace boost::filesystem;
    using namespace ndn::ndns;
    using ndn::ndns::ConfigSection;

    if (section.begin() == section.end()) {
      throw Error("zones section is empty");
    }

    std::string dbFile = DEFAULT_DATABASE_PATH "/" "ndns.db";
    ConfigSection::const_assoc_iterator item = section.find("dbFile");
    if (item != section.not_found()) {
      dbFile = item->second.get_value<std::string>();
    }
    NDNS_LOG_INFO("DbFile = " << dbFile);
    m_dbMgr = unique_ptr<DbMgr>(new DbMgr(dbFile));

    std::string validatorConfigFile = DEFAULT_CONFIG_PATH "/" "validator.conf";
    item = section.find("validatorConfigFile");
    if (item != section.not_found()) {
      validatorConfigFile = item->second.get_value<std::string>();
    }
    NDNS_LOG_INFO("ValidatorConfigFile = " << validatorConfigFile);
    m_validator = unique_ptr<Validator>(new Validator(m_validatorFace, validatorConfigFile));

    for (const auto& option : section) {
      Name name;
      Name cert;
      if (option.first == "zone") {
        try {
          name = option.second.get<Name>("name"); // exception leads to exit
        }
        catch (const std::exception& e) {
          NDNS_LOG_ERROR("Required `name' attribute missing in `zone' section");
          throw Error("Required `name' attribute missing in `zone' section");
        }
        try {
          cert = option.second.get<Name>("cert");
        }
        catch (std::exception&) {
          ;
        }


        if (!m_keyChain.doesIdentityExist(name)) {
          NDNS_LOG_FATAL("Identity: " << name << " does not exist in the KeyChain");
          throw Error("Identity does not exist in the KeyChain");
        }

        if (cert.empty()) {
          try {
            cert = m_keyChain.getDefaultCertificateNameForIdentity(name);
          }
          catch (std::exception& e) {
            NDNS_LOG_FATAL("Identity: " << name << " does not have default certificate. "
                           << e.what());
            throw Error("identity does not have default certificate");
          }
        }
        else {
          if (!m_keyChain.doesCertificateExist(cert)) {
            throw Error("Certificate `" + cert.toUri() + "` does not exist in the KeyChain");
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
  unique_ptr<Validator> m_validator;
  unique_ptr<DbMgr> m_dbMgr;
  std::vector<shared_ptr<NameServer>> m_servers;
  KeyChain m_keyChain;
};

} // namespace ndns
} // namespace ndn

int
main(int argc, char* argv[])
{
  using std::string;
  using ndn::ndns::ConfigFile;
  using namespace ndn::ndns;

  ndn::ndns::log::init();
  string configFile = DEFAULT_CONFIG_PATH "/" "ndns.conf";

  try {
    namespace po = boost::program_options;
    po::variables_map vm;

    po::options_description generic("Generic Options");
    generic.add_options()("help,h", "print help message");

    po::options_description config("Configuration");
    config.add_options()
      ("config,c", po::value<string>(&configFile), "set the path of configuration file")
      ;

    po::options_description cmdline_options;
    cmdline_options.add(generic).add(config);

    po::parsed_options parsed =
      po::command_line_parser(argc, argv).options(cmdline_options).run();

    po::store(parsed, vm);
    po::notify(vm);

    if (vm.count("help")) {
      std::cout << "Usage:\n"
                << "  ndns-daemon [-c configFile]\n"
                << std::endl;
      std::cout << generic << config << std::endl;
      return 0;
    }
  }
  catch (const std::exception& ex) {
    std::cerr << "Parameter Error: " << ex.what() << std::endl;

    return 1;
  }
  catch (...) {
    std::cerr << "Parameter Unknown error" << std::endl;
    return 1;
  }

  boost::asio::io_service io;
  ndn::Face face(io);
  ndn::Face validatorFace(io);

  try {
    // NFD does not to forward Interests to the face it was received from.
    // If the name server and its validator share same face,
    // the validator cannot be forwarded to the name server itself
    // For current, two faces are used here.

    // refs: http://redmine.named-data.net/issues/2206
    // @TODO enhance validator to get the certificate from the local db if it has

    NdnsDaemon daemon(configFile, face, validatorFace);
    face.processEvents();
  }
  catch (std::exception& e) {
    NDNS_LOG_FATAL("ERROR: " << e.what());
    return 1;
  }

  return 0;
}
