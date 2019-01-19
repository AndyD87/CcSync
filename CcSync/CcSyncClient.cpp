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
 * @copyright Andreas Dirmeier (C) 2017
 * @author    Andreas Dirmeier
 * @par       Web: http://coolcow.de
 * @version   0.01
 * @date      2016-04
 * @par       Language   C++ ANSI V3
 * @brief     Implemtation of class CcSyncClient
 */
#include "CcSyncClient.h"
#include "CcSyncGlobals.h"
#include "CcSyncRequest.h"
#include "CcSyncResponse.h"
#include "CcKernel.h"
#include "CcFile.h"
#include "CcDirectory.h"
#include "CcSyncDirectory.h"
#include "Hash/CcCrc32.h"
#include "CcSslSocket.h"
#include "Network/CcCommonPorts.h"
#include "CcJson/CcJsonDocument.h"
#include "CcUserList.h"
#include "CcKernel.h"
#include "CcSyncDbClient.h"
#include "CcSyncLog.h"
#include "CcGlobalStrings.h"

CcSyncClient::CcSyncClient(const CcString& sConfigFilePath)
{
  if (sConfigFilePath.length() == 0)
  {
    CcString sConfigFile = CcKernel::getUserDataDir();
    sConfigFile.appendPath(CcSyncGlobals::ConfigDirName);
    sConfigFile.appendPath(CcSyncGlobals::Client::ConfigFileName);
    if (CcFile::exists(sConfigFile))
    {
      init(sConfigFile);
    }
    else
    {
      CcSyncLog::writeDebug("No configuration file in config-directory", ESyncLogTarget::Client);
    }
  }
  else
  {
    CcFile oFileObject(sConfigFilePath);
    if (oFileObject.isDir())
    {
      CcString sPath = sConfigFilePath;
      sPath.appendPath(CcSyncGlobals::Client::ConfigFileName);
      if (CcFile::exists(sPath))
      {
        init(sPath);
      }
      else
      {
        sPath = sConfigFilePath;
        sPath.appendPath(CcSyncGlobals::ConfigDirName);
        sPath.appendPath(CcSyncGlobals::Client::ConfigFileName);
        if (CcFile::exists(sPath))
        {
          init(sPath);
        }
      }
    }
    else if (oFileObject.isFile())
    {
      init(sConfigFilePath);
    }
    else
    {
      CcSyncLog::writeWarning("Configuration not found.", ESyncLogTarget::Client);
    }
  }
}

CcSyncClient::CcSyncClient(const CcSyncClient& oToCopy)
{
  operator=(oToCopy);
}

CcSyncClient::CcSyncClient(CcSyncClient&& oToMove)
{
  operator=(std::move(oToMove));
}

CcSyncClient::~CcSyncClient(void)
{
  deinit();
}

CcSyncClient& CcSyncClient::operator=(const CcSyncClient& oToCopy)
{
  CCUNUSED(oToCopy);
  CcSyncLog::writeError("Do not copy CcSyncClient!!!", ESyncLogTarget::Client);
  return *this;
}

CcSyncClient& CcSyncClient::operator=(CcSyncClient&& oToMove)
{
  if (this != &oToMove)
  {
    CcSyncLog::writeError("Do not move CcSyncClient!!!", ESyncLogTarget::Client);
  }
  return *this;
}

bool CcSyncClient::login()
{
  m_bLogin = false;
  if (m_pAccount != nullptr)
  {
    m_oRequest.setAccountLogin(m_pAccount->getName(), m_pAccount->getName(), m_pAccount->getPassword().getString());
    if (sendRequestGetResponse())
    {
      if (!m_oResponse.hasError())
      {
        m_sSession = m_oResponse.getSession();
        m_bLogin = true;
      }
    }
  }
  if(m_bLogin == false)
  {
    // Logout and cleanup
    logout();
  }
  return m_bLogin;
}

bool CcSyncClient::isLoggedIn()
{
  return m_bLogin;
}

void CcSyncClient::logout()
{
  if (m_pAccount != nullptr)
  {
    m_oRequest.init(ESyncCommandType::Close);
    sendRequestGetResponse();
    m_oBackupDirectories.clear();
    m_pDatabase.deleteCurrent();
    m_pAccount = nullptr;
  }
  close();
}

bool CcSyncClient::isAdmin()
{
  if (m_pAccount != nullptr)
  {
    m_oRequest.init(ESyncCommandType::AccountRights);
    if (sendRequestGetResponse())
    {
      ESyncRights eRights = m_oResponse.getAccountRight();
      if (eRights == ESyncRights::Admin)
      {
        return true;
      }
    }
  }
  return false;
}

bool CcSyncClient::serverRescan(bool bDeep)
{
  if (m_pAccount != nullptr)
  {
    m_oRequest.setServerRescan(bDeep);
    if (sendRequestGetResponse())
    {
      return true;
    }
  }
  return false;
}

void CcSyncClient::cleanDatabase()
{
  for (CcSyncDirectory& oDirectory : m_oBackupDirectories)
  {
    m_pDatabase->cleanDirectory(oDirectory.getName());
  }
}

void CcSyncClient::doRemoteSync(const CcString& sDirectoryName)
{
  if (m_bLogin)
  {
    for (CcSyncDirectory& oDirectory : m_oBackupDirectories)
    {
      if (oDirectory.getName() == sDirectoryName &&
          oDirectory.getLocation() != "")
      {
        if (!CcDirectory::exists(oDirectory.getLocation()))
        {
          CcSyncLog::writeError("Directory is set but not existing: " + oDirectory.getLocation(), ESyncLogTarget::Client);
        }
        else if (oDirectory.isLocked())
        {
          CcSyncLog::writeError("Directory is locked: " + oDirectory.getName(), ESyncLogTarget::Client);
        }
        else if (serverDirectoryEqual(oDirectory, CcSyncGlobals::Database::RootDirId))
        {
          CcSyncLog::writeDebug("Client is up to date", ESyncLogTarget::Client);
        }
        else
        {
          m_pDatabase->beginTransaction();
          CcSyncLog::writeDebug("Client is not up to date with Server, start equalizing", ESyncLogTarget::Client);
          doRemoteSyncDir(oDirectory, CcSyncGlobals::Database::RootDirId);
          m_pDatabase->endTransaction();
        }
      }
    }
  }
  else
  {
    CcSyncLog::writeError("Not yet logged in.", ESyncLogTarget::Client);
  }
}

void CcSyncClient::doRemoteSyncAll()
{
  if (m_bLogin)
  {
    for (CcSyncDirectory& oDirectory : m_oBackupDirectories)
    {
      doRemoteSync(oDirectory.getName());
    }
  }
  else
  {
    CcSyncLog::writeError("Not yet logged in.", ESyncLogTarget::Client);
  }
}

void CcSyncClient::doLocalSync(const CcString& sDirectoryName, bool bDeepScan)
{
  for (CcSyncDirectory& oDirectory : m_oBackupDirectories)
  {
    if (oDirectory.getName() == sDirectoryName)
    {
      oDirectory.scan(bDeepScan);
      break;
    }
  }
}

void CcSyncClient::doLocalSyncAll( bool bDeepScan)
{
  for (CcSyncDirectory& oDirectory : m_oBackupDirectories)
  {
    if(oDirectory.getLocation() != "" &&
       CcDirectory::exists(oDirectory.getLocation()) &&
      !oDirectory.isLocked())
    {
      oDirectory.scan(bDeepScan);
    }
  }
}

