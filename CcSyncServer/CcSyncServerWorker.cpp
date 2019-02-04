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
 * @brief     Implemtation of class CcSyncServerWorker
 */
#include "CcSyncDbClient.h"
#include "CcSyncServerWorker.h"
#include "CcSyncGlobals.h"
#include "CcKernel.h"
#include "CcDirectory.h"
#include "CcSslSocket.h"
#include "CcSyncServer.h"
#include "CcSyncClientConfig.h"
#include "CcSyncDirectory.h"
#include "CcSyncServerDirectory.h"
#include "CcSqlite.h"
#include "Hash/CcCrc32.h"
#include "CcSyncServerRescanWorker.h"

class CcSyncServerWorkerPrivate
{
public:
  CcSyncServerWorkerPrivate(CcSyncServer* oServer) :
    oRescanWorker(oServer)
  { }
  CcSyncServerRescanWorker oRescanWorker;
  bool  m_bStopCalled = false;
};

CcSyncServerWorker::CcSyncServerWorker(CcSyncServer* oServer, CcSocket oSocket) :
  m_oServer(oServer),
  m_oSocket(oSocket)
{
  m_pPrivate = new CcSyncServerWorkerPrivate(oServer);
  CCMONITORNEW(m_pPrivate);
}

CcSyncServerWorker::~CcSyncServerWorker(void)
{
  CCDELETE(m_pPrivate);
}

void CcSyncServerWorker::run()
{
  while (getThreadState() == EThreadState::Running &&
         m_pPrivate->m_bStopCalled == false)
  {
    if (getRequest())
    {
      if (m_oUser.isValid())
        m_oUser.getDatabase()->beginTransaction();
      ESyncCommandType eCommandType = m_oRequest.getCommandType();
      switch (eCommandType)
      {
        case ESyncCommandType::Close:
          m_oSocket.close();
          enterState(EThreadState::Stopping);
          break;
        case ESyncCommandType::ServerGetInfo:
          m_oResponse.init(eCommandType);
          doServerGetInfo();
          break;
        case ESyncCommandType::ServerAccountCreate:
          m_oResponse.init(eCommandType);
          doServerAccountCreate();
          break;
        case ESyncCommandType::ServerAccountRescan:
          m_oResponse.init(eCommandType);
          doServerRescan();
          break;
        case ESyncCommandType::ServerAccountRemove:
          m_oResponse.init(eCommandType);
          doServerAccountRemove();
          break;
        case ESyncCommandType::ServerStop:
          m_oResponse.init(eCommandType);
          doServerStop();
          enterState(EThreadState::Stopping);
          break;
        case ESyncCommandType::AccountCreate:
          m_oResponse.init(eCommandType);
          doAccountCreate();
          break;
        case ESyncCommandType::AccountLogin:
          m_oResponse.init(eCommandType);
          doAccountLogin();
          break;
        case ESyncCommandType::AccountGetData:
          m_oResponse.init(eCommandType);
          doAccountGetData();
          break;
        case ESyncCommandType::AccountSetData:
          m_oResponse.init(eCommandType);
          doAccountSetData();
          break;
        case ESyncCommandType::AccountGetDirectoryList:
          m_oResponse.init(eCommandType);
          doAccountGetDirectoryList();
          break;
        case ESyncCommandType::AccountGetCommandList:
          m_oResponse.init(eCommandType);
          doUserGetCommandList();
          break;
        case ESyncCommandType::AccountCreateDirectory:
          m_oResponse.init(eCommandType);
          doAccountCreateDirectory();
          break;
        case ESyncCommandType::AccountRemoveDirectory:
          m_oResponse.init(eCommandType);
          doAccountRemoveDirectory();
          break;
        case ESyncCommandType::AccountRights:
          m_oResponse.init(eCommandType);
          doAccountRights();
          break;
        case ESyncCommandType::AccountDatabaseUpdateChanged:
          m_oResponse.init(eCommandType);
          doAccountDatabaseUpdateChanged();
          break;
        case ESyncCommandType::DirectoryGetFileList:
          m_oResponse.init(eCommandType);
          doDirectoryGetFileList();
          break;
        case ESyncCommandType::DirectoryGetFileInfo:
          m_oResponse.init(eCommandType);
          doDirectoryGetFileInfo();
          break;
        case ESyncCommandType::DirectoryGetDirectoryInfo:
          m_oResponse.init(eCommandType);
          doDirectoryGetDirectoryInfo();
          break;
        case ESyncCommandType::DirectoryCreateDirectory:
          m_oResponse.init(eCommandType);
          doDirectoryCreateDirectory();
          break;
        case ESyncCommandType::DirectoryRemoveDirectory:
          m_oResponse.init(eCommandType);
          doDirectoryRemoveDirectory();
          break;
        case ESyncCommandType::DirectoryUploadFile:
          m_oResponse.init(eCommandType);
          doDirectoryUploadFile();
          break;
        case ESyncCommandType::DirectoryRemoveFile:
          m_oResponse.init(eCommandType);
          doDirectoryRemoveFile();
          break;
        case ESyncCommandType::DirectoryDownloadFile:
          m_oResponse.init(eCommandType);
          doDirectoryDownloadFile();
          break;
        default:
          m_oResponse.init(ESyncCommandType::Unknown);
          m_oResponse.setResult(false);
          m_oResponse.setError(EStatus::CommandUnknown, "Unknown Command");
          sendResponse();
      }
      if (m_oUser.isValid())
        m_oUser.getDatabase()->endTransaction();
    }
    else
    {
      m_oResponse.setError(EStatus::CommandError, "Request malformed. Connection will get closed.");
      sendResponse();
      m_oSocket.close();
      enterState(EThreadState::Stopping);
    }
  }

  if(m_pPrivate->m_bStopCalled == false)
    m_oServer->workerDone(this);
}

