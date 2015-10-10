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

#include "ndns-label.hpp"
#include "logger.hpp"
#include "clients/response.hpp"
#include "clients/query.hpp"
#include "clients/iterative-query-controller.hpp"
#include "validator.hpp"
#include "util/util.hpp"

#include <ndn-cxx/security/key-chain.hpp>
#include <ndn-cxx/face.hpp>
#include <boost/program_options.hpp>
#include <boost/asio.hpp>
#include <boost/filesystem.hpp>
#include <boost/noncopyable.hpp>

#include <memory>
#include <string>

NDNS_LOG_INIT("NdnsDig")

namespace ndn {
namespace ndns {

class NdnsDig
{
public:
  NdnsDig(const Name& hint, const Name& dstLabel,
          const name::Component& rrType, bool shouldValidateIntermediate)
    : m_dstLabel(dstLabel)
    , m_rrType(rrType)
    , m_hint(hint)
    , m_interestLifetime(DEFAULT_INTEREST_LIFETIME)
    , m_validator(m_face)
    , m_shouldValidateIntermediate(shouldValidateIntermediate)
    , m_hasError(false)
  {
    if (m_shouldValidateIntermediate)
      m_ctr = std::unique_ptr<IterativeQueryController>
        (new IterativeQueryController(m_dstLabel, m_rrType, m_interestLifetime,
                                      bind(&NdnsDig::onSucceed, this, _1, _2),
                                      bind(&NdnsDig::onFail, this, _1, _2),
                                      m_face, &m_validator));
    else
      m_ctr = std::unique_ptr<IterativeQueryController>
        (new IterativeQueryController(m_dstLabel, m_rrType, m_interestLifetime,
                                      bind(&NdnsDig::onSucceed, this, _1, _2),
                                      bind(&NdnsDig::onFail, this, _1, _2),
                                      m_face, nullptr));
  }

  void
  run()
  {
    NDNS_LOG_INFO(" =================================== "
                  << "start to dig label = " << this->m_dstLabel
                  << " for type = " << this->m_rrType
                  << " =================================== ");

    try {
      m_ctr->start(); // non-block, may throw exception
      m_face.processEvents();
    }
    catch (std::exception& e) {
      std::cerr << "Error: " << e.what();
      m_hasError = true;
    }
  }

  void
  stop()
  {
    m_face.getIoService().stop();
    NDNS_LOG_TRACE("application stops.");
  }

  void
  setStartZone(const Name& start)
  {
    m_ctr->setStartComponentIndex(start.size());
  }

private:
  void
  onSucceed(const Data& data, const Response& response)
  {
    NDNS_LOG_INFO("Dig get following Response (need verification):");
    Name name = Name().append(response.getZone()).append(response.getRrLabel());
    if (name == m_dstLabel && m_rrType == response.getRrType()) {
      NDNS_LOG_INFO("This is the final response returned by zone=" << response.getZone()
                    << " and NdnsType=" << response.getNdnsType()
                    << ". It contains " << response.getRrs().size() << " RR(s)");

      std::string msg;
      size_t i = 0;
      for (const auto& rr : response.getRrs()) {
        try {
          msg = std::string(reinterpret_cast<const char*>(rr.value()), rr.value_size());
          NDNS_LOG_INFO("succeed to get the info from RR[" << i << "]"
                        "type=" << rr.type() << " content=" << msg);
        }
        catch (std::exception& e) {
          NDNS_LOG_INFO("error to get the info from RR[" << i << "]"
                        "type=" << rr.type());
        }
        ++i;
      }
    }
    else {
      NDNS_LOG_INFO("[* !! *] This is not final response.The target Label: "
                    << m_dstLabel << " may not exist");
    }

    if (m_dstFile.empty()) {
      ;
    }
    else if (m_dstFile == "-") {
      output(data, std::cout, true);
    }
    else {
      NDNS_LOG_INFO("output Data packet to " << m_dstFile << " with BASE64 encoding format");
      std::filebuf fb;
      fb.open(m_dstFile, std::ios::out);
      std::ostream os(&fb);
      output(data, os, false);
    }

    NDNS_LOG_INFO(response);

    NDNS_LOG_TRACE("to verify the response");
    m_validator.validate(data,
                         bind(&NdnsDig::onDataValidated, this, _1),
                         bind(&NdnsDig::onDataValidationFailed, this, _1, _2)
                         );
  }


