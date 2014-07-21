/*
 * myping.cc
 *
 *  Created on: 21 Jun, 2014
 *      Author: shock
 */

#include <boost/noncopyable.hpp>
#include <boost/asio.hpp>
#include <ndn-cxx/face.hpp>
#include <ndn-cxx/name.hpp>
#include <ndn-cxx/interest.hpp>
#include <ndn-cxx/selectors.hpp>

#include <query.h>
#include <rr.h>

#include <unistd.h>
#include <stdlib.h> //atoi



#include <map>
#include <vector>
#include <sstream>

//using std::cout;
//using std::cin;
//using std::cerr;
//using std::endl;
//using std::map;
//using std::vector;
//using std::exception;
//using std::pair;
//using std::string;

using namespace std;

namespace ndn{


class Resolver : boost::noncopyable
{

	class RequestInfo
	{
	public:
		RequestInfo(unsigned long seq)
		: m_seq(seq)
		{
		}

		void
		SendInterest()
		{
			//m_sentT = time::system_clock::now();
		}
		void
		GetData()
		{
			//this->m_receT = time::system_clock::now();
		}

		string
		toString()
		{
			stringstream str;
			str<<"m_seq="<<m_seq;
			//s +="m_seq="+m_seq+" m_sentT="+" m_receT=";
			return str.str();
		}

	public:
		unsigned long m_seq;
		time::milliseconds m_sentT;
		time::milliseconds m_receT;
	};

public:
Resolver(char *programName, char *scope, char *domain)
: m_programName (programName)
, m_authorityZone (scope)
, m_rrLabel(domain)
, m_interestLifetime (time::milliseconds(4000))
, m_face (m_ioService)
{
	cout<<"Create "<<m_programName<<endl;
}

void
onInterest(const Name &name, const Interest &interest)
{

	cout<<"-> Interest: "<<name<<std::endl;

}
void
onData(const ndn::Interest& interest, Data& data)
{
	Response response(data);
	response
}
void
onTimeout(const ndn::Interest& interest)
{
	cout<<"!- Interest Timeout"<<interest.getName()<<endl;
	Name iName = interest.getName();
	unsigned long seq = (unsigned long)iName.get(iName.size()-1).toNumber();

	this->tryExpress();


}


void
doExpress(Interest interest)
{
	m_face.expressInterest(interest, boost::bind(&onData, this, _1, _2),
			boost::bind(&onTimeout, this, _1));
}

void
tryExpress()
{

	this->doExpress();

}


void
singalHandler()
{
	this->stop();
}

void
stop()
{
	cout<<"resolve stops"<<endl;
	this->m_face.shutdown();
	this->m_ioService.stop();
}

void
run()
{

	boost::asio::signal_set signalSet(m_ioService, SIGINT, SIGTERM);
	signalSet.async_wait(boost::bind(&singalHandler, this));

	this->m_query.setAuthorityZone(this->m_authorityZone);
	this->m_query.setRrLabel(this->m_rrLabel);
	this->m_query.setRrType(RRType::NS);

	Interest interest = m_query.toWire();
	this->doExpress(interest);

	try{
		m_face.processEvents();
	} catch (exception &e){
		cerr<<"Error: "<<e.what()<<endl;
		this->stop();
	}

}


void
resolve()
{

}


private:
	char *m_programName;
	Name m_authorityZone;
	Name m_rrLabel;
	//Name m_name;
	time::milliseconds m_interestLifetime;


	Query m_query;

	boost::asio::io_service m_ioService;
	//This line MUST be before m_face declaration, or leads to segment error: 11

	Face m_face;


};//class MyPing

}//namespace


void
usage()
{
	cout<<"-h to show the hint"<<endl;
}

int
main(int argc, char *argv[])
{

	if (argc < 2)
	{
		cout<<"You must have at least one parameter: resolve domain [scope]"<<endl;
		return 0;
	}


	char * domain = argv[1];
	char * scope;
	if (argc == 3)
		scope = argv[2];
	else
		scope = "/";


	ndn::Resolver resolver(argv[0], scope, domain);
	resolver.run();

	return 0;
}//main


