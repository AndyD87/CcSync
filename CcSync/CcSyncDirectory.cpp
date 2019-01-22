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
 * @brief     Implemtation of class CcSyncDirectory
 */
#include "CcSyncDirectory.h"
#include "CcFile.h"
#include "CcDirectory.h"
#include "CcSyncGlobals.h"
#include "CcFile.h"
#include "CcDirectory.h"
#include "Hash/CcCrc32.h"
#include "Hash/CcMd5.h"
#include "CcSqlite.h"
#include "CcSyncFileInfo.h"
#include "CcSyncFileInfoList.h"
#include "CcSyncDbClient.h"
#include "CcString.h"
#include "CcKernel.h"
#include "CcDateTime.h"
#include "CcSyncLog.h"
#include "CcGlobalStrings.h"

CcSyncDirectory::CcSyncDirectory(const CcSyncDirectory& oToCopy) :
  m_pDatabase(oToCopy.m_pDatabase),
  m_pConfig(oToCopy.m_pConfig)
{
}

CcSyncDirectory::~CcSyncDirectory( void )
{
}

CcSyncDirectory& CcSyncDirectory::operator=(const CcSyncDirectory& oToCopy)
{
  m_pConfig = oToCopy.m_pConfig;
  m_pDatabase = oToCopy.m_pDatabase;
  m_uiRootId = oToCopy.m_uiRootId;
  return *this;
}

void CcSyncDirectory::init(CcSyncDbClientPointer& pDatabase, CcSyncDirectoryConfig* pConfig)
{
  m_pDatabase = pDatabase;
  m_pConfig = pConfig;
  if (pDatabase != nullptr)
    pDatabase->setupDirectory(getName());
}

void CcSyncDirectory::scan( bool bDeepSearch)
{
  if (CcDirectory::exists(m_pConfig->getLocation()))
  {
    m_pDatabase->beginTransaction();
    scanSubDir(m_uiRootId, m_pConfig->getLocation(), bDeepSearch);
    m_pDatabase->endTransaction();
  }
}

bool CcSyncDirectory::validate()
{
  bool bRet = true;
  if (m_pDatabase != nullptr)
  {
    m_pDatabase->setupDirectory(getName());
  }
  return bRet;
}

bool CcSyncDirectory::queueHasItems()
{
  return m_pDatabase->queueHasItem(m_pConfig->getName());
}

EBackupQueueType CcSyncDirectory::queueGetNext(CcSyncFileInfo& oFileInfo, uint64 &uiQueueIndex)
{
  return m_pDatabase->queueGetNext(getName(), oFileInfo, uiQueueIndex);
}

void CcSyncDirectory::queueFinalizeDirectory(CcSyncFileInfo& oFileInfo, uint64 uiQueueIndex)
{
  m_pDatabase->queueFinalizeDirectory(getName(), oFileInfo, uiQueueIndex);
}

void CcSyncDirectory::queueFinalizeFile(uint64 uiQueueIndex)
{
  if(uiQueueIndex != 0)
    m_pDatabase->queueFinalizeFile(getName(), uiQueueIndex);
}

void CcSyncDirectory::queueIncrementItem(uint64 uiQueueIndex)
{
  if(uiQueueIndex != 0)
    m_pDatabase->queueIncrementItem(getName(), uiQueueIndex);
}

void CcSyncDirectory::queueReset()
{
  m_pDatabase->beginTransaction();
  m_pDatabase->queueReset(getName());
  m_pDatabase->endTransaction();
}

void CcSyncDirectory::queueResetAttempts()
{
  m_pDatabase->beginTransaction();
  m_pDatabase->queueResetAttempts(getName());
  m_pDatabase->endTransaction();
}

void CcSyncDirectory::queueDownloadDirectory(const CcSyncFileInfo& oFileInfo)
{
  m_pDatabase->queueDownloadDirectory(getName(), oFileInfo);
}

void CcSyncDirectory::queueDownloadFile(const CcSyncFileInfo& oFileInfo)
{
  m_pDatabase->queueDownloadFile(getName(), oFileInfo);
}

