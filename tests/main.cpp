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

#include "config.hpp"
#include "logger.hpp"

#include <boost/version.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/noncopyable.hpp>
#include <boost/filesystem.hpp>

#include <fstream>
#include <iostream>

namespace ndn {
namespace ndns {
namespace tests {

class GlobalConfigurationFixture : boost::noncopyable
{
public:
  GlobalConfigurationFixture()
  {
    log::init("unit-tests.log4cxx");

    if (std::getenv("HOME"))
      m_home = std::getenv("HOME");

    boost::filesystem::path dir(TEST_CONFIG_PATH);
    dir /= "test-home";
    setenv("HOME", dir.generic_string().c_str(), 1);
    boost::filesystem::create_directories(dir);
    std::ofstream clientConf((dir / ".ndn" / "client.conf").generic_string().c_str());
    clientConf << "pib=pib-sqlite3\n"
               << "tpm=tpm-file" << std::endl;
  }

  ~GlobalConfigurationFixture()
  {
    if (!m_home.empty()) {
      setenv("HOME", m_home.c_str(), 1);
    }
  }

private:
  std::string m_home;
};

BOOST_GLOBAL_FIXTURE(GlobalConfigurationFixture)
#if (BOOST_VERSION >= 105900)
;
#endif // BOOST_VERSION >= 105900

} // namespace tests
} // namespace ndns
} // namespace ndn
