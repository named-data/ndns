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
#include "name-server.hpp"

namespace ndn {
namespace ndns {
NameServer::NameServer(const char *programName, const char *prefix,
    const char *nameZone, const string dbfile)
  : NDNApp(programName, prefix)
  , m_zone(Name(nameZone))
  , m_zoneMgr(m_zone)
{
  m_zoneMgr.setDbfile(dbfile);
  //m_zoneMgr.lookupId();
} //NameServer Construction

void NameServer::onInterest(const Name &name, const Interest &interest)
{

  cout << "[* -> *] receive Interest: " << interest.getName().toUri()
      << std::endl;
  Query query;
  if (!query.fromInterest(interest)) {
    cout << "can resolve the Query from Interest: " << endl;
    return;
  }

  /*
   * query.getAuthorityZone is routable name, not the zone's service name
   if (query.getAuthorityZone() != m_zoneMgr.getZone().getAuthorizedName())
   {
   cout<<"Query is intent to zone: "<<query.getAuthorityZone()
   <<". This is "<<m_zoneMgr.getZone().getAuthorizedName()<<endl;
   return;
   }
   */

  Response response;
  Name name2 = interest.getName();
  name2.appendVersion();
  response.setQueryName(name2);
  RRMgr mgr(m_zone, query, response);

  if (mgr.lookup() < 0) {
    cout << "[* !! *] lookup error, then exit: " << mgr.getErr() << endl;
    return;
  }

  if (response.getRrs().size() > 0) {
    response.setResponseType(Response::NDNS_Resp);
  } else {

    if (query.getRrType() == RR::NS) {
      int count = mgr.count();
      if (count < 0) {
        cout << "[* !! *] lookup error, then exit: " << mgr.getErr() << endl;
        return;
      } else if (count > 0) {
        response.setResponseType(Response::NDNS_Auth);
      } else {
        response.setResponseType(Response::NDNS_Nack);
      }
    } else {
      response.setResponseType(Response::NDNS_Nack);
    }
  }

  Data data = response.toData();
  data.setFreshnessPeriod(response.getFreshness());

  m_keyChain.sign(data);
  m_face.put(data);
  cout << "[* <- *] send response: " << response << ": " << data << endl;
} //onInterest

void NameServer::run()
{
  //m_zoneMgr.lookupId();
  if (m_zoneMgr.getZone().getId() == 0) {
    m_hasError = true;
    m_error = "cannot get Zone.id from database for name="
        + m_zone.getAuthorizedName().toUri();
    stop();
  }

  boost::asio::signal_set signalSet(m_ioService, SIGINT, SIGTERM);
  signalSet.async_wait(boost::bind(&NDNApp::signalHandler, this));
  // boost::bind(&NdnTlvPingServer::signalHandler, this)
  Name name;
  name.set(m_prefix);
  name.append(Query::toString(Query::QUERY_DNS));

  std::cout<<"========= NDNS Name Server for Zone "
            <<m_zoneMgr.getZone().getAuthorizedName().toUri()
            <<" Starts with Prefix "<<m_prefix;
  if (m_enableForwardingHint > 0) {
    std::cout<<" & ForwardingHint "<<m_forwardingHint.toUri();
  }
    std::cout<<"============="<<std::endl;

  m_face.setInterestFilter(name, bind(&NameServer::onInterest, this, _1, _2),
      bind(&NDNApp::onRegisterFailed, this, _1, _2));
 std::cout<<"Name Server Register Name Prefix: "<<name<<std::endl;

  if (m_enableForwardingHint > 0) {
    Name name2 = Name(m_forwardingHint);
    name2.append(ndn::ndns::label::ForwardingHintLabel);
    name2.append(name);
    m_face.setInterestFilter(name2, bind(&NameServer::onInterest, this, _1, _2),
        bind(&NDNApp::onRegisterFailed, this, _1, _2));
    std::cout<<"Name Server Register Name Prefix: "<<name2<<std::endl;
  }


  try {
    m_face.processEvents();
  } catch (std::exception& e) {
    m_hasError = true;
    m_error = "ERROR: ";
    m_error += e.what();
    stop();
  }

} //run

} //namespace ndns
} /* namespace ndn */