const CcString& CcSyncDirectory::getName() const
{
  if (m_pConfig != nullptr)
    return m_pConfig->getName();
  else
    return CcGlobalStrings::Empty;
}

bool CcSyncDirectory::getInnerPathById(CcSyncFileInfo& oFileInfo)
{
  oFileInfo.dirPath() = m_pDatabase->getInnerPathById(getName(), oFileInfo.getDirId());
  return true;
}

bool CcSyncDirectory::getFullDirPathById(CcSyncFileInfo& oFileInfo)
{
  oFileInfo.systemRootPath() = m_pConfig->getLocation();
  getInnerPathById(oFileInfo);
  return true;
}

bool CcSyncDirectory::fileListInsert(CcSyncFileInfo& oFileInfo)
{
  return m_pDatabase->fileListInsert(getName(), oFileInfo);
}

bool CcSyncDirectory::fileListUpdate(CcSyncFileInfo& oFileInfo, bool bMoveToHistory, bool bDoUpdateParents)
{
  bool bRet = false;
  if (bMoveToHistory)
  {
    getFullDirPathById(oFileInfo);
    if (m_pDatabase->isHistoryEnabled())
    {
      CcDateTime oCurrentTime = CcKernel::getDateTime();
      CcString sPathInHistory = getHistoryDir();
      sPathInHistory.appendPath(oCurrentTime.getString("yyyy/MM/dd"));
      if (CcDirectory::create(sPathInHistory, true))
      {
        sPathInHistory.appendPath(oCurrentTime.getString("hh-mm-ss-zzzuuu."));
        sPathInHistory.append(oFileInfo.getName());
        if (CcFile::move(oFileInfo.getSystemFullPath(), sPathInHistory))
        {
          oFileInfo.changed() = oCurrentTime.getTimestampS();
          if(m_pDatabase->fileListUpdate(getName(), oFileInfo, bDoUpdateParents))
          {
            bRet = historyInsert(EBackupQueueType::UpdateFile, oFileInfo);
          }
        }
        else if (!CcFile::exists(oFileInfo.getSystemFullPath()))
        {
          oFileInfo.changed() = oCurrentTime.getTimestampS();
          CcSyncLog::writeWarning("File was not existing to move: "+oFileInfo.getSystemFullPath(), ESyncLogTarget::Common);
          bRet = m_pDatabase->fileListUpdate(getName(), oFileInfo, bDoUpdateParents);
        }
        else
        {
          CcSyncLog::writeError("Moving File to History failed: "+sPathInHistory, ESyncLogTarget::Common);
        }
      }
      else
      {
        CcSyncLog::writeError("Creating Path in History failed: "+sPathInHistory, ESyncLogTarget::Common);
      }
    }
  }
  else
  {
    bRet = m_pDatabase->fileListUpdate(getName(), oFileInfo, bDoUpdateParents);
  }
  return bRet;
}

bool CcSyncDirectory::fileListRemove(CcSyncFileInfo& oFileInfo, bool bDoUpdateParents, bool bKeepFile)
{
  bool bRet = false;
  getFullDirPathById(oFileInfo);
  if (m_pDatabase->isHistoryEnabled())
  {
    if(CcFile::exists(oFileInfo.getSystemFullPath()))
    {
      CcDateTime oCurrentTime = CcKernel::getDateTime();
      CcString sPathInHistory = getHistoryDir();
      sPathInHistory.appendPath(oCurrentTime.getString("yyyy/MM/dd"));
      if (CcDirectory::create(sPathInHistory, true))
      {
        sPathInHistory.appendPath(oCurrentTime.getString("hh-mm-ss-zzzuuu."));
        sPathInHistory.append(oFileInfo.getName());
        if (CcFile::move(oFileInfo.getSystemFullPath(), sPathInHistory))
        {
          // Securely update all values with database values
          oFileInfo.changed() = oCurrentTime.getTimestampS();
          historyInsert(EBackupQueueType::RemoveFile, oFileInfo);
          bRet = m_pDatabase->fileListRemove(getName(), oFileInfo, bDoUpdateParents);
        }
        else
        {
          CcSyncLog::writeError("CcSyncDirectory::fileListRemove Failed to move File");
        }
      }
      else
      {
        CcSyncLog::writeError("CcSyncDirectory::fileListRemove Failed to create Path in History");
      }
    }
    else
    {
      // File not found for history, mark as failed remove
      historyInsert(EBackupQueueType::RemoveErrorFile, oFileInfo);
      bRet = m_pDatabase->fileListRemove(getName(), oFileInfo, bDoUpdateParents);
    }
  }
  else
  {
    getFullDirPathById(oFileInfo);
    if ( bKeepFile == true ||
         (CcFile::exists(oFileInfo.getSystemFullPath()) &&
          CcFile::remove(oFileInfo.getSystemFullPath())))
    {
      if (m_pDatabase->fileListRemove(getName(), oFileInfo, bDoUpdateParents))
      {
        bRet = true;
      }
      else
      {
        CcSyncLog::writeError("CcSyncDirectory::fileListRemove failed to remove file database");
      }
    }
    else
    {
      CcSyncLog::writeError("CcSyncDirectory::fileListRemove failed to remove file from disk");
    }
  }
  return bRet;
}

