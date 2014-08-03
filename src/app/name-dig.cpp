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
#include "name-dig.hpp"

namespace ndn {
namespace ndns {

NameDig::NameDig(const char *programName, const char *prefix)
  : NDNApp(programName, prefix)
  , m_resolverName(Name("/"))
  , m_dstLabel(Name(prefix))
  , m_rrType(RR::TXT)
{
  //prefix in this app is the m_dstLabel
  this->setInterestLifetime(time::milliseconds(10000));
}

NameDig::~NameDig()
{
  // TODO Auto-generated destructor stub
}

void NameDig::onData(const Interest& interest, Data& data)
{
  Response re;
  re.fromData(data);
  cout << "get data:->" << data.getName() << endl;
  cout << "get response:->" << re << endl;

  response = re;

  m_rrs = re.getRrs();

  vector<RR>::iterator iter = m_rrs.begin();

  while (iter != m_rrs.end()) {
    RR rr = *iter;
    cout << rr << endl;
    iter++;
  }

  this->stop();
}

void NameDig::sendQuery()
{
  Query q;
  q.setAuthorityZone(this->m_resolverName);
  q.setRrLabel(m_dstLabel);
  q.setQueryType(Query::QUERY_DNS_R);
  q.setRrType(m_rrType);

  Interest interest = q.toInterest();
  interest.setInterestLifetime(this->m_interestLifetime);
  try {
    m_face.expressInterest(interest,
        boost::bind(&NameDig::onData, this, _1, _2),
        boost::bind(&NameDig::onTimeout, this, _1));
    std::cout << "[* <- *] send Interest: " << interest.getName().toUri()
        << std::endl;
  } catch (std::exception& e) {
    m_hasError = true;
    m_error = e.what();
  }
  m_interestTriedNum += 1;
}

void NameDig::onTimeout(const Interest& interest)
{
  std::cout << "[* !! *] timeout Interest" << interest.getName() << std::endl;

  if (m_interestTriedNum >= m_interestTriedMax) {
    m_error = "All Interests timeout";
    m_hasError = true;
    this->stop();
  } else {
    sendQuery();
  }

}

void NameDig::run()
{

  this->sendQuery();

  try {
    m_face.processEvents();
  } catch (std::exception& e) {
    m_error = e.what();
    m_hasError = true;
    this->stop();
  }

}

} /* namespace ndns */
} /* namespace ndn */