void CcSyncClient::doQueue(const CcString& sDirectoryName)
{
  for (CcSyncDirectory& oDirectory : m_oBackupDirectories)
  {
    if (oDirectory.getName() == sDirectoryName)
    {
      oDirectory.queueResetAttempts();
      while (oDirectory.queueHasItems())
      {
        if (connect() == false)
        {
          CcSyncLog::writeDebug("Connection Lost, stop process", ESyncLogTarget::Client);
          break;
        }
        else if (m_bLogin == false)
        {
          CcSyncLog::writeDebug("Login not yet done, stop process", ESyncLogTarget::Client);
          break;
        }
        else
        {
          CcSyncFileInfo oFileInfo;
          uint64 uiQueueIndex = 0;
          m_pDatabase->beginTransaction();
          EBackupQueueType eQueueType = oDirectory.queueGetNext(oFileInfo, uiQueueIndex);
          switch (eQueueType)
          {
            case EBackupQueueType::AddDir:
              doAddDir(oDirectory, oFileInfo, uiQueueIndex);
              break;
            case EBackupQueueType::RemoveDir:
              doRemoveDir(oDirectory, oFileInfo, uiQueueIndex);
              break;
            case EBackupQueueType::UpdateDir:
              doUpdateDir(oDirectory, oFileInfo, uiQueueIndex);
              break;
            case EBackupQueueType::DownloadDir:
              doDownloadDir(oDirectory, oFileInfo, uiQueueIndex);
              break;
            case EBackupQueueType::AddFile:
              doUploadFile(oDirectory, oFileInfo, uiQueueIndex);
              break;
            case EBackupQueueType::RemoveFile:
              doRemoveFile(oDirectory, oFileInfo, uiQueueIndex);
              break;
            case EBackupQueueType::UpdateFile:
              doUpdateFile(oDirectory, oFileInfo, uiQueueIndex);
              break;
            case EBackupQueueType::DownloadFile:
              doDownloadFile(oDirectory, oFileInfo, uiQueueIndex);
              break;
            default:
              oDirectory.queueIncrementItem(uiQueueIndex);
          }
          m_pDatabase->endTransaction();
        }
      }
    }
  }
}

void CcSyncClient::doQueues()
{
  for (CcSyncDirectory& oDirectory : m_oBackupDirectories)
  {
    doQueue(oDirectory.getName());
  }
}

void CcSyncClient::doUpdateChanged()
{
  for (CcSyncDirectory& oDirectory : m_oBackupDirectories)
  {
    m_pDatabase->beginTransaction();
    m_pDatabase->directoryListUpdateChangedAll(oDirectory.getName());
    m_pDatabase->endTransaction();
  }
  m_oRequest.init(ESyncCommandType::AccountDatabaseUpdateChanged);
  if (!sendRequestGetResponse())
  {
    CCDEBUG("Update changed in database request failed");
  }
}

bool CcSyncClient::doAccountCreateDirectory(const CcString sDirectoryName, const CcString& sDirectoryPath)
{
  bool bSuccess = false;
  m_oRequest.setAccountCreateDirectory(sDirectoryName);
  if (sendRequestGetResponse() &&
      m_oResponse.hasError() == false)
  {
    if (sDirectoryPath.length() == 0 ||
        CcDirectory::exists(sDirectoryPath) ||
        CcDirectory::create(sDirectoryPath, true))
    {
      m_pDatabase->setupDirectory(sDirectoryName);
      bSuccess = m_pAccount->addAccountDirectory(sDirectoryName, sDirectoryPath);
      checkSqlTables();
    }
    else
    {
      CcSyncLog::writeDebug("Directory create failed: " + sDirectoryPath, ESyncLogTarget::Client);
    }
  }
  else
  {
    CcSyncLog::writeError(m_oResponse.getErrorMsg(), ESyncLogTarget::Client);
  }
  return bSuccess;
}

bool CcSyncClient::doAccountRemoveDirectory(const CcString sDirectoryName)
{
  bool bSuccess = false;
  m_oRequest.setAccountRemoveDirectory(sDirectoryName);
  if (sendRequestGetResponse())
  {
    m_pDatabase->removeDirectory(sDirectoryName);
    bSuccess = m_pAccount->removeAccountDirectory(sDirectoryName);
  }
  else
  {
    CcSyncLog::writeError(m_oResponse.getErrorMsg(), ESyncLogTarget::Client);
  }
  return bSuccess;
}

void CcSyncClient::resetQueue(const CcString sDirectoryName)
{
  for (CcSyncDirectory& oDirectory : m_oBackupDirectories)
  {
    if (oDirectory.getName() == sDirectoryName)
      oDirectory.queueReset();
  }
}

void CcSyncClient::resetQueues()
{
  for (CcSyncDirectory& oDirectory : m_oBackupDirectories)
  {
    oDirectory.queueReset();
  }
}

bool CcSyncClient::updateFromRemoteAccount()
{
  bool bRet = false;
  CcString sRet;
  if (m_pAccount != nullptr)
  {
    m_oRequest.init(ESyncCommandType::AccountGetData);
    m_oRequest.setSession(m_sSession);
    if (sendRequestGetResponse())
    {
      CcSyncAccountConfig oAccountConfig = m_oResponse.getAccountConfig();
      if (oAccountConfig.getName() == m_pAccount->getName())
      {
        for (const CcSyncDirectoryConfig& oServerDirectory : oAccountConfig.getDirectoryList())
        {
          bool bFound = false;
          for (CcSyncDirectoryConfig& oClientDirectory : m_pAccount->directoryList())
          {
            if (oServerDirectory.getName() == oClientDirectory.getName())
            {
              if (oServerDirectory.getBackupCommand() != oClientDirectory.getBackupCommand())
                oClientDirectory.setBackupCommand(oServerDirectory.getBackupCommand());
              if (oServerDirectory.getRestoreCommand() != oClientDirectory.getRestoreCommand())
                oClientDirectory.setRestoreCommand(oServerDirectory.getRestoreCommand());
              bFound = true;
              break;
            }
          }
          if (bFound == false)
          {
            m_pAccount->addAccountDirectory(oServerDirectory.getName(), "");
          }
        }
      }
    }
  }
  return bRet;
}

bool CcSyncClient::updateToRemoteAccount()
{
  bool bRet = false;
  CcString sRet;
  if (m_pAccount != nullptr)
  {
    m_oRequest.init(ESyncCommandType::AccountSetData);
    m_oRequest.addAccountInfo(*m_pAccount.ptr());
    if (sendRequestGetResponse())
    {
      bRet = true;
    }
  }
  return bRet;
}

bool CcSyncClient::setDirectoryLock(const CcString& sDirname)
{
  bool bSuccess = false;
  if (m_pAccount != nullptr)
  {
    for (CcSyncDirectoryConfig& oDirConfig : m_pAccount->directoryList())
    {
      if (oDirConfig.getName() == sDirname)
      {
        CcString sPath = oDirConfig.getLocation();
        if (CcDirectory::exists(sPath))
        {
          sPath.appendPath(CcSyncGlobals::LockFile);
          if (CcFile::exists(sPath))
          {
            bSuccess = true;
          }
          else
          {
            CcFile oFile(sPath);
            if (oFile.open(EOpenFlags::Write))
            {
              oFile.writeString(CcSyncGlobals::LockFile);
              oFile.close();
              bSuccess = true;
            }
          }
        }
        break;
      }
    }
  }
  return bSuccess;
}

bool CcSyncClient::setDirectoryUnlock(const CcString& sDirname)
{
  bool bSuccess = false;
  if (m_pAccount != nullptr)
  {
    for (CcSyncDirectoryConfig& oDirConfig : m_pAccount->directoryList())
    {
      if (oDirConfig.getName() == sDirname)
      {
        CcString sPath = oDirConfig.getLocation();
        if (CcDirectory::exists(sPath))
        {
          sPath.appendPath(CcSyncGlobals::LockFile);
          if (CcFile::exists(sPath))
          {
            bSuccess = CcFile::remove(sPath);
          }
          else
          {
            bSuccess = true;
          }
        }
        break;
      }
    }
  }
  return bSuccess;
}

void CcSyncClient::close()
{
  m_oSocket.close();
  m_bLogin = false;
}


void CcSyncClient::reconnect()
{
  close();
  while(m_uiReconnections < CcSyncGlobals::MaxReconnections &&
        connect())
  {
    if (login())
      break;
    else
      close();
  }
}

bool CcSyncClient::selectAccount(const CcString& sNewAccount)
{
  bool bRet = false;
  if (m_pAccount != nullptr)
  {
    m_pAccount = nullptr;
    m_oBackupDirectories.clear();
  }
  if (sNewAccount.length() == 0)
    m_pAccount = m_oConfig.getFirstAccountConfig();
  else
    m_pAccount = m_oConfig.getAccountConfig(sNewAccount);
  if (m_pAccount != nullptr)
  {
    CcString sClientConfigDir = CcKernel::getUserDataDir();
    sClientConfigDir.appendPath(CcSyncGlobals::ConfigDirName);
    sClientConfigDir.appendPath(m_pAccount->getAccountDirName());
    // Check for Client config dir
    if (CcDirectory::exists(sClientConfigDir) ||
        CcDirectory::create(sClientConfigDir, true))
    {
      if (setupDatabase())
      {
        bRet = m_pAccount->isValid();
      }
    }
    else
    {
      CcSyncLog::writeInfo("Client location not found " + sClientConfigDir, ESyncLogTarget::Client);
      bRet = false;
    }
  }
  return bRet;
}