CcSyncFileInfoList CcSyncDirectory::getDirectoryInfoListById(uint64 uiDirId)
{
  return m_pDatabase->getDirectoryInfoListById(getName(), uiDirId);
}

CcSyncFileInfoList CcSyncDirectory::getFileInfoListById(uint64 uiDirId)
{
  return m_pDatabase->getFileInfoListById(getName(), uiDirId);
}

CcSyncFileInfo CcSyncDirectory::getDirectoryInfoById(uint64 uiDirId)
{
  return m_pDatabase->getDirectoryInfoById(getName(), uiDirId);
}

CcSyncFileInfo CcSyncDirectory::getDirectoryInfoFromSubdir(uint64 uiDirId, const CcString& sSubDirName)
{
  return m_pDatabase->getDirectoryInfoFromSubdir(getName(), uiDirId, sSubDirName);
}

CcSyncFileInfo CcSyncDirectory::getFileInfoById(uint64 uiFileId)
{
  return m_pDatabase->getFileInfoById(getName(), uiFileId);
}

CcSyncFileInfo CcSyncDirectory::getFileInfoByFilename(uint64 uiDirId, const CcString& sFileName)
{
  return m_pDatabase->getFileInfoByFilename(getName(), uiDirId, sFileName);
}

bool CcSyncDirectory::fileListCreate(CcSyncFileInfo& oFileInfo)
{
  bool bRet = false;
  getFullDirPathById(oFileInfo);
  CcFile oFile(oFileInfo.getSystemFullPath());
  if (oFile.open(EOpenFlags::Write | EOpenFlags::Attributes))
  {
    if (oFile.setModified(CcDateTimeFromSeconds( oFileInfo.getModified())))
    {
      oFileInfo.changed() = CcKernel::getDateTime().getTimestampS();
      // make sure that this file id is not already used!
      if (fileListExists(oFileInfo.getId()))
        fileListRemove(oFileInfo, true, false);

      if (m_pDatabase->fileListInsert(getName(), oFileInfo))
      {
        bRet = true;
      }
    }
    oFile.close();
  }
  return bRet;
}

bool CcSyncDirectory::fileListExists(uint64 uiFileId)
{
  return m_pDatabase->fileListExists(getName(), uiFileId);
}

