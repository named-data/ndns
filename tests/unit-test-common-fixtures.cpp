/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014-2016,  Regents of the University of California,
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

#include "test-common.hpp"

namespace ndn {
namespace ndns {
namespace tests {

BaseFixture::BaseFixture()
{
}

UnitTestTimeFixture::UnitTestTimeFixture()
  : steadyClock(make_shared<time::UnitTestSteadyClock>())
  , systemClock(make_shared<time::UnitTestSystemClock>())
{
  time::setCustomClocks(steadyClock, systemClock);
}

UnitTestTimeFixture::~UnitTestTimeFixture()
{
  time::setCustomClocks(nullptr, nullptr);
}

void
UnitTestTimeFixture::advanceClocks(const time::nanoseconds& tick, size_t nTicks)
{
  this->advanceClocks(tick, tick * nTicks);
}

void
UnitTestTimeFixture::advanceClocks(const time::nanoseconds& tick, const time::nanoseconds& total)
{
  BOOST_ASSERT(tick > time::nanoseconds::zero());
  BOOST_ASSERT(total >= time::nanoseconds::zero());

  time::nanoseconds remaining = total;
  while (remaining > time::nanoseconds::zero()) {
    if (remaining >= tick) {
      steadyClock->advance(tick);
      systemClock->advance(tick);
      remaining -= tick;
    }
    else {
      steadyClock->advance(remaining);
      systemClock->advance(remaining);
      remaining = time::nanoseconds::zero();
    }

    if (m_io.stopped())
      m_io.reset();
    m_io.poll();
  }
}

} // namespace tests
} // namespace ndns
} // namespace ndn
