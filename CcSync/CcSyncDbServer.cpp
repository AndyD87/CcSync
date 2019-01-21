/*
 * This file is part of CcSync.
 *
 * CcSync is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * CcSync is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with CcSync.  If not, see <http://www.gnu.org/licenses/>.
 **/
/**
 * @file
 * @copyright Andreas Dirmeier (C) 2016
 * @author    Andreas Dirmeier
 * @par       Web: http://coolcow.de
 * @version   0.01
 * @date      2016-04
 * @par       Language   C++ ANSI V3
 * @brief     Implemtation of class CcSyncDbServer
 */
#include "CcSyncDbServer.h"
#include "CcString.h"
#include "CcSqlite.h"
#include "CcSyncGlobals.h"
#include "CcKernel.h"
#include "CcDateTime.h"
#include "CcSyncLog.h"

CcSyncDbServer::CcSyncDbServer( const CcString& sPath )
{
  openDatabase(sPath);
}

CcSyncDbServer::CcSyncDbServer( const CcSyncDbServer& oToCopy )
{
  operator=(oToCopy);
}

CcSyncDbServer::CcSyncDbServer( CcSyncDbServer&& oToMove )
{
  operator=(std::move(oToMove));
}

CcSyncDbServer::~CcSyncDbServer( void )
{
  if (m_pDatabase != nullptr)
  {
    m_pDatabase->close();
  }
}

CcSyncDbServer& CcSyncDbServer::operator=(const CcSyncDbServer& oToCopy)
{
  CCUNUSED(oToCopy);
  return *this;
}

CcSyncDbServer& CcSyncDbServer::operator=(CcSyncDbServer&& oToMove)
{
  if(this != &oToMove)
  {
  }
  return *this;
}

bool CcSyncDbServer::operator==(const CcSyncDbServer& oToCompare) const
{
  bool bRet = false;
  CCUNUSED(oToCompare);
  return bRet;
}

bool CcSyncDbServer::operator!=(const CcSyncDbServer& oToCompare) const
{
  return !operator==(oToCompare);
}

bool CcSyncDbServer::openDatabase(const CcString& sPath)
{
  m_pDatabase = new CcSqlite();
  CCMONITORNEW(m_pDatabase.getPtr());
  bool bRet = false;
  m_pDatabase->setDatabase(sPath);
  if (m_pDatabase->open())
  {
    if (!m_pDatabase->tableExists(CcSyncGlobals::Server::Database::TableNameUser))
    {
      if (m_pDatabase->query(getDbCreateUser()).ok())
      {
        bRet = true;
      }
      else
      {
        CcSyncLog::writeError("Unable to create UserTable for " + sPath, ESyncLogTarget::Server);
      }
    }
    else
    {
      bRet = true;
    }
  }
  else
  {
    CcSyncLog::writeError("Unable to open database for " + sPath, ESyncLogTarget::Server);
  }
  return bRet;
}

bool CcSyncDbServer::userExistsInDatabase(const CcString& sAccountName, const CcString& sUsername)
{
  bool bRet = false;
  CcString sRequest = "SELECT `";
  sRequest << CcSyncGlobals::Database::User::Id << "` FROM `";
  sRequest << CcSyncGlobals::Server::Database::TableNameUser;
  sRequest << "` WHERE ";
  sRequest << "`" << CcSyncGlobals::Database::User::Account << "` = '" << CcSqlite::escapeString(sAccountName) << "' ";
  sRequest << "AND `" << CcSyncGlobals::Database::User::Username << "` = '" << CcSqlite::escapeString(sUsername) << "'";
  CcSqlResult sResult = m_pDatabase->query(sRequest);
  if (sResult.ok() &&
    sResult.size() > 0)
  {
    bRet = true;
  }
  return bRet;
}

bool CcSyncDbServer::updateUser(const CcString& sAccountName, const CcString& sUsername, const CcString& sToken)
{
  CcString sUpdateString = getDbUpdateUser(sAccountName, sUsername, sToken);
  CcSqlResult oResult = m_pDatabase->query(sUpdateString);
  if (oResult.ok())
  {
    return true;
  }
  CcSyncLog::writeError("Update user in database failed " + sAccountName, ESyncLogTarget::Server);
  return false;
}