void CcSyncDirectory::scanSubDir(uint64 uiDbIndex, const CcString& sPath, bool bDeepSearch)
{
  CcSyncFileInfoList oDirectoryInfoList = getDirectoryInfoListById(uiDbIndex);
  CcSyncFileInfoList oFileInfoList = getFileInfoListById(uiDbIndex);
  CcFileInfoList oSystemFileList = CcDirectory::getFileList(sPath);
  for (size_t i=0; i < oSystemFileList.size(); i++)
  {
    const CcFileInfo& oSystemFileInfo = oSystemFileList[i];
    if(oSystemFileInfo.getName().length() == 0)
    {
      // Do not process empty file names
      // They will be generated, for example, from broken links
    }
    else if (oSystemFileInfo.isDir())
    {
      if (oDirectoryInfoList.containsDirectory(oSystemFileInfo.getName()))
      {
        CcString sNextPath(sPath);
        sNextPath.appendPath(oSystemFileInfo.getName());
        const CcSyncFileInfo& oBackupDirectoryInfo = oDirectoryInfoList.getDirectory(oSystemFileInfo.getName());
        if (oBackupDirectoryInfo != oSystemFileInfo)
        {
          queueUpdateDir(oBackupDirectoryInfo);
        }
        scanSubDir(oBackupDirectoryInfo.getId(), sNextPath, bDeepSearch);
        oDirectoryInfoList.removeFile(oSystemFileInfo.getName());
      }
      else
      {
        queueAddDir(0, uiDbIndex, sPath, oSystemFileInfo);
      }
    }
    else
    {
      if (oSystemFileInfo.getName().endsWith(CcSyncGlobals::TemporaryExtension))
      {
        CcString sPathToFile = sPath;
        sPathToFile.appendPath(oSystemFileInfo.getName());
        if (CcFile::remove(sPathToFile))
        {
          CcSyncLog::writeDebug("Temporary file found and removed: " + sPathToFile);
        }
        else
        {
          CcSyncLog::writeError("Temporary file found but failed to remove: " + sPathToFile);
        }
      }
      else if (oFileInfoList.containsFile(oSystemFileInfo.getName()))
      {
        const CcSyncFileInfo& oBackupFileInfo = oFileInfoList.getFile(oSystemFileInfo.getName());
        if (oBackupFileInfo != oSystemFileInfo)
        {
          queueUpdateFile(oBackupFileInfo);
        }
        oFileInfoList.removeFile(oSystemFileInfo.getName());
      }
      else
      {
        queueAddFile(0, uiDbIndex, oSystemFileInfo);
      }
    }
  }
  // FileList is processed, start removing unfound files in database
  for (const CcSyncFileInfo& oBackupFileInfo : oFileInfoList)
  {
    if (oBackupFileInfo.getName().endsWith(CcSyncGlobals::TemporaryExtension))
    {
      m_pDatabase->fileListRemove(getName(), oBackupFileInfo, false);
    }
    else
    {
      queueRemoveFile(0, oBackupFileInfo);
    }
  }
  for (const CcSyncFileInfo& oBackupFileInfo : oDirectoryInfoList)
  {
    if (oBackupFileInfo.getName().endsWith(CcSyncGlobals::TemporaryExtension))
    {
      m_pDatabase->directoryListRemove(getName(), oBackupFileInfo, false);
    }
    else
    {
      queueRemoveDir(0, oBackupFileInfo);
    }
  }
}

void CcSyncDirectory::queueAddDir(uint64 uiDependent, uint64 uiDirId, const CcString& sParentPath, const CcFileInfo& oDirectoryInfo)
{
  CcString sRecursivePath(sParentPath);
  sRecursivePath.appendPath(oDirectoryInfo.getName());
  uint64 uiNewQueueId = m_pDatabase->queueInsert(getName(), uiDependent, EBackupQueueType::AddDir, 0, uiDirId, oDirectoryInfo.getName());
  if (uiNewQueueId > 0)
  {
    CcFileInfoList oFileInfoList = CcDirectory::getFileList(sRecursivePath);
    for (CcFileInfo& oFileInfo : oFileInfoList)
    {
      if (oFileInfo.isDir())
      {
        queueAddDir(uiNewQueueId, 0, sRecursivePath, oFileInfo);
      }
      else
      {
        queueAddFile(uiNewQueueId, 0, oFileInfo);
      }
    }
  }
  else
  {
    CcSyncLog::writeError("Adding Directory to database");
  }
}

void CcSyncDirectory::queueUpdateDir(const CcSyncFileInfo& oFileInfo)
{
  uint64 uiId = m_pDatabase->queueInsert(getName(), 0, EBackupQueueType::UpdateDir, oFileInfo.getId(), oFileInfo.getDirId(), oFileInfo.getName());
  if (uiId == 0)
  {
    CcSyncLog::writeError("Adding Files to database");
  }
}

