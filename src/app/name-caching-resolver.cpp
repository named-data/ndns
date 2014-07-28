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
#include "name-caching-resolver.hpp"

namespace ndn {
namespace ndns {

NameCachingResolver::NameCachingResolver(const char *programName, const char *prefix)
: NDNApp(programName, prefix)
//, m_resolverType(CachingResolver)
{
  this->setInterestLifetime(time::milliseconds(2000));
}



void
NameCachingResolver::run()
{
  boost::asio::signal_set signalSet(m_ioService, SIGINT, SIGTERM);
  signalSet.async_wait(boost::bind(&NDNApp::signalHandler, this));
 // boost::bind(&NdnTlvPingServer::signalHandler, this)

  Name name(m_prefix);
  name.append(Query::toString(Query::QUERY_DNS_R));

  m_face.setInterestFilter(name,
               bind(&NameCachingResolver::onInterest,
                  this, _1, _2),
               bind(&NDNApp::onRegisterFailed,
                  this, _1,_2));

  std::cout << "\n=== NDNS Resolver "<<m_programName
            <<" with routeble prefix "<< name.toUri()
            << " starts===\n" << std::endl;

  try {
    m_face.processEvents();
  }
  catch (std::exception& e) {
    std::cerr << "ERROR: " << e.what() << std::endl;
    m_hasError = true;
    m_ioService.stop();
  }
}



void
NameCachingResolver::onData(const Interest& interest, Data &data, IterativeQuery& iq)
{
  if (interest.getName() != iq.getLastInterest().getName())
  {
    std::cout<<iq<<std::endl;
    std::cout<<"waiting for "<<iq.getLastInterest().getName().toUri()<<std::endl;
    std::cout<<"coming data "<<data.getName().toUri()<<std::endl;
    return;
  }

  iq.doData(data);

  if (iq.getStep() == IterativeQuery::AnswerStub)
  {
    Data data = iq.getLastResponse().toData();
    Name name = iq.getQuery().getAuthorityZone();
    name.append(Query::toString(iq.getQuery().getQueryType()));
    name.append(iq.getQuery().getRrLabel());
    name.append(RR::toString(iq.getQuery().getRrType()));
    name.appendVersion();
    data.setName(name);
    data.setFreshnessPeriod(iq.getLastResponse().getFreshness());

    m_keyChain.sign(data);
    m_face.put(data);
    std::cout<<"[* <- *] answer Response ("
        <<Response::toString(iq.getLastResponse().getResponseType())<<") to stub:"<<std::endl;
    std::cout<<iq.getLastResponse()<<std::endl;
    for (int i=0; i<15; i++)
    {
      std::cout<<"----";
    }
    std::cout<<std::endl<<std::endl;

  } else if (iq.getStep() == IterativeQuery::NSQuery){
    resolve(iq);
  } else if (iq.getStep() == IterativeQuery::RRQuery) {
    resolve(iq);
  } else if (iq.getStep() == IterativeQuery::Abort) {
    return;
  } else {
    std::cout<<"let me see the current step="<<IterativeQuery::toString(iq.getStep())<<std::endl;
    std::cout<<iq<<std::endl;
  }

}


void
NameCachingResolver::answerRespNack(IterativeQuery& iq)
{
  Response re;

  Name name = iq.getQuery().getAuthorityZone();
  name.append(Query::toString(iq.getQuery().getQueryType()));
  name.append(iq.getQuery().getRrLabel());
  name.append(RR::toString(iq.getQuery().getRrType()));
  name.appendVersion();

  re.setResponseType(Response::NDNS_Nack);
  re.setFreshness(this->getContentFreshness());
  re.setQueryName(name);
  Data data = re.toData();

  m_keyChain.sign(data);
  m_face.put(data);
  std::cout<<"[* <- *] answer RRs to stub:"<<std::endl;
  std::cout<<iq.getLastResponse().getStringRRs()<<std::endl;
  for (int i=0; i<15; i++)
  {
    std::cout<<"----";
  }
  std::cout<<std::endl<<std::endl;
}

void
NameCachingResolver::onInterest(const Name &name, const Interest &interest)
{
  Query query;
  query.fromInterest(interest);
  std::cout<<"[* -> *] receive Interest: "<<interest.getName().toUri()<<std::endl;
  if (query.getQueryType() == Query::QUERY_DNS)
  {
    cout<<m_programName<<" is not in charge of Query_DNS"<<endl;
  }

  IterativeQuery iq(query);
  //iq.setQuery(query);
  resolve(iq);

}


void
NameCachingResolver::onTimeout(const Interest& interest, IterativeQuery& iq)
{
  std::cout<<"[* !! *] timeout Interest "<<interest.getName().toUri()<<" timeouts"<<std::endl;

  iq.doTimeout();
  return;
}

void
NameCachingResolver::resolve(IterativeQuery& iq) {

  Interest interest = iq.toLatestInterest();

  //must be set before express interest,since the call will back call iq
  //if set after, then the iq in call of onData will return nothing
  //be very careful here,  as a new guy to c++

  interest.setInterestLifetime(this->getInterestLifetime());
  iq.setLastInterest(interest);
  try {
    m_face.expressInterest(interest,
            boost::bind(&NameCachingResolver::onData, this, _1, _2, iq),
            boost::bind(&NameCachingResolver::onTimeout, this, _1, iq)
            );
  }catch(std::exception& e) {
    std::cerr << "ERROR: " << e.what() << std::endl;
  }


  std::cout<<"[* <- *] send Interest: "<<interest.getName().toUri()<<std::endl;
  std::cout<<iq<<std::endl;
}

} /* namespace ndns */
} /* namespace ndn */
