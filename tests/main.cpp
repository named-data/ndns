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

#define BOOST_TEST_MAIN 1
#define BOOST_TEST_DYN_LINK 1

#include <boost/version.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/noncopyable.hpp>

#include "logger.hpp"
#include "config.hpp"

namespace ndn {
namespace ndns {
namespace tests {

class UnitTestsLogging : boost::noncopyable
{
public:
  UnitTestsLogging()
  {
    log::init("unit-tests.log4cxx");
  }
};

BOOST_GLOBAL_FIXTURE(UnitTestsLogging)
#if (BOOST_VERSION >= 105900)
;
#endif // BOOST_VERSION >= 105900

} // namespace tests
} // namespace ndns
} // namespace ndn
