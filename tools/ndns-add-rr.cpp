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

#include "mgmt/management-tool.hpp"
#include "ndns-label.hpp"
#include "logger.hpp"
#include "util/util.hpp"
#include "daemon/rrset-factory.hpp"

#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>

#include <string>

NDNS_LOG_INIT("AddRrTool")

int
main(int argc, char* argv[])
{
  using std::string;
  using namespace ndn;
  using namespace ndns;

  ndn::ndns::log::init();
  int ttlInt = -1;
  int versionInt = -1;
  string zoneStr;
  Name dsk;
  string db;
  string rrLabelStr;
  string rrTypeStr;
  std::vector<std::string> content;
  string file = "-";
  string encoding = "base64";
  bool setFile = false;
  bool needResign = true;
  try {
    namespace po = boost::program_options;
    po::variables_map vm;

    po::options_description options("Generic Options");
    options.add_options()
      ("help,h", "print help message")
      ("db,b", po::value<std::string>(&db), "Set the path of NDNS server database. "
       "Default: " DEFAULT_DATABASE_PATH "/ndns.db")
      ;

    po::options_description config("Record Options");
    config.add_options()
      ("dsk,d", po::value<Name>(&dsk), "Set the name of DSK's certificate. "
       "Default: use default DSK and its default certificate")
      ("content,c", po::value<std::vector<std::string>>(&content),
       "Set the content of resource record. Default: empty string")
      ("ttl,a", po::value<int>(&ttlInt), "Set ttl of the rrset. Default: 3600 seconds")
      ("version,v", po::value<int>(&ttlInt), "Set version of the rrset. Default: Unix Timestamp")
      ("file,f", po::value<string>(&file), "Set path to file containing a rrset. If set, label, "
       "type, content-type, content, and version parameters will be ignored. Default is stdin(-)")
      ("encoding,e", po::value<string>(&encoding),
       "Set encoding format of input file. Default: base64")
      ("resign,r", po::value<bool>(&needResign), "Resign the input with DSK. Default is true")
      ;

    // add "Record Options" as a separate section
    options.add(config);

    po::options_description hidden("Hidden Options");
    hidden.add_options()
      ("zone", po::value<string>(&zoneStr), "host zone name")
      ("label", po::value<string>(&rrLabelStr), "label of resource record.")
      ("type", po::value<string>(&rrTypeStr), "Set the type of resource record.")
      ;

    po::positional_options_description positional;
    positional.add("zone", 1);
    positional.add("label", 1);
    positional.add("type", 1);
    positional.add("content", -1);

    po::options_description cmdlineOptions;
    cmdlineOptions.add(options).add(hidden);

    // po::options_description configFileOptions;
    // configFileOptions.add(config).add(hidden);

    po::parsed_options parsed =
      po::command_line_parser(argc, argv).options(cmdlineOptions).positional(positional).run();

    po::store(parsed, vm);
    po::notify(vm);

    if (vm.count("help")) {
      std::cout << "Usage: ndns-add-rr [options] zone label type [content ...]" << std::endl;
      std::cout << "       ndns-add-rr [options] zone [-f file] [-e raw|base64|hex]" << std::endl
                << std::endl;
      std::cout << options << std::endl;
      return 0;
    }

    if (vm.count("zone") == 0) {
      std::cerr << "Error: zone, label, and type must be specified" << std::endl;
      return 1;
    }

    if (vm.count("file") == 0) {
      if (vm.count("label") == 0) {
        std::cerr << "Error: label and type must be specified" << std::endl;
        return 1;
      }

      if (vm.count("type") == 0) {
        std::cerr << "Error: type must be specified" << std::endl;
        return 1;
      }
    }
    else {
      if (vm.count("resign"))  {
        needResign = true;
      }
      setFile = true;
    }
  }
  catch (const std::exception& ex) {
    std::cerr << "Parameter Error: " << ex.what() << std::endl;
    return 1;
  }

  try {
    Name zoneName(zoneStr);
    Name label(rrLabelStr);
    name::Component type(rrTypeStr);
    KeyChain keyChain;

    time::seconds ttl;
    if (ttlInt == -1)
      ttl = ndns::DEFAULT_CACHE_TTL;
    else
      ttl = time::seconds(ttlInt);
    uint64_t version = static_cast<uint64_t>(versionInt);

    if (setFile) {
      ndn::io::IoEncoding ioEncoding;
      if (encoding == "raw") {
        ioEncoding = ndn::io::NO_ENCODING;
      }
      else if (encoding == "hex") {
        ioEncoding = ndn::io::HEX;
      }
      else if (encoding == "base64") {
        ioEncoding = ndn::io::BASE64;
      }
      else {
        std::cerr << "Error: not supported encoding format '" << encoding
                  << "' (valid options are: raw, hex, and base64)" << std::endl;
        return 1;
      }
      ndn::ndns::ManagementTool tool(db, keyChain);
      tool.addRrsetFromFile(zoneName, file, ttl, dsk, ioEncoding, needResign);
    }
    else {
      RrsetFactory rrsetFactory(db, zoneName, keyChain, dsk);
      rrsetFactory.checkZoneKey();
      Rrset rrset;

      if (type == label::NS_RR_TYPE) {
        ndn::DelegationList delegations;
        for (const auto& i : content) {
          std::vector<string> data;
          boost::split(data, i, boost::is_any_of(","));
          uint64_t priority = boost::lexical_cast<uint64_t>(data[0]);
          delegations.insert(priority, Name(data[1]));
        }

        rrset = rrsetFactory.generateNsRrset(label, type,
                                             version, ttl, delegations);
      }
      else if (type == label::TXT_RR_TYPE) {
        rrset = rrsetFactory.generateTxtRrset(label, type,
                                              version, ttl, content);
      }

      ndn::ndns::ManagementTool tool(db, keyChain);

      if (label.size() > 1) {
        NDNS_LOG_TRACE("add multi-level label Rrset, using the same TTL as the Rrset");
        tool.addMultiLevelLabelRrset(rrset, rrsetFactory, ttl);
      }
      else {
        tool.addRrset(rrset);
      }
    }

    /// @todo Report success or failure
    //        May be also show the inserted record in ndns-list-zone format
  }
  catch (const std::exception& ex) {
    std::cerr << "Error: " << ex.what() << std::endl;
    return 1;
  }
}
