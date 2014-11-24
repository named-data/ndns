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

#include "clients/response.hpp"
#include "clients/query.hpp"
#include "ndns-label.hpp"
#include "validator.hpp"
#include "ndns-enum.hpp"
#include "ndns-tlv.hpp"
#include "logger.hpp"
#include "daemon/db-mgr.hpp"
#include "util/util.hpp"

#include <ndn-cxx/security/key-chain.hpp>
#include <ndn-cxx/data.hpp>
#include <ndn-cxx/util/io.hpp>
#include <ndn-cxx/encoding/block.hpp>
#include <ndn-cxx/encoding/block-helpers.hpp>
#include <boost/noncopyable.hpp>
#include <boost/program_options.hpp>
#include <boost/asio.hpp>
#include <boost/filesystem.hpp>

#include <string>
#include <tuple>

namespace ndn {
namespace ndns {
NDNS_LOG_INIT("NdnsUpdate");

class NdnsUpdate : noncopyable
{
public:
  NdnsUpdate(const Name& hint, const Name& zone, const Name& rrLabel,
             const name::Component& rrType, NdnsType ndnsType, const Name& certName,
             Face& face)
    : m_queryType(rrType == label::CERT_RR_TYPE ?
                  label::NDNS_CERT_QUERY : label::NDNS_ITERATIVE_QUERY)
    , m_rrType(rrType)
    , m_hint(hint)
    , m_zone(zone)
    , m_certName(certName)
    , m_interestLifetime(DEFAULT_INTEREST_LIFETIME)
    , m_face(face)
    , m_validator(face)
    , m_hasError(false)
  {
    m_update.setZone(m_zone);
    m_update.setRrLabel(rrLabel);
    m_update.setQueryType(m_queryType);
    m_update.setRrType(name::Component(rrType));
    m_update.setNdnsType(ndnsType);
  }

  void
  run()
  {
    NDNS_LOG_INFO(" =================================== "
                  << "start to update RR at Zone = " << this->m_zone
                  << " with rrLabel = " << this->m_update.getRrLabel()
                  << " and rrType = " << this->m_rrType
                  << " =================================== ");
    NDNS_LOG_INFO("certificate to sign the data is: " << m_certName);

    Interest interest = this->makeUpdateInterest();
    NDNS_LOG_TRACE("[* <- *] send Update: " << m_update);
    m_face.expressInterest(interest,
                           bind(&NdnsUpdate::onData, this, _1, _2),
                           bind(&NdnsUpdate::onTimeout, this, _1) //dynamic binding
                           );
    try {
      m_face.processEvents();
    }
    catch (std::exception& e) {
      NDNS_LOG_FATAL("Face fails to process events: " << e.what());
      m_hasError = true;
    }
  }

  void
  stop()
  {
    m_face.getIoService().stop();
  }

private:
  void
  onData(const Interest& interest, const Data& data)
  {
    NDNS_LOG_INFO("get response of Update");
    int ret = -1;
    std::string msg;
    std::tie(ret, msg) = this->parseResponse(data);
    NDNS_LOG_INFO("Return Code: " << ret << ", and Update "
                  << (ret == UPDATE_OK ? "succeeds" : "fails"));
    if (ret != UPDATE_OK)
      m_hasError = true;

    if (!msg.empty()) {
      NDNS_LOG_INFO("Return Msg: " << msg);
    }

    NDNS_LOG_INFO("to verify the response");
    m_validator.validate(data,
                         bind(&NdnsUpdate::onDataValidated, this, _1),
                         bind(&NdnsUpdate::onDataValidationFailed, this, _1, _2)
                         );
  }

  std::tuple<int, std::string>
  parseResponse(const Data& data)
  {
    int ret = -1;
    std::string msg;
    Block blk = data.getContent();
    blk.parse();
    Block block = blk.blockFromValue();
    block.parse();
    Block::element_const_iterator val = block.elements_begin();
    for (; val != block.elements_end(); ++val) {
      if (val->type() == ndns::tlv::UpdateReturnCode) { // the first must be return code
        ret = readNonNegativeInteger(*val);
      }
      else if (val->type() == ndns::tlv::UpdateReturnMsg) {
        msg =  std::string(reinterpret_cast<const char*>(val->value()), val->value_size());
      }
    }

    return std::make_tuple(ret, msg);
  }

  /**
   * @brief construct a query (interest) which contains the update information
   */
  Interest
  makeUpdateInterest()
  {
    shared_ptr<Data> data = m_update.toData();
    m_keyChain.sign(*data, m_certName);

    Query q(m_hint, m_zone, label::NDNS_ITERATIVE_QUERY);
    q.setRrLabel(Name().append(data->wireEncode()));
    q.setRrType(label::NDNS_UPDATE_LABEL);
    q.setInterestLifetime(m_interestLifetime);

    return q.toInterest();
  }

private:
  void
  onTimeout(const ndn::Interest& interest)
  {
    NDNS_LOG_TRACE("Update timeouts");
    m_hasError = true;
    this->stop();
  }

  void
  onDataValidated(const shared_ptr<const Data>& data)
  {
    NDNS_LOG_INFO("data pass verification");
    this->stop();
  }

  void
  onDataValidationFailed(const shared_ptr<const Data>& data, const std::string& str)
  {
    NDNS_LOG_INFO("data does not pass verification");
    m_hasError = true;
    this->stop();
  }

public:
  void
  setUpdateAppContent(const Block& block)
  {
    m_update.setAppContent(block);
  }

  void
  addUpdateRr(const Block& block)
  {
    m_update.addRr(block);
  }

