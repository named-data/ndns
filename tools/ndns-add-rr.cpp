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

#include "mgmt/management-tool.hpp"
#include "ndns-label.hpp"
#include "logger.hpp"
#include "util/util.hpp"

#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>

#include <string>

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
  string ndnsTypeStr;
  std::vector<std::string> content;
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
      ("content-type,t", po::value<string>(&ndnsTypeStr), "Set the ndnsType of the resource record."
        "Options: resp|nack|auth|raw (will try guess type by default)")
      ("dsk,d", po::value<Name>(&dsk), "Set the name of DSK's certificate. "
        "Default: use default DSK and its default certificate")
      ("content,c", po::value<std::vector<std::string>>(&content),
       "Set the content of resource record. Default: empty string")
      ("ttl,a", po::value<int>(&ttlInt), "Set ttl of the rrset. Default: 3600 seconds")
      ("version,v", po::value<int>(&ttlInt), "Set version of the rrset. Default: Unix Timestamp")
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
      std::cout << "Usage: ndns-add-rr [options] zone label type [content ...]" << std::endl
                << std::endl;
      std::cout << options << std::endl;
      return 0;
    }

    if (vm.count("zone") == 0) {
      std::cerr << "Error: zone, label, and type must be specified" << std::endl;
      return 1;
    }

    if (vm.count("label") == 0) {
      std::cerr << "Error: label and type must be specified" << std::endl;
      return 1;
    }

    if (vm.count("type") == 0) {
      std::cerr << "Error: type must be specified" << std::endl;
      return 1;
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

    if (ndnsTypeStr.empty()) {
      if (rrTypeStr == "NS" || rrTypeStr == "TXT")
        ndnsTypeStr = "resp";
      else if (rrTypeStr == "ID-CERT") {
        ndnsTypeStr = "raw";
      }
      else {
        std::cerr << "Error: content type needs to be explicitly set using --content-type option"
                  << std::endl;
        return 1;
      }
    }
    NdnsType ndnsType = ndns::toNdnsType(ndnsTypeStr);
    time::seconds ttl;
    if (ttlInt == -1)
      ttl = ndns::DEFAULT_CACHE_TTL;
    else
      ttl = time::seconds(ttlInt);
    uint64_t version = static_cast<uint64_t>(versionInt);

    ndn::ndns::ManagementTool tool(db);
    tool.addRrSet(zoneName, label, type, ndnsType, version, content, dsk, ttl);

    /// @todo Report success or failure
    //        May be also show the inserted record in ndns-list-zone format
  }
  catch (const std::exception& ex) {
    std::cerr << "Error: " << ex.what() << std::endl;
    return 1;
  }
}