bool CcSyncClient::createConfig(const CcString& sConfigDir)
{
  CcString sConfigFile;
  if (sConfigDir.length() > 0)
  {
    sConfigFile = sConfigDir;
  }
  else
  {
    sConfigFile = CcKernel::getUserDataDir();
  }
  sConfigFile.appendPath(CcSyncGlobals::ConfigDirName);
  if (CcDirectory::exists(sConfigFile) ||
      CcDirectory::create(sConfigFile, true))
  {
    sConfigFile.appendPath(CcSyncGlobals::Client::ConfigFileName);
    return m_oConfig.create(sConfigFile);
  }
  else
  { 
    CcSyncLog::writeDebug("Client config dir not found and not created.", ESyncLogTarget::Client);
    return false;
  }
}

bool CcSyncClient::addAccount(const CcString& sUsername, const CcString& sPassword, const CcString& sServer, const CcString& sPort)
{
  return m_oConfig.addAccount(sUsername, sPassword, sServer, sPort);
}

bool CcSyncClient::addRemoteAccount(const CcString& sUsername, const CcString& sPassword)
{
  m_oRequest.setServerCreateAccount(sUsername, sPassword);
  if(sendRequestGetResponse())
  {
    return true;
  }
  return false;
}

bool CcSyncClient::removeAccount(const CcString& sUsername)
{
  return m_oConfig.removeAccount(sUsername);
}

CcString CcSyncClient::getAccountInfo()
{
  CcString sRet;
  if (m_pAccount != nullptr)
  {
    sRet << "Name:     " << m_pAccount->getName() << CcGlobalStrings::EolOs;
    sRet << "Database: " << m_pAccount->getDatabaseFilePath() << CcGlobalStrings::EolOs;
    for (const CcSyncDirectoryConfig& oDirConfig : m_pAccount->getDirectoryList())
    {
      sRet << "--------------------" << oDirConfig.getName() << CcGlobalStrings::EolOs;
      sRet << "Directory: " << oDirConfig.getName() << CcGlobalStrings::EolOs;
      sRet << "  Path:    " << oDirConfig.getLocation() << CcGlobalStrings::EolOs;
      sRet << "  Options: " << oDirConfig.getLocation() << CcGlobalStrings::EolOs;
      if(oDirConfig.getBackupCommand().length() > 0 )
      {
        sRet << "    BCmd:    " << oDirConfig.getBackupCommand() << CcGlobalStrings::EolOs;
      }
      if(oDirConfig.getRestoreCommand().length() > 0 )
      {
        sRet << "    RCmd:    " << oDirConfig.getRestoreCommand() << CcGlobalStrings::EolOs;
      }
      if(oDirConfig.getGroupId() != UINT32_MAX )
      {
        sRet << "    GroupId: " << CcString::fromNumber(oDirConfig.getGroupId()) << CcGlobalStrings::EolOs;
      }
      if(oDirConfig.getUserId() != UINT32_MAX )
      {
        sRet << "    UserId:  " << CcString::fromNumber(oDirConfig.getUserId()) << CcGlobalStrings::EolOs;
      }
    }
    if (m_pAccount->isValid())
    {
      sRet << "Valid:     " << CcGlobalStrings::True << CcGlobalStrings::EolOs;
    }
    else
    {
      sRet << "Valid:     " << CcGlobalStrings::False << CcGlobalStrings::EolOs;
    }
  }
  else
  {
    sRet = "No Account selected";
  }
  return sRet;
}

CcString CcSyncClient::getDirectoryInfo(const CcString& sDirectoryName)
{
  CcString sRet;
  if (m_pAccount != nullptr)
  {
    CcSyncDirectoryConfig* pDirConfig = m_pAccount->directoryList().getDirectoryByName(sDirectoryName);
    if (pDirConfig != nullptr)
    {
      sRet << "Directory: " << pDirConfig->getName() << CcGlobalStrings::EolOs;
      sRet << "  Path:    " << pDirConfig->getLocation() << CcGlobalStrings::EolOs;
      sRet << "  BCmd:    " << pDirConfig->getBackupCommand() << CcGlobalStrings::EolOs;
      sRet << "  RCmd:    " << pDirConfig->getRestoreCommand() << CcGlobalStrings::EolOs;
    }
  }
  else
  {
    sRet = "No Account selected";
  }
  return sRet;
}

CcStringList CcSyncClient::getAccountList()
{
  CcStringList oAccountList;
  for (CcSyncAccountConfig& rAccount : m_oConfig.getAccountList())
  {
    oAccountList.append(rAccount.getAccountDirName());
  }
  return oAccountList;
}

CcStringList CcSyncClient::getDirectoryList()
{
  CcStringList oAccountList;
  if (m_pAccount != nullptr)
  {
    for (CcSyncDirectoryConfig& rDirectory : m_pAccount->getDirectoryList())
    {
      oAccountList.append(rDirectory.getName());
    }
  }
  return oAccountList;
}

bool CcSyncClient::changeHostname(const CcString& sHostName)
{
  bool bSuccess = false;
  if (m_pAccount != nullptr)
  {
    CcString sOldDir = m_pAccount->getAccountDirName();
    if (m_pAccount->changeHostname(sHostName))
    {
      CcString sNewDir = m_pAccount->getAccountDirName();
      CcString sFullOldDir = CcKernel::getUserDataDir();
      CcString sFullNewDir = CcKernel::getUserDataDir();
      sFullOldDir.appendPath(CcSyncGlobals::ConfigDirName);
      sFullNewDir.appendPath(CcSyncGlobals::ConfigDirName);
      sFullOldDir.appendPath(sOldDir);
      sFullNewDir.appendPath(sNewDir);
      deinit();
      bSuccess = CcDirectory::move(sFullOldDir, sFullNewDir);
      if (bSuccess)
      {
        if (selectAccount(sNewDir))
        {
          if (setupDatabase())
          {
            bSuccess = true;
          }
        }
      }
      if(!bSuccess)
        CcSyncLog::writeError("Failed to move AccountDir from " + sFullOldDir + " to " + sFullNewDir, ESyncLogTarget::Client);
    }
  }
  return bSuccess;
}

CcSyncClient* CcSyncClient::create(const CcString& sConfigFilePath)
{
  CcSyncClient* pNew = new CcSyncClient(sConfigFilePath);
  CCMONITORNEW(pNew);
  return pNew;
}

void CcSyncClient::remove(CcSyncClient* pToRemove)
{
  if (pToRemove != nullptr)
  {
    CCDELETE(pToRemove);
  }
};

void CcSyncClient::init(const CcString& sConfigFile)
{
  m_bConfigAvailable = true;
  if (!m_oConfig.readConfig(sConfigFile))
  {
    CcSyncLog::writeDebug("No configuration file in config-directory", ESyncLogTarget::Client);
  }
}

void CcSyncClient::deinit()
{
  close();
  m_pAccount = nullptr;
  m_pDatabase = nullptr;
  m_oBackupDirectories.clear();
  m_oRequest.data().clear();
  m_oResponse.data().clear();
}

bool CcSyncClient::setupDatabase()
{
  bool bRet = false;
  m_sDatabaseFile = CcKernel::getUserDataDir();
  m_sDatabaseFile.appendPath(CcSyncGlobals::ConfigDirName);
  m_sDatabaseFile.appendPath(m_pAccount->getAccountDirName());
  m_sDatabaseFile.appendPath(m_pAccount->getDatabaseFilePath());
  m_pDatabase = new CcSyncDbClient();
  m_pDatabase->diableDisable();
  CCMONITORNEW(m_pDatabase.getPtr());
  if (m_pDatabase->openDatabase(m_sDatabaseFile))
  {
    if (!checkSqlTables())
    {
      setupSqlTables();
    }
    CcSyncLog::writeDebug("User Database connected: " + m_pAccount->getDatabaseFilePath(), ESyncLogTarget::Client);
    bRet = true;
  }
  else
    CcSyncLog::writeError("Unable to create Database for " + m_pAccount->getDatabaseFilePath(), ESyncLogTarget::Client);
  return bRet;
}

