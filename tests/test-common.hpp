/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2017, Regents of the University of California.
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

#ifndef NDNS_TESTS_TEST_COMMON_HPP
#define NDNS_TESTS_TEST_COMMON_HPP

#include "logger.hpp"
#include "boost-test.hpp"
#include "unit-test-common-fixtures.hpp"
#include "identity-management-fixture.hpp"

#include <ndn-cxx/name.hpp>
#include <ndn-cxx/data.hpp>
#include <ndn-cxx/interest.hpp>
#include <ndn-cxx/link.hpp>
#include <ndn-cxx/lp/nack.hpp>
#include <ndn-cxx/util/dummy-client-face.hpp>
#include <ndn-cxx/security/signing-helpers.hpp>

#include <boost/version.hpp>
#include <boost/asio.hpp>
#include <boost/filesystem.hpp>

#include <fstream>

namespace ndn {
namespace ndns {
namespace tests {

using ndn::security::v2::KeyChain;
using ndn::security::Identity;
using ndn::security::pib::Key;
using ndn::security::v2::Certificate;

/** \brief create an Interest
 *  \param name Interest name
 *  \param nonce if non-zero, set Nonce to this value
 *               (useful for creating Nack with same Nonce)
 */
shared_ptr<Interest>
makeInterest(const Name& name, uint32_t nonce = 0);

/** \brief create a Data with fake signature
 *  \note Data may be modified afterwards without losing the fake signature.
 *        If a real signature is desired, sign again with KeyChain.
 */
shared_ptr<Data>
makeData(const Name& name);

/** \brief add a fake signature to Data
 */
Data&
signData(Data& data);

/** \brief add a fake signature to Data
 */
inline shared_ptr<Data>
signData(shared_ptr<Data> data)
{
  signData(*data);
  return data;
}

/** \brief create a Nack
 *  \param name Interest name
 *  \param nonce Interest nonce
 *  \param reason Nack reason
 */
lp::Nack
makeNack(const Name& name, uint32_t nonce, lp::NackReason reason);

/** \brief replace a name component
 *  \param[inout] name name
 *  \param index name component index
 *  \param a arguments to name::Component constructor
 */
template<typename...A>
void
setNameComponent(Name& name, ssize_t index, const A& ...a)
{
  Name name2 = name.getPrefix(index);
  name2.append(name::Component(a...));
  name2.append(name.getSubName(name2.size()));
  name = name2;
}

template<typename Packet, typename...A>
void
setNameComponent(Packet& packet, ssize_t index, const A& ...a)
{
  Name name = packet.getName();
  setNameComponent(name, index, a...);
  packet.setName(name);
}

} // namespace tests
} // namespace ndns
} // namespace ndn

#endif // NDNS_TESTS_TEST_COMMON_HPP
