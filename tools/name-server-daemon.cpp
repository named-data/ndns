/*
 *  NameServer.cpp
 *
 *  Created on: 18 Jul, 2014
 *      Author: Xiaoke JIANG
 *
 */


#include "app/name-server.hpp"

int main(int argc, char * argv[])
{
  //char *programName, char *prefix, char *nameZone

  if (argc != 3)
  {
    return 0;
  }

	ndn::ndns::NameServer server(argv[0], argv[1], argv[2]);
	server.run();

	cout<<"the server ends with hasError="<<server.hasError()<<endl;

	if (server.hasError()){
		return 0;
	} else {
		return 1;
	}

}