bool CcSyncClient::checkSqlTables()
{
  bool bRet = true;
  CcSqlResult oResult;
  for (CcSyncDirectoryConfig& oConfigDirectory : m_pAccount->directoryList())
  {
    // Check if Directory already initialized
    bool bDirectoryExists = false;
    for (CcSyncDirectory& oDirectory : m_oBackupDirectories)
    {
      if (oDirectory.getName() == oConfigDirectory.getName())
      {
        bDirectoryExists = true;
        break;
      }
    }
    if (bDirectoryExists == false)
    {
      CcSyncDirectory oDirectory;
      oDirectory.init(m_pDatabase, &oConfigDirectory);
      bRet = oDirectory.validate();
      if (bRet)
      {
        m_oBackupDirectories.append(oDirectory);
      }
    }
  }
  return bRet;
}

bool CcSyncClient::setupSqlTables()
{
  bool bRet = true;
  return bRet;
}

bool CcSyncClient::connect()
{
  bool bRet = false;
  if (m_oSocket.isValid() && m_uiReconnections < CcSyncGlobals::MaxReconnections)
  {
    bRet = true;
  }
  else
  {
    m_oSocket = new CcSslSocket();
    CCMONITORNEW(m_oSocket.getPtr());
    if (static_cast<CcSslSocket*>(m_oSocket.getRawSocket())->initClient())
    {
      if (m_oSocket.connect(m_pAccount->getServer().getHostname(), m_pAccount->getServer().getPortString()))
      {
        // reset counter;
        m_uiReconnections = 0;
        m_oSocket.setTimeout(CcDateTimeFromSeconds(30));
        bRet = true;
      }
      else
      {
        m_uiReconnections++;
        m_oSocket.close();
      }
    }
    else
    {
      m_uiReconnections++;
      m_oSocket.close();
    }
  }
  return bRet;
}

bool CcSyncClient::sendRequestGetResponse()
{
  bool bRet = false;
  CcJsonDocument oJsonDoc(m_oRequest.getData());
  if (connect())
  {
    m_oResponse.clear();
    if (m_oSocket.writeArray(oJsonDoc.getDocument()))
    {
      if (m_oRequest.getCommandType() != ESyncCommandType::Close)
      {
        CcString sRead;
        size_t uiReadSize = 0;
        CcByteArray oLastRead(static_cast<size_t>(CcSyncGlobals::MaxResponseSize));
        do
        {
          uiReadSize = m_oSocket.readArray(oLastRead, false);
          if ( uiReadSize > 0 && uiReadSize <= CcSyncGlobals::MaxResponseSize)
          {
            sRead.append(oLastRead, 0, uiReadSize);
          }
        } while ( uiReadSize <= CcSyncGlobals::MaxResponseSize &&
                  sRead.length() <= CcSyncGlobals::MaxResponseSize  &&
                  CcJsonDocument::isValidData(sRead) == false);

        if (uiReadSize > CcSyncGlobals::MaxResponseSize)
        {
          CcSyncLog::writeError("Read from socket failed, try reconnection", ESyncLogTarget::Client);
          CcSyncLog::writeError("Request:", ESyncLogTarget::Client);
          CcSyncLog::writeError(oJsonDoc.getDocument(), ESyncLogTarget::Client);
          CcSyncLog::writeError("Response 24 signs:", ESyncLogTarget::Client);
          CcSyncLog::writeError(sRead.substr(0,24), ESyncLogTarget::Client);
        }
        else if (sRead.length() > CcSyncGlobals::MaxResponseSize)
        {
          CcSyncLog::writeError("Incoming data exceed maximum", ESyncLogTarget::Client);
          CcSyncLog::writeError("Request:", ESyncLogTarget::Client);
          CcSyncLog::writeError(oJsonDoc.getDocument(), ESyncLogTarget::Client);
          CcSyncLog::writeError("Response 24 signs:", ESyncLogTarget::Client);
          CcSyncLog::writeError(sRead.substr(0, 24), ESyncLogTarget::Client);
          reconnect();
        }
        else
        {
          m_oResponse.parseData(sRead);
          if (m_oResponse.getCommandType() == m_oRequest.getCommandType())
          {
            if (m_oResponse.hasError() == false)
            {
              bRet = true;
            }
          }
          else
          {
            CcSyncLog::writeError("Wrong server response, try reconnect.", ESyncLogTarget::Client);
            CcJsonDocument oDoc(m_oRequest.getData());
            CCDEBUG(oDoc.getDocument());
            CCDEBUG(CcString(sRead));
            reconnect();
          }
        }
      }
    }
    else
    {
      CcSyncLog::writeError("Get response failed, try reconnect.", ESyncLogTarget::Client);
      reconnect();
    }
  }
  return bRet;
}

void CcSyncClient::recursiveRemoveDirectory(CcSyncDirectory& oDirectory, CcSyncFileInfo& oFileInfo)
{
  CcSyncFileInfoList oClientDirectories = oDirectory.getDirectoryInfoListById(oFileInfo.getId());
  CcSyncFileInfoList oClientFiles = oDirectory.getFileInfoListById(oFileInfo.getId());

  for (CcSyncFileInfo& oClientFileInfo : oClientFiles)
  {
    oDirectory.fileListRemove(oClientFileInfo, false, false);
  }
  for (CcSyncFileInfo& oClientDirInfo : oClientDirectories)
  {
    recursiveRemoveDirectory(oDirectory, oClientDirInfo);
  }

  oDirectory.getFullDirPathById(oFileInfo);
  if (CcDirectory::exists(oFileInfo.getSystemFullPath()))
  {
    CcDirectory::remove(oFileInfo.getSystemFullPath(), true);
  }
}

bool CcSyncClient::sendFile(CcSyncFileInfo& oFileInfo)
{
  bool bRet = false;
  CcFile oFile(oFileInfo.getSystemFullPath());
  CcCrc32 oCrc;
  if(oFile.open(EOpenFlags::Read | EOpenFlags::ShareRead))
  {
    bool bTransfer = true;
    CcByteArray oBuffer(static_cast<size_t>(CcSyncGlobals::TransferSize));
    size_t uiLastTransferSize;
    while (bTransfer)
    {
      uiLastTransferSize = oFile.readArray(oBuffer, false);
      if (uiLastTransferSize != 0 && uiLastTransferSize <= oBuffer.size())
      {
        oCrc.append(oBuffer.getArray(), uiLastTransferSize);
        if (m_oSocket.write(oBuffer.getArray(), uiLastTransferSize) == false)
        {
          bTransfer = false;
          CcSyncLog::writeError("Write failed during File transfer, reconnect", ESyncLogTarget::Client);
          reconnect();
        }
      }
      else
      {
        bRet = true;
        bTransfer = false;
      }
    }
    oFile.close();
  }
  if (bRet == true)
  {
    oFileInfo.crc() = oCrc.getValueUint32();
    m_oRequest.setCrc(oCrc);
    if (sendRequestGetResponse())
    {
      if (m_oResponse.hasError() == false)
        bRet = true;
      else
        bRet = false;
    }
    else
    {
      bRet = false;
    }
  }
  return bRet;
}

bool CcSyncClient::receiveFile(CcFile* pFile, CcSyncFileInfo& oFileInfo)
{
  bool bRet = false;
  bool bTransfer = true;
  CcCrc32 oCrc;
  uint64 uiReceived = 0;
  size_t uiBufferSize = static_cast<size_t>(CcSyncGlobals::TransferSize);
  if (oFileInfo.getFileSize() < CcSyncGlobals::TransferSize)
  {
    uiBufferSize = static_cast<size_t>(oFileInfo.getFileSize());
  }
  CcByteArray oByteArray(uiBufferSize);
  while (bTransfer)
  {
    if (uiReceived < oFileInfo.getFileSize())
    {
      size_t uiReadSize = m_oSocket.readArray(oByteArray, false);
      if (uiReadSize <= uiBufferSize)
      {
        oCrc.append(oByteArray.getArray(), uiReadSize);
        uiReceived += uiReadSize;
        if (!pFile->write(oByteArray.getArray(), uiReadSize))
        {
          bRet = false;
          bTransfer = false;
        }
      }
      else
      {
        bTransfer = false;
        CcSyncLog::writeError("Error during socket read, reconnect", ESyncLogTarget::Client);
        reconnect();
      }
    }
    else
    {
      bTransfer = false;
      oFileInfo.crc() = oCrc.getValueUint32();
      m_oRequest.setCrc(oCrc);
      if (sendRequestGetResponse())
      {
        if (m_oResponse.hasError() == false)
          bRet = true;
        else
          bRet = false;
      }
      else
      {
        bRet = false;
      }
    }
  }
  return bRet;
}

