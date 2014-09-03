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

#include "logger.hpp"

#include <log4cxx/logger.h>
#include <log4cxx/basicconfigurator.h>
#include <log4cxx/consoleappender.h>
#include <log4cxx/patternlayout.h>
#include <log4cxx/level.h>
#include <log4cxx/propertyconfigurator.h>
#include <log4cxx/defaultconfigurator.h>
#include <log4cxx/helpers/exception.h>

#include <unistd.h>

namespace ndn {
namespace ndns {
namespace log {

void
init(const std::string& configFile/* = "log4cxx.properties"*/)
{
  using namespace log4cxx;
  using namespace log4cxx::helpers;

  if (access(configFile.c_str(), R_OK) == 0) {
    PropertyConfigurator::configureAndWatch(configFile.c_str());
  }
  else {
    PatternLayoutPtr   layout(new PatternLayout("%d{HH:mm:ss} %p %c{1} - %m%n"));
    ConsoleAppenderPtr appender(new ConsoleAppender(layout));

    BasicConfigurator::configure(appender);
    Logger::getRootLogger()->setLevel(log4cxx::Level::getInfo());
  }
}

} // namespace log
} // namespace ndns
} // namespace ndn