bool CcSyncServerWorker::getRequest()
{
  bool bRet = false;
  CcByteArray oData(static_cast<size_t>(CcSyncGlobals::MaxRequestSize));
  m_oSocket.readArray(oData);
  if (m_oRequest.parseData(oData))
  {
    bRet = true;
  }
  else
  {
    m_oResponse.setError(EStatus::CommandError, "Message malformed");
  }
  return bRet;
}

bool CcSyncServerWorker::sendResponse()
{
  return m_oSocket.writeArray(m_oResponse.getBinary());
}

bool CcSyncServerWorker::loadConfigsBySessionRequest()
{
  bool bRet = false;
  // Check all required data
  if (m_oUser.isValid() )
  { 
    bRet = true;
  }
  else if (m_oRequest.data().contains(CcSyncGlobals::Commands::Session))
  {
    if (loadConfigsBySession(m_oRequest.data()[CcSyncGlobals::Commands::Session].getValue().getString()))
    {
      bRet = true;
    }
    else
    {
      m_oResponse.setError(EStatus::LoginFailed, "Error: Session not valid");
      bRet = false;
    }
  }
  else
  {
    m_oResponse.setError(EStatus::LoginFailed, "Error: No login data not available");
  }
  return bRet;
}


bool CcSyncServerWorker::loadConfigsBySession(const CcString& sSession)
{
  bool bRet = false;
  m_oUser = m_oServer->getUserByToken(sSession);
  if (m_oUser.isValid())
  {
    bRet = true;
  }
  else
  {
    m_oResponse.setError(EStatus::LoginFailed, "Error: Login data not valid");
  }
  return bRet;
}