uint64 CcSyncDirectory::queueRemoveDir(uint64 uiDependent, const CcSyncFileInfo& oFileInfo)
{
  CcSyncFileInfoList oDirectoryInfoList = getDirectoryInfoListById(oFileInfo.getId());
  CcSyncFileInfoList oFileInfoList = getFileInfoListById(oFileInfo.getId());
  // FileList is processed, start removing unfound files in database
  for (const CcSyncFileInfo& oBackupFileInfo : oFileInfoList)
  {
    uiDependent = queueRemoveFile(uiDependent, oBackupFileInfo);
  }
  for (const CcSyncFileInfo& oBackupFileInfo : oDirectoryInfoList)
  {
    uiDependent = queueRemoveDir(uiDependent, oBackupFileInfo);
  }
  uiDependent = m_pDatabase->queueInsert(getName(), uiDependent, EBackupQueueType::RemoveDir, oFileInfo.getId(), oFileInfo.getDirId(), oFileInfo.getName());
  if (uiDependent == 0)
  {
    CcSyncLog::writeError("Adding Files to database");
  }
  return uiDependent;
}

uint64 CcSyncDirectory::queueRemoveFile(uint64 uiDependent, const CcSyncFileInfo& oFileInfo)
{
  uint64 uiId = m_pDatabase->queueInsert(getName(), uiDependent, EBackupQueueType::RemoveFile, oFileInfo.getId(), oFileInfo.getDirId(), oFileInfo.getName());
  if (uiId == 0)
  {
    CcSyncLog::writeError("Adding Files to database");
  }
  return uiId;
}

void CcSyncDirectory::queueAddFile(uint64 uiDependent, uint64 uiDirId, const CcFileInfo& oFileInfo)
{
  uint64 uiId = m_pDatabase->queueInsert(getName(), uiDependent, EBackupQueueType::AddFile, 0, uiDirId, oFileInfo.getName());
  if (uiId == 0)
  {
    CcSyncLog::writeError("Adding file to queue");
  }
}

void CcSyncDirectory::queueUpdateFile(const CcSyncFileInfo& oFileInfo)
{
  uint64 bSuccess = m_pDatabase->queueInsert(getName(), 0, EBackupQueueType::UpdateFile, oFileInfo.getId(), oFileInfo.getDirId(), oFileInfo.getName());
  if (bSuccess == 0)
  {
    CcSyncLog::writeError("Adding Files to database");
  }
}

bool CcSyncDirectory::directoryListCreate(CcSyncFileInfo& oFileInfo)
{
  getFullDirPathById(oFileInfo);
  CcDirectory oNewDirectory(oFileInfo.getSystemFullPath());
  if (oNewDirectory.create(oFileInfo.getSystemFullPath(), true))
  {
    CcFile oTempFile(oFileInfo.getSystemFullPath());
    if (oTempFile.open(EOpenFlags::Attributes))
    {
      oFileInfo.modified() = oTempFile.getCreated().getTimestampS();
      oTempFile.close();
      oFileInfo.changed() = CcKernel::getDateTime().getTimestampS();
      if (m_pDatabase->directoryListInsert(getName(), oFileInfo))
      {
        return true;
      }
      else
      {
        CcSyncLog::writeDebug("Adding new Directory to Database failed: " + oFileInfo.getSystemFullPath());
        oNewDirectory.remove();
      }
    }
    else
    {
      CcSyncLog::writeDebug("Reading directory information failed: " + oFileInfo.getSystemFullPath());
      oNewDirectory.remove();
    }
  }
  else
  {
    CcSyncLog::writeDebug("Creating new Directory failed: " + oFileInfo.getSystemFullPath());
  }
  return false;
}

bool CcSyncDirectory::directoryListUpdate(const CcSyncFileInfo& oFileInfo)
{
  bool bRet = false;
  if (m_pDatabase->directoryListUpdate(getName(), oFileInfo))
  {
    bRet = true;
  }
  else
  {
    CcSyncLog::writeDebug("Updating Directory in Database failed: " + oFileInfo.getName());
  }
  return bRet;
}

