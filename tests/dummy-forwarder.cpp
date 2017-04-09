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

#include "dummy-forwarder.hpp"

#include <boost/asio/io_service.hpp>

namespace ndn {
namespace ndns {
namespace tests {

DummyForwarder::DummyForwarder(boost::asio::io_service& io, KeyChain& keyChain)
  : m_io(io)
  , m_keyChain(keyChain)
{
}

Face&
DummyForwarder::addFace()
{
  auto face = std::make_shared<util::DummyClientFace>(m_io, m_keyChain, util::
                                                      DummyClientFace::Options{true, true});
  face->onSendInterest.connect([this, face] (const Interest& interest) {
      for (auto& otherFace : m_faces) {
        if (&*face == &*otherFace) {
          continue;
        }
        otherFace->receive(interest);
      }
    });

  face->onSendData.connect([this, face] (const Data& data) {
      for (auto& otherFace : m_faces) {
        if (&*face == &*otherFace) {
          continue;
        }
        otherFace->receive(data);
      }
    });

  face->onSendNack.connect([this, face] (const lp::Nack& nack) {
      for (auto& otherFace : m_faces) {
        if (&*face == &*otherFace) {
          continue;
        }
        otherFace->receive(nack);
      }
    });

  m_faces.push_back(face);
  return *face;
}

} // namespace tests
} // namespace ndns
} // namespace ndn
