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

#include "validator.hpp"
#include "config.hpp"
#include "certificate-fetcher-ndns-cert.hpp"
#include "logger.hpp"

#include <ndn-cxx/security/v2/validation-policy-config.hpp>

namespace ndn {
namespace ndns {

NDNS_LOG_INIT(Validator);

std::string NdnsValidatorBuilder::VALIDATOR_CONF_FILE = DEFAULT_CONFIG_PATH "/" "validator.conf";

unique_ptr<security::v2::Validator>
NdnsValidatorBuilder::create(Face& face,
                             size_t nsCacheSize,
                             size_t startComponentIndex,
                             const std::string& confFile)
{
  auto validator = make_unique<security::v2::Validator>(make_unique<security::v2::ValidationPolicyConfig>(),
                                                        make_unique<CertificateFetcherNdnsCert>(face,
                                                                                                nsCacheSize,
                                                                                                startComponentIndex));
  security::v2::ValidationPolicyConfig& policy = dynamic_cast<security::v2::ValidationPolicyConfig&>(validator->getPolicy());
  policy.load(confFile);
  NDNS_LOG_TRACE("Validator loads configuration: " << confFile);

  return validator;
}

} // namespace ndns
} // namespace ndn
