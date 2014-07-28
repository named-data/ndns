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

#include "../boost-test.hpp"

#include "db/zone-mgr.hpp"
#include "zone.hpp"

//#include <ndn-cxx/name.hpp>
namespace ndn {
namespace ndns {
namespace tests {

using namespace std;

BOOST_AUTO_TEST_SUITE(zoneMgr)

BOOST_AUTO_TEST_CASE(db)
{
  string label = "zoneMgr::db";
  printbegin(label);
  Zone zone;
  zone.setAuthorizedName(Name("/net"));
  zone.setId(11);

  ZoneMgr mgr(zone); //the construction will lookup zone.id, /net's id is 2
  BOOST_CHECK_EQUAL(mgr.getZone().getId(), 2);
  BOOST_CHECK_EQUAL(zone.getId(), 2);


  zone.setId(11);
  BOOST_CHECK_EQUAL(mgr.getZone().getId(), 11);

  zone.setId(12);
  BOOST_CHECK_EQUAL(mgr.getZone().getId(), 12);

  mgr.lookupId();
  cout<<"zone.name="<<mgr.getZone().getAuthorizedName()<<endl;
  cout<<"zone.id="<<mgr.getZone().getId()<<endl;
  cout<<"zone.id="<<mgr.getZone().getId()<<endl;
  cout<<"&zone="<<&zone<<endl;
  cout<<"&mgr.getZone()"<<&(mgr.getZone())<<endl;
  BOOST_CHECK_EQUAL(mgr.getZone().getId(), 2);
  BOOST_CHECK_EQUAL(zone.getId(), 2);

  printend(label);
}
BOOST_AUTO_TEST_SUITE_END()

} // namespace tests
} // namespace ndns
} // namespace ndn
