/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
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

#ifndef NDNS_VALIDATOR_CERTIFICATE_FETCHER_NDNS_APPCERT_HPP
#define NDNS_VALIDATOR_CERTIFICATE_FETCHER_NDNS_APPCERT_HPP

#include <ndn-cxx/ims/in-memory-storage.hpp>
#include <ndn-cxx/security/v2/validator.hpp>

namespace ndn {
namespace ndns {

/**
 * @brief Fetch NDNS-stored application certificate(APPCERT type record)
 * By an iterative-query process, it will retrieve the record, execute authentications,
 * and de-encapsulate record to get application's certificate.
 */
class CertificateFetcherAppCert : public security::v2::CertificateFetcher
{
public:
  explicit
  CertificateFetcherAppCert(Face& face,
                            size_t nsCacheSize = 500,
                            size_t startComponentIndex = 0);

protected:
  /**
   * @brief retrive appcert record, validate, and de-encapsulate
   * This method will first retrive the record by an iterative query.
   * Then it will pass it to validator.
   * If validated, de-encapsulate and call continueValidation.
   */
  void
  doFetch(const shared_ptr<security::v2::CertificateRequest>& certRequest,
          const shared_ptr<security::v2::ValidationState>& state,
          const ValidationContinuation& continueValidation) override;

private:
  /**
   * @brief Callback invoked when rrset is retrived, including nack
   */
  void
  onQuerySuccessCallback(const Data& data,
                         const shared_ptr<security::v2::CertificateRequest>& certRequest,
                         const shared_ptr<security::v2::ValidationState>& state,
                         const ValidationContinuation& continueValidation);

  /**
   * @brief Callback invoked when iterative query failed
   *
   * @todo retry for some amount of time
   */
  void
  onQueryFailCallback(const std::string& errMsg,
                      const shared_ptr<security::v2::CertificateRequest>& certRequest,
                      const shared_ptr<security::v2::ValidationState>& state,
                      const ValidationContinuation& continueValidation);

  /**
   * @brief Callback invoked when rrset validation succeeded
   */
  void
  onValidationSuccessCallback(const Data& data,
                              const shared_ptr<security::v2::CertificateRequest>& certRequest,
                              const shared_ptr<security::v2::ValidationState>& state,
                              const ValidationContinuation& continueValidation);

  /**
   * @brief Callback invoked when rrset validation failed
   */
  void
  onValidationFailCallback(const security::v2::ValidationError& err,
                           const shared_ptr<security::v2::CertificateRequest>& certRequest,
                           const shared_ptr<security::v2::ValidationState>& state,
                           const ValidationContinuation& continueValidation);

private:
  Face& m_face;
  unique_ptr<security::v2::Validator> m_validator;
  InMemoryStorage* m_nsCache;
  size_t m_startComponentIndex;
};

} // namespace ndns
} // namespace ndn

#endif // NDNS_VALIDATOR_CERTIFICATE_FETCHER_NDNS_APPCERT_HPP