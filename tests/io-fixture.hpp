/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2020,  Regents of the University of California,
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

#ifndef NDNS_TESTS_IO_FIXTURE_HPP
#define NDNS_TESTS_IO_FIXTURE_HPP

#include "clock-fixture.hpp"

#include <boost/asio/io_service.hpp>

namespace ndn {
namespace ndns {
namespace tests {

class IoFixture : public ClockFixture
{
private:
  void
  afterTick() final
  {
    if (m_io.stopped()) {
#if BOOST_VERSION >= 106600
      m_io.restart();
#else
      m_io.reset();
#endif
    }
    m_io.poll();
  }

protected:
  boost::asio::io_service m_io;
};

} // namespace tests
} // namespace ndns
} // namespace ndn

#endif // NDNS_TESTS_IO_FIXTURE_HPP
