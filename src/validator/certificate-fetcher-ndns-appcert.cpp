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

#include "certificate-fetcher-ndns-appcert.hpp"
#include "certificate-fetcher-ndns-cert.hpp"
#include "clients/iterative-query-controller.hpp"

#include "validator.hpp"
#include "clients/response.hpp"

namespace ndn {
namespace ndns {

using security::Certificate;

CertificateFetcherAppCert::CertificateFetcherAppCert(Face& face,
                                                     size_t nsCacheSize,
                                                     size_t startComponentIndex)
  : m_face(face)
  , m_validator(NdnsValidatorBuilder::create(face, nsCacheSize, startComponentIndex))
  , m_startComponentIndex(startComponentIndex)
{
  m_nsCache = dynamic_cast<CertificateFetcherNdnsCert&>(m_validator->getFetcher()).getNsCache();
}

void
CertificateFetcherAppCert::doFetch(const shared_ptr<security::CertificateRequest>& certRequest,
                                   const shared_ptr<security::ValidationState>& state,
                                   const ValidationContinuation& continueValidation)
{
  const Name& key = certRequest->interest.getName();
  auto query = make_shared<IterativeQueryController>(key,
                                                     label::APPCERT_RR_TYPE,
                                                     certRequest->interest.getInterestLifetime(),
                                                     [=] (const Data& data, const Response& response) {
                                                       onQuerySuccessCallback(data, certRequest, state, continueValidation);
                                                     },
                                                     [=] (uint32_t errCode, const std::string& errMsg) {
                                                       onQueryFailCallback(errMsg, certRequest, state, continueValidation);
                                                     },
                                                     m_face,
                                                     nullptr,
                                                     m_nsCache);
  query->setStartComponentIndex(m_startComponentIndex);
  query->start();
  state->setTag(make_shared<IterativeQueryTag>(query));
}

void
CertificateFetcherAppCert::onQuerySuccessCallback(const Data& data,
                                                  const shared_ptr<security::CertificateRequest>& certRequest,
                                                  const shared_ptr<security::ValidationState>& state,
                                                  const ValidationContinuation& continueValidation)
{
  m_validator->validate(data,
                        [=] (const Data& d) {
                          onValidationSuccessCallback(d, certRequest, state, continueValidation);
                        },
                        [=] (const Data&, const security::ValidationError& err) {
                          onValidationFailCallback(err, certRequest, state, continueValidation);
                        });
}

void
CertificateFetcherAppCert::onQueryFailCallback(const std::string& errMsg,
                                               const shared_ptr<security::CertificateRequest>& certRequest,
                                               const shared_ptr<security::ValidationState>& state,
                                               const ValidationContinuation& continueValidation)
{
  state->removeTag<IterativeQueryTag>();
  state->fail({security::ValidationError::Code::CANNOT_RETRIEVE_CERT, "Cannot fetch certificate due to " +
               errMsg + " `" + certRequest->interest.getName().toUri() + "`"});
}

void
CertificateFetcherAppCert::onValidationSuccessCallback(const Data& data,
                                                       const shared_ptr<security::CertificateRequest>& certRequest,
                                                       const shared_ptr<security::ValidationState>& state,
                                                       const ValidationContinuation& continueValidation)
{
  state->removeTag<IterativeQueryTag>();

  if (data.getContentType() == NDNS_NACK) {
    return state->fail({security::ValidationError::Code::CANNOT_RETRIEVE_CERT,
                        "Cannot fetch certificate: got Nack for query `" +
                        certRequest->interest.getName().toUri() + "`"});
  }

  Certificate cert;
  try {
    cert = Certificate(data.getContent().blockFromValue());
  }
  catch (const ndn::tlv::Error& e) {
    return state->fail({security::ValidationError::Code::MALFORMED_CERT, "Fetched a malformed "
                        "certificate `" + data.getName().toUri() + "` (" + e.what() + ")"});
  }

  continueValidation(cert, state);
}

void
CertificateFetcherAppCert::onValidationFailCallback(const security::ValidationError& err,
                                                    const shared_ptr<security::CertificateRequest>& certRequest,
                                                    const shared_ptr<security::ValidationState>& state,
                                                    const ValidationContinuation& continueValidation)
{
  state->removeTag<IterativeQueryTag>();
  state->fail({security::ValidationError::Code::CANNOT_RETRIEVE_CERT,
               "Cannot fetch certificate due to NDNS validation error: " +
               err.getInfo() + " `" + certRequest->interest.getName().toUri() + "`"});
}

} // namespace ndns
} // namespace ndn