bool CcSyncDbServer::insertUser(const CcString& sAccountName, const CcString& sUsername, const CcString& sToken)
{
  CcString sRequest = getDbInsertUser(sAccountName, sUsername, sToken);
  CcSqlResult oResult = m_pDatabase->query(sRequest);
  if (oResult.ok())
  {
    return true;
  }
  CcSyncLog::writeError("Insert user to database failed " + sAccountName, ESyncLogTarget::Server);
  return false;
}

bool CcSyncDbServer::getUserByToken(const CcString& sToken, CcString& sAccountName, CcString& sUserName)
{
  CcString sSqlQuery("SELECT `");
  sSqlQuery << CcSyncGlobals::Database::User::Account << "`, `" << CcSyncGlobals::Database::User::Username << "` FROM ";
  sSqlQuery << "`" << CcSyncGlobals::Server::Database::TableNameUser << "` ";
  sSqlQuery << "WHERE `" << CcSyncGlobals::Database::User::Session << "`='" << CcSqlite::escapeString(sToken) << "'";
  CcSqlResult oResult = m_pDatabase->query(sSqlQuery);
  if (oResult.ok() &&
      oResult.size() > 0)
  {
    sAccountName = oResult[0][0].getString();
    sUserName = oResult[0][1].getString();
    return true;
  }
  return false;
}

CcString CcSyncDbServer::getDbCreateUser()
{
  CcString sRet(CcSyncGlobals::Database::CreateTable);
  sRet << CcSyncGlobals::Server::Database::TableNameUser;
  sRet << "` ("\
    "`" << CcSyncGlobals::Database::User::Id << "` INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,"\
    "`" << CcSyncGlobals::Database::User::Account << "` TEXT,"\
    "`" << CcSyncGlobals::Database::User::Username << "` TEXT,"\
    "`" << CcSyncGlobals::Database::User::Session << "` TEXT,"\
    "`" << CcSyncGlobals::Database::User::SessionEnd << "` TIMESTAMP)";
  return sRet;
}

CcString CcSyncDbServer::getDbInsertUser(const CcString& sAccountName, const CcString& sUsername, const CcString& sToken)
{
  CcString sRet(CcSyncGlobals::Database::Insert);
  sRet << CcSyncGlobals::Server::Database::TableNameUser << "`("
       << "`" << CcSyncGlobals::Database::User::Id << "`,"
       << "`" << CcSyncGlobals::Database::User::Account << "`,"
       << "`" << CcSyncGlobals::Database::User::Username << "`,"
       << "`" << CcSyncGlobals::Database::User::Session << "`,"
       << "`" << CcSyncGlobals::Database::User::SessionEnd << "`"
       << ") "
       << "VALUES ("
       << "NULL,"
       << "'" << CcSqlite::escapeString(sAccountName) << "',"
       << "'" << CcSqlite::escapeString(sUsername) << "',"
       << "'" << CcSqlite::escapeString(sToken) << "',"
       << CcString::fromNumber(CcKernel::getDateTime().getTimestampS() + CcSyncGlobals::SessionLength)
       << ")";
  return sRet;
}

CcString CcSyncDbServer::getDbUpdateUser(const CcString& sAccountName, const CcString& sUsername, const CcString& sToken)
{
  CcString sRet(CcSyncGlobals::Database::Update);
  sRet << CcSyncGlobals::Server::Database::TableNameUser
       << "` SET "
       << "`" << CcSyncGlobals::Database::User::Session << "` = '" << CcSqlite::escapeString(sToken) << "' ,"
       << "`" << CcSyncGlobals::Database::User::SessionEnd << "` = CURRENT_TIMESTAMP "
       << "WHERE "
       << "`" << CcSyncGlobals::Database::User::Account << "` = '" << CcSqlite::escapeString(sAccountName) << "'"
       << "AND `" << CcSyncGlobals::Database::User::Username << "` = '" << CcSqlite::escapeString(sUsername) << "'";
  return sRet;
}
