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

#include "name-server.hpp"
#include "logger.hpp"
#include <ndn-cxx/encoding/encoding-buffer.hpp>
#include <ndn-cxx/security/signing-helpers.hpp>

namespace ndn {
namespace ndns {

NDNS_LOG_INIT(NameServer);

const time::milliseconds NAME_SERVER_DEFAULT_CONTENT_FRESHNESS(4000);

NameServer::NameServer(const Name& zoneName, const Name& certName, Face& face, DbMgr& dbMgr,
                       KeyChain& keyChain, security::Validator& validator)
  : m_zone(zoneName)
  , m_dbMgr(dbMgr)
  , m_ndnsPrefix(zoneName)
  , m_certName(certName)
  , m_contentFreshness(NAME_SERVER_DEFAULT_CONTENT_FRESHNESS)
  , m_face(face)
  , m_keyChain(keyChain)
  , m_validator(validator)
{
  m_dbMgr.find(m_zone);

  if (m_zone.getId() == 0) {
    NDNS_LOG_FATAL("m_zone does not exist: " << zoneName);
    NDN_THROW(Error("Zone " + zoneName.toUri() + " does not exist in the database"));
  }

  m_ndnsPrefix.append(ndns::label::NDNS_ITERATIVE_QUERY);

  m_face.setInterestFilter(m_ndnsPrefix,
                           bind(&NameServer::onInterest, this, _1, _2),
                           bind(&NameServer::onRegisterFailed, this, _1, _2)
                           );

  NDNS_LOG_INFO("Zone: " << m_zone.getName() << " binds "
                << "Prefix: " << m_ndnsPrefix
                << " with Certificate: " << m_certName
                );
}

void
NameServer::onInterest(const Name& prefix, const Interest& interest)
{
  label::MatchResult re;
  if (!label::matchName(interest, m_zone.getName(), re))
    return;

  if (re.rrType == ndns::label::NDNS_UPDATE_LABEL) {
    this->handleUpdate(prefix, interest, re); // NDNS Update
  }
  else {
    this->handleQuery(prefix, interest, re);  // NDNS Iterative query
  }
}

void
NameServer::handleQuery(const Name& prefix, const Interest& interest, const label::MatchResult& re)
{
  Rrset rrset(&m_zone);
  rrset.setLabel(re.rrLabel);
  rrset.setType(re.rrType);

  NDNS_LOG_TRACE("query record: " << interest.getName());

  if (m_dbMgr.find(rrset) &&
      (re.version.empty() || re.version == rrset.getVersion())) {
    // find the record: NDNS-RESP, NDNS-AUTH, NDNS-RAW, or NDNS-NACK
    shared_ptr<Data> answer = make_shared<Data>(rrset.getData());
    NDNS_LOG_TRACE("answer query with existing Data: " << answer->getName());
    m_face.put(*answer);
  }
  else {
    Name name = interest.getName();
    name.appendVersion();
    shared_ptr<Data> answer = make_shared<Data>(name);
    Rrset doe(&m_zone);
    // currently, there is only one DoE record contains everything
    doe.setLabel(Name(re.rrLabel).append(re.rrType));
    doe.setType(label::DOE_RR_TYPE);
    if (!m_dbMgr.findLowerBound(doe)) {
        NDNS_LOG_FATAL("fail to find DoE record of zone:" + m_zone.getName().toUri());
        NDN_THROW(std::runtime_error("fail to find DoE record of zone:" + m_zone.getName().toUri()));
    }

    answer->setContent(doe.getData());
    answer->setFreshnessPeriod(this->getContentFreshness());
    answer->setContentType(NDNS_NACK);
    // give this NACk a random signature
//added_GM, by liupenghui
#if 1
	ndn::security::pib::Key key = m_keyChain.getPib().getDefaultIdentity().getDefaultKey();
	if (key.getKeyType() == KeyType::SM2)
	  m_keyChain.sign(*answer, ndn::security::SigningInfo(m_keyChain.getPib().getDefaultIdentity()).setDigestAlgorithm(ndn::DigestAlgorithm::SM3));
	else
	  m_keyChain.sign(*answer);
#else
	m_keyChain.sign(*answer);
#endif

    NDNS_LOG_TRACE("answer query with NDNS-NACK: " << answer->getName());
    m_face.put(*answer);
  }
}

void
NameServer::handleUpdate(const Name& prefix, const Interest& interest, const label::MatchResult& re)
{
  if (re.rrLabel.size() == 1) {
    // for current, we only allow Update message contains one Data, and ignore others
    auto it = re.rrLabel.begin();
    shared_ptr<Data> data;
    try {
      // blockFromValue may throw exception, which should not lead to failure of name server
      const Block& block = it->blockFromValue();
      data = make_shared<Data>(block);
    }
    catch (const std::exception& e) {
      NDNS_LOG_WARN("exception when getting update info: " << e.what());
      return;
    }
    m_validator.validate(*data,
                         bind(&NameServer::doUpdate, this, interest.shared_from_this(), data),
                         [] (const Data&, const security::ValidationError&) {
                           NDNS_LOG_WARN("Ignoring update that did not pass the verification. "
                                         "Check the root certificate");
                         });
  }
}

void
NameServer::onRegisterFailed(const ndn::Name& prefix, const std::string& reason)
{
  NDNS_LOG_FATAL("fail to register prefix=" << prefix << ". Due to: " << reason);
  NDN_THROW(Error("zone " + m_zone.getName().toUri() + " register prefix: " +
                  prefix.toUri() + " fails. due to: " + reason));
}

void
NameServer::doUpdate(const shared_ptr<const Interest>& interest,
                     const shared_ptr<const Data>& data)
{
  label::MatchResult re;
  try {
    if (!label::matchName(*data, m_zone.getName(), re))
      return;
  }
  catch (const std::exception& e) {
    NDNS_LOG_INFO("Error while name/certificate matching: " << e.what());
  }

  Rrset rrset(&m_zone);
  rrset.setLabel(re.rrLabel);
  rrset.setType(re.rrType);

  Name name = interest->getName();
  name.appendVersion();
  shared_ptr<Data> answer = make_shared<Data>(name);
  answer->setFreshnessPeriod(this->getContentFreshness());
  answer->setContentType(NDNS_RESP);

  Block blk(ndn::ndns::tlv::RrData);
  try {
    if (m_dbMgr.find(rrset)) {
      const name::Component& newVersion = re.version;
      if (newVersion > rrset.getVersion()) {
        // update existing record
        rrset.setVersion(newVersion);
        rrset.setData(data->wireEncode());
        m_dbMgr.update(rrset);
        blk.push_back(makeNonNegativeIntegerBlock(ndn::ndns::tlv::UpdateReturnCode, UPDATE_OK));
        blk.encode(); // must
        answer->setContent(blk);
        NDNS_LOG_TRACE("replace old record and answer update with UPDATE_OK");
      }
      else {
        blk.push_back(makeNonNegativeIntegerBlock(ndn::ndns::tlv::UpdateReturnCode, UPDATE_FAILURE));
        blk.encode();
        answer->setContent(blk);
        NDNS_LOG_TRACE("answer update with UPDATE_FAILURE");
      }
    }
    else {
      // insert new record
      rrset.setVersion(re.version);
      rrset.setData(data->wireEncode());
      rrset.setTtl(m_zone.getTtl());
      m_dbMgr.insert(rrset);
      blk.push_back(makeNonNegativeIntegerBlock(ndn::ndns::tlv::UpdateReturnCode, UPDATE_OK));
      blk.encode();
      answer->setContent(blk);
      NDNS_LOG_TRACE("insert new record and answer update with UPDATE_OK");
    }
  }
  catch (const std::exception& e) {
    blk.push_back(makeNonNegativeIntegerBlock(ndn::ndns::tlv::UpdateReturnCode, UPDATE_FAILURE));
    blk.encode(); // must
    answer->setContent(blk);
    NDNS_LOG_INFO("Error processing the update: " << e.what()
                  << ". Update may need sudo privilege to write DbFile");
    NDNS_LOG_TRACE("exception happens and answer update with UPDATE_FAILURE");
  }
//added_GM liupenghui
#if 1
	  ndn::security::Identity identity;
	  ndn::security::pib::Key key;
	  
	  ndn::Name identityName = ndn::security::extractIdentityFromCertName(m_certName);
	  ndn::Name keyName = ndn::security::extractKeyNameFromCertName(m_certName);
	  identity = m_keyChain.getPib().getIdentity(identityName);
	  key = identity.getKey(keyName);
	  if (key.getKeyType() == KeyType::SM2)
	    m_keyChain.sign(*answer, signingByCertificate(m_certName).setDigestAlgorithm(ndn::DigestAlgorithm::SM3));
	  else
	    m_keyChain.sign(*answer, signingByCertificate(m_certName));
#else
	  m_keyChain.sign(*answer, signingByCertificate(m_certName));
#endif
  m_face.put(*answer);
}

} // namespace ndns
} // namespace ndn
