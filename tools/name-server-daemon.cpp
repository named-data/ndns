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

#include "app/name-server.hpp"
#include "boost/program_options.hpp"
#include "boost/filesystem.hpp"

int main(int argc, char * argv[])
{
  string route = "/";
  string zoneName = "/";
  string dbfile="src/db/ndns-local.db";
  string hint = "/";

  try{

    namespace po = boost::program_options;
    po::variables_map vm;


    po::options_description generic("Generic Options");
    generic.add_options()
        ("help,h", "print help message")
        ;


    po::options_description config("Configuration");
    config.add_options()
             ("dbfile,f", po::value<std::string>(&dbfile), "set database file. default: src/db/ndns-local.db")
             ("hint,H", po::value<std::string>(&hint), "set Forwarding Hint, which is disable by default")
             ;


    po::options_description hidden("Hidden Options");
    hidden.add_options()
             ("prefix,p", po::value<string>(&route), "routable prefix of the server")
             ("zone,z", po::value<string>(&zoneName), "name of the zone")
            ;

    po::positional_options_description postion;
    postion.add("prefix", 1);
    postion.add("zone", 1);




    po::options_description cmdline_options;
    cmdline_options.add(generic).add(config).add(hidden);

    po::options_description config_file_options;
    config_file_options.add(config).add(hidden);

    po::options_description visible("Allowed options");
    visible.add(generic).add(config);


    po::parsed_options parsed = po::command_line_parser(argc, argv).options(cmdline_options).positional(postion).run();


    po::store(parsed, vm);
    po::notify(vm);

    if (vm.count("help"))
    {
      cout<<"E.g: name-server-daemon /name/of/zone /routable/prefix/announced"<<endl;
      cout<<visible<<endl;
      return 0;
    }

    cout<<"zone="<<zoneName<<" routablePrefix="<<route<<" dbfile="<<dbfile<<endl;
  }
  catch(const std::exception& ex)
  {
          cout << "Parameter Error: " << ex.what() << endl;
          return 0;
  }
  catch(...)
  {
          cout << "Parameter Unknown error" << endl;
          return 0;
  }



	ndn::ndns::NameServer server(argv[0], route.c_str(), zoneName.c_str(), dbfile);

	if (hint != "/") {
	  server.setEnableForwardingHint(1);
	  server.setForwardingHint(Name(hint));
	}


	server.run();

	cout<<"the server ends with hasError="<<server.hasError()<<endl;

	if (server.hasError()){
		return 0;
	} else {
		return 1;
	}

}

