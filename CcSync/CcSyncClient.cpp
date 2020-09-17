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
#include "CcConsole.h"

#include "private/CcSyncWorkerClientDownload.h"

CcSyncClient::CcSyncClient(const CcString& sConfigFilePath, bool bCreate)
{
  if (sConfigFilePath.length() == 0)
  {
    CcString sConfigFile = CcKernel::getUserDataDir();
    sConfigFile.appendPath(CcSyncGlobals::ConfigDirName);
    CcString sDir = sConfigFile;
    sConfigFile.appendPath(CcSyncGlobals::Client::ConfigFileName);
    if (CcFile::exists(sConfigFile))
    {
      init(sConfigFile);
    }
    else if (bCreate && (CcDirectory::exists(sDir) || CcDirectory::create(sDir, true)))
    {
      m_oConfig.create(sConfigFile);
    }
    else
    {
      CcSyncLog::writeDebug("No configuration file in config-directory", ESyncLogTarget::Client);
    }
  }
  else
  {
    m_sConfigPath = sConfigFilePath;
    CcFile oFileObject(sConfigFilePath);
    if (oFileObject.isDir())
    {
      CcString sPath = sConfigFilePath;
      CcString sDir = sConfigFilePath;
      sPath.appendPath(CcSyncGlobals::Client::ConfigFileName);
      if (CcFile::exists(sPath))
      {
        init(sPath);
      }
      else if (bCreate && (CcDirectory::exists(sDir) || CcDirectory::create(sDir, true)))
      {
        m_oConfig.create(sPath);
      }
      else
      {
        CcString sOldPath = sConfigFilePath;
        sOldPath.appendPath(CcSyncGlobals::ConfigDirName);
        sOldPath.appendPath(CcSyncGlobals::Client::ConfigFileName);
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

CcSyncClient::~CcSyncClient(void)
{
  deinit();
}

bool CcSyncClient::login()
{
  m_bLogin = false;
  if (m_pAccount != nullptr)
  {
    m_oCom.getRequest().setAccountLogin(m_pAccount->getName(), m_pAccount->getName(), m_pAccount->getPassword().getString());
    if (m_oCom.sendRequestGetResponse())
    {
      if (!m_oCom.getResponse().hasError())
      {
        m_oCom.getSession() = m_oCom.getResponse().getSession();
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
    m_oCom.getRequest().init(ESyncCommandType::Close);
    m_oCom.sendRequestGetResponse();
    m_oBackupDirectories.clear();
    m_pDatabase.deleteCurrent();
    m_pAccount = nullptr;
  }
  m_oCom.close();
}

bool CcSyncClient::isAdmin()
{
  if (m_pAccount != nullptr)
  {
    m_oCom.getRequest().init(ESyncCommandType::AccountRights);
    if (m_oCom.sendRequestGetResponse())
    {
      ESyncRights eRights = m_oCom.getResponse().getAccountRight();
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
    m_oCom.getRequest().setServerRescan(bDeep);
    if (m_oCom.sendRequestGetResponse())
    {
      return true;
    }
  }
  return false;
}

bool CcSyncClient::serverStop()
{
  if (m_pAccount != nullptr)
  {
    m_oCom.getRequest().setServerStop();
    if (m_oCom.sendRequestGetResponse())
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
        if (m_oCom.connect(m_pAccount->getServer()) == false)
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
          CcSync::ISyncWorkerBase* pWorker = nullptr;
          CcSyncFileInfo oFileInfo;
          uint64 uiQueueIndex = 0;
          m_pDatabase->beginTransaction();
          EBackupQueueType eQueueType = oDirectory.queueGetNext(oFileInfo, uiQueueIndex);
          switch (eQueueType)
          {
            case EBackupQueueType::CreateDir:
              doCreateDir(oDirectory, oFileInfo, uiQueueIndex);
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
            case EBackupQueueType::DownloadFile:
            {
              CCNEW(pWorker, CcSync::CcSyncWorkerClientDownload, oDirectory, oFileInfo, uiQueueIndex, m_oCom);
              break;
            }
            default:
              oDirectory.queueIncrementItem(uiQueueIndex);
          }
          if(pWorker)
          {
            pWorker->start();
            uint16 uiCounter = 0;
            while(pWorker->isInProgress())
            {
              if(uiCounter >= 10)
              {
                uiCounter = 0;
                CcConsole::writeSameLine(pWorker->getProgressMessage());
              }
              else
              {
                CcKernel::sleep(20);
                uiCounter++;
              }
            }
            CcConsole::writeSameLine(CcGlobalStrings::Empty);
            CcConsole::writeLine(pWorker->getProgressMessage());
            CCDELETE(pWorker);
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
  m_oCom.getRequest().init(ESyncCommandType::AccountDatabaseUpdateChanged);
  if (!m_oCom.sendRequestGetResponse())
  {
    CCDEBUG("Update changed in database request failed");
  }
}

bool CcSyncClient::doAccountCreateDirectory(const CcString sDirectoryName, const CcString& sDirectoryPath)
{
  bool bSuccess = false;
  m_oCom.getRequest().setAccountCreateDirectory(sDirectoryName);
  if (m_oCom.sendRequestGetResponse() &&
      m_oCom.getResponse().hasError() == false)
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
    CcSyncLog::writeError(m_oCom.getResponse().getErrorMsg(), ESyncLogTarget::Client);
  }
  return bSuccess;
}

bool CcSyncClient::doAccountRemoveDirectory(const CcString sDirectoryName)
{
  bool bSuccess = false;
  m_oCom.getRequest().setAccountRemoveDirectory(sDirectoryName);
  if (m_oCom.sendRequestGetResponse())
  {
    m_pDatabase->removeDirectory(sDirectoryName);
    bSuccess = m_pAccount->removeAccountDirectory(sDirectoryName);
  }
  else
  {
    CcSyncLog::writeError(m_oCom.getResponse().getErrorMsg(), ESyncLogTarget::Client);
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
  bool bRet = true;
  CcString sRet;
  if (m_pAccount != nullptr)
  {
    m_oCom.getRequest().init(ESyncCommandType::AccountGetData);
    m_oCom.getRequest().setSession(m_oCom.getSession());
    if (m_oCom.sendRequestGetResponse())
    {
      CcSyncAccountConfig oAccountConfig = m_oCom.getResponse().getAccountConfig();
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
      else
      {
        bRet = false;
      }
    }
    else
    {
      bRet = false;
    }
  }
  else
  {
    bRet = false;
  }
  return bRet;
}

bool CcSyncClient::updateToRemoteAccount()
{
  bool bRet = false;
  CcString sRet;
  if (m_pAccount != nullptr)
  {
    m_oCom.getRequest().init(ESyncCommandType::AccountSetData);
    m_oCom.getRequest().addAccountInfo(*m_pAccount.ptr());
    if (m_oCom.sendRequestGetResponse())
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
    if (m_sConfigPath.length() > 0)
      sClientConfigDir = m_sConfigPath;
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
  if (bRet)
  {
    m_oCom.setUrl(m_pAccount->getServer());
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
  else if (m_sConfigPath.length() > 0)
  {
    sConfigFile = m_sConfigPath;
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
  m_oCom.getRequest().setServerCreateAccount(sUsername, sPassword);
  if(m_oCom.sendRequestGetResponse())
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
      if (m_sConfigPath.length() > 0)
      {
        sFullNewDir = m_sConfigPath;
        sFullOldDir = m_sConfigPath;
      }
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

CcSyncClient* CcSyncClient::create(const CcString& sConfigFilePath, bool bCreate)
{
  CcSyncClient* pNew = new CcSyncClient(sConfigFilePath, bCreate);
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
  m_oCom.close();
  m_pAccount = nullptr;
  m_pDatabase = nullptr;
  m_oBackupDirectories.clear();
  m_oCom.getRequest().data().clear();
  m_oCom.getResponse().data().clear();
}

bool CcSyncClient::setupDatabase()
{
  bool bRet = false;

  m_sDatabaseFile = CcKernel::getUserDataDir();
  if (m_sConfigPath.length() > 0)
    m_sDatabaseFile = m_sConfigPath;
  m_sDatabaseFile.appendPath(CcSyncGlobals::ConfigDirName);
  m_sDatabaseFile.appendPath(m_pAccount->getAccountDirName());
  m_sDatabaseFile.appendPath(m_pAccount->getDatabaseFilePath());
  m_pDatabase = new CcSyncDbClient();
  m_pDatabase->historyDisable();
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

void CcSyncClient::recursiveRemoveDirectory(CcSyncDirectory& oDirectory, CcSyncFileInfo& oFileInfo)
{
  CcSyncFileInfoList oClientDirectories = oDirectory.getDirectoryInfoListById(oFileInfo.getId());
  CcSyncFileInfoList oClientFiles = oDirectory.getFileInfoListById(oFileInfo.getId());

  for (CcSyncFileInfo& oClientFileInfo : oClientFiles)
  {
    oDirectory.getFullDirPathById(oClientFileInfo);
    CcFileInfo oSystemFileInfo = CcFile::getInfo(oClientFileInfo.getSystemFullPath());
    // Just delete if file is same as known
    if (oClientFileInfo == oSystemFileInfo)
    {
      oDirectory.fileListRemove(oClientFileInfo, false, false);
    }
  }
  for (CcSyncFileInfo& oClientDirInfo : oClientDirectories)
  {
    recursiveRemoveDirectory(oDirectory, oClientDirInfo);
  }

  oDirectory.getFullDirPathById(oFileInfo);
  if (CcDirectory::exists(oFileInfo.getSystemFullPath()))
  {
    //! let it fail if unregistered files are available
    CcDirectory::remove(oFileInfo.getSystemFullPath(), false);
  }
  oDirectory.directoryListRemove(oFileInfo, false);
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
        size_t uiTransfered = m_oCom.getSocket().write(oBuffer.getArray(), uiLastTransferSize);
        if (uiTransfered != uiLastTransferSize)
        {
          bTransfer = false;
          CcSyncLog::writeError("Write failed during File transfer, reconnect", ESyncLogTarget::Client);
          m_oCom.reconnect();
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
    m_oCom.getRequest().setCrc(oCrc);
    if (m_oCom.sendRequestGetResponse())
    {
      bRet = m_oCom.getResponse().hasError() == false;
    }
    else
    {
      bRet = false;
    }
  }
  return bRet;
}

bool CcSyncClient::doRemoteSyncDir(CcSyncDirectory& oDirectory, uint64 uiDirId)
{
  bool bRet = false;
  m_oCom.getRequest().setDirectoryGetFileList(oDirectory.getName(), uiDirId);
  if (m_oCom.sendRequestGetResponse() &&
      m_oCom.getResponse().hasError() == false)
  {
    bRet = true;
    CcSyncFileInfoList oServerDirectories;
    CcSyncFileInfoList oServerFiles;
    m_oCom.getResponse().getDirectoryDirectoryInfoList(oServerDirectories, oServerFiles);
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
          bool bDoUpdate=  false;
          if(oClientDirInfo.getName() != oServerDirInfo.getName())
          {
            oDirectory.getFullDirPathById(oClientDirInfo);
            oDirectory.getFullDirPathById(oServerDirInfo);
            if(CcDirectory::exists(oClientDirInfo.getSystemFullPath()))
            {
              CcDirectory::move(oClientDirInfo.getSystemFullPath(), oServerDirInfo.getSystemFullPath());
            }
            bDoUpdate = true;
          }
          if(oClientDirInfo.getMd5() != oServerDirInfo.getMd5())
          {
            doRemoteSyncDir(oDirectory, oServerDirInfo.getId());
          }
          if(bDoUpdate)
          {
            // Update directory info in database
            oDirectory.directoryListUpdate(oServerDirInfo);
          }
        }
        // Remove Directory from current list
        oClientDirectories.removeFile(oServerDirInfo.getId());
      }
      else
      {
        if (oClientDirectories.containsDirectory(oServerDirInfo.getName()))
        {
          CcSyncDirInfo oDirInfo = oClientDirectories.getDirectory(oServerDirInfo.getName());
          if(oDirectory.directoryListExists(oServerDirInfo.getId()))
          {
            oDirectory.directoryListRemove(oDirInfo, false);
            oDirectory.directoryListUpdateId(oServerDirInfo.getId(), oServerDirInfo);
          }
          else
          {
            oDirectory.directoryListUpdateId(oDirInfo.getId(), oServerDirInfo);
          }
          doRemoteSyncDir(oDirectory, oServerDirInfo.getId());
          oClientDirectories.removeFile(oServerDirInfo.getName());
        }
        else
        {
          oDirectory.getFullDirPathById(oServerDirInfo);
          if (CcDirectory::exists(oServerDirInfo.getSystemFullPath()))
          {
            if (oDirectory.directoryListInsert(oServerDirInfo, false))
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
          oDirectory.fileListRemove(oServerFileInfo, false, true);
          oDirectory.fileListInsert(oServerFileInfo, false);
        }
        oClientFiles.removeFile(oServerFileInfo.getId());
      }
      else
      {
        if (oClientFiles.containsFile(oServerFileInfo.getName()))
        {
          CcSyncFileInfo& oClientFileInfo = oClientFiles.getFile(oServerFileInfo.getName());
          oDirectory.getFullDirPathById(oClientFileInfo);
          if (CcFile::exists(oClientFileInfo.getSystemFullPath()))
          {
            CcFileInfo oLocalFileInfo = CcFile::getInfo(oClientFileInfo.getSystemFullPath());
            // Remove from database if local timestamp is older or if known file was removed
            if (oLocalFileInfo.getModified().getTimestampS() <= oServerFileInfo.getModified() ||
                oLocalFileInfo.getModified().getTimestampS() == oClientFileInfo.getModified())
            {
              // Remove from database and disk
              oDirectory.fileListRemove(oClientFileInfo, false, false);
            }
            else
            {
              // Remove from only from database to add by local compare
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
          oDirectory.fileListInsert(oServerFileInfo, false);
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
  m_oCom.getRequest().setDirectoryGetDirectoryInfo(oDirectory.getName(), uiDirId);
  if (m_oCom.sendRequestGetResponse() &&
      m_oCom.getResponse().hasError() == false)
  {
    CcSyncDirInfo oDirInfoServer = m_oCom.getResponse().getFileInfo();
    CcSyncDirInfo oDirInfoClient = oDirectory.getDirectoryInfoById(uiDirId);
    if (oDirInfoServer == oDirInfoClient)
    {
      bRet = true;
    }
  }
  return bRet;
}

bool CcSyncClient::doCreateDir(CcSyncDirectory& oDirectory, CcSyncFileInfo& oDirInfo, uint64 uiQueueIndex)
{
  bool bRet = false;
  oDirectory.getFullDirPathById(oDirInfo);
  if (oDirInfo.fromSystemDirectory())
  {
    m_oCom.getRequest().setDirectoryCreateDirectory(oDirectory.getName(), oDirInfo);
    if (m_oCom.sendRequestGetResponse() &&
      m_oCom.getResponse().hasError() == false)
    {
      bRet = true;
      CcSyncFileInfo oResponseFileInfo = m_oCom.getResponse().getFileInfo();
      oDirectory.directoryListInsert(oResponseFileInfo, true);
      oDirectory.queueFinalizeDirectory(oResponseFileInfo, uiQueueIndex);
      CcSyncLog::writeDebug("Directory successfully added: " + oDirInfo.getName());
    }
    else
    {
      switch(m_oCom.getResponse().getError().getError())
      {
        case EStatus::FSDirNotFound:
          // Parent direcotry not existing
          CcSyncLog::writeError("Parent directory not existing:  " + oDirInfo.dirPath(), ESyncLogTarget::Client);
    CCFALLTHROUGH;
        default:
          oDirectory.queueIncrementItem(uiQueueIndex);
          CcSyncLog::writeError("Directory failed to add:  " + oDirInfo.getName(), ESyncLogTarget::Client);
          CcSyncLog::writeError("    ErrorMsg: " + m_oCom.getResponse().getErrorMsg(), ESyncLogTarget::Client);
      }
    }
  }
  else
  {
    oDirectory.queueIncrementItem(uiQueueIndex);
    CcSyncLog::writeError("Queued Directory not found:  " + oDirInfo.getDirPath());
    CcSyncLog::writeError("    ErrorMsg: " + m_oCom.getResponse().getErrorMsg(), ESyncLogTarget::Client);
  }
  return bRet;
}

bool CcSyncClient::doRemoveDir(CcSyncDirectory& oDirectory, CcSyncFileInfo& oDirInfo, uint64 uiQueueIndex)
{
  bool bRet = false;
  m_oCom.getRequest().setDirectoryRemoveDirectory(oDirectory.getName(), oDirInfo);
  if (m_oCom.sendRequestGetResponse() ||
      m_oCom.getResponse().hasError())
  {
    if (m_oCom.getResponse().hasError() == false)
    {
      oDirectory.directoryListRemove(oDirInfo, true);
      // do not updated dependencies, use finalize file instead
      oDirectory.queueFinalizeFile(uiQueueIndex);
      bRet = true;
    }
    else
    {
      switch (m_oCom.getResponse().getError().getError())
      {
        case EStatus::FSDirNotEmpty:
          // If directory is not empty, but our list is empty, we should remove from database,
          // because we will get the new files from server on next sync
          // fall through
        case EStatus::FSDirNotFound:
          oDirectory.directoryListRemove(oDirInfo, true);
          // do not updated dependencies, use finalize file instead
          oDirectory.queueFinalizeFile(uiQueueIndex);
          bRet = true;
          break;
        default:
          CcSyncLog::writeError("Error on deleting Directory: " + oDirInfo.getRelativePath(), ESyncLogTarget::Client);
          CcSyncLog::writeError("    ErrorMsg: " + m_oCom.getResponse().getErrorMsg(), ESyncLogTarget::Client);
          oDirectory.queueIncrementItem(uiQueueIndex);
      }
    }
  }
  else
  {
    CcSyncLog::writeError("RemoveDirectory request failed: " + oDirInfo.getName(), ESyncLogTarget::Client);
    CcSyncLog::writeError("    ErrorMsg: " + m_oCom.getResponse().getErrorMsg(), ESyncLogTarget::Client);
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

bool CcSyncClient::refresh(const CcString& sDirname)
{
  bool bSuccess = false;
  for (CcSyncDirectory& oDirectory : m_oBackupDirectories)
  {
    if(oDirectory.getName() == sDirname)
    {
      oDirectory.directoryListUpdateChanged(1);
      bSuccess = true;
      break;
    }
  }
  return bSuccess;
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
  m_oCom.getRequest().setDirectoryGetDirectoryInfo(oDirectory.getName(), oFileInfo.getId());
  if (m_oCom.sendRequestGetResponse())
  {
    CcSyncDirInfo oDirInfo = m_oCom.getResponse().getFileInfo();
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
      if (oDirectory.directoryListInsert(oDirInfo, true))
      {
        CcSyncLog::writeDebug("Directory Added: " + oDirInfo.getName(), ESyncLogTarget::Client);
        bRet = true;
        oDirectory.queueFinalizeDirectory(oDirInfo, uiQueueIndex);
        doRemoteSyncDir(oDirectory, oDirInfo.id());
      }
      else
      {
        CcSyncLog::writeError("Directory not inserted in list found", ESyncLogTarget::Client);
        CcSyncLog::writeError("    ErrorMsg: " + m_oCom.getResponse().getErrorMsg(), ESyncLogTarget::Client);
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
    CcSyncLog::writeError("    ErrorMsg: " + m_oCom.getResponse().getErrorMsg(), ESyncLogTarget::Client);
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
    m_oCom.getRequest().setDirectoryUploadFile(oDirectory.getName(), oFileInfo);
    if (m_oCom.sendRequestGetResponse())
    {
      if (m_oCom.getResponse().hasError() == false)
      {
        if (sendFile(oFileInfo))
        {
          CcSyncFileInfo oResponseFileInfo = m_oCom.getResponse().getFileInfo();
          if(oDirectory.fileNameInDirExists(oFileInfo.getDirId(), oFileInfo))
          {
            CcSyncFileInfo oFileToDelete = oDirectory.getFileInfoByFilename(oFileInfo.getDirId(), oFileInfo.getName());
            oDirectory.fileListRemove(oFileToDelete, false, true);
          }
          if (oDirectory.fileListInsert(oResponseFileInfo, true))
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
          CcSyncLog::writeError("    ErrorMsg: " + m_oCom.getResponse().getErrorMsg(), ESyncLogTarget::Client);
        }
      }
      else
      {
        switch (m_oCom.getResponse().getError().getError())
        {
          case EStatus::FSFileAlreadyExisting:
            if (m_oCom.getResponse().data().contains(CcSyncGlobals::FileInfo::Id, EJsonDataType::Value))
            {
              CcSyncFileInfo oResponseFileInfo = m_oCom.getResponse().getFileInfo();
              oDirectory.fileListInsert(oResponseFileInfo, true);
              oDirectory.queueFinalizeFile(uiQueueIndex);
              CcSyncLog::writeDebug("File Successfully uploaded: " + oFileInfo.getName(), ESyncLogTarget::Client);
              bRet = true;
            }
            // fall through
          default:
            oDirectory.queueIncrementItem(uiQueueIndex);
            CcSyncLog::writeError("Sending file failed: " + oFileInfo.getSystemFullPath(), ESyncLogTarget::Client);
            CcSyncLog::writeError("    ErrorMsg: " + m_oCom.getResponse().getErrorMsg(), ESyncLogTarget::Client);
        }
      }
    }
    else
    {
      CcSyncLog::writeError("Sending file failed: " + oFileInfo.getSystemFullPath(), ESyncLogTarget::Client);
      CcSyncLog::writeError("    ErrorMsg: " + m_oCom.getResponse().getErrorMsg(), ESyncLogTarget::Client);
      oDirectory.queueIncrementItem(uiQueueIndex);
    }
  }
  else
  {
    oDirectory.queueIncrementItem(uiQueueIndex);
    CcSyncLog::writeError("Queued Directory not found: " + oFileInfo.getDirPath(), ESyncLogTarget::Client);
    CcSyncLog::writeError("    ErrorMsg: " + m_oCom.getResponse().getErrorMsg(), ESyncLogTarget::Client);
  }
  return bRet;
}

bool CcSyncClient::doRemoveFile(CcSyncDirectory& oDirectory, CcSyncFileInfo& oFileInfo, uint64 uiQueueIndex)
{
  bool bRet = false;
  oDirectory.getFullDirPathById(oFileInfo);
  m_oCom.getRequest().setDirectoryRemoveFile(oDirectory.getName(), oFileInfo);
  if (m_oCom.sendRequestGetResponse() ||
      m_oCom.getResponse().hasError())
  {
    if (m_oCom.getResponse().hasError() == false)
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
        CcSyncLog::writeError("    ErrorMsg: " + m_oCom.getResponse().getErrorMsg(), ESyncLogTarget::Client);
        oDirectory.queueIncrementItem(uiQueueIndex);
      }
    }
    else
    {
      switch (m_oCom.getResponse().getError().getError())
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
          CcSyncLog::writeError("    ErrorMsg: " + m_oCom.getResponse().getErrorMsg(), ESyncLogTarget::Client);
          oDirectory.queueIncrementItem(uiQueueIndex);
      }
    }
  }
  else
  {
    CcSyncLog::writeError("RemoveFile request failed: " + oFileInfo.getName(), ESyncLogTarget::Client);
    CcSyncLog::writeError("    ErrorMsg: " + m_oCom.getResponse().getErrorMsg(), ESyncLogTarget::Client);
    oDirectory.queueIncrementItem(uiQueueIndex);
  }
  return bRet;
}

void CcSyncClient::setFileInfo(const CcString& sPathToFile, uint32 uiUserId, uint32 uiGroupId, int64 iModified)
{
#ifndef WIN32
  if (uiGroupId != UINT32_MAX)
  {
    CcFile::setGroupId(sPathToFile, uiGroupId);
  }
  if (uiUserId != UINT32_MAX)
  {
    CcFile::setUserId(sPathToFile, uiUserId);
  }
#else
  CCUNUSED(uiUserId);
  CCUNUSED(uiGroupId);
#endif
  CcFile::setModified(sPathToFile, CcDateTimeFromSeconds(iModified));
}
