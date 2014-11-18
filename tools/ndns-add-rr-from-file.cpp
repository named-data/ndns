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
#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>
#include <string>


// @todo combine this command with ndns-add-rr
int
main(int argc, char* argv[])
{
  using std::string;
  using namespace ndn;
  using namespace ndns;

  ndn::ndns::log::init();
  int ttlInt = -1;
  string zoneStr;
  string dskStr;
  string db;
  string file = "-";
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
      ("file,f", po::value<string>(&file), "Path to the data. Default is stdin(-)")
      ("dsk,d", po::value<std::string>(&dskStr), "Set the name of DSK's certificate. "
        "Default: use default DSK and its default certificate")
      ("ttl,a", po::value<int>(&ttlInt), "Set ttl of the rrset. Default: 3600 seconds")
      ;

    options.add(config);

    po::options_description hidden("Hidden Options");
    hidden.add_options()
      ("zone", po::value<string>(&zoneStr), "host zone name")
      ;

    po::positional_options_description postion;
    postion.add("zone", 1);
    postion.add("file", 1);

    po::options_description cmdlineOptions;
    cmdlineOptions.add(options).add(hidden);

    // po::options_description config_file_options;
    // config_file_options.add(config).add(hidden);

    po::parsed_options parsed =
      po::command_line_parser(argc, argv).options(cmdlineOptions).positional(postion).run();

    po::store(parsed, vm);
    po::notify(vm);

    if (vm.count("help")) {
      std::cout << "Usage: ndns-add-rr-from-file [-b db] zone [-f file] [-d dskCert] [-a ttl]"
        " [file]" << std::endl
                << std::endl;
      std::cout << options << std::endl;
      return 0;
    }

    if (vm.count("zone") == 0) {
      std::cerr << "Error: zone must be specified" << std::endl;
      return 1;
    }
  }
  catch (const std::exception& ex) {
    std::cerr << "Parameter Error: " << ex.what() << std::endl;
    return 1;
  }

  try {
    Name zoneName(zoneStr);
    Name dskName(dskStr);
    time::seconds ttl;
    if (ttlInt == -1)
      ttl = ndns::DEFAULT_CACHE_TTL;
    else
      ttl = time::seconds(ttlInt);

    ndn::ndns::ManagementTool tool(db);
    tool.addRrSet(zoneName, file, ttl, dskName);
  }
  catch (const std::exception& ex) {
    std::cerr << "Error: " << ex.what() << std::endl;
    return 1;
  }
}
