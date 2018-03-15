/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2018, Regents of the University of California.
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

#include "test-common.hpp"

namespace ndn {
namespace ndns {
namespace tests {

class GlobalConfigurationFixture : boost::noncopyable
{
public:
  GlobalConfigurationFixture()
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

    boost::filesystem::path dir(TEST_CONFIG_PATH);
    dir /= "test-home";
    setenv("HOME", dir.generic_string().c_str(), 1);

    setenv("NDN_CLIENT_PIB", "pib-sqlite3", 1);
    setenv("NDN_CLIENT_TPM", "tpm-file", 1);
    boost::filesystem::create_directories(dir);
  }

  ~GlobalConfigurationFixture()
  {
    if (!m_home.empty()) {
      setenv("HOME", m_home.c_str(), 1);
    }
    if (!m_pib.empty()) {
      setenv("NDN_CLIENT_PIB", m_home.c_str(), 1);
    }
    if (!m_tpm.empty()) {
      setenv("NDN_CLIENT_TPM", m_home.c_str(), 1);
    }
  }

private:
  std::string m_home;
  std::string m_pib;
  std::string m_tpm;
};

BOOST_GLOBAL_FIXTURE(GlobalConfigurationFixture)
#if (BOOST_VERSION >= 105900)
;
#endif // BOOST_VERSION >= 105900

} // namespace tests
} // namespace ndns
} // namespace ndn
