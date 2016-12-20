/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014-2016, Regents of the University of California.
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

#include "logger.hpp"

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/classification.hpp>

#include "test-common.hpp"

namespace ndn {
namespace ndns {
namespace tests {

BOOST_AUTO_TEST_SUITE(Logger)

class Fixture
{
public:
  Fixture()
    : m_logConfigFilename(boost::filesystem::unique_path().native())
    , m_logFilename(boost::filesystem::unique_path().native())
  {
  }

  ~Fixture()
  {
    boost::filesystem::remove(boost::filesystem::path(getLogConfigFilename()));
    boost::filesystem::remove(boost::filesystem::path(getLogFilename()));

    log::init("unit-tests.log4cxx");
  }

  const std::string&
  getLogConfigFilename()
  {
    return m_logConfigFilename;
  }

  const std::string&
  getLogFilename()
  {
    return m_logFilename;
  }

  void
  verifyOutput(const std::string expected[], size_t nExpected)
  {
    std::ifstream is(getLogFilename().c_str());
    std::string buffer((std::istreambuf_iterator<char>(is)),
                       (std::istreambuf_iterator<char>()));

    std::vector<std::string> components;
    boost::split(components, buffer, boost::is_any_of(" ,\n"));

    // expected + number of timestamps (one per log statement) + trailing newline of last statement
    BOOST_REQUIRE_EQUAL(components.size(), nExpected);

    for (size_t i = 0; i < nExpected; ++i) {
      if (expected[i] == "")
        continue;

      BOOST_CHECK_EQUAL(components[i], expected[i]);
    }
  }

private:
  std::string m_logConfigFilename;
  std::string m_logFilename;
};

BOOST_FIXTURE_TEST_CASE(AllLevels, Fixture)
{
  {
    std::ofstream of(getLogConfigFilename().c_str());
    of << "log4j.rootLogger=TRACE, FILE\n"
       << "log4j.appender.FILE=org.apache.log4j.FileAppender\n"
       << "log4j.appender.FILE.layout=org.apache.log4j.PatternLayout\n"
       << "log4j.appender.FILE.File=" << getLogFilename() << "\n"
       << "log4j.appender.FILE.ImmediateFlush=true\n"
       << "log4j.appender.FILE.layout.ConversionPattern=%d{HH:mm:ss} %p %c{1} - %m%n\n";
  }

  log::init(getLogConfigFilename());

  NDNS_LOG_INIT("DefaultConfig");

  NDNS_LOG_TRACE("trace-message-JHGFDSR^1");
  NDNS_LOG_DEBUG("debug-message-IGg2474fdksd-fo-" << 15 << 16 << 17);
  NDNS_LOG_INFO("info-message-Jjxjshj13");
  NDNS_LOG_WARN("warning-message-XXXhdhd11" << 1 <<"x");
  NDNS_LOG_ERROR("error-message-!#$&^%$#@");
  NDNS_LOG_FATAL("fatal-message-JJSjaamcng");

  const std::string EXPECTED[] =
    {
      "", "TRACE", "DefaultConfig", "-", "trace-message-JHGFDSR^1",
      "", "DEBUG", "DefaultConfig", "-", "debug-message-IGg2474fdksd-fo-151617",
      "", "INFO",  "DefaultConfig", "-", "info-message-Jjxjshj13",
      "", "WARN",  "DefaultConfig", "-", "warning-message-XXXhdhd111x",
      "", "ERROR", "DefaultConfig", "-", "error-message-!#$&^%$#@",
      "", "FATAL", "DefaultConfig", "-", "fatal-message-JJSjaamcng",
      "",
    };

  verifyOutput(EXPECTED, sizeof(EXPECTED) / sizeof(std::string));
}

BOOST_FIXTURE_TEST_CASE(UpToInfo, Fixture)
{
  {
    std::ofstream of(getLogConfigFilename().c_str());
    of << "log4j.rootLogger=INFO, FILE\n"
       << "log4j.appender.FILE=org.apache.log4j.FileAppender\n"
       << "log4j.appender.FILE.layout=org.apache.log4j.PatternLayout\n"
       << "log4j.appender.FILE.File=" << getLogFilename() << "\n"
       << "log4j.appender.FILE.ImmediateFlush=true\n"
       << "log4j.appender.FILE.layout.ConversionPattern=%d{HH:mm:ss} %p %c{1} - %m%n\n";
  }

  log::init(getLogConfigFilename());

  NDNS_LOG_INIT("DefaultConfig");

  NDNS_LOG_TRACE("trace-message-JHGFDSR^1");
  NDNS_LOG_DEBUG("debug-message-IGg2474fdksd-fo-" << 15 << 16 << 17);
  NDNS_LOG_INFO("info-message-Jjxjshj13");
  NDNS_LOG_WARN("warning-message-XXXhdhd11" << 1 <<"x");
  NDNS_LOG_ERROR("error-message-!#$&^%$#@");
  NDNS_LOG_FATAL("fatal-message-JJSjaamcng");

  const std::string EXPECTED[] =
    {
      "", "INFO",  "DefaultConfig", "-", "info-message-Jjxjshj13",
      "", "WARN",  "DefaultConfig", "-", "warning-message-XXXhdhd111x",
      "", "ERROR", "DefaultConfig", "-", "error-message-!#$&^%$#@",
      "", "FATAL", "DefaultConfig", "-", "fatal-message-JJSjaamcng",
      "",
    };

  verifyOutput(EXPECTED, sizeof(EXPECTED) / sizeof(std::string));
}

BOOST_FIXTURE_TEST_CASE(OnlySelectedModule, Fixture)
{
  {
    std::ofstream of(getLogConfigFilename().c_str());
    of << "log4j.rootLogger=OFF, FILE\n"
       << "log4j.appender.FILE=org.apache.log4j.FileAppender\n"
       << "log4j.appender.FILE.layout=org.apache.log4j.PatternLayout\n"
       << "log4j.appender.FILE.File=" << getLogFilename() << "\n"
       << "log4j.appender.FILE.ImmediateFlush=true\n"
       << "log4j.appender.FILE.layout.ConversionPattern=%d{HH:mm:ss} %p %c - %m%n\n"
       << "log4j.logger.SelectedModule=TRACE\n"
       << "log4j.logger.SelectedModule.Submodule=FATAL\n";
  }

  log::init(getLogConfigFilename());

  {
    NDNS_LOG_INIT("DefaultConfig");

    NDNS_LOG_TRACE("trace-message-JHGFDSR^1");
    NDNS_LOG_DEBUG("debug-message-IGg2474fdksd-fo-" << 15 << 16 << 17);
    NDNS_LOG_INFO("info-message-Jjxjshj13");
    NDNS_LOG_WARN("warning-message-XXXhdhd11" << 1 <<"x");
    NDNS_LOG_ERROR("error-message-!#$&^%$#@");
    NDNS_LOG_FATAL("fatal-message-JJSjaamcng");
  }

  {
    NDNS_LOG_INIT("SelectedModule");

    NDNS_LOG_TRACE("trace-message-JHGFDSR^1");
    NDNS_LOG_DEBUG("debug-message-IGg2474fdksd-fo-" << 15 << 16 << 17);
    NDNS_LOG_INFO("info-message-Jjxjshj13");
    NDNS_LOG_WARN("warning-message-XXXhdhd11" << 1 <<"x");
    NDNS_LOG_ERROR("error-message-!#$&^%$#@");
    NDNS_LOG_FATAL("fatal-message-JJSjaamcng");
  }

  {
    NDNS_LOG_INIT("SelectedModule.Submodule");

    NDNS_LOG_TRACE("trace-message-JHGFDSR^1");
    NDNS_LOG_DEBUG("debug-message-IGg2474fdksd-fo-" << 15 << 16 << 17);
    NDNS_LOG_INFO("info-message-Jjxjshj13");
    NDNS_LOG_WARN("warning-message-XXXhdhd11" << 1 <<"x");
    NDNS_LOG_ERROR("error-message-!#$&^%$#@");
    NDNS_LOG_FATAL("fatal-message-JJSjaamcng");
  }

  const std::string EXPECTED[] =
    {
      "", "TRACE", "SelectedModule", "-", "trace-message-JHGFDSR^1",
      "", "DEBUG", "SelectedModule", "-", "debug-message-IGg2474fdksd-fo-151617",
      "", "INFO",  "SelectedModule", "-", "info-message-Jjxjshj13",
      "", "WARN",  "SelectedModule", "-", "warning-message-XXXhdhd111x",
      "", "ERROR", "SelectedModule", "-", "error-message-!#$&^%$#@",
      "", "FATAL", "SelectedModule", "-", "fatal-message-JJSjaamcng",
      "", "FATAL", "SelectedModule.Submodule", "-", "fatal-message-JJSjaamcng",
      "",
    };

  verifyOutput(EXPECTED, sizeof(EXPECTED) / sizeof(std::string));
}


BOOST_AUTO_TEST_SUITE_END()

} // namespace tests
} // namespace ndns
} // namespace ndn