bool CcSyncClient::doRemoteSyncDir(CcSyncDirectory& oDirectory, uint64 uiDirId)
{
  bool bRet = false;
  m_oRequest.setDirectoryGetFileList(oDirectory.getName(), uiDirId);
  if (sendRequestGetResponse() &&
      m_oResponse.hasError() == false)
  {
    bRet = true;
    CcSyncFileInfoList oServerDirectories;
    CcSyncFileInfoList oServerFiles;
    m_oResponse.getDirectoryDirectoryInfoList(oServerDirectories, oServerFiles);
    CcSyncFileInfoList oClientDirectories = oDirectory.getDirectoryInfoListById(uiDirId);
    CcSyncFileInfoList oClientFiles = oDirectory.getFileInfoListById(uiDirId);
    for (CcSyncDirInfo& oServerDirInfo : oServerDirectories)
    {
      if (oClientDirectories.containsDirectory(oServerDirInfo.getId()))
      {
        CcSyncDirInfo& oClientDirInfo = oClientDirectories.getFile(oServerDirInfo.getId());
        // Compare Server Directory with Client Directory
        if (oClientDirInfo != oServerDirInfo)
        {
          // Update directory info in database
          // @todo change attributes if required
          oDirectory.directoryListUpdate(oServerDirInfo);
        }
        doRemoteSyncDir(oDirectory, oServerDirInfo.getId());
        // Remove Directory from current list
        oClientDirectories.removeFile(oServerDirInfo.getId());
      }
      else
      {
        if (oClientDirectories.containsDirectory(oServerDirInfo.getName()))
        {
          CcSyncDirInfo oDirInfo = oClientDirectories.getDirectory(oServerDirInfo.getName());
          oDirectory.directoryListUpdateId(oDirInfo.getId(), oServerDirInfo);
          doRemoteSyncDir(oDirectory, oServerDirInfo.getId());
          oClientDirectories.removeFile(oServerDirInfo.getName());
        }
        else
        {
          oDirectory.getFullDirPathById(oServerDirInfo);
          if (CcDirectory::exists(oServerDirInfo.getSystemFullPath()))
          {
            if (oDirectory.directoryListInsert(oServerDirInfo))
            {
              doRemoteSyncDir(oDirectory, oServerDirInfo.getId());
            }
            else
            {
              oDirectory.queueDownloadDirectory(oServerDirInfo);
            }
          }
          else
          {
            oDirectory.queueDownloadDirectory(oServerDirInfo);
          }
        }
      }
    }

    // Search Filelist
    for (CcSyncFileInfo& oServerFileInfo : oServerFiles)
    {
      if (oClientFiles.containsFile(oServerFileInfo.getId()))
      {
        CcSyncFileInfo& oFileInfo = oClientFiles.getFile(oServerFileInfo.getId());
        if (oFileInfo != oServerFileInfo)
        {
          // @todo always downloading works, okay!
          oDirectory.fileListUpdate(oServerFileInfo, false);
        }
        oClientFiles.removeFile(oServerFileInfo.getId());
      }
      else
      {
        if (oClientFiles.containsFile(oServerFileInfo.getName()))
        {
          CcSyncFileInfo& oClientFileInfo = oClientFiles.getFile(oServerFileInfo.getName());
          oDirectory.getFullDirPathById(oClientFileInfo);
          if (CcFile::exists(oServerFileInfo.getSystemFullPath()))
          {
            CcFileInfo oLocalFileInfo = CcFile::getInfo(oServerFileInfo.getSystemFullPath());
            if (oLocalFileInfo.getModified().getTimestampS() <= oClientFileInfo.getModified())
            {
              // Remove from database and disk
              oDirectory.fileListRemove(oClientFileInfo, false, false);
            }
            else
            {
              // Remove from database
              oDirectory.fileListRemove(oClientFileInfo, false, true);
            }
          }
          else
          {
            // Remove from database
            oDirectory.fileListRemove(oClientFileInfo, false, true);
          }
          oClientFiles.removeFile(oServerFileInfo.getName());
        }
        oDirectory.getFullDirPathById(oServerFileInfo);
        if (CcFile::exists(oServerFileInfo.getSystemFullPath()))
        {
          // File still existing, insert fileinfo and wait for local sync
          oDirectory.fileListInsert(oServerFileInfo);
        }
        else
        {
          oDirectory.queueDownloadFile(oServerFileInfo);
        }
      }
    }
    // remove all not listed files on local directory
    for (CcSyncFileInfo& oClientFileInfo : oClientFiles)
    {
      oDirectory.fileListRemove(oClientFileInfo, false, false);
    }
    // remove all not listed directories on local directory
    for (CcSyncFileInfo& oClientDirInfo : oClientDirectories)
    {
      recursiveRemoveDirectory(oDirectory, oClientDirInfo);
    }
    oDirectory.directoryListUpdateChanged(uiDirId);
  }
  return bRet;
}

bool CcSyncClient::serverDirectoryEqual(CcSyncDirectory& oDirectory, uint64 uiDirId)
{
  bool bRet = false;
  m_oRequest.setDirectoryGetDirectoryInfo(oDirectory.getName(), uiDirId);
  if (sendRequestGetResponse() &&
      m_oResponse.hasError() == false)
  {
    CcSyncDirInfo oDirInfoServer = m_oResponse.getFileInfo();
    CcSyncDirInfo oDirInfoClient = oDirectory.getDirectoryInfoById(uiDirId);
    if (oDirInfoServer == oDirInfoClient)
    {
      bRet = true;
    }
  }
  return bRet;
}

bool CcSyncClient::doAddDir(CcSyncDirectory& oDirectory, CcSyncFileInfo& oDirInfo, uint64 uiQueueIndex)
{
  bool bRet = false;
  oDirectory.getFullDirPathById(oDirInfo);
  if (oDirInfo.fromSystemDirectory())
  {
    m_oRequest.setDirectoryCreateDirectory(oDirectory.getName(), oDirInfo);
    if (sendRequestGetResponse() &&
      m_oResponse.hasError() == false)
    {
      bRet = true;
      CcSyncFileInfo oResponseFileInfo = m_oResponse.getFileInfo();
      oDirectory.directoryListInsert(oResponseFileInfo);
      oDirectory.queueFinalizeDirectory(oResponseFileInfo, uiQueueIndex);
      CcSyncLog::writeDebug("Directory successfully added: " + oDirInfo.getName());
    }
    else
    {
      oDirectory.queueIncrementItem(uiQueueIndex);
      CcSyncLog::writeError("Directory failed to add:  " + oDirInfo.getName(), ESyncLogTarget::Client);
      CcSyncLog::writeError("    ErrorMsg: " + m_oResponse.getErrorMsg(), ESyncLogTarget::Client);
    }
  }
  else
  {
    oDirectory.queueIncrementItem(uiQueueIndex);
    CcSyncLog::writeError("Queued Directory not found:  " + oDirInfo.getDirPath());
    CcSyncLog::writeError("    ErrorMsg: " + m_oResponse.getErrorMsg(), ESyncLogTarget::Client);
  }
  return bRet;
}