bool CcSyncDirectory::directoryListUpdateId(uint64 uiDirectoryId, const CcSyncFileInfo& oFileInfo)
{
  bool bRet = false;
  if (m_pDatabase->directoryListUpdateId(getName(), uiDirectoryId, oFileInfo))
  {
    bRet = true;
  }
  else
  {
    CcSyncLog::writeDebug("Updating Directory in Database failed: " + oFileInfo.getName() + " Id: " + CcString::fromNumber(oFileInfo.getId()));
  }
  return bRet;
}

bool CcSyncDirectory::directoryListRemove(CcSyncFileInfo& oFileInfo, bool bDoUpdateParents)
{
  bool bRet = false;
  getFullDirPathById(oFileInfo);
  CcDateTime oCurrentTime = CcKernel::getDateTime();
  // Remove Directory if exists
  if (CcDirectory::exists(oFileInfo.getSystemFullPath()) == false ||
      CcDirectory::remove(oFileInfo.getSystemFullPath(), true))
  {
    oFileInfo.changed() = oCurrentTime.getTimestampS();
    historyInsert(EBackupQueueType::RemoveDir, oFileInfo);
    if (m_pDatabase->directoryListRemove(getName(), oFileInfo, bDoUpdateParents))
    {
      bRet = true;
    }
    else
    {
      CcSyncLog::writeError("CcSyncDirectory::directoryListRemove failed to remove dir database");
    }
  }
  else
  {
    CcSyncLog::writeError("CcSyncDirectory::directoryListRemove failed to remove dir from disk");
  }
  return bRet;
}

bool CcSyncDirectory::directoryListExists(uint64 uiDirId)
{
  return m_pDatabase->directoryListExists(getName(), uiDirId);
}

bool CcSyncDirectory::directoryListSubDirExists(uint64 uiParentDirId, const CcString& sName)
{
  return m_pDatabase->directoryListSubDirExists(getName(), uiParentDirId, sName);
}

bool CcSyncDirectory::directoryListEmpty(uint64 uiDirId)
{
  return m_pDatabase->directoryListEmpty(getName(), uiDirId);
}

bool CcSyncDirectory::directoryListInsert(CcSyncFileInfo& oFileInfo)
{
  return m_pDatabase->directoryListInsert(getName(), oFileInfo);
}

void CcSyncDirectory::directoryListUpdateChanged(uint64 uiDirId)
{
  return m_pDatabase->directoryListUpdateChanged(getName(), uiDirId);
}

bool CcSyncDirectory::historyInsert(EBackupQueueType eQueueType, const CcSyncFileInfo& oFileInfo)
{
  return m_pDatabase->historyInsert(getName(), eQueueType, oFileInfo);
}

bool CcSyncDirectory::fileIdInDirExists(uint64 uiDirId, const CcSyncFileInfo& oFileInfo)
{
  return m_pDatabase->fileListFileIdExists(getName(), uiDirId, oFileInfo);
}

bool CcSyncDirectory::fileNameInDirExists(uint64 uiDirId, const CcSyncFileInfo& oFileInfo)
{
  return m_pDatabase->fileListFileNameExists(getName(), uiDirId, oFileInfo);
}

CcString CcSyncDirectory::getTemporaryDir()
{
  CcString sTempDir(m_pConfig->getLocation());
  sTempDir.append("_temp");
  if (CcDirectory::exists(sTempDir) == false)
  {
    CcDirectory::create(sTempDir, true);
  }
  return sTempDir;
}

CcString CcSyncDirectory::getHistoryDir()
{
  CcString sTempDir(m_pConfig->getLocation());
  sTempDir.append("_History");
  if (CcDirectory::exists(sTempDir) == false)
  {
    CcDirectory::create(sTempDir, true);
  }
  return sTempDir;
}

uint32 CcSyncDirectory::getUserId()
{
  return m_pConfig->getUserId();
}

uint32 CcSyncDirectory::getGroupId()
{
  return m_pConfig->getGroupId();
}

bool CcSyncDirectory::isLocked()
{
  CcString sBasePath = m_pConfig->getLocation();
  sBasePath.appendPath(CcSyncGlobals::LockFile);
  if (CcFile::exists(sBasePath))
    return true;
  else
    return false;
}
