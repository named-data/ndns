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

#include "app/name-dig.hpp"
#include "boost/program_options.hpp"
#include "boost/filesystem.hpp"

#include "version.hpp"

using namespace ndn;
using namespace ndn::ndns;
using namespace std;


int main(int argc, char * argv[])
{
  //char *programName, char *dstLabel, char *resolverName
  //const char *programName = argv[0];
  std::string appName = boost::filesystem::basename(argv[0]);

  string dstLabel;
  string resolver;
  int waitingSeconds=10;
  string rrType = "TXT";
  int tryMax = 1;
  try{

    namespace po = boost::program_options;
    po::variables_map vm;


    po::options_description generic("Generic Options");
    generic.add_options()
        ("help,h", "print help message")
        ;


    po::options_description config("Configuration");
    config.add_options()
             ("waiting,w", po::value<int>(&waitingSeconds), "set waiting seconds for every Interest. default: 10")
             ("rrtype,t", po::value<std::string>(&rrType), "set request RR Type. default: TXT")
             ("trynum,n", po::value<int>(&tryMax), "set maximal Interest Tried Number. default: 2")
             ;


    po::options_description hidden("Hidden Options");
    hidden.add_options()
            ("name", po::value<string>(&dstLabel), "name to be resolved")
            ("resolver", po::value<string>(&resolver), "routable prefix of resolver")
            ;

    po::positional_options_description postion;
    postion.add("name", 1);
    postion.add("resolver", 1);



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
      cout<<"E.g: dig /name/to/be/resolved /routable/prefix/of/resolver"<<endl;
      cout<<visible<<endl;
      return 0;
    }

    cout<<"name="<<dstLabel<<" resolver="<<resolver<<" waiting="<<waitingSeconds<<"s RRType="<<rrType<<" tryMax="<<tryMax<<endl;
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


  NameDig dig(argv[0], argv[1]);

  dig.setResolverName(Name(resolver));

  dig.setInterestTriedMax(tryMax);

  dig.setRrType(RR::toRRType(rrType));

  dig.run();

  if (dig.hasError())
  {
    cout<<"\n\n"<<dig.getProgramName()<<" cannot find any records for Name"
        <<dig.getDstLabel().toUri()<<" from resolver "
        <<resolver<<". Due to:"<<std::endl;
    cout<<"Error: "<<dig.getErr()<<endl;
  } else
  {
    Name re = dig.getResponse().getQueryName();
    if (dig.getRrs().size() == 0)
    {
      cout<<"Dig found no record(s) for Name "
          <<dig.getDstLabel().toUri()<<std::endl;
      cout<<"Final Response Name: "<<re.toUri()<<std::endl;
    }
    else {
      cout<<"Success to the dig "<<dig.getDstLabel().toUri()<<" by Resolver "
        <<resolver<<endl;

      vector<RR> rrs = dig.getRrs();
      vector<RR>::const_iterator iter = rrs.begin();
      while (iter != rrs.end())
      {
        cout<<" "<<*iter<<"\n";
        iter ++;
      }
      cout<<"Final Response Name: "<<re.toUri()<<std::endl;
    }
  }

}