bool CcSyncServerWorker::receiveFile(CcFile* pFile, CcSyncFileInfo& oFileInfo)
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
  size_t uiLastReceived;
  while (bTransfer)
  {
    if (uiReceived < oFileInfo.getFileSize())
    {
      uiLastReceived = m_oSocket.readArray(oByteArray, false);
      if(oByteArray.size() < uiLastReceived)
      {
        bRet = false;
        bTransfer = false;
      }
      else if (uiLastReceived > 0)
      {
        oCrc.append(oByteArray.getArray(), uiLastReceived);
        uiReceived += uiLastReceived;
        if (!pFile->write(oByteArray.getArray(), uiLastReceived))
        {
          bRet = false;
          bTransfer = false;
        }
      }
      else
      {
        bTransfer = false;
      }
    }
    else
    {
      oFileInfo.crc() = oCrc.getValueUint32();
      bTransfer = false;
      CcByteArray oResponseByteArray(static_cast<size_t>(CcSyncGlobals::TransferSize));
      m_oSocket.readArray(oResponseByteArray);
      m_oRequest.parseData(oResponseByteArray);
      if (m_oRequest.getCommandType() == ESyncCommandType::Crc)
      {
        if (m_oRequest.getCrc() == oCrc)
        {
          bRet = true;
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
  }
  return bRet;
}

bool CcSyncServerWorker::sendFile(const CcString& sPath)
{
  bool bRet = false;
  CcFile oFile(sPath);
  CcCrc32 oCrc;
  if (oFile.open(EOpenFlags::Read | EOpenFlags::ShareRead))
  {
    bool bTransfer = true;
    CcByteArray oBuffer(static_cast<size_t>(CcSyncGlobals::TransferSize));
    size_t uiLastTransferSize;
    while (bTransfer)
    {
      uiLastTransferSize = oFile.readArray(oBuffer, false);
      if (uiLastTransferSize > 0 && uiLastTransferSize <= oBuffer.size() )
      {
        oCrc.append(oBuffer.getArray(), uiLastTransferSize);
        if (m_oSocket.write(oBuffer.getArray(), uiLastTransferSize) != uiLastTransferSize)
        {
          bTransfer = false;
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
    CcByteArray oByteArray(static_cast<size_t>(CcSyncGlobals::TransferSize));
    m_oSocket.readArray(oByteArray);
    m_oRequest.parseData(oByteArray);
    if (m_oRequest.getCommandType() == ESyncCommandType::Crc)
    {
      if (m_oRequest.getCrc() == oCrc)
      {
        bRet = true;
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
  return bRet;
}

bool CcSyncServerWorker::loadDirectory()
{
  bool bRet = false;
  // Check all required data
  if (m_oRequest.data().contains(CcSyncGlobals::Commands::DirectoryGetFileList::DirectoryName))
  {
    CcString sDirectoryName = m_oRequest.data()[CcSyncGlobals::Commands::DirectoryGetFileList::DirectoryName].getValue().getString();
    if (m_oDirectory.getName() == sDirectoryName)
    {
      return true;
    }
    else
    {
      CcSyncDirectoryConfig* pDirectory = m_oUser.getAccountConfig()->directoryList().getDirectoryByName(sDirectoryName);
      if (pDirectory != nullptr)
      {
        m_oDirectory.init(m_oUser.getDatabase(), pDirectory);
        bRet = true;
      }
      else
      {
        m_oResponse.setError(EStatus::ConfigFolderNotFound, "Requested Directory not found.");
      }
    }
  }
  else
  {
    m_oResponse.setError(EStatus::CommandRequiredParameter, "No Directory specified.");
  }
  return bRet;
}

void CcSyncServerWorker::doServerGetInfo()
{
  m_oResponse.setError(EStatus::CommandNotImplemented, "Not yet implemented");
  sendResponse();
}

void CcSyncServerWorker::doServerRescan()
{
  if (loadConfigsBySessionRequest() &&
      m_oUser.getRights() >= ESyncRights::Admin)
  {
    bool bDeep = m_oRequest.getServerRescan();
    if (m_pPrivate->oRescanWorker.start(bDeep))
    {
      m_oResponse.setError(EStatus::NoError, "Rescan in progress, it cant take a lot of time.");
    }
    m_oResponse.setError(EStatus::CommandNotImplemented, "Currently not implemented");
  }
  else
  {
    m_oResponse.setError(EStatus::UserAccessDenied, "No permission to create Account");
  }
  sendResponse();
}

void CcSyncServerWorker::doServerAccountCreate()
{
  if (loadConfigsBySessionRequest() &&
      m_oUser.getRights() >= ESyncRights::Admin)
  {
    CcString sName = m_oRequest.getName();
    CcString sPassword = m_oRequest.getPassword();
    if (sName.length() > 0 && sPassword.length() > 0)
    {
      if(!m_oServer->createAccount(sName, sPassword, false))
      {
        m_oResponse.setError(EStatus::UserError, "Failed to create User");
      }
    }
    else
    {
      m_oResponse.setError(EStatus::CommandRequiredParameter, "Required Parameters are not available or empty");
    }
  }
  else
  {
    m_oResponse.setError(EStatus::UserAccessDenied, "No permission to create Account");
  }
  sendResponse();
}

void CcSyncServerWorker::doServerAccountRemove()
{
  if (loadConfigsBySessionRequest() &&
      m_oUser.getRights() >= ESyncRights::Admin)
  {
    CcString sName = m_oRequest.getName();
    if (sName.length() > 0)
    {
      if (!m_oServer->removeAccount(sName))
      {
        m_oResponse.setError(EStatus::UserError, "Failed to remove Account");
      }
    }
    else
    {
      m_oResponse.setError(EStatus::CommandRequiredParameter, "Required Parameters are not available or empty");
    }
  }
  else
  {
    m_oResponse.setError(EStatus::UserAccessDenied, "No permission to create Account");
  }
  sendResponse();
}

void CcSyncServerWorker::doServerStop()
{
  if (loadConfigsBySessionRequest() &&
      m_oUser.getRights() >= ESyncRights::Admin)
  {
    m_oServer->workerDone(this);
    m_oServer->shutdown();
  }
  else
  {
    m_oResponse.setError(EStatus::UserAccessDenied, "No permission to stop Server");
  }
  sendResponse();
}

void CcSyncServerWorker::doAccountCreate()
{
  m_oResponse.setError(EStatus::CommandNotImplemented, "Not yet implemented");
  
  sendResponse();
}

void CcSyncServerWorker::doAccountLogin()
{
  if (m_oRequest.getData().contains(CcSyncGlobals::Commands::AccountLogin::Account) &&
      m_oRequest.getData().contains(CcSyncGlobals::Commands::AccountLogin::Username) &&
      m_oRequest.getData().contains(CcSyncGlobals::Commands::AccountLogin::Password) )
  {
    CcString sAccount = m_oRequest.data()[CcSyncGlobals::Commands::AccountLogin::Account].getValue().getString();
    CcString sUserName = m_oRequest.data()[CcSyncGlobals::Commands::AccountLogin::Username].getValue().getString();
    CcString sPassword = m_oRequest.data()[CcSyncGlobals::Commands::AccountLogin::Password].getValue().getString();
    CcSyncUser oUser = m_oServer->loginUser(sAccount, sUserName, sPassword);
    if (oUser.isValid())
    {
      m_oUser = oUser;
      m_oResponse.setLogin(oUser.getToken());
    }
    else
    {
      CcSyncLog::writeDebug("AccountLogin failed: " + sUserName);
      m_oResponse.setError(EStatus::LoginFailed, "Login Failed");
    }
  }
  else
  {
    CcSyncLog::writeDebug("AccountLogin failed due to wrong parameters");
    m_oResponse.setError(EStatus::CommandRequiredParameter, "User login data missing.");
  }
  sendResponse();
}

void CcSyncServerWorker::doAccountGetData()
{
  if (loadConfigsBySessionRequest())
  {
    m_oResponse.addAccountInfo(*m_oUser.getAccountConfig().ptr());
  }
  else
  {
    CcSyncLog::writeDebug("AccountLogin failed due to wrong parameters");
    m_oResponse.setError(EStatus::CommandRequiredParameter, "User login data missing.");
  }
  sendResponse();
}

void CcSyncServerWorker::doAccountSetData()
{
  if (m_oUser.getRights() >= ESyncRights::Account)
  {
    if (loadConfigsBySessionRequest())
    {
      CcSyncAccountConfig oClientAccount = m_oRequest.getAccountConfig();
      if (oClientAccount.getName() == m_oUser.getAccountConfig()->getName())
      {
        for (CcSyncDirectoryConfig& oClientDirectory : oClientAccount.directoryList())
        {
          for (CcSyncDirectoryConfig& oServerDirectory : m_oUser.getAccountConfig()->directoryList())
          {
            if (oServerDirectory.getName() == oClientDirectory.getName())
            {
              if (oServerDirectory.getBackupCommand() != oClientDirectory.getBackupCommand())
                  oServerDirectory.setBackupCommand(oClientDirectory.getBackupCommand());
              if (oServerDirectory.getRestoreCommand() != oClientDirectory.getRestoreCommand())
                oServerDirectory.setRestoreCommand(oClientDirectory.getRestoreCommand());
              break;
            }
          }
        }
      }
    }
  }
  sendResponse();
}

void CcSyncServerWorker::doAccountGetDirectoryList()
{
  m_oResponse.setError(EStatus::CommandNotImplemented, "Not yet implemented");
  sendResponse();
}

void CcSyncServerWorker::doAccountCreateDirectory()
{
  if (loadConfigsBySessionRequest())
  {
    if (m_oUser.getRights() >= ESyncRights::Account)
    {
      CcJsonData oDirname = m_oRequest.getData()[CcSyncGlobals::Commands::AccountCreateDirectory::DirectoryName];
      if (oDirname.isNotNull() && oDirname.isValue())
      {
        CcString sName = oDirname.getValue().getString();
        if (sName.length() == 0)
        {
          m_oResponse.setError(EStatus::FSDirCreateFailed, "Name not found");
        }
        else if (sName.contains(" "))
        {
          m_oResponse.setError(EStatus::FSDirCreateFailed, "Space is not allowed in directory name");
        }
        else if (m_oUser.getAccountConfig()->getDirectoryList().containsDirectory(sName))
        {
          // Directory already exists nothing to do more.
        }
        else
        {
          CcString sDirectoryPath = m_oServer->config().getLocation().getPath();
          sDirectoryPath.appendPath(m_oUser.getAccountConfig()->getName());
          sDirectoryPath.appendPath(sName);
          if (CcDirectory::exists(sDirectoryPath) ||
              CcDirectory::create(sDirectoryPath, true))
          {
            m_oUser.getDatabase()->setupDirectory(sName);
            m_oUser.getAccountConfig()->addAccountDirectory(sName, sDirectoryPath);
          }
          else
          {
            m_oResponse.setError(EStatus::FSDirCreateFailed, "No permission to create directory");
          }
        }
      }
      else
      {
        m_oResponse.setError(EStatus::CommandRequiredParameter, "Directory name not found");
      }
    }
    else
    {
      m_oResponse.setError(EStatus::UserAccessDenied, "This user " + m_oUser.getAccountConfig()->getName() + " has no permission to create directory: "
        + CcString::fromNumber(static_cast<uint32>(m_oUser.getRights())));
    }
  }
  else
  {
    m_oResponse.setError(EStatus::UserAccessDenied, "No permission");
  }
  sendResponse();
}

void CcSyncServerWorker::doAccountRemoveDirectory()
{
  if (loadConfigsBySessionRequest())
  {
    if (m_oUser.getRights() >= ESyncRights::Account)
    {
      CcJsonData oDirname = m_oRequest.getData()[CcSyncGlobals::Commands::AccountCreateDirectory::DirectoryName];
      if (oDirname.isNotNull() && oDirname.isValue())
      {
        CcString sName = oDirname.getValue().getString();
        if (sName.length() == 0)
        {
          m_oResponse.setError(EStatus::FSDirCreateFailed, "Name not found");
        }
        else if (m_oUser.getAccountConfig()->getDirectoryList().containsDirectory(sName))
        {
          m_oUser.getAccountConfig()->removeAccountDirectory(sName);
        }
        else
        {
          m_oResponse.setError(EStatus::FSDirNotFound, "Directory not found");
        }
      }
      else
      {
        m_oResponse.setError(EStatus::CommandRequiredParameter, "Directory name not found");
      }
    }
    else
    {
      m_oResponse.setError(EStatus::UserAccessDenied, "This user " + m_oUser.getAccountConfig()->getName() + " has no permission to create directory: "
        + CcString::fromNumber(static_cast<uint32>(m_oUser.getRights())));
    }
  }
  else
  {
    m_oResponse.setError(EStatus::UserAccessDenied, "No permission");
  }
  sendResponse();
}

void CcSyncServerWorker::doAccountRights()
{
  // Check all required data
  if (loadConfigsBySessionRequest())
  {
    ESyncRights eRights = m_oUser.getRights();
    m_oResponse.setAccountRight(eRights);
  }
  else
  {
    m_oResponse.setError(EStatus::UserAccessDenied, "No permission");
  }
  sendResponse();
}

void CcSyncServerWorker::doAccountDatabaseUpdateChanged()
{
  // Check all required data
  if (loadConfigsBySessionRequest())
  {
    for (const CcSyncDirectoryConfig& sDirectory : m_oUser.getAccountConfig()->getDirectoryList())
    {
      m_oUser.getDatabase()->beginTransaction();
      m_oUser.getDatabase()->directoryListUpdateChangedAll(sDirectory.getName());
      m_oUser.getDatabase()->endTransaction();
    }
  }
  else
  {
    m_oResponse.setError(EStatus::UserAccessDenied, "No permission");
  }
  sendResponse();
}

void CcSyncServerWorker::doUserGetCommandList()
{
  m_oResponse.setError(EStatus::CommandNotImplemented, "Not yet implemented");
  sendResponse();
}

void CcSyncServerWorker::doDirectoryGetFileList()
{
  // Check all required data
  if (loadConfigsBySessionRequest() &&
      loadDirectory())
  {
    if (m_oRequest.data().contains(CcSyncGlobals::Commands::DirectoryGetFileList::Id))
    {
      size_t uiId = m_oRequest.data()[CcSyncGlobals::Commands::DirectoryGetFileList::Id].getValue().getSize();
      CcSyncFileInfoList oDirectoryInfos = m_oDirectory.getDirectoryInfoListById(uiId);
      CcSyncFileInfoList oFileInfos      = m_oDirectory.getFileInfoListById(uiId);
      //CcSyncLog::writeDebug("DirId: " + CcString::fromNumber(uiId) + " Dirs: " + CcString::fromNumber(oDirectoryInfos.size()) + " Files: " + CcString::fromNumber(oFileInfos.size()));
      m_oResponse.addDirectoryDirectoryInfoList(oDirectoryInfos, oFileInfos);
    }
    else
    {
      if (!m_oRequest.data().contains(CcSyncGlobals::FileInfo::DirId))
        m_oResponse.setError(EStatus::CommandRequiredParameter, "Required parameter not found: " + CcSyncGlobals::FileInfo::DirId);
      else if (!m_oRequest.data().contains(CcSyncGlobals::FileInfo::Name))
        m_oResponse.setError(EStatus::CommandRequiredParameter, "Required parameter not found: " + CcSyncGlobals::FileInfo::Name);
      else if (!m_oRequest.data().contains(CcSyncGlobals::FileInfo::Modified))
        m_oResponse.setError(EStatus::CommandRequiredParameter, "Required parameter not found: " + CcSyncGlobals::FileInfo::Modified);
      else
        m_oResponse.setError(EStatus::CommandRequiredParameter, "At least one parameter is missing.");
    }
  }
  sendResponse();
}

void CcSyncServerWorker::doDirectoryGetFileInfo()
{
  // Check all required data
  if (loadConfigsBySessionRequest() &&
      loadDirectory())
  {
    if (m_oRequest.data().contains(CcSyncGlobals::Commands::DirectoryGetFileList::Id))
    {
      uint64 uiFileId = m_oRequest.data()[CcSyncGlobals::Commands::DirectoryGetFileInfo::Id].getValue().getUint64();
      CcSyncFileInfo oDirInfo = m_oDirectory.getFileInfoById(uiFileId);
      m_oResponse.addFileInfo(oDirInfo);
    }
    else
    {
      m_oResponse.setError(EStatus::CommandRequiredParameter, "Required parameters not found.");
    }
  }
  sendResponse();
}

void CcSyncServerWorker::doDirectoryGetDirectoryInfo()
{
  if (loadConfigsBySessionRequest() &&
    loadDirectory())
  {
    if (m_oRequest.data().contains(CcSyncGlobals::Commands::DirectoryGetDirectoryInfo::Id))
    {
      uint64 uiDirId = m_oRequest.data()[CcSyncGlobals::Commands::DirectoryGetDirectoryInfo::Id].getValue().getUint64();
      CcSyncDirInfo oDirInfo = m_oDirectory.getDirectoryInfoById(uiDirId);
      m_oResponse.addFileInfo(oDirInfo);
    }
    else
    {
      if (!m_oRequest.data().contains(CcSyncGlobals::FileInfo::DirId))
        m_oResponse.setError(EStatus::CommandRequiredParameter, "Required parameter not found: " + CcSyncGlobals::FileInfo::DirId);
      else if (!m_oRequest.data().contains(CcSyncGlobals::FileInfo::Name))
        m_oResponse.setError(EStatus::CommandRequiredParameter, "Required parameter not found: " + CcSyncGlobals::FileInfo::Name);
      else if (!m_oRequest.data().contains(CcSyncGlobals::FileInfo::Modified))
        m_oResponse.setError(EStatus::CommandRequiredParameter, "Required parameter not found: " + CcSyncGlobals::FileInfo::Modified);
      else
        m_oResponse.setError(EStatus::CommandRequiredParameter, "At least one parameter is missing.");
    }
  }
  sendResponse();
}

void CcSyncServerWorker::doDirectoryCreateDirectory()
{
  if (loadConfigsBySessionRequest() &&
    loadDirectory())
  {
    if (m_oRequest.data().contains(CcSyncGlobals::FileInfo::Name) &&
        m_oRequest.data().contains(CcSyncGlobals::FileInfo::Modified))
    {
      CcSyncFileInfo oFileInfo = m_oRequest.getFileInfo();
      if (m_oDirectory.directoryListSubDirExists(oFileInfo.getDirId(), oFileInfo.getName()) == false)
      {
        if (m_oDirectory.directoryListCreate(oFileInfo, true))
        {
          m_oResponse.addFileInfo(oFileInfo);
        }
        else
        {
          m_oResponse.setError(EStatus::FSDirCreateFailed, "Error on creating sub directory.");
        }
      }
      else
      {
        // Directory already exits, turn back Info
        oFileInfo = m_oDirectory.getDirectoryInfoFromSubdir(oFileInfo.getDirId(), oFileInfo.getName());
        if (oFileInfo.getName().length() > 0)
        {
          m_oResponse.addFileInfo(oFileInfo);
        }
        else
        {
          m_oResponse.setError(EStatus::FSDirAlreadyExists, "Enexpected: Directory already exists, but not findable?");
        }
      }
    }
    else
    {
      if (!m_oRequest.data().contains(CcSyncGlobals::FileInfo::DirId))
        m_oResponse.setError(EStatus::CommandRequiredParameter, "Required parameter not found: " + CcSyncGlobals::FileInfo::DirId);
      else if (!m_oRequest.data().contains(CcSyncGlobals::FileInfo::Name))
        m_oResponse.setError(EStatus::CommandRequiredParameter, "Required parameter not found: " + CcSyncGlobals::FileInfo::Name);
      else if (!m_oRequest.data().contains(CcSyncGlobals::FileInfo::Modified))
        m_oResponse.setError(EStatus::CommandRequiredParameter, "Required parameter not found: " + CcSyncGlobals::FileInfo::Modified);
      else
        m_oResponse.setError(EStatus::CommandRequiredParameter, "At least one parameter is missing.");
    }
  }
  sendResponse();
}

void CcSyncServerWorker::doDirectoryRemoveDirectory()
{
  if (loadConfigsBySessionRequest() &&
      loadDirectory())
  {
    if (m_oRequest.data().contains(CcSyncGlobals::FileInfo::Id) &&
        m_oRequest.data().contains(CcSyncGlobals::FileInfo::Name))
    {
      CcSyncFileInfo oFileInfo = m_oRequest.getFileInfo();
      if (m_oDirectory.directoryListExists(oFileInfo.getId()))
      {
        if (m_oDirectory.directoryListEmpty(oFileInfo.getId()))
        {
          m_oDirectory.directoryListRemove(oFileInfo, true);
        }
        else
        {
          m_oResponse.setError(EStatus::FSDirNotEmpty, "Directory is not empty");
        }
      }
      else
      {
        m_oResponse.setError(EStatus::FSDirNotFound, "Directory Not Found in database");
      }
    }
    else
    {
      if (!m_oRequest.data().contains(CcSyncGlobals::FileInfo::DirId))
        m_oResponse.setError(EStatus::CommandRequiredParameter, "Required parameter not found: " + CcSyncGlobals::FileInfo::DirId);
      else if (!m_oRequest.data().contains(CcSyncGlobals::FileInfo::Name))
        m_oResponse.setError(EStatus::CommandRequiredParameter, "Required parameter not found: " + CcSyncGlobals::FileInfo::Name);
      else if (!m_oRequest.data().contains(CcSyncGlobals::FileInfo::Modified))
        m_oResponse.setError(EStatus::CommandRequiredParameter, "Required parameter not found: " + CcSyncGlobals::FileInfo::Modified);
      else
        m_oResponse.setError(EStatus::CommandRequiredParameter, "At least one parameter is missing.");
    }
  }
  sendResponse();
}

void CcSyncServerWorker::doDirectoryUploadFile()
{
  if (loadConfigsBySessionRequest() &&
      loadDirectory())
  {
    if (m_oRequest.data().contains(CcSyncGlobals::FileInfo::DirId) &&
        m_oRequest.data().contains(CcSyncGlobals::FileInfo::Name) &&
        m_oRequest.data().contains(CcSyncGlobals::FileInfo::Modified))
    {
      CcSyncFileInfo oFileInfo = m_oRequest.getFileInfo();
      if (m_oDirectory.directoryListExists(oFileInfo.getDirId()))
      {
        m_oDirectory.getFullDirPathById(oFileInfo);
        CcString sTempFilePath = oFileInfo.getSystemFullPath();
        sTempFilePath.append(CcSyncGlobals::TemporaryExtension);
        CcFile oFile(sTempFilePath);
        if (oFile.open(EOpenFlags::Overwrite))
        {
          sendResponse();
          if (receiveFile(&oFile, oFileInfo))
          {
            oFile.close();
            bool bSuccess = true;
            if (m_oDirectory.fileNameInDirExists(oFileInfo.getDirId(), oFileInfo))
            {
              CcSyncFileInfo oFileToDelete = m_oDirectory.getFileInfoByFilename(oFileInfo.getDirId(), oFileInfo.getName());
              m_oDirectory.fileListRemove(oFileToDelete, false, false);
            }
            if (CcFile::exists(oFileInfo.getSystemFullPath()))
            {
              if (!CcFile::remove(oFileInfo.getSystemFullPath()))
              {
                bSuccess = CcFile::remove(sTempFilePath);
                if(bSuccess == false)
                {
                  CcSyncLog::writeDebug("Failed to remove original File: " + oFileInfo.getSystemFullPath());
                }
              }
            }
            if (bSuccess)
            {
              bSuccess = CcFile::move(sTempFilePath, oFileInfo.getSystemFullPath());
              if (!bSuccess)
              {
                m_oResponse.init(ESyncCommandType::Crc);
                m_oResponse.setError(EStatus::FSFileCreateFailed, "Failed to move temporary File");
                CcSyncLog::writeDebug("Failed to move temporary File: ");
                CcSyncLog::writeDebug("  " + sTempFilePath + " -> " + oFileInfo.getSystemFullPath());
              }
            }
            if (bSuccess)
            {
              if (m_oDirectory.fileListCreate(oFileInfo, true))
              {
                m_oResponse.init(ESyncCommandType::Crc);
                m_oResponse.addFileInfo(oFileInfo);
              }
              else
              {
                m_oResponse.init(ESyncCommandType::Crc);
                m_oResponse.setError(EStatus::FSFileCreateFailed, "File add to database failed");
                CcFile::remove(sTempFilePath);
              }
            }
          }
          else
          {
            oFile.close();
            CcFile::remove(oFileInfo.getSystemFullPath());
            m_oResponse.init(ESyncCommandType::Crc);
            m_oResponse.setError(EStatus::FileTransferFailed, "Crc comparision failed");
          }
        }
        else
        {
          m_oResponse.setError(EStatus::FSFileCreateFailed, "Unable to create temporary file for upload.");
        }
      }
      else
      {
        m_oResponse.setError(EStatus::FSDirNotFound, "Parent Directory for file not existing: " + CcString::fromNumber( oFileInfo.getDirId()));
      }
    }
    else
    {
      if (!m_oRequest.data().contains(CcSyncGlobals::FileInfo::DirId))
        m_oResponse.setError(EStatus::CommandRequiredParameter, "Required parameter not found: " + CcSyncGlobals::FileInfo::DirId);
      if (!m_oRequest.data().contains(CcSyncGlobals::FileInfo::Name))
        m_oResponse.setError(EStatus::CommandRequiredParameter, "Required parameter not found: " + CcSyncGlobals::FileInfo::Name);
      if (!m_oRequest.data().contains(CcSyncGlobals::FileInfo::Modified))
        m_oResponse.setError(EStatus::CommandRequiredParameter, "Required parameter not found: " + CcSyncGlobals::FileInfo::Modified);
    }
  }
  sendResponse();
}

void CcSyncServerWorker::doDirectoryRemoveFile()
{
  if (loadConfigsBySessionRequest() &&
      loadDirectory())
  {
    if (m_oRequest.data().contains(CcSyncGlobals::FileInfo::DirId)           &&
        m_oRequest.data().contains(CcSyncGlobals::FileInfo::Name))
    {
      CcSyncFileInfo oFileInfo = m_oRequest.getFileInfo();
      if (m_oDirectory.directoryListExists(oFileInfo.getDirId()) == false)
      {
        m_oResponse.setError(EStatus::FSDirNotFound, "Directory Not Found");
      }
      else if(m_oDirectory.fileIdInDirExists(oFileInfo.getDirId(), oFileInfo) == false)
      {
        m_oResponse.setError(EStatus::FSFileNotFound, "File Not Found");
      }
      else
      {
        m_oDirectory.fileListRemove(oFileInfo, true, false);
      }
    }
    else
    {
      m_oResponse.setError(EStatus::CommandRequiredParameter, "Required parameters not found.");
    }
  }
  sendResponse();
}

void CcSyncServerWorker::doDirectoryDownloadFile()
{
  if (loadConfigsBySessionRequest() &&
      loadDirectory())
  {
    if (m_oRequest.data().contains(CcSyncGlobals::FileInfo::Id))
    {
      uint64 uiFileId = m_oRequest.data()[CcSyncGlobals::FileInfo::Id].getValue().getUint64();
      CcSyncFileInfo oFileInfo = m_oDirectory.getFileInfoById(uiFileId);
      m_oDirectory.getFullDirPathById(oFileInfo);
      if (CcFile::exists(oFileInfo.getSystemFullPath()))
      {
        bool bSuccess = true;
        // Check if Database file is up to date with disk file
        CcFileInfo oFileInfoLocal = CcFile(oFileInfo.getSystemFullPath()).getInfo();
        if(oFileInfo != oFileInfoLocal)
        {
          // We have to update our file info in Database
          oFileInfo.fromSystemFile(true);
          bSuccess = m_oDirectory.fileListRemove(oFileInfo, false, true);
          bSuccess = m_oDirectory.fileListCreate(oFileInfo, true);
          if(bSuccess == false)
          {
            CcSyncLog::writeDebug("DirectoryDownloadFile send File failed:");
            CcSyncLog::writeDebug("    " + oFileInfo.getSystemFullPath());
            m_oResponse.setError(EStatus::FSFileError, "File in database differ with local");
          }
        }
        if(bSuccess == true)
        {
          m_oResponse.addFileInfo(oFileInfo);
          sendResponse();
          if (sendFile(oFileInfo.getSystemFullPath()))
          {
            m_oResponse.init(ESyncCommandType::Crc);
          }
          else
          {
            CcSyncLog::writeDebug("DirectoryDownloadFile send File failed:");
            CcSyncLog::writeDebug("    " + oFileInfo.getSystemFullPath());
            m_oResponse.setError(EStatus::FSFileCrcFailed, "FileTransfer failed");
          }
        }
      }
      else
      {
        m_oResponse.setError(EStatus::FSFileCrcFailed, oFileInfo.getRelativePath() + " not existing, Server will delete file in list");
        m_oDirectory.fileListRemove(oFileInfo, true, false);
      }
    }
    else
    {
      m_oResponse.setError(EStatus::CommandRequiredParameter, "Required parameters not found.");
    }
  }
  sendResponse();
}

void CcSyncServerWorker::onStop()
{
  m_pPrivate->m_bStopCalled = true;
  if(getThreadState() == EThreadState::Running)
  {
    enterState(EThreadState::Stopping);
  }
  m_oSocket.close();
}
