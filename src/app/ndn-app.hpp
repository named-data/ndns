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
#ifndef NDNS_NDN_APP_HPP
#define NDNS_NDN_APP_HPP

#include <ndn-cxx/face.hpp>
#include <ndn-cxx/name.hpp>
#include <ndn-cxx/security/key-chain.hpp>

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/noncopyable.hpp>

#include "ndns-label.hpp"

using namespace std;

namespace ndn {
namespace ndns {

class NDNApp
{
public:
  NDNApp(const char *programName, const char *prefix);
  virtual ~NDNApp();

  virtual void onData(const ndn::Interest& interest, Data& data)
  {
  }
  virtual void onTimeout(const ndn::Interest& interest)
  {
    std::cout << "!- Interest Timeout" << interest.getName() << std::endl;
  }

  virtual void onRegisterFailed(const ndn::Name& prefix,
      const std::string& reason)
  {
    m_error = "ERROR: Failed to register prefix in local hub's daemon";
    m_error += " due to: ";
    m_error += reason;
    m_hasError = true;
    this->stop();
  }

  virtual void onInterest(const Name &name, const Interest& interest)
  {

  }

  virtual void signalHandler()
  {
    this->stop();
    exit(1);
  }

  virtual void stop()
  {
    std::cout << m_programName << " stops" << std::endl;
    m_ioService.stop();
    m_face.shutdown();
    if (hasError()) {
      cout << m_error << endl;
    }
  }

  bool hasError() const
  {
    return m_hasError;
  }

  void setHasError(bool hasError)
  {
    m_hasError = hasError;
  }

  const char* getPrefix() const
  {
    return m_prefix;
  }

  void setPrefix(char* prefix)
  {
    m_prefix = prefix;
  }

  const char* getProgramName() const
  {
    return m_programName;
  }

  void setProgramName(char* programName)
  {
    m_programName = programName;
  }

  const string& getErr() const
  {
    return m_error;
  }

  void setErr(const string& err)
  {
    m_error = err;
  }

  uint32_t getInterestTriedMax() const
  {
    return m_interestTriedMax;
  }

  void setInterestTriedMax(uint32_t interestTriedMax)
  {
    m_interestTriedMax = interestTriedMax;
  }

  time::milliseconds getContentFreshness() const
  {
    return m_contentFreshness;
  }

  void setContentFreshness(time::milliseconds contentFreshness)
  {
    m_contentFreshness = contentFreshness;
  }

  time::milliseconds getInterestLifetime() const
  {
    return m_interestLifetime;
  }

  void setInterestLifetime(time::milliseconds interestLifetime)
  {
    m_interestLifetime = interestLifetime;
  }

  const Name& getForwardingHint() const
  {
    return m_forwardingHint;
  }

  void setForwardingHint(const Name& forwardingHint)
  {
    m_forwardingHint = forwardingHint;
  }

  unsigned short getEnableForwardingHint() const
  {
    return m_enableForwardingHint;
  }

  void setEnableForwardingHint(unsigned short enableForwardingHint)
  {
    m_enableForwardingHint = enableForwardingHint;
  }

protected:
  const char* m_programName;
  const char* m_prefix;bool m_hasError;
  string m_error;
  time::milliseconds m_contentFreshness;
  time::milliseconds m_interestLifetime;

  uint32_t m_interestTriedMax;
  uint32_t m_interestTriedNum;

  //Name m_name;
  boost::asio::io_service m_ioService;
  //Zone m_zone;
  //ZoneMgr m_zoneMgr;

  Face m_face;
  KeyChain m_keyChain;

  /**
   * forwarding hint
   */
  unsigned short m_enableForwardingHint;
  Name m_forwardingHint;
};

} //namespace ndns
} /* namespace ndn */

#endif /* NDN_APP_HPP_ */
