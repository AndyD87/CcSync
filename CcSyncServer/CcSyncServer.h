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

 * @page      CcSync
 * @subpage   CcSyncServer
 *
 * @page      CcSyncServer
 * @copyright Andreas Dirmeier (C) 2017
 * @author    Andreas Dirmeier
 * @par       Web: http://coolcow.de
 * @version   0.01
 * @date      2016-04
 * @par       Language   C++ ANSI V3
 * @brief     Class CcSyncServer
 **/
#ifndef _CcSyncServer_H_
#define _CcSyncServer_H_

#include "CcBase.h"
#include "CcUserList.h"
#include "CcSyncServerConfig.h"
#include "CcApp.h"
#include "CcSyncDbServer.h"
#include "CcSyncServerAccount.h"
#include "CcSslSocket.h"
#include "Network/CcSocket.h"
#include "CcArguments.h"

/**
 * @brief Class impelmentation
 */
class CcSyncServer : public CcApp
{
public:
  /**
   * @brief Constructor
   */
  CcSyncServer(int pArgc, char **ppArgv);

  /**
   * @brief CopyConstructor
   */
  CcSyncServer( const CcSyncServer& oToCopy );

  /**
   * @brief MoveConstructor
   */
  CcSyncServer( CcSyncServer&& oToMove );

  /**
   * @brief Destructor
   */
  virtual ~CcSyncServer( void );

  void run() override;
  void runHelp();
  void runServer();

  void onStop() override;

  CcSyncServerConfig& config()
    { return m_oConfig; }

  CcSyncDbServer& database()
    { return m_oDatabase; }

  CcSyncServer& operator=(const CcSyncServer& oToCopy);
  CcSyncServer& operator=(CcSyncServer&& oToMove);
  CcSyncUser loginUser(const CcString& sAccount, const CcString& sUserName, const CcString& sPassword);

  CcSyncUser getUserByToken(const CcString& sToken);
  CcSyncUser getUserByName(const CcString& sName);

  void setConfigDir(const CcString& sConfigDir)
    { m_sConfigDir = sConfigDir;}

  virtual CcVersion getVersion() const override;

  bool createConfig();
  bool createAccount(const CcString& sUsername, const CcString& sPassword, bool bAdmin);
  bool removeAccount(const CcString& sUsername);


private:
  bool setupDatabase();

private:
  CcArguments           m_oArguments;
  CcString              m_sConfigDir;
  bool                  m_bOverwriteDefaultDirs = false;
  CcString              m_sDatabaseFile;
  CcSyncServerConfig    m_oConfig;
  CcSyncDbServer        m_oDatabase;
  CcSocket              m_oSocket;
};

#endif /* _CcSyncServer_H_ */
