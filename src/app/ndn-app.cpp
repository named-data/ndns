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
#include "ndn-app.hpp"

namespace ndn {
namespace ndns {

NDNApp::NDNApp(const char *programName, const char *prefix)
  : m_programName(programName)
  , m_prefix(prefix)
  , m_hasError(false)
  , m_contentFreshness(time::milliseconds(40000))
  , m_interestLifetime(time::milliseconds(4000))
  , m_interestTriedMax(2)
  , m_interestTriedNum(0)
  , m_face(m_ioService)
{
}

NDNApp::~NDNApp()
{
  m_programName = 0;
  m_prefix = 0;
}





} //namespace ndns
} /* namespace ndn */
