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
#ifndef NAME_RESOLVER_HPP
#define NAME_RESOLVER_HPP

#include <boost/asio.hpp>
#include <boost/noncopyable.hpp>

#include <ndn-cxx/face.hpp>
#include <ndn-cxx/name.hpp>
#include <ndn-cxx/security/key-chain.hpp>

#include "zone.hpp"
#include "db/zone-mgr.hpp"
#include "db/rr-mgr.hpp"
#include "query.hpp"
#include "response.hpp"
#include "rr.hpp"
#include "iterative-query.hpp"
#include "ndn-app.hpp"

using namespace std;

namespace ndn {
namespace ndns {


class NameCachingResolver : public NDNApp{
enum ResolverType
{
  StubResolver,
  CachingResolver
};

public:

NameCachingResolver(const char *programName, const char *prefix);


void
resolve(IterativeQuery& iq);

void
answerRespNack(IterativeQuery& iq);

using NDNApp::onData;
void
onData(const Interest& interest, Data &data, IterativeQuery& iq);

void
onInterest(const Name &name, const Interest &interest);

using NDNApp::onTimeout;

void
onTimeout(const Interest& interest, IterativeQuery& iq);

void
run();

private:
  //ResolverType m_resolverType;

};

} /* namespace ndns */
} /* namespace ndn */

#endif /* NAME_RESOLVER_HPP_ */
