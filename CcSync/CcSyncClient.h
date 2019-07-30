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
 * @subpage   CcSyncClient
 *
 * @page      CcSyncClient
 * @copyright Andreas Dirmeier (C) 2017
 * @author    Andreas Dirmeier
 * @par       Web: http://coolcow.de
 * @version   0.01
 * @date      2016-04
 * @par       Language   C++ ANSI V3
 * @brief     Class CcSyncClient
 **/
#ifndef _CcSyncClient_H_
#define _CcSyncClient_H_

#include "CcBase.h"
#include "CcSync.h"
#include "CcString.h"
#include "CcList.h"
#include "CcSyncClientConfig.h"
#include "CcSyncDbClient.h"
#include "CcSyncDirectory.h"
#include "Network/CcSocket.h"
#include "CcSyncRequest.h"
#include "CcSyncResponse.h"
#include "CcSslSocket.h"

#ifdef _MSC_VER
template class CcSyncSHARED CcList<CcSyncDirectory>;
template class CcSyncSHARED CcSharedPointer<CcSslSocket>;
#endif

// Forward Declarrations
class CcFile;

/**
 * @brief Class impelmentation
 */
class CcSyncSHARED CcSyncClient
{
public:

  /**
   * @brief Constructor
   */
  CcSyncClient(const CcString& sConfigFilePath);

  /**
   * @brief CopyConstructor
   */
  CcSyncClient( const CcSyncClient& oToCopy );

  /**
   * @brief MoveConstructor
   */
  CcSyncClient( CcSyncClient&& oToMove );

  /**
   * @brief Destructor
   */
  virtual ~CcSyncClient(void);

  CcSyncClient& operator=(const CcSyncClient& oToCopy);
  CcSyncClient& operator=(CcSyncClient&& oToMove);

  bool login();
  bool isLoggedIn();
  void logout();
  bool isAdmin();
  bool serverRescan(bool bDeep=false);
  bool serverStop();
  void cleanDatabase();
  void doRemoteSync(const CcString& sDirectoryName);
  void doRemoteSyncAll();
  void doLocalSync(const CcString& sDirectoryName, bool bDeepScan = false);
  void doLocalSyncAll(bool bDeepScan = false);
  void doQueue(const CcString& sDirectoryName);
  void doQueues();
  void doUpdateChanged();
  bool updateFromRemoteAccount();
  bool updateToRemoteAccount();
  bool setDirectoryLock(const CcString& sDirname);
  bool setDirectoryUnlock(const CcString& sDirname);
  bool updateDirectoryBackupCommand(const CcString& sDirname, const CcString& sBackupCommand);
  bool updateDirectoryRestoreCommand(const CcString& sDirname, const CcString& sBackupCommand);
  bool verify(const CcString& sDirname);
  bool refresh(const CcString& sDirname);
  bool updateDirectorySetUser(const CcString& sDirname, const CcString& sUser);
  bool updateDirectorySetGroup(const CcString& sDirname, const CcString& sGroup);
  bool doAccountCreateDirectory(const CcString sDirectoryName, const CcString& sLocalDirectoryPath = "");
  bool doAccountRemoveDirectory(const CcString sDirectoryName);
  void resetQueue(const CcString sDirectoryName);
  void resetQueues();
  void close();
  void reconnect();
  bool selectAccount(const CcString& sNewAccount);
  bool createConfig(const CcString& sConfigDir);
  bool addAccount(const CcString& sUsername, const CcString& sPassword, const CcString& sServer, const CcString& sPort);
  bool addRemoteAccount(const CcString& sUsername, const CcString& sPassword);
  bool removeAccount(const CcString& sUsername);
  bool isConfigAvailable()
    {return m_bConfigAvailable;}
  CcString getAccountInfo();
  CcString getDirectoryInfo(const CcString& sDirectoryName);
  const CcString& getAccountName()
    { return m_pAccount->getName(); }
  CcStringList getAccountList();
  CcStringList getDirectoryList();

  CcSyncAccountConfigHandle& getCurrentAccountConfig()
    { return m_pAccount; }

  bool changeHostname(const CcString& sHostName);

  static CcSyncClient* create(const CcString& sConfigFilePath);
  static void remove(CcSyncClient* pToRemove);

private: // Methods
  void init(const CcString& sConfigFile);
  void deinit();
  bool setupDatabase();
  bool checkSqlTables();
  bool setupSqlTables();
  bool connect();
  bool sendRequestGetResponse();
  void recursiveRemoveDirectory(CcSyncDirectory& oDirectory, CcSyncFileInfo& oFileInfo);
  bool sendFile(CcSyncFileInfo& oFileInfo);
  bool receiveFile(CcFile* pFile, CcSyncFileInfo& oFileInfo);
  bool doRemoteSyncDir(CcSyncDirectory& oDirectory, uint64 uiDirId);
  bool serverDirectoryEqual(CcSyncDirectory& oDirectory, uint64 uiDirId);
  bool doCreateDir(CcSyncDirectory& oDirectory, CcSyncFileInfo& oFileInfo, uint64 uiQueueIndex);
  bool doRemoveDir(CcSyncDirectory& oDirectory, CcSyncFileInfo& oFileInfo, uint64 uiQueueIndex);
  bool doUpdateDir(CcSyncDirectory& oDirectory, CcSyncFileInfo& oFileInfo, uint64 uiQueueIndex);
  bool doDownloadDir(CcSyncDirectory& oDirectory, CcSyncFileInfo& oDirInfo, uint64 uiQueueIndex);
  bool doUploadFile(CcSyncDirectory& oDirectory, CcSyncFileInfo& oFileInfo, uint64 uiQueueIndex);
  bool doRemoveFile(CcSyncDirectory& oDirectory, CcSyncFileInfo& oFileInfo, uint64 uiQueueIndex);
  bool doDownloadFile(CcSyncDirectory& oDirectory, CcSyncFileInfo& oFileInfo, uint64 uiQueueIndex);
  static void setFileInfo(const CcString& sPathToFile, uint32 uiUserId, uint32 uiGroupId, int64 iModified);

private: // Member
  CcString                      m_sConfigPath;
  CcString                      m_sDatabaseFile;
  CcSyncClientConfig            m_oConfig;
  CcSyncAccountConfigHandle     m_pAccount    = nullptr;
  CcSyncDbClientPointer         m_pDatabase   = nullptr;
  CcList<CcSyncDirectory>       m_oBackupDirectories;
  CcSocket                      m_oSocket;
  CcString        m_sSession;
  CcSyncResponse  m_oResponse;
  CcSyncRequest   m_oRequest;
  bool            m_bLogin = false;
  bool            m_bConfigAvailable = false;
  size_t          m_uiReconnections = 0;
};

#endif /* _CcSyncClient_H_ */