bool CcSyncClient::doRemoveDir(CcSyncDirectory& oDirectory, CcSyncFileInfo& oDirInfo, uint64 uiQueueIndex)
{
  bool bRet = false;
  m_oRequest.setDirectoryRemoveDirectory(oDirectory.getName(), oDirInfo);
  if (sendRequestGetResponse() ||
      m_oResponse.hasError())
  {
    if (m_oResponse.hasError() == false)
    {
      oDirectory.directoryListRemove(oDirInfo);
      // do not updated dependencies, use finalize file instead
      oDirectory.queueFinalizeFile(uiQueueIndex);
      bRet = true;
    }
    else
    {
      switch (m_oResponse.getError().getError())
      {
        case EStatus::FSDirNotEmpty:
          // If directory is not empty, but our list is empty, we should remove from database,
          // because we will get the new files from server on next sync
          // fall through
        case EStatus::FSDirNotFound:
          oDirectory.directoryListRemove(oDirInfo);
          // do not updated dependencies, use finalize file instead
          oDirectory.queueFinalizeFile(uiQueueIndex);
          bRet = true;
          break;
        default:
          CcSyncLog::writeError("Error on deleting Directory: " + oDirInfo.getName(), ESyncLogTarget::Client);
          CcSyncLog::writeError("    ErrorMsg: " + m_oResponse.getErrorMsg(), ESyncLogTarget::Client);
          oDirectory.queueIncrementItem(uiQueueIndex);
      }
    }
  }
  else
  {
    CcSyncLog::writeError("RemoveDirectory request failed: " + oDirInfo.getName(), ESyncLogTarget::Client);
    CcSyncLog::writeError("    ErrorMsg: " + m_oResponse.getErrorMsg(), ESyncLogTarget::Client);
    oDirectory.queueIncrementItem(uiQueueIndex);
  }
  return bRet;
}

bool CcSyncClient::updateDirectoryBackupCommand(const CcString& sDirname, const CcString& sBackupCommand)
{
  bool bSuccess = false;
  if (m_pAccount != nullptr)
  {
    for (CcSyncDirectoryConfig& oDirConfig : m_pAccount->directoryList())
    {
      if (oDirConfig.getName() == sDirname)
      {
        oDirConfig.setBackupCommand(sBackupCommand);
        bSuccess = true;
      }
    }
    if (bSuccess)
    {
      bSuccess = updateToRemoteAccount();
    }
  }
  return false;
}

bool CcSyncClient::updateDirectoryRestoreCommand(const CcString& sDirname, const CcString& sRestoreCommand)
{
  bool bSuccess = false;
  if (m_pAccount != nullptr)
  {
    for (CcSyncDirectoryConfig& oDirConfig : m_pAccount->directoryList())
    {
      if (oDirConfig.getName() == sDirname)
      {
        oDirConfig.setRestoreCommand(sRestoreCommand);
        bSuccess = true;
      }
    }
    if (bSuccess)
    {
      bSuccess = updateToRemoteAccount();
    }
  }
  return false;
}

bool CcSyncClient::verify(const CcString& sDirname)
{
  if (m_pDatabase != nullptr)
  {
    m_pDatabase->beginTransaction();
    m_pDatabase->directoryListSearchDouble(sDirname);
    m_pDatabase->directoryListUpdateChangedAll(sDirname);
    m_pDatabase->directoryListSearchTemporary(sDirname);
    m_pDatabase->fileListSearchTemporary(sDirname);
    m_pDatabase->endTransaction();
    return true;
  }
  else
    return false;
}

bool CcSyncClient::updateDirectorySetUser(const CcString& sDirname, const CcString& sUser)
{
  bool bSuccess = false;
  if (m_pAccount != nullptr)
  {
    for (CcSyncDirectoryConfig& oDirConfig : m_pAccount->directoryList())
    {
      if (oDirConfig.getName() == sDirname)
      {
        oDirConfig.setUser(sUser);
        bSuccess = true;
      }
    }
    if (bSuccess)
    {
      bSuccess = updateToRemoteAccount();
    }
  }
  return false;
}

bool CcSyncClient::updateDirectorySetGroup(const CcString& sDirname, const CcString& sGroup)
{
  bool bSuccess = false;
  if (m_pAccount != nullptr)
  {
    for (CcSyncDirectoryConfig& oDirConfig : m_pAccount->directoryList())
    {
      if (oDirConfig.getName() == sDirname)
      {
        oDirConfig.setGroup(sGroup);
        bSuccess = true;
      }
    }
    if (bSuccess)
    {
      bSuccess = updateToRemoteAccount();
    }
  }
  return false;
}

bool CcSyncClient::doUpdateDir(CcSyncDirectory& oDirectory, CcSyncFileInfo& oFileInfo, uint64 uiQueueIndex)
{
  bool bRet = false;
  CCUNUSED(oDirectory);
  CCUNUSED(oFileInfo);
  CCUNUSED(uiQueueIndex);
  CcSyncLog::writeDebug("@TODO Implementation", ESyncLogTarget::Client);
  oDirectory.queueIncrementItem(uiQueueIndex);
  return bRet;
}

bool CcSyncClient::doDownloadDir(CcSyncDirectory& oDirectory, CcSyncFileInfo& oFileInfo, uint64 uiQueueIndex)
{
  bool bRet = false;
  m_oRequest.setDirectoryGetDirectoryInfo(oDirectory.getName(), oFileInfo.getId());
  if (sendRequestGetResponse())
  {
    CcSyncDirInfo oDirInfo = m_oResponse.getFileInfo();
    oDirectory.getFullDirPathById(oDirInfo);
    CcDirectory oDirectoryHandl(oDirInfo.getSystemFullPath());
    if (oDirectoryHandl.exists() ||
        oDirectoryHandl.create(true))
    {
#ifndef WIN32
      if (oDirectory.getGroupId() != UINT32_MAX)
      {
        CcFile::setGroupId(oDirInfo.getSystemFullPath(), oDirectory.getGroupId());
      }
      if (oDirectory.getUserId() != UINT32_MAX)
      {
        CcFile::setUserId(oDirInfo.getSystemFullPath(), oDirectory.getUserId());
      }
#endif
      if (oDirectory.directoryListInsert(oDirInfo))
      {
        CcSyncLog::writeDebug("Directory Added: " + oDirInfo.getName(), ESyncLogTarget::Client);
        bRet = true;
        oDirectory.queueFinalizeDirectory(oDirInfo, uiQueueIndex);
        doRemoteSyncDir(oDirectory, oDirInfo.id());
      }
      else
      {
        CcSyncLog::writeError("Directory not inserted in list found", ESyncLogTarget::Client);
        CcSyncLog::writeError("    ErrorMsg: " + m_oResponse.getErrorMsg(), ESyncLogTarget::Client);
        oDirectory.queueIncrementItem(uiQueueIndex);
      }
    }
    else
    {
      CcSyncLog::writeError("Unable to create directory", ESyncLogTarget::Client);
      oDirectory.queueIncrementItem(uiQueueIndex);
    }
  }
  else
  {
    CcSyncLog::writeError("Directory not found", ESyncLogTarget::Client);
    CcSyncLog::writeError("    ErrorMsg: " + m_oResponse.getErrorMsg(), ESyncLogTarget::Client);
    oDirectory.queueIncrementItem(uiQueueIndex);
  }
  return bRet;
}

bool CcSyncClient::doUploadFile(CcSyncDirectory& oDirectory, CcSyncFileInfo& oFileInfo, uint64 uiQueueIndex)
{
  bool bRet = false;
  oDirectory.getFullDirPathById(oFileInfo);
  if (oFileInfo.fromSystemFile(false))
  {
    m_oRequest.setDirectoryUploadFile(oDirectory.getName(), oFileInfo);
    if (sendRequestGetResponse())
    {
      if (m_oResponse.hasError() == false)
      {
        if (sendFile(oFileInfo))
        {
          CcSyncFileInfo oResponseFileInfo = m_oResponse.getFileInfo();
          if (oDirectory.fileListInsert(oResponseFileInfo))
          {
            oDirectory.queueFinalizeFile(uiQueueIndex);
            CcSyncLog::writeDebug("File Successfully uploaded: " + oFileInfo.getName(), ESyncLogTarget::Client);
          }
          else
          {
            oDirectory.queueIncrementItem(uiQueueIndex);
            CcSyncLog::writeError("Inserting to filelist failed: " + oFileInfo.getSystemFullPath(), ESyncLogTarget::Client);
          }
          bRet = true;
        }
        else
        {
          oDirectory.queueIncrementItem(uiQueueIndex);
          CcSyncLog::writeError("Sending file failed: " + oFileInfo.getSystemFullPath(), ESyncLogTarget::Client);
          CcSyncLog::writeError("    ErrorMsg: " + m_oResponse.getErrorMsg(), ESyncLogTarget::Client);
        }
      }
      else
      {
        switch (m_oResponse.getError().getError())
        {
          case EStatus::FSFileAlreadyExisting:
            if (m_oResponse.data().contains(CcSyncGlobals::FileInfo::Id))
            {
              CcSyncFileInfo oResponseFileInfo = m_oResponse.getFileInfo();
              oDirectory.fileListInsert(oResponseFileInfo);
              oDirectory.queueFinalizeFile(uiQueueIndex);
              CcSyncLog::writeDebug("File Successfully uploaded: " + oFileInfo.getName(), ESyncLogTarget::Client);
              bRet = true;
            }
            // fall through
          default:
            oDirectory.queueIncrementItem(uiQueueIndex);
            CcSyncLog::writeError("Sending file failed: " + oFileInfo.getSystemFullPath(), ESyncLogTarget::Client);
            CcSyncLog::writeError("    ErrorMsg: " + m_oResponse.getErrorMsg(), ESyncLogTarget::Client);
        }
      }
    }
    else
    {
      CcSyncLog::writeError("Sending file failed: " + oFileInfo.getSystemFullPath(), ESyncLogTarget::Client);
      CcSyncLog::writeError("    ErrorMsg: " + m_oResponse.getErrorMsg(), ESyncLogTarget::Client);
      oDirectory.queueIncrementItem(uiQueueIndex);
    }
  }
  else
  {
    oDirectory.queueIncrementItem(uiQueueIndex);
    CcSyncLog::writeError("Queued Directory not found: " + oFileInfo.getDirPath(), ESyncLogTarget::Client);
    CcSyncLog::writeError("    ErrorMsg: " + m_oResponse.getErrorMsg(), ESyncLogTarget::Client);
  }
  return bRet;
}

