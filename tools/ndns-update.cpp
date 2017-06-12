/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014-2016, Regents of the University of California.
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

NDNS_LOG_INIT("NdnsUpdate")

class NdnsUpdate : noncopyable
{
public:
  NdnsUpdate(const Name& zone, const shared_ptr<Data>& update, Face& face)
    : m_zone(zone)
    , m_interestLifetime(DEFAULT_INTEREST_LIFETIME)
    , m_face(face)
    , m_validator(face)
    , m_update(update)
    , m_hasError(false)
  {
  }

  void
  start()
  {
    NDNS_LOG_INFO(" ================ "
                  << "start to update RR at Zone = " << this->m_zone
                  << " new RR is: " << m_update->getName()
                  <<" =================== ");

    NDNS_LOG_INFO("new RR is signed by: "
                  << m_update->getSignature().getKeyLocator().getName());

    Interest interest = this->makeUpdateInterest();
    NDNS_LOG_TRACE("[* <- *] send Update: " << m_update->getName().toUri());
    m_face.expressInterest(interest,
                           bind(&NdnsUpdate::onData, this, _1, _2),
                           bind(&NdnsUpdate::onTimeout, this, _1), // nack
                           bind(&NdnsUpdate::onTimeout, this, _1) //dynamic binding
                           );
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
    Query q(m_zone, label::NDNS_ITERATIVE_QUERY);
    q.setRrLabel(Name().append(m_update->wireEncode()));
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
  setInterestLifetime(const time::milliseconds& interestLifetime)
  {
    m_interestLifetime = interestLifetime;
  }

  bool
  hasError() const
  {
    return m_hasError;
  }

private:
  Name m_zone;

  time::milliseconds m_interestLifetime;

  Face& m_face;
  Validator m_validator;
  KeyChain m_keyChain;

  shared_ptr<Data> m_update;
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

  Name zone;
  int ttl = 4;
  Name rrLabel;
  string rrType = "TXT";
  string contentTypeStr = "resp";
  Name certName;
  std::vector<string> contents;
  string contentFile;
  shared_ptr<Data> update;

  try {
    namespace po = boost::program_options;
    po::variables_map vm;

    po::options_description generic("Generic Options");
    generic.add_options()("help,h", "print help message");

    po::options_description config("Configuration");
    config.add_options()
      ("ttl,T", po::value<int>(&ttl), "TTL of query. default: 4 sec")
      ("rrtype,t", po::value<string>(&rrType), "set request RR Type. default: TXT")
      ("contentType,n", po::value<string>(&contentTypeStr), "Set the contentType of the resource record. "
       "Potential values are [blob|link|nack|auth|resp]. Default: resp")
      ("cert,c", po::value<Name>(&certName), "set the name of certificate to sign the update")
      ("content,o", po::value<std::vector<string>>(&contents)->multitoken(),
       "set the content of the RR")
      ("contentFile,f", po::value<string>(&contentFile), "set the path of file which contain"
       " Response packet in base64 format")
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

    po::options_description visible("Usage: ndns-update zone rrLabel [-t rrType] [-T TTL] "
                                    "[-n NdnsContentType] [-c cert] "
                                    "[-f contentFile]|[-o content]\n"
                                    "Allowed options");

    visible.add(generic).add(config);

    po::parsed_options parsed =
      po::command_line_parser(argc, argv).options(cmdline_options).positional(postion).run();

    po::store(parsed, vm);
    po::notify(vm);

    if (vm.count("help")) {
      std::cout << visible << std::endl;
      return 0;
    }


    if (vm.count("content") && vm.count("contentFile")) {
      std::cerr << "both -o content and -f contentFile are set. Only one is allowed" << std::endl;
      return 1;
    }

    if (!vm.count("contentFile")) {
      NDNS_LOG_TRACE("content option is set. try to figure out the certificate");
      if (!vm.count("zone") || !vm.count("rrlabel")) {
        std::cerr << "-o option must be set together with -z zone and -r rrLabel" << std::endl;
        return 1;
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
        } // for

        if (certName.empty()) {
          std::cerr << "cannot figure out the certificate automatically. "
                    << "please set it with -c CERT_NAEME" << std::endl;
          return 1;
        }
      }
      else {
        if (!keyChain.doesCertificateExist(certName)) {
          std::cerr << "certificate: " << certName << " does not exist" << std::endl;
          return 1;
        }
      }

      NdnsContentType contentType = toNdnsContentType(contentTypeStr);

      if (contentType == ndns::NDNS_UNKNOWN) {
        std::cerr << "unknown NdnsContentType: " << contentTypeStr << std::endl;
        return 1;
      }

      Response re;
      re.setZone(zone);
      re.setRrLabel(rrLabel);
      name::Component qType = (rrType == "ID-CERT" ?
                               ndns::label::NDNS_CERT_QUERY : ndns::label::NDNS_ITERATIVE_QUERY);

      re.setQueryType(qType);
      re.setRrType(name::Component(rrType));
      re.setContentType(contentType);

      for (const auto& content : contents) {
        re.addRr(makeBinaryBlock(ndns::tlv::RrData, content.c_str(), content.size()));

        // re.addRr(content);
      }

      update = re.toData();
      keyChain.sign(*update, certName);
    }
    else {
      try {
        update = ndn::io::load<ndn::Data>(contentFile);
        NDNS_LOG_TRACE("load data " << update->getName() << " from content file: " << contentFile);
      }
      catch (const std::exception& e) {
        std::cerr << "Error: load Data packet from file: " << contentFile
                  << ". Due to: " << e.what() << std::endl;
        return 1;
      }

      try {
        // must check the Data is a legal Response with right name
        shared_ptr<Regex> regex = make_shared<Regex>("(<>*)<KEY>(<>+)<ID-CERT><>*");
        shared_ptr<Regex> regex2 = make_shared<Regex>("(<>*)<NDNS>(<>+)");

        Name zone2;
        if (regex->match(update->getName())) {
          zone2 = regex->expand("\\1");
        }
        else if (regex2->match(update->getName())) {
          zone2 = regex2->expand("\\1");
        }
        else {
          std::cerr << "The loaded Data packet cannot be stored in NDNS "
            "since its does not have a proper name" << std::endl;
          return 1;
        }

        if (vm.count("zone") && zone != zone2) {
          std::cerr << "The loaded Data packet is supposed to be stored at zone: " << zone2
                    << " instead of zone: " << zone << std::endl;
          return 1;
        }
        else {
          zone = zone2;
        }

        Response re;
        re.fromData(zone, *update);

        if (vm.count("rrlabel") && rrLabel != re.getRrLabel()) {
          std::cerr << "The loaded Data packet is supposed to have rrLabel: " << re.getRrLabel()
                    << " instead of label: " << rrLabel << std::endl;
          return 1;
        }

        if (vm.count("rrtype") && name::Component(rrType) != re.getRrType()) {
          std::cerr << "The loaded Data packet is supposed to have rrType: " << re.getRrType()
                    << " instead of label: " << rrType << std::endl;
          return 1;
        }
      }
      catch (const std::exception& e) {
        std::cerr << "Error: the loaded Data packet cannot parse to a Response stored at zone: "
                  << zone << std::endl;
        return 1;
      }

    }
  }
  catch (const std::exception& ex) {
    std::cerr << "Parameter Error: " << ex.what() << std::endl;
    return 1;
  }

  Face face;
  try {
    NdnsUpdate updater(zone, update, face);
    updater.setInterestLifetime(ndn::time::seconds(ttl));

    updater.start();
    face.processEvents();
    if (updater.hasError())
      return 1;
    else
      return 0;
  }
  catch (const ndn::ValidatorConfig::Error& e) {
    std::cerr << "Fail to create the validator: " << e.what() << std::endl;
    return 1;
  }
  catch (const std::exception& e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return 1;
  }
}
