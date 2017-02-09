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

#include "test-common.hpp"

#include <ndn-cxx/security/signature-sha256-with-rsa.hpp>

namespace ndn {
namespace ndns {
namespace tests {

shared_ptr<Interest>
makeInterest(const Name& name, uint32_t nonce)
{
  auto interest = make_shared<Interest>(name);
  if (nonce != 0) {
    interest->setNonce(nonce);
  }
  return interest;
}

shared_ptr<Data>
makeData(const Name& name)
{
  auto data = make_shared<Data>(name);
  return signData(data);
}

Data&
signData(Data& data)
{
  ndn::SignatureSha256WithRsa fakeSignature;
  fakeSignature.setValue(ndn::encoding::makeEmptyBlock(tlv::SignatureValue));
  data.setSignature(fakeSignature);
  data.wireEncode();

  return data;
}

lp::Nack
makeNack(const Name& name, uint32_t nonce, lp::NackReason reason)
{
  Interest interest(name);
  interest.setNonce(nonce);
  lp::Nack nack(std::move(interest));
  nack.setReason(reason);
  return nack;
}

} // namespace tests
} // namespace ndns
} // namespace ndn
