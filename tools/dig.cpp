/*
 *  NameServer.cpp
 *
 *  Created on: 18 Jul, 2014
 *      Author: Xiaoke JIANG
 *
 */


#include "app/name-dig.hpp"
#include "boost/program_options.hpp"
#include "boost/filesystem.hpp"

using namespace ndn;
using namespace ndn::ndns;
using namespace std;

void
usage()
{
  cout<<"\n Usage: \n"
      << "dig /name/to/be/resolved [options]\n"
      <<"\t[-t seconds]         - set the maximal waiting time. default: 10\n"
      <<"\t[-p prefix]          - set the routable prefix of caching resolver. default: / \n"
      <<"\t[-n number]          - set the maximal tried time. default: 2\n"
      <<"\t[-r rr_type]         - set the RR type. default: TXT\n"
      <<"\t[-h]                 - set the help message\n"

      <<"\ne.g.: dig /net/ndnsim/www -p /localhost -t 5 -n 2 -r TXT\n"
      ;

}

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

    /*
    po::options_description desc("Generic Options");
    desc.add_options()
        ("help,h", "print help message")
        ("name", po::value<string>(&dstLabel)->required()->composing(), "name to be resolved")
        ("resolver", po::value<string>(&resolver)->composing(), "routable prefix of resolver")
        ("waiting,w", po::value<int>(&waitingSeconds), "set waiting seconds for every Interest. default: 10")
        ("rrtype", po::value<std::string>(), "set request RR Type. default: TXT")
        ("try", po::value<int>(&tryMax), "set maximal Interest Tried Number. default: 2")
        //("positional,p", po::value< vector<string> >(), "positional optionals")
        ;

    */

    po::options_description generic("Generic Options");
    generic.add_options()
        ("help,h", "print help message")
        ;


    po::options_description config("Configuration");
    config.add_options()
             ("waiting,w", po::value<int>(&waitingSeconds), "set waiting seconds for every Interest. default: 10")
             ("rrtype,t", po::value<std::string>(), "set request RR Type. default: TXT")
             ("trynum,n", po::value<int>(&tryMax), "set maximal Interest Tried Number. default: 2")
             ;


    po::options_description hidden("Hidden Options");
    hidden.add_options()
            ("name", po::value<string>(&dstLabel)->required(), "name to be resolved")
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
      cout<<visible<<endl;
      return 0;
    }

    cout<<"waiting="<<waitingSeconds<<"s RRType="<<rrType<<" tryMax="<<tryMax<<endl;
  }
  catch(const std::exception& ex)
  {
          cout << "Parameter Error: " << ex.what() << endl;
  }
  catch(...)
  {
          cout << "Parameter Unknown error" << endl;
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
    if (dig.getRrs().size() == 0)
    {
      cout<<"Dig found no record(s) for Name "
          <<dig.getDstLabel().toUri()<<std::endl;
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

    }
  }

}