  void
  onFail(uint32_t errCode, const std::string& errMsg)
  {
    NDNS_LOG_INFO("fail to get response: errCode=" << errCode << " msg=" << errMsg);
    m_hasError = true;
    this->stop();
  }

  void
  onDataValidated(const shared_ptr<const Data>& data)
  {
    NDNS_LOG_INFO("final data pass verification");
    this->stop();
  }

  void
  onDataValidationFailed(const shared_ptr<const Data>& data, const std::string& str)
  {
    NDNS_LOG_INFO("final data does not pass verification");
    m_hasError = true;
    this->stop();
  }

public:
  void
  setInterestLifetime(const time::milliseconds& lifetime)
  {
    m_interestLifetime = lifetime;
  }

  const bool
  hasError() const
  {
    return m_hasError;
  }

  void
  setDstFile(const std::string& dstFile)
  {
    m_dstFile = dstFile;
  }

private:
  Name m_dstLabel;
  name::Component m_rrType;

  Name m_hint;
  Name m_certName;
  time::milliseconds m_interestLifetime;

  Face m_face;

  Validator m_validator;
  bool m_shouldValidateIntermediate;
  std::unique_ptr<QueryController> m_ctr;

  bool m_hasError;
  std::string m_dstFile;
};

} // namespace ndns
} // namespace ndn


int
main(int argc, char* argv[])
{
  ndn::ndns::log::init();
  using std::string;
  using namespace ndn;

  Name dstLabel;
  int ttl = 4;
  string rrType = "TXT";
  string dstFile;
  bool shouldValidateIntermediate = true;
  Name start("/ndn");

  try {
    namespace po = boost::program_options;
    po::variables_map vm;

    po::options_description generic("Generic Options");
    generic.add_options()("help,h", "print help message");

    po::options_description config("Configuration");
    config.add_options()
      ("timeout,T", po::value<int>(&ttl), "query timeout. default: 4 sec")
      ("rrtype,t", po::value<std::string>(&rrType), "set request RR Type. default: TXT")
      ("dstFile,d", po::value<std::string>(&dstFile), "set output file of the received Data. "
       "if omitted, not print; if set to be -, print to stdout; else print to file")
      ("start,s", po::value<Name>(&start)->default_value("/ndn"), "set first zone to query")
      ("not-validate,n", "trigger not validate intermediate results")
      ;

    po::options_description hidden("Hidden Options");
    hidden.add_options()
      ("name", po::value<Name>(&dstLabel), "name to be resolved")
      ;
    po::positional_options_description postion;
    postion.add("name", 1);

    po::options_description cmdline_options;
    cmdline_options.add(generic).add(config).add(hidden);

    po::options_description config_file_options;
    config_file_options.add(config).add(hidden);

    po::options_description visible("Usage: ndns-dig /name/to/be/resolved [-t rrType] [-T ttl]"
                                    "[-d dstFile] [-s startZone] [-n]\n"
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

    if (!vm.count("name")) {
      std::cerr << "must contain a target label parameter." << std::endl;
      std::cerr << visible << std::endl;
      return 1;
    }

    if (!start.isPrefixOf(dstLabel)) {
      std::cerr << "Error: start zone " << start << " is not prefix of the target label "
                << dstLabel << std::endl;
      return 1;
    }

    if (vm.count("not-validate")) {
      shouldValidateIntermediate = false;
    }

    if (ttl < 0) {
      std::cerr << "Error: ttl parameter cannot be negative" << std::endl;
      return 1;
    }
  }
  catch (const std::exception& ex) {
    std::cerr << "Parameter Error: " << ex.what() << std::endl;
    return 1;
  }

  try {
    ndn::ndns::NdnsDig dig("", dstLabel, ndn::name::Component(rrType), shouldValidateIntermediate);
    dig.setInterestLifetime(ndn::time::seconds(ttl));
    dig.setDstFile(dstFile);

    // Due to ndn testbed does not contain the root zone
    // dig here starts from the TLD (Top-level Domain)
    // precondition is that TLD : 1) only contains one component in its name; 2) its name is routable
    dig.setStartZone(start);

    dig.run();

    if (dig.hasError())
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
