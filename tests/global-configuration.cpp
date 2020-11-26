/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2020, Regents of the University of California.
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

#include "boost-test.hpp"

#include <boost/filesystem.hpp>
#include <stdlib.h>

namespace ndn {
namespace tests {

class GlobalConfiguration
{
public:
  GlobalConfiguration()
  {
    if (getenv("HOME") != nullptr) {
      m_home = getenv("HOME");
    }
    if (getenv("NDN_CLIENT_PIB") != nullptr) {
      m_pib = getenv("NDN_CLIENT_PIB");
    }
    if (getenv("NDN_CLIENT_TPM") != nullptr) {
      m_tpm = getenv("NDN_CLIENT_TPM");
    }

    auto testHome = boost::filesystem::path(UNIT_TESTS_TMPDIR) / "test-home";
    setenv("HOME", testHome.c_str(), 1);

    boost::filesystem::create_directories(testHome);

    setenv("NDN_CLIENT_PIB", "pib-sqlite3", 1);
    setenv("NDN_CLIENT_TPM", "tpm-file", 1);
  }

  ~GlobalConfiguration() noexcept
  {
    if (!m_home.empty()) {
      setenv("HOME", m_home.data(), 1);
    }
    if (!m_pib.empty()) {
      setenv("NDN_CLIENT_PIB", m_home.data(), 1);
    }
    if (!m_tpm.empty()) {
      setenv("NDN_CLIENT_TPM", m_home.data(), 1);
    }
  }

private:
  std::string m_home;
  std::string m_pib;
  std::string m_tpm;
};

#if BOOST_VERSION >= 106500
BOOST_TEST_GLOBAL_CONFIGURATION(GlobalConfiguration);
#elif BOOST_VERSION >= 105900
BOOST_GLOBAL_FIXTURE(GlobalConfiguration);
#else
BOOST_GLOBAL_FIXTURE(GlobalConfiguration)
#endif

} // namespace tests
} // namespace ndn
