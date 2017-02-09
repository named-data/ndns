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

#ifndef NDNS_TESTS_UNIT_TEST_COMMON_FIXTURES_HPP
#define NDNS_TESTS_UNIT_TEST_COMMON_FIXTURES_HPP

#include "boost-test.hpp"

#include <boost/asio.hpp>
#include <ndn-cxx/util/time-unit-test-clock.hpp>

namespace ndn {
namespace ndns {
namespace tests {

/** \brief base test fixture
 *
 *  Every test case should be based on this fixture,
 *  to have per test case io_service initialization.
 */
class BaseFixture
{
protected:
  BaseFixture();

protected:
  /** \brief reference to global io_service
   */
  boost::asio::io_service m_io;
};

/** \brief a base test fixture that overrides steady clock and system clock
 */
class UnitTestTimeFixture : public virtual BaseFixture
{
protected:
  UnitTestTimeFixture();

  ~UnitTestTimeFixture();

  /** \brief advance steady and system clocks
   *
   *  Clocks are advanced in increments of \p tick for \p nTicks ticks.
   *  After each tick, global io_service is polled to process pending I/O events.
   *
   *  Exceptions thrown during I/O events are propagated to the caller.
   *  Clock advancing would stop in case of an exception.
   */
  void
  advanceClocks(const time::nanoseconds& tick, size_t nTicks = 1);

  /** \brief advance steady and system clocks
   *
   *  Clocks are advanced in increments of \p tick for \p total time.
   *  The last increment might be shorter than \p tick.
   *  After each tick, global io_service is polled to process pending I/O events.
   *
   *  Exceptions thrown during I/O events are propagated to the caller.
   *  Clock advancing would stop in case of an exception.
   */
  void
  advanceClocks(const time::nanoseconds& tick, const time::nanoseconds& total);

protected:
  shared_ptr<time::UnitTestSteadyClock> steadyClock;
  shared_ptr<time::UnitTestSystemClock> systemClock;

  friend class LimitedIo;
};

} // namespace tests
} // namespace ndns
} // namespace ndn

#endif // NDNS_TESTS_UNIT_TEST_COMMON_FIXTURES_HPP
