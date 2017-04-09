/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014-2017,  Regents of the University of California,
 *                           Arizona Board of Regents,
 *                           Colorado State University,
 *                           University Pierre & Marie Curie, Sorbonne University,
 *                           Washington University in St. Louis,
 *                           Beijing Institute of Technology,
 *                           The University of Memphis.
 *
 * This file is part of NDNS (Named Data Networking Domain Name Service) and is
 * based on the code written as part of NFD (Named Data Networking Daemon).
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

#include <ndn-cxx/interest.hpp>
#include <ndn-cxx/data.hpp>
#include <ndn-cxx/lp/nack.hpp>
#include <ndn-cxx/util/dummy-client-face.hpp>
#include <ndn-cxx/security/key-chain.hpp>

#ifndef NDNS_TESTS_TEST_DUMMY_FORWARDER_HPP
#define NDNS_TESTS_TEST_DUMMY_FORWARDER_HPP

namespace ndn {
namespace ndns {
namespace tests {

/**
 * @brief Very basic implementation of the dummy forwarder
 *
 * Interests expressed by any added face, will be forwarded to all other faces.
 * Similarly, any pushed data, will be pushed to all other faces.
 */
class DummyForwarder
{
public:
  DummyForwarder(boost::asio::io_service& io, KeyChain& keyChain);

  Face&
  addFace();

  Face&
  getFace(size_t nFace)
  {
    return *m_faces.at(nFace);
  }

private:
  boost::asio::io_service& m_io;
  KeyChain& m_keyChain;
  std::vector<shared_ptr<util::DummyClientFace>> m_faces;
};

} // namespace tests
} // namespace ndns
} // namespace ndn

#endif // NDNS_TESTS_TEST_DUMMY_FORWARDER_HPP
