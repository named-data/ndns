/*
 *  NameServer.cpp
 *
 *  Created on: 18 Jul, 2014
 *      Author: Xiaoke JIANG
 *
 */

#include <boost/asio.hpp>
#include <boost/noncopyable.hpp>

#include <ndn-cxx/face.hpp>
#include <ndn-cxx/name.hpp>
#include <ndn-cxx/security/key-chain.hpp>



using namespace std;
using namespace ndn;


namespace ndn{
class NameServer : boost::noncopyable
{
public:
explicit
NameServer(char *programName)
: m_programName(programName)
, m_hasError(false)
, m_freshnessPeriod(time::milliseconds(1000))
, m_face(m_ioService)
{

}//NameServer Construction


void
onInterest(const Name &name, const Interest &interest)
{

	cout<<"-> Interest: "<<name<<std::endl;

}
void
onData(const ndn::Interest& interest, Data& data)
{
	Name dName = data.getName();
	if (not m_name.isPrefixOf(dName) )
	{
		cout<<"!! ILLEGAL data: "<<dName<<", which does not starts with "<<m_name<<endl;
		return;
	}

	cout<<"-> Data: "<<dName<<endl;

	Name iName = interest.getName();
	unsigned long seq = (unsigned long)iName.get(iName.size()-1).toNumber();


	this->tryExpress();

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
onRegisterFailed(const ndn::Name& prefix, const std::string& reason)
{
  std::cerr << "ERROR: Failed to register prefix in local hub's daemon" << std::endl;
  std::cerr << "REASON: " << reason << std::endl;
  m_hasError = true;
  this->stop();
}

void
DoExpress()
{
	Name name = this->m_name;


	Selectors selector;
	selector.setMustBeFresh(true);

	Interest interest = Interest(name, selector, -1, this->m_freshnessPeriod, 1);

	m_face.expressInterest(interest, boost::bind(&NameServer::onData, this, _1, _2),
			boost::bind(&NameServer::onTimeout, this, _1));


	//m_face.expressInterest(interest, boost::bind(&MyPing::OnData, this, _1, _2),
	//		boost::bind(&MyPing::OnTimeout, this, _1));
}

void
tryExpress()
{
	this->DoExpress();
}

void
singalHandler()
{
	cout<<"Fail to Register"<<endl;
	this->stop();
	exit(1);
}

void
stop()
{
	cout<<"program "<<this->m_programName<<" stops"<<endl;
	this->m_face.shutdown();
	this->m_ioService.stop();
}




void
run()
{
	std::cout << "\n=== NDNS Server for Zone " << m_prefix <<" starts===\n" << std::endl;

	boost::asio::signal_set signalSet(m_ioService, SIGINT, SIGTERM);
	signalSet.async_wait(bind(&NameServer::singalHandler, this));

	m_name.set(m_prefix);

	m_face.setInterestFilter(m_name,
							 bind(&NameServer::onInterest,
								  this, _1, _2),
							 bind(&NameServer::onRegisterFailed,
								  this, _1,_2));
	try {
	  m_face.processEvents();
	}
	catch (std::exception& e) {
	  std::cerr << "ERROR: " << e.what() << std::endl;
	  m_hasError = true;
	  m_ioService.stop();
	}

}

	bool hasError() const {
		return m_hasError;
	}


public:
	KeyChain m_keyChain;
	bool m_hasError;

	time::milliseconds m_freshnessPeriod;
	char* m_programName;
	char* m_prefix;
	Name m_name;
	boost::asio::io_service m_ioService;
	Face m_face;

};//clcass NameServer
}//namespace ndn


int main(int argc, char * argv[])
{
	NameServer server;
	server.run();

	cout<<"the server ends with hasError="<<server.hasError()<<endl;

	if (server.hasError()){
		return 0;
	} else {
		return 1;
	}

}

