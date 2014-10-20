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

#include "daemon/name-server.hpp"
#include "logger.hpp"
#include "config.hpp"
#include "daemon/config-file.hpp"
#include "ndn-cxx/security/key-chain.hpp"
#include <boost/program_options.hpp>

namespace ndn {
namespace ndns {

NDNS_LOG_INIT("NdnsDaemon");

/**
 * @brief Name Server Daemon
 * @note NdnsDaemon allows multiple name servers hosted by the same daemon, and they
 * share same KeyChain, DbMgr, Validator and Face
 */
class NdnsDaemon : noncopyable
{
  DEFINE_ERROR(Error, std::runtime_error);

public:
  explicit
  NdnsDaemon(const std::string& configFile, Face& face)
    : m_configFile(configFile)
    , m_face(face)
    , m_validator(face)
  {
    try {
      ConfigFile config;
      NDNS_LOG_TRACE("configFile: " << configFile);

      config.addSectionHandler("zones",
                               bind(&NdnsDaemon::processZonesSection, this, _1, _3));
      config.addSectionHandler("hints",
                               bind(&NdnsDaemon::processHintsSection, this, _1, _3));

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
  processHintsSection(const ndn::ndns::ConfigSection& section, const std::string& filename)
  {
    // hint is not supported yet
    ;
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

    ConfigSection::const_assoc_iterator item = section.find("dbFile");
    if (item != section.not_found()) {
      m_dbFile = item->second.get_value<std::string>();
    }
    else {
      m_dbFile = "/usr/local/var/ndns/ndns.db";
    }
    NDNS_LOG_TRACE("dbFile " << m_dbFile);

    m_dbMgr = make_shared<DbMgr>(m_dbFile);

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

        if (cert.empty()) {
          cert = m_keyChain.getDefaultCertificateNameForIdentity(name);
        }
        else {
          if (!m_keyChain.doesCertificateExist(cert)) {
            throw Error("Certificate `" + cert.toUri() + "` does not exist in the KeyChain");
          }
        }
        NDNS_LOG_TRACE("name = " << name << " cert = " << cert);
        m_servers.push_back(make_shared<NameServer>(name, cert, m_face, *m_dbMgr,
                                                    m_keyChain, m_validator));
      }
    } // for
  }

private:
  std::string m_configFile;
  Face& m_face;
  Validator m_validator;
  std::string m_dbFile;
  shared_ptr<DbMgr> m_dbMgr;
  std::vector<shared_ptr<NameServer> > m_servers;
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

  try {
    ndn::Face face;
    NdnsDaemon nsd(configFile, face);

    boost::asio::signal_set signalSet(face.getIoService(), SIGINT, SIGTERM);

    signalSet.async_wait([&face] (const boost::system::error_code&, const int) {
        face.getIoService().stop();
      });

    face.processEvents();
  }
  catch (std::exception& e) {
    NDNS_LOG_FATAL("ERROR: " << e.what());
    return 2;
  }

  return 0;
}