  void
  setInterestLifetime(const time::milliseconds& interestLifetime)
  {
    m_interestLifetime = interestLifetime;
  }

  const bool
  hasError() const
  {
    return m_hasError;
  }

  const Response&
  getUpdate() const
  {
    return m_update;
  }

private:
  name::Component m_queryType; ///< NDNS or KEY
  name::Component m_rrType;

  Name m_hint;
  Name m_zone;
  Name m_certName;
  time::milliseconds m_interestLifetime;

  Face& m_face;
  Validator m_validator;
  KeyChain m_keyChain;

  Response m_update;
  bool m_hasError;
};

} // namespace ndns
} // namespace ndn

int
main(int argc, char* argv[])
{
  ndn::ndns::log::init();
  using std::string;
  using namespace ndn;
  using namespace ndn::ndns;

  Name hint;
  Name zone;
  int ttl = 4;
  Name rrLabel;
  string rrType = "TXT";
  string ndnsTypeStr = "resp";
  Name certName;
  std::vector<string> contents;
  string contentFile;
  string dstFile;
  ndn::Block block;
  try {
    namespace po = boost::program_options;
    po::variables_map vm;

    po::options_description generic("Generic Options");
    generic.add_options()("help,h", "print help message");

    po::options_description config("Configuration");
    config.add_options()
      ("hint,H", po::value<Name>(&hint), "forwarding hint")
      ("ttl,T", po::value<int>(&ttl), "TTL of query. default: 4 sec")
      ("rrtype,t", po::value<string>(&rrType), "set request RR Type. default: TXT")
      ("ndnsType,n", po::value<string>(&ndnsTypeStr), "Set the ndnsType of the resource record. "
       "Potential values are [resp|nack|auth|raw]. Default: resp")
      ("cert,c", po::value<Name>(&certName), "set the name of certificate to sign the update")
      ("content,o", po::value<std::vector<string>>(&contents)->multitoken(),
       "set the content of the RR")
      ("contentFile,f", po::value<string>(&contentFile), "set the path of file which contain"
       " content of the RR in base64 format")
      ;

    po::options_description hidden("Hidden Options");
    hidden.add_options()
      ("zone,z", po::value<Name>(&zone), "zone the record is delegated")
      ("rrlabel,l", po::value<Name>(&rrLabel), "set request RR Label")
      ;
    po::positional_options_description postion;
    postion.add("zone", 1);
    postion.add("rrlabel", 1);

    po::options_description cmdline_options;
    cmdline_options.add(generic).add(config).add(hidden);

    po::options_description config_file_options;
    config_file_options.add(config).add(hidden);

    po::options_description visible("Allowed options");
    visible.add(generic).add(config);

    po::parsed_options parsed =
      po::command_line_parser(argc, argv).options(cmdline_options).positional(postion).run();

    po::store(parsed, vm);
    po::notify(vm);

    if (vm.count("help")) {
      std::cout << "Usage: ndns-update zone rrLabel [-t rrType] [-T TTL] "
        "[-H hint] [-n NdnsType] [-c cert] [-f contentFile]|[-o content]" << std::endl;
      std::cout << visible << std::endl;
      return 0;
    }

    KeyChain keyChain;
    if (certName.empty()) {
      Name name = Name().append(zone).append(rrLabel);
      // choosing the longest match of the identity who also have default certificate
      for (size_t i = name.size() + 1; i > 0; --i) { // i >=0 will present warnning
        Name tmp = name.getPrefix(i - 1);
        if (keyChain.doesIdentityExist(tmp)) {
          try {
            certName = keyChain.getDefaultCertificateNameForIdentity(tmp);
            break;
          }
          catch (std::exception&) {
            // If it cannot get a default certificate from one identity,
            // just ignore this one try next identity.
            ;
          }
        }
      }
    }
    else {
      if (!keyChain.doesCertificateExist(certName)) {
        std::cerr << "certificate: " << certName << " does not exist" << std::endl;
        return 0;
      }
    }

    if (certName.empty()) {
      std::cerr << "cannot figure out the certificate automatically. "
                << "please set it with -c CERT_NAEME" << std::endl;
    }

    if (vm.count("content") && vm.count("contentFile")) {
      std::cerr << "both content and contentFile are set. Only one is allowed" << std::endl;
      return 0;
    }

    if (!contentFile.empty()) {
      shared_ptr<ndn::Data> data = ndn::io::load<ndn::Data>(contentFile);
      block = data->wireEncode();
    }
  }
  catch (const std::exception& ex) {
    std::cerr << "Parameter Error: " << ex.what() << std::endl;
    return 0;
  }

  Face face;
  NdnsType ndnsType = toNdnsType(ndnsTypeStr);

  NdnsUpdate update(hint, zone, rrLabel, ndn::name::Component(rrType),
                    ndnsType, certName, face);
  update.setInterestLifetime(ndn::time::seconds(ttl));

  if (!contentFile.empty()) {
    if (!block.empty()) {
      if (ndnsType == ndn::ndns::NDNS_RAW)
        update.setUpdateAppContent(block);
      else {
        update.addUpdateRr(block);
      }
    }
  }
  else {
    if (ndnsType == ndn::ndns::NDNS_RAW) {
      // since NDNS_RAW's message tlv type cannot be decided, here we stop this option
      std::cerr << "--content (-o) does not support NDNS-RAW Response" << std::endl;
      return 0;
    }
    else {
      for (const auto& content : contents) {
        block = ndn::dataBlock(ndn::ndns::tlv::RrData, content.c_str(), content.size());
        update.addUpdateRr(block);
      }
    }
  }

  update.run();

  if (update.hasError())
    return 1;
  else
    return 0;
}
