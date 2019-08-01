/*
 * This file is part of CcOS.
 *
 * CcOS is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * CcOS is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with CcOS.  If not, see <http://www.gnu.org/licenses/>.
 **/
/**
 * @page      CcUtil
 * @subpage   CTestClient
 *
 * @page      CTestClient
 * @copyright Andreas Dirmeier (C) 2017
 * @author    Andreas Dirmeier
 * @par       Web:      http://coolcow.de/projects/CcOS
 * @par       Language: C++11
 * @brief     Class CTestClient
 **/
#ifndef _CTestClient_H_
#define _CTestClient_H_

#include "CcBase.h"
#include "CcProcess.h"
#include "CcStringList.h"
#include "CcSyncTestGlobals.h"

/**
 * @brief Class impelmentation
 */
class CTestClient 
{
public:
  /**
   * @brief Constructor
   */
  CTestClient(const CcString& sServerExePath, const CcString& sConfigDir);

  /**
   * @brief Destructor
   */
  virtual ~CTestClient( void );

  void resetArguments();

  bool clientExists();
  bool addNewServer(const CcString& sServerName, const CcString& sServerPort, const CcString& sUsername, const CcString& sPassword);
  bool login(const CcString& sServerName, const CcString& sUsername);
  bool logout();
  bool sync();
  bool checkLogin(const CcString& sServerName, const CcString& sUsername);
  bool createSyncDirectory(const CcString & sDirectoryPath);
  bool createDirectory(const CcString & sDirectoryPath);
  bool createFile(const CcString & sPathInDir, const CcString &sContent);
  bool serverShutdown();

private:
  CcString readWithTimeout(const CcString& sStringEnd, const CcDateTime &oTimeeout = CcSyncTestGlobals::DefaultSyncTimeout);
  bool readUntilSucceeded(const CcString& sStringEnd);

private:
  CcProcess m_oClientProc;
  CcString  m_sConfigDir;
  CcStringList m_sSyncDirs;
  CcString  m_sUsername;
};

#endif /* _CTestClient_H_ */
