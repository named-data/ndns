/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2019, Regents of the University of California.
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

#ifndef NDNS_UTIL_UTIL_HPP
#define NDNS_UTIL_UTIL_HPP

#include "ndns-enum.hpp"

#include <ndn-cxx/data.hpp>

namespace ndn {
namespace ndns {

std::string
getDefaultDatabaseFile();

NdnsContentType
toNdnsContentType(const std::string& str);

/**
 * @brief print the data in a flexible way
 * @param data The data to be printed
 * @param os the ostream that received printed message
 * @param isPretty whether to use pretty way
 */
void
output(const Data& data, std::ostream& os, bool isPretty);

} // namespace ndns
} // namespace ndn

#endif // NDNS_UTIL_UTIL_HPP