bool CcSyncClient::doRemoveFile(CcSyncDirectory& oDirectory, CcSyncFileInfo& oFileInfo, uint64 uiQueueIndex)
{
  bool bRet = false;
  oDirectory.getFullDirPathById(oFileInfo);
  m_oRequest.setDirectoryRemoveFile(oDirectory.getName(), oFileInfo);
  if (sendRequestGetResponse() ||
      m_oResponse.hasError())
  {
    if (m_oResponse.hasError() == false)
    {
      if(oDirectory.fileListRemove(oFileInfo, true, true))
      {
        oDirectory.queueFinalizeFile(uiQueueIndex);
        CcSyncLog::writeDebug("File successfully removed: " + oFileInfo.getName(), ESyncLogTarget::Client);
        bRet = true;
      }
      else
      {
        CcSyncLog::writeError("Error on deleting File in Directory: " + oFileInfo.getName(), ESyncLogTarget::Client);
        CcSyncLog::writeError("    ErrorMsg: " + m_oResponse.getErrorMsg(), ESyncLogTarget::Client);
        oDirectory.queueIncrementItem(uiQueueIndex);
      }
    }
    else
    {
      switch (m_oResponse.getError().getError())
      {
        case EStatus::FSDirNotFound:
          // fall through
        case EStatus::FSFileNotFound:
          oDirectory.fileListRemove(oFileInfo, true, false);
          oDirectory.queueFinalizeFile(uiQueueIndex);
          CcSyncLog::writeDebug("File successfully removed: " + oFileInfo.getName(), ESyncLogTarget::Client);
          bRet = true;
          break;
        default:
          CcSyncLog::writeError("Error on deleting File: " + oFileInfo.getName(), ESyncLogTarget::Client);
          CcSyncLog::writeError("    ErrorMsg: " + m_oResponse.getErrorMsg(), ESyncLogTarget::Client);
          oDirectory.queueIncrementItem(uiQueueIndex);
      }
    }
  }
  else
  {
    CcSyncLog::writeError("RemoveFile request failed: " + oFileInfo.getName(), ESyncLogTarget::Client);
    CcSyncLog::writeError("    ErrorMsg: " + m_oResponse.getErrorMsg(), ESyncLogTarget::Client);
    oDirectory.queueIncrementItem(uiQueueIndex);
  }
  return bRet;
}

bool CcSyncClient::doUpdateFile(CcSyncDirectory& oDirectory, CcSyncFileInfo& oFileInfo, uint64 uiQueueIndex)
{
  bool bRet = false;
  bool bDoRemoveUpload = false;
  m_oRequest.setDirectoryGetFileInfo(oDirectory.getName(), oFileInfo.getId());
  if (sendRequestGetResponse() &&
      m_oResponse.hasError() == false)
  {
    CcSyncFileInfo oServerFileInfo = m_oResponse.getFileInfo();
    oDirectory.getFullDirPathById(oServerFileInfo);
    // check if id exists in database
    CcSyncFileInfo oClientFileInfo = oDirectory.getFileInfoById(oServerFileInfo.getId());
    oDirectory.getFullDirPathById(oClientFileInfo);
    if (oClientFileInfo.getId() == 0)
    {
      oClientFileInfo.fromSystemFile(false);
      if (oClientFileInfo.getFileSize() == oServerFileInfo.getFileSize() &&
          oClientFileInfo.fromSystemFile(true) &&
          oClientFileInfo.getCrc() == oServerFileInfo.getCrc())
      {
        setFileInfo(oFileInfo.getSystemFullPath(), oDirectory.getUserId(), oDirectory.getGroupId(), oServerFileInfo.getModified());
        if (oDirectory.fileListUpdate(oClientFileInfo, false))
        {
          bRet = true;
        }
        else
        {
          CcSyncLog::writeDebug("Update file, update in database failed: " + oServerFileInfo.getName(), ESyncLogTarget::Client);
          oDirectory.queueIncrementItem(uiQueueIndex);
        }
      }
      else
      {
        m_oRequest.init(ESyncCommandType::DirectoryDownloadFile);
        bRet = doDownloadFile(oDirectory, oFileInfo, uiQueueIndex);
      }
      if (bRet == true)
      {
        oDirectory.queueFinalizeFile(uiQueueIndex);
      }
    }
    else
    {
      oClientFileInfo.fromSystemFile(false);
      if (oClientFileInfo.modified() > oServerFileInfo.modified())
      {
        if (oClientFileInfo.getFileSize() == oServerFileInfo.getFileSize() &&
            oClientFileInfo.fromSystemFile(true) &&
            oClientFileInfo.getCrc() == oServerFileInfo.getCrc())
        {
          if (oServerFileInfo.modified() < 0)
          {
            bDoRemoveUpload = true;
          }
          else if (oDirectory.fileListUpdate(oServerFileInfo, false))
          {
            setFileInfo(oFileInfo.getSystemFullPath(), oDirectory.getUserId(), oDirectory.getGroupId(), oServerFileInfo.getModified());
            oDirectory.queueFinalizeFile(uiQueueIndex);
          }
          else
          {
            CcSyncLog::writeDebug("Update file, update in database failed: " + oServerFileInfo.getName(), ESyncLogTarget::Client);
            oDirectory.queueIncrementItem(uiQueueIndex);
          }
        }
        else
        {
          bDoRemoveUpload = true;
        }
      }
      else if (oClientFileInfo.modified() < oServerFileInfo.modified())
      {
        if (oClientFileInfo.getFileSize() == oServerFileInfo.getFileSize() &&
            oClientFileInfo.getCrc() == oServerFileInfo.getCrc())
        {
          if (oDirectory.fileListUpdate(oServerFileInfo, false))
          {
            setFileInfo(oFileInfo.getSystemFullPath(), oDirectory.getUserId(), oDirectory.getGroupId(), oServerFileInfo.getModified());
            oDirectory.queueFinalizeFile(uiQueueIndex);
          }
          else
          {
            CcSyncLog::writeDebug("Update file, update in database failed: " + oServerFileInfo.getName(), ESyncLogTarget::Client);
            oDirectory.queueIncrementItem(uiQueueIndex);
          }
        }
        else
        {
          oDirectory.fileListRemove(oClientFileInfo, false, false);
          m_oRequest.init(ESyncCommandType::DirectoryDownloadFile);
          bRet = doDownloadFile(oDirectory, oFileInfo, uiQueueIndex);
        }
      }
      else
      {
        if (oClientFileInfo.getFileSize() == oServerFileInfo.getFileSize() &&
            oClientFileInfo.getCrc() == oServerFileInfo.getCrc())
        {
          if (oDirectory.fileListUpdate(oServerFileInfo, false))
          {
            setFileInfo(oFileInfo.getSystemFullPath(), oDirectory.getUserId(), oDirectory.getGroupId(), oServerFileInfo.getModified());
            oDirectory.queueFinalizeFile(uiQueueIndex);
          }
          else
          {
            CcSyncLog::writeDebug("3 Update file, update in database failed: " + oServerFileInfo.getName(), ESyncLogTarget::Client);
            oDirectory.queueIncrementItem(uiQueueIndex);
          }
        }
        else
        {
          oDirectory.fileListRemove(oClientFileInfo, false, false);
          m_oRequest.init(ESyncCommandType::DirectoryDownloadFile);
          bRet = doDownloadFile(oDirectory, oFileInfo, uiQueueIndex);
        }
      }
    }
  }
  else
  {
    bDoRemoveUpload = true;
  }
  if(bDoRemoveUpload)
  {
    oDirectory.getFullDirPathById(oFileInfo);
    if (oFileInfo.fromSystemFile(false))
    {
      m_oRequest.setDirectoryUpdateFile(oDirectory.getName(), oFileInfo);
      if (sendRequestGetResponse())
      {
        if (m_oResponse.hasError() == false)
        {
          if (sendFile(oFileInfo))
          {
            CcSyncFileInfo oResponseFileInfo = m_oResponse.getFileInfo();
            if (oDirectory.fileListUpdate(oResponseFileInfo, false))
            {
              oDirectory.queueFinalizeFile(uiQueueIndex);
              CcSyncLog::writeDebug("File Successfully updated: " + oFileInfo.getName(), ESyncLogTarget::Client);
            }
            else
            {
              oDirectory.queueIncrementItem(uiQueueIndex);
              CcSyncLog::writeError("Updating in filelist failed: " + oFileInfo.getSystemFullPath(), ESyncLogTarget::Client);
            }
            bRet = true;
          }
          else
          {
            oDirectory.queueIncrementItem(uiQueueIndex);
            CcSyncLog::writeError("Sending file for update failed: " + oFileInfo.getSystemFullPath(), ESyncLogTarget::Client);
            CcSyncLog::writeError("    ErrorMsg: " + m_oResponse.getErrorMsg(), ESyncLogTarget::Client);
          }
        }
        else
        {
          switch (m_oResponse.getError().getError())
          {
            case EStatus::FSFileAlreadyExisting:
              if (m_oResponse.data().contains(CcSyncGlobals::FileInfo::Id))
              {
                CcSyncFileInfo oResponseFileInfo = m_oResponse.getFileInfo();
                oDirectory.fileListInsert(oResponseFileInfo);
                oDirectory.queueFinalizeFile(uiQueueIndex);
                CcSyncLog::writeDebug("File Successfully updated: " + oFileInfo.getName(), ESyncLogTarget::Client);
                bRet = true;
                break;
              }
              // fall through
            default:
              oDirectory.queueIncrementItem(uiQueueIndex);
              CcSyncLog::writeError("Sending file failed: " + oFileInfo.getSystemFullPath(), ESyncLogTarget::Client);
              CcSyncLog::writeError("    ErrorMsg: " + m_oResponse.getErrorMsg(), ESyncLogTarget::Client);
          }
        }
      }
      else
      {
        CcSyncLog::writeError("Sending file failed: " + oFileInfo.getSystemFullPath(), ESyncLogTarget::Client);
        CcSyncLog::writeError("    ErrorMsg: " + m_oResponse.getErrorMsg(), ESyncLogTarget::Client);
        oDirectory.queueIncrementItem(uiQueueIndex);
      }
    }
    else
    {
      oDirectory.queueIncrementItem(uiQueueIndex);
      CcSyncLog::writeError("Queued Directory not found: " + oFileInfo.getDirPath(), ESyncLogTarget::Client);
      CcSyncLog::writeError("    ErrorMsg: " + m_oResponse.getErrorMsg(), ESyncLogTarget::Client);
    }
  }
  return bRet;
}

