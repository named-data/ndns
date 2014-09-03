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

#ifndef NDNS_LOGGER_HPP
#define NDNS_LOGGER_HPP

#include <log4cxx/logger.h>

namespace ndn {
namespace ndns {
namespace log {

void
init(const std::string& configFile = "log4cxx.properties");

// The following has to be pre-processor defines in order to properly determine
// log locations

#define NDNS_LOG_INIT(name)                                             \
  static log4cxx::LoggerPtr staticModuleLogger = log4cxx::Logger::getLogger(name);

#define NDNS_LOG_TRACE(x)                       \
  LOG4CXX_TRACE(staticModuleLogger, x);

#define NDNS_LOG_DEBUG(x)                       \
  LOG4CXX_DEBUG(staticModuleLogger, x);

#define NDNS_LOG_INFO(x)                        \
  LOG4CXX_INFO(staticModuleLogger, x);

#define NDNS_LOG_WARN(x)                        \
  LOG4CXX_WARN(staticModuleLogger, x);

#define NDNS_LOG_ERROR(x)                       \
  LOG4CXX_ERROR(staticModuleLogger, x);

#define NDNS_LOG_FATAL(x)                       \
  LOG4CXX_FATAL(staticModuleLogger, x);

#define NDNS_LOG_FUNCTION(x)                                            \
  LOG4CXX_TRACE(staticModuleLogger, __FUNCTION__ << "(" << x << ")");

#define NDNS_LOG_FUNCTION_NOARGS                                \
  LOG4CXX_TRACE(staticModuleLogger, __FUNCTION__ << "()");


} // namespace log
} // namespace ndns
} // namespace ndn

#endif // NDNS_LOGGER_HPP
