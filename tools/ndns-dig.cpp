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

#include <ndn-cxx/security/key-chain.hpp>
#include <ndn-cxx/face.hpp>
#include <boost/program_options.hpp>
#include <boost/asio.hpp>
#include <boost/filesystem.hpp>
#include <boost/noncopyable.hpp>

#include <memory>
#include <string>


namespace ndn {
namespace ndns {
NDNS_LOG_INIT("NdnsDig");

class NdnsDig
{
public:
  NdnsDig(const Name& hint, const Name& dstLabel,
          const name::Component& rrType)
    : m_dstLabel(dstLabel)
    , m_rrType(rrType)
    , m_hint(hint)
    , m_interestLifetime(DEFAULT_INTEREST_LIFETIME)
    , m_validator(m_face)
    , m_ctr(new IterativeQueryController(m_dstLabel, m_rrType, m_interestLifetime,
                                         bind(&NdnsDig::onSucceed, this, _1, _2),
                                         bind(&NdnsDig::onFail, this, _1, _2),
                                         m_face))
    , m_hasError(false)
  {
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
      NDNS_LOG_FATAL("Error: Face fails to process events: " << e.what());
      m_hasError = true;
    }
  }

  void
  stop()
  {
    m_face.getIoService().stop();
    NDNS_LOG_TRACE("application stops.");
  }

private:
  void
  onSucceed(const Data& data, const Response& response)
  {
    NDNS_LOG_INFO("Dig get following final Response (need verification):");
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

private:
  Name m_dstLabel;
  name::Component m_rrType;

  Name m_hint;
  Name m_certName;
  time::milliseconds m_interestLifetime;

  Face m_face;

  Validator m_validator;
  std::unique_ptr<QueryController> m_ctr;

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

  Name dstLabel;
  int ttl = 4;
  string rrType = "TXT";

  try {
    namespace po = boost::program_options;
    po::variables_map vm;

    po::options_description generic("Generic Options");
    generic.add_options()("help,h", "print help message");

    po::options_description config("Configuration");
    config.add_options()
      ("timeout,T", po::value<int>(&ttl), "waiting seconds of query. default: 10 sec")
      ("rrtype,t", po::value<std::string>(&rrType), "set request RR Type. default: TXT")
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

    po::options_description visible("Allowed options");
    visible.add(generic).add(config);

    po::parsed_options parsed =
      po::command_line_parser(argc, argv).options(cmdline_options).positional(postion).run();

    po::store(parsed, vm);
    po::notify(vm);

    if (vm.count("help")) {
      std::cout << "Usage: dig /name/to/be/resolved [-t rrType] [-T ttl]" << std::endl;
      std::cout << visible << std::endl;
      return 0;
    }
  }
  catch (const std::exception& ex) {
    std::cerr << "Parameter Error: " << ex.what() << std::endl;
    return 0;
  }

  ndn::ndns::NdnsDig dig("", dstLabel, ndn::name::Component(rrType));
  dig.setInterestLifetime(ndn::time::milliseconds(ttl * 1000));

  dig.run();
  if (dig.hasError())
    return 1;
  else
    return 0;
}
