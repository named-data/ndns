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

#ifndef NDNS_DB_MGR_CPP
#define NDNS_DB_MGR_CPP

#include <sqlite3.h>
#include <iostream>


namespace ndn {
namespace ndns {


class DBMgr
{
public:
  enum m_status
  {
    DBConnected,
    DBClosed,
    DBError
  };

public:
  DBMgr();
  virtual ~DBMgr();

  void open();
  void close();
  void execute(std::string sql,
        int (*callback)(void*,int,char**,char**), void * paras);

  inline void addResultNum(){
    m_resultNum += 1;
  }
  inline void clearResultNum() {
    m_resultNum = 0;
  }

  const std::string& getDbfile() const {
    return m_dbfile;
  }

  void setDbfile(const std::string& dbfile) {
    this->m_dbfile = dbfile;
  }

  const std::string& getErr() const {
    return m_err;
  }

  void setErr(const std::string& err) {
    this->m_err = err;
  }

  int getReCode() const {
    return m_reCode;
  }

  void setReCode(int reCode) {
    this->m_reCode = reCode;
  }

  m_status getStatus() const {
    return m_status;
  }

  void setStatus(m_status status) {
    this->m_status = status;
  }

  int getResultNum() const {
    return m_resultNum;
  }

  void setResultNum(int resultNum) {
    m_resultNum = resultNum;
  }


private:
  std::string m_dbfile;
  sqlite3 *m_conn;
  std::string m_err;
  int m_reCode;
  m_status m_status;
  int m_resultNum;
}; //class DBMgr
}//namespace ndns
}//namespace ndn

#endif
