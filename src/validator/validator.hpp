/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2017, Regents of the University of California.
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

#ifndef NDNS_VALIDATOR_VALIDATOR_HPP
#define NDNS_VALIDATOR_VALIDATOR_HPP

#include "config.hpp"

#include <ndn-cxx/data.hpp>
#include <ndn-cxx/security/v2/validator.hpp>

namespace ndn {
namespace ndns {

class NdnsValidatorBuilder
{
public:
  static std::string VALIDATOR_CONF_FILE;

  static unique_ptr<security::v2::Validator>
  create(Face& face,
         size_t nsCacheSize = 500,
         size_t startComponentIndex = 0,
         const std::string& confFile = VALIDATOR_CONF_FILE);
};

} // namespace ndns
} // namespace ndn

#endif // NDNS_VALIDATOR_VALIDATOR_HPP