bool CcSyncClient::doDownloadFile(CcSyncDirectory& oDirectory, CcSyncFileInfo& oFileInfo, uint64 uiQueueIndex)
{
  bool bRet = false;
  m_oRequest.setDirectoryDownloadFile(oDirectory.getName(), oFileInfo.getId());
  if (sendRequestGetResponse())
  {
    oFileInfo = m_oResponse.getFileInfo();
    oDirectory.getFullDirPathById(oFileInfo);
    if (CcDirectory::exists(oFileInfo.getSystemDirPath()) ||
        CcDirectory::create(oFileInfo.getSystemDirPath(), true) )
    {
      CcString sTempFilePath = oFileInfo.getSystemFullPath();
      sTempFilePath.append(CcSyncGlobals::TemporaryExtension);
      CcFile oFile(sTempFilePath);
      if (oFile.open(EOpenFlags::Overwrite))
      {
        if (receiveFile(&oFile, oFileInfo))
        {
          oFile.close();
          bool bSuccess = true;
          if (CcFile::exists(oFileInfo.getSystemFullPath()))
          {
            if (!CcFile::remove(oFileInfo.getSystemFullPath()))
            {
              bSuccess = false;
              CcFile::remove(sTempFilePath);
              CcSyncLog::writeDebug("Failed to remove original File: " + oFileInfo.getSystemFullPath(), ESyncLogTarget::Client);
            }
          }
          if (bSuccess)
          {
            bSuccess = CcFile::move(sTempFilePath, oFileInfo.getSystemFullPath());
            if (bSuccess == false)
            {
              CcFile::remove(sTempFilePath);
              CcSyncLog::writeDebug("Failed to move temporary File: ", ESyncLogTarget::Client);
              CcSyncLog::writeDebug("  " + sTempFilePath + " -> " + oFileInfo.getSystemFullPath(), ESyncLogTarget::Client);
            }
          }
          if (bSuccess)
          {
            setFileInfo(oFileInfo.getSystemFullPath(), oDirectory.getUserId(), oDirectory.getGroupId(), oFileInfo.getModified());
            if (oDirectory.fileListInsert(oFileInfo))
            {
              CcSyncLog::writeDebug("File downloaded: " + oFileInfo.getName(), ESyncLogTarget::Client);
              bRet = true;
              oDirectory.queueFinalizeFile(uiQueueIndex);
            }
            else
            {
              CcSyncLog::writeError("Insert to Filelist failed", ESyncLogTarget::Client);
              CcSyncLog::writeError("    ErrorMsg: " + m_oResponse.getErrorMsg(), ESyncLogTarget::Client);
              oDirectory.queueIncrementItem(uiQueueIndex);
            }
          }
        }
        else
        {
          oFile.close();
          CcFile::remove(sTempFilePath);
          CcSyncLog::writeError("File download failed", ESyncLogTarget::Client);
          CcSyncLog::writeError("    ErrorMsg: " + m_oResponse.getErrorMsg(), ESyncLogTarget::Client);
          oDirectory.queueIncrementItem(uiQueueIndex);
        }
      }
      else
      {
        CcSyncLog::writeError("Unable to create file", ESyncLogTarget::Client);
        CcSyncLog::writeError("    ErrorMsg: " + m_oResponse.getErrorMsg(), ESyncLogTarget::Client);
        oDirectory.queueIncrementItem(uiQueueIndex);
      }
    }
    else
    {
      CcSyncLog::writeError("Directory for download not found: " + oFileInfo.getSystemFullPath(), ESyncLogTarget::Client);
      oDirectory.queueIncrementItem(uiQueueIndex);
    }
  }
  else
  {
    CcSyncLog::writeError("DownloadFile request failed", ESyncLogTarget::Client);
    CcSyncLog::writeError("    ErrorMsg: " + m_oResponse.getErrorMsg(), ESyncLogTarget::Client);
    oDirectory.queueIncrementItem(uiQueueIndex);
  }
  return bRet;
}

void CcSyncClient::setFileInfo(const CcString& sPathToFile, uint32 uiUserId, uint32 uiGroupId, int64 iModified)
{
#ifndef WIN32
  if (oDirectory.getGroupId() != UINT32_MAX)
  {
    CcFile::setGroupId(sPathToFile, uiGroupId);
  }
  if (oDirectory.getUserId() != UINT32_MAX)
  {
    CcFile::setUserId(sPathToFile, uiUserId);
  }
#endif
  CcFile::setModified(sPathToFile, CcDateTimeFromSeconds(iModified));
}