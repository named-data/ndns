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

#ifndef NDNS_LOGGER_HPP
#define NDNS_LOGGER_HPP

#include <ndn-cxx/util/logger.hpp>

#define NDNS_LOG_INIT(name) NDN_LOG_INIT(ndns.name)

#define NDNS_LOG_FATAL(x) NDN_LOG_FATAL(x)
#define NDNS_LOG_ERROR(x) NDN_LOG_ERROR(x)
#define NDNS_LOG_WARN(x)  NDN_LOG_WARN(x)
#define NDNS_LOG_INFO(x)  NDN_LOG_INFO(x)
#define NDNS_LOG_DEBUG(x) NDN_LOG_DEBUG(x)
#define NDNS_LOG_TRACE(x) NDN_LOG_TRACE(x)

#define NDNS_LOG_FUNCTION(x)     NDN_LOG_TRACE(__FUNCTION__ << "(" << x << ")")
#define NDNS_LOG_FUNCTION_NOARGS NDN_LOG_TRACE(__FUNCTION__ << "()")

#endif // NDNS_LOGGER_HPP
