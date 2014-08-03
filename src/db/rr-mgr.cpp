/*
 * rr-mgr.cpp
 *
 *  Created on: 23 Jul, 2014
 *      Author: shock
 */

#include "rr-mgr.hpp"

namespace ndn {
namespace ndns {
RRMgr::RRMgr(Zone& zone, Query& query, Response& response)
  : m_count(0U)
  , m_zone(zone)
  , m_query(query)
  , m_response(response)
{

}

RRMgr::~RRMgr()
{
  // TODO Auto-generated destructor stub
}

int RRMgr::count()
{
  std::string sql;
  sql = "SELECT count(*) FROM rrs INNER JOIN rrsets ON rrs.rrset_id=rrsets.id";
  sql += " WHERE rrsets.zone_id=";
  sql += std::to_string(m_zone.getId());
  sql += " AND ";
  sql += "rrsets.type=\'";
  sql += RR::toString(m_query.getRrType());
  sql += "\' AND ";
  sql += "rrsets.label LIKE\'";
  sql += m_query.getRrLabel().toUri() + "/%\'";

  std::cout << "sql=" << sql << std::endl;
  this->execute(sql, static_callback_countRr, this);

  if (this->getStatus() == DBMgr::DBError) {
    return -1;
  }

  return this->m_count;
}
int RRMgr::callback_countRr(int argc, char **argv, char **azColName)
{
  this->addResultNum();

  std::cout << this->getResultNum() << "th result: " << "count=" << argv[0]
      << std::endl;
  m_count = std::atoi(argv[0]);
  return 0;
}

int RRMgr::lookup()
{
  std::string sql;

  sql =
      "SELECT rrs.ttl, rrs.rrdata, rrs.id FROM rrs INNER JOIN rrsets ON rrs.rrset_id=rrsets.id";
  sql += " WHERE rrsets.zone_id=";
  sql += std::to_string(m_zone.getId());
  sql += " AND ";
  sql += "rrsets.type=\'";
  sql += RR::toString(m_query.getRrType());
  sql += "\' AND ";
  sql += "rrsets.label=\'";
  sql += m_query.getRrLabel().toUri() + "\'";
  sql += " ORDER BY rrs.id";

  std::cout << "sql=" << sql << std::endl;
  this->execute(sql, static_callback_getRr, this);

  if (this->getStatus() == DBMgr::DBError) {
    return -1;
  }

  //m_response.setQueryName(m_query.getAuthorityZone());
  return 0;
}

int RRMgr::callback_getRr(int argc, char **argv, char **azColName)
{
  this->addResultNum();
  if (argc < 1) {
    this->setErr(
        "No RRType=" + RR::toString(m_query.getRrType()) + " and label="
            + m_query.getRrLabel().toUri() + " and zone="
            + m_zone.getAuthorizedName().toUri());
    return 0;
  }

  std::cout << this->getResultNum() << "th result: " << "id=" << argv[2]
      << " ttl=" << argv[0] << " rrdata=" << argv[1] << std::endl;

  m_response.setFreshness(time::milliseconds(std::atoi(argv[0])));

  m_response.addRr(std::atoi(argv[2]), argv[1]);
  //std::cout<<"after add a Rr: current size="<<m_response.getRrs().size()<<std::endl;
  //m_response.setFreshness(time::milliseconds(std::atoi(argv[0])));
  return 0;
}

}  //namespace ndnsn
} /* namespace ndn */
