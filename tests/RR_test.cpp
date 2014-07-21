/*
 * RR_test.cpp
 *
 *  Created on: 18 Jul, 2014
 *      Author: shock
 */

#include "../src/RR.h"


int
main(int argc, char *argv[])
{
	ndn::RR rr;
	rr.setRrdata("www2.ex.com");

	ndn::Block block = rr.wireEncode();

	ndn::RR rr2;
	rr2.wireDecode(block);
	cout<<rr.getRrdata()<<endl;

	return 0;
}//main
