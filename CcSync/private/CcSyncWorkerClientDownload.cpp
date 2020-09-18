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
 * @brief     Implemtation of class CcSyncWorkerClientDownload
 */
#include "CcSyncWorkerClientDownload.h"
#include "CcSyncLog.h"
#include "CcDirectory.h"
#include "CcFile.h"
#include "Hash/CcCrc32.h"
#include "CcKernel.h"
#include "CcStringUtil.h"

namespace CcSync
{

CcSyncWorkerClientDownload::CcSyncWorkerClientDownload(CcSyncDirectory& oDirectory, CcSyncFileInfo& oFileInfo, uint64 uiQueueIndex, CcSyncClientCom& pSocket) :
  ISyncWorkerBase(oDirectory, oFileInfo, uiQueueIndex, pSocket),
  m_oStartTime(CcKernel::getUpTime())
{
}

void CcSyncWorkerClientDownload::run()
{
  bool bRet = false;
  m_oCom.getRequest().setDirectoryDownloadFile(m_oDirectory.getName(), m_oFileInfo.getId());
  if (m_oCom.sendRequestGetResponse())
  {
    m_oFileInfo = m_oCom.getResponse().getFileInfo();
    m_oDirectory.getFullDirPathById(m_oFileInfo);
    if (CcDirectory::exists(m_oFileInfo.getSystemDirPath()) ||
      CcDirectory::create(m_oFileInfo.getSystemDirPath(), true))
    {
      CcString sTempFilePath = m_oFileInfo.getSystemFullPath();
      sTempFilePath.append(CcSyncGlobals::TemporaryExtension);
      CcFile oFile(sTempFilePath);
      if (oFile.open(EOpenFlags::Overwrite))
      {
        if (receiveFile(&oFile))
        {
          oFile.close();
          if (m_oDirectory.fileNameInDirExists(m_oFileInfo.getDirId(), m_oFileInfo))
          {
            CcSyncFileInfo oFileToDelete = m_oDirectory.getFileInfoByFilename(m_oFileInfo.getDirId(), m_oFileInfo.getName());
            m_oDirectory.fileListRemove(oFileToDelete, false, false);
          }
          bool bSuccess = true;
          if (CcFile::exists(m_oFileInfo.getSystemFullPath()))
          {
            if (!CcFile::remove(m_oFileInfo.getSystemFullPath()))
            {
              bSuccess = false;
              CcFile::remove(sTempFilePath);
              CcSyncLog::writeDebug("Failed to remove original File: " + m_oFileInfo.getSystemFullPath(), ESyncLogTarget::Client);
            }
          }
          if (bSuccess)
          {
            bSuccess = CcFile::move(sTempFilePath, m_oFileInfo.getSystemFullPath());
            if (bSuccess == false)
            {
              CcFile::remove(sTempFilePath);
              CcSyncLog::writeDebug("Failed to move temporary File: ", ESyncLogTarget::Client);
              CcSyncLog::writeDebug("  " + sTempFilePath + " -> " + m_oFileInfo.getSystemFullPath(), ESyncLogTarget::Client);
            }
          }
          if (bSuccess)
          {
            setFileInfo(m_oFileInfo.getSystemFullPath(), m_oDirectory.getUserId(), m_oDirectory.getGroupId(), m_oFileInfo.getModified());
            if (m_oDirectory.fileListInsert(m_oFileInfo, true))
            {
              CcSyncLog::writeDebug("File downloaded: " + m_oFileInfo.getName(), ESyncLogTarget::Client);
              bRet = true;
              m_oDirectory.queueFinalizeFile(m_uiQueueIndex);
            }
            else
            {
              CcSyncLog::writeError("Insert to Filelist failed", ESyncLogTarget::Client);
              CcSyncLog::writeError("    ErrorMsg: " + m_oCom.getResponse().getErrorMsg(), ESyncLogTarget::Client);
              m_oDirectory.queueIncrementItem(m_uiQueueIndex);
            }
          }
        }
        else
        {
          oFile.close();
          CcFile::remove(sTempFilePath);
          CcSyncLog::writeError("File download failed", ESyncLogTarget::Client);
          CcSyncLog::writeError("    ErrorMsg: " + m_oCom.getResponse().getErrorMsg(), ESyncLogTarget::Client);
          m_oDirectory.queueIncrementItem(m_uiQueueIndex);
        }
      }
      else
      {
        CcSyncLog::writeError("Unable to create file", ESyncLogTarget::Client);
        CcSyncLog::writeError("    ErrorMsg: " + m_oCom.getResponse().getErrorMsg(), ESyncLogTarget::Client);
        m_oDirectory.queueIncrementItem(m_uiQueueIndex);
      }
    }
    else
    {
      CcSyncLog::writeError("Directory for download not found: " + m_oFileInfo.getSystemFullPath(), ESyncLogTarget::Client);
      m_oDirectory.queueIncrementItem(m_uiQueueIndex);
    }
  }
  else
  {
    CcSyncLog::writeError("DownloadFile request failed", ESyncLogTarget::Client);
    CcSyncLog::writeError("    ErrorMsg: " + m_oCom.getResponse().getErrorMsg(), ESyncLogTarget::Client);
    m_oDirectory.queueIncrementItem(m_uiQueueIndex);
  }
  if(bRet)
  {
    setExitCode(0);
  }
}

double CcSyncWorkerClientDownload::getProgress()
{
  double fProgress = 0.0;
  if (m_oFileInfo.getFileSize() != 0.0)
    fProgress = (static_cast<double>(m_uiReceived) / static_cast<double>(m_oFileInfo.getFileSize())) * 100.0;
  return fProgress;
}

CcString CcSyncWorkerClientDownload::getProgressMessage()
{
  CcString sMessage = "Download File: ";
  CcString sRate = CcStringUtil::getHumanReadableSizePerSeconds(m_uiReceived, CcKernel::getUpTime() - m_oStartTime);
  sMessage  << m_oFileInfo.getRelativePath() << " (" 
            << CcString::fromNumber(getProgress(), 1, true) 
            << "%, with " << sRate << ")   ";
  return sMessage;
}

void CcSyncWorkerClientDownload::setFileInfo(const CcString& sPathToFile, uint32 uiUserId, uint32 uiGroupId, int64 iModified)
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

bool CcSyncWorkerClientDownload::receiveFile(CcFile* pFile)
{
  bool bRet = false;
  bool bTransfer = true;
  CcCrc32 oCrc;
  size_t uiBufferSize = static_cast<size_t>(CcSyncGlobals::TransferSize);
  if (m_oFileInfo.getFileSize() < CcSyncGlobals::TransferSize)
  {
    uiBufferSize = static_cast<size_t>(m_oFileInfo.getFileSize());
  }
  CcByteArray oByteArray(uiBufferSize);
  while (bTransfer)
  {
    if (m_uiReceived < m_oFileInfo.getFileSize())
    {
      size_t uiReadSize = m_oCom.getSocket().readArray(oByteArray, false);
      if (uiReadSize <= uiBufferSize)
      {
        oCrc.append(oByteArray.getArray(), uiReadSize);
        m_uiReceived += uiReadSize;
        if (pFile->write(oByteArray.getArray(), uiReadSize) != uiReadSize)
        {
          bTransfer = false;
        }
      }
      else
      {
        bTransfer = false;
        CcSyncLog::writeError("Error during socket read, reconnect", ESyncLogTarget::Client);
        m_oCom.reconnect();
      }
    }
    else
    {
      bTransfer = false;
      m_oFileInfo.crc() = oCrc.getValueUint32();
      m_oCom.getRequest().setCrc(oCrc);
      if (m_oCom.sendRequestGetResponse())
      {
        if (m_oCom.getResponse().hasError() == false)
          bRet = true;
      }
    }
  }
  return bRet;
}

}
