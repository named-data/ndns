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
#include "config.hpp"

#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>
#include <string>

NDNS_LOG_INIT("NdnsCreateZone")

int
main(int argc, char* argv[])
{
  using std::string;
  using namespace ndn;
  ndn::ndns::log::init();
  int cacheTtlInt = -1;
  int certTtlInt = -1;
  string zoneStr;
  string parentStr;
  string dskStr;
  string kskStr;
  string dkeyStr;
  string db;
  try {
    namespace po = boost::program_options;
    po::variables_map vm;

    po::options_description options("Generic Options");
    options.add_options()
      ("help,h", "print help message")
      ("db,b", po::value<std::string>(&db), "Set the path of NDNS server database. "
        "Default: " DEFAULT_DATABASE_PATH "/ndns.db")
      ;

    po::options_description config("Zone Options");
    config.add_options()
      ("cacheTtl,a", po::value<int>(&cacheTtlInt), "Set ttl of records of the zone and its "
        "DSK CERT. Default: 3600 seconds")
      ("certTtl,e", po::value<int>(&certTtlInt), "Set ttl of DSK and KSK certificates. "
        "Default: 365 days")
      ("parent,p", po::value<std::string>(&parentStr), "Set the parent zone of the zone to be "
        "created. Default: the zone's direct parent")
      ("dsk,d", po::value<std::string>(&dskStr), "Set the name of DSK's certificate, "
        "Default: generate new key and certificate")
      ("ksk,k", po::value<std::string>(&kskStr), "Set the name of KSK's certificate, "
        "Default: generate new key and certificate")
      ("dkey,g", po::value<std::string>(&dkeyStr), "Set the name of DKEY's certificate, "
        "Default: generate new key and certificate")
      ;

    options.add(config);

    po::options_description hidden("Hidden Options");
    hidden.add_options()
      ("zone", po::value<string>(&zoneStr), "name of the zone to be created")
      ;
    po::positional_options_description postion;
    postion.add("zone", 1);

    po::options_description cmdline_options;
    cmdline_options.add(options).add(hidden);

    // po::options_description config_file_options;
    // config_file_options.add(config).add(hidden);

    po::parsed_options parsed =
      po::command_line_parser(argc, argv).options(cmdline_options).positional(postion).run();

    po::store(parsed, vm);
    po::notify(vm);

    if (vm.count("help")) {
      std::cout << "Usage: ndns-create-zone [-b db] zone [-a cacheTtl] [-e certTtl] [-p parent] "
        "[-d dskCert] [-k kskCert] [-g dkeyCert]" << std::endl;
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
    Name zone(zoneStr);
    Name parent(parentStr);
    if (!zone.empty() && parentStr.empty())
      parent = zone.getPrefix(-1);

    Name ksk(kskStr);
    Name dsk(dskStr);
    Name dkey(dkeyStr);

    time::seconds cacheTtl;
    time::seconds certTtl;
    if (cacheTtlInt == -1)
      cacheTtl = ndns::DEFAULT_CACHE_TTL;
    else
      cacheTtl = time::seconds(cacheTtlInt);

    if (certTtlInt == -1)
      certTtl = ndns::DEFAULT_CERT_TTL;
    else
      certTtl = time::seconds(certTtlInt);

    KeyChain keyChain;
    ndn::ndns::ManagementTool tool(db, keyChain);
    ndn::ndns::Zone createdZone = tool.createZone(zone, parent, cacheTtl, certTtl, ksk, dsk, dkey);
    ndn::security::v2::Certificate dkeyCert = tool.getZoneDkey(createdZone);
    NDNS_LOG_INFO("Generated DKEY " << dkeyCert.getName());
    ndn::io::save(dkeyCert, std::cout);
  }
  catch (const std::exception& ex) {
    std::cerr << "Error: " << ex.what() << std::endl;
    return 1;
  }
}
