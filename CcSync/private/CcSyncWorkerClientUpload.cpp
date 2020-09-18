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
 * @brief     Implemtation of class CcSyncWorkerClientUpload
 */
#include "CcSyncWorkerClientUpload.h"
#include "CcSyncLog.h"
#include "CcDirectory.h"
#include "CcFile.h"
#include "Hash/CcCrc32.h"
#include "CcKernel.h"
#include "CcStringUtil.h"

namespace CcSync
{

CcSyncWorkerClientUpload::CcSyncWorkerClientUpload(CcSyncDirectory& oDirectory, CcSyncFileInfo& oFileInfo, uint64 uiQueueIndex, CcSyncClientCom& pSocket) :
  ISyncWorkerBase(oDirectory, oFileInfo, uiQueueIndex, pSocket),
  m_oStartTime(CcKernel::getUpTime())
{
}

void CcSyncWorkerClientUpload::run()
{
  bool bRet = false;
  m_oDirectory.getFullDirPathById(m_oFileInfo);
  if (m_oFileInfo.fromSystemFile(false))
  {
    m_oCom.getRequest().setDirectoryUploadFile(m_oDirectory.getName(), m_oFileInfo);
    if (m_oCom.sendRequestGetResponse())
    {
      if (m_oCom.getResponse().hasError() == false)
      {
        if (sendFile())
        {
          CcSyncFileInfo oResponseFileInfo = m_oCom.getResponse().getFileInfo();
          if (m_oDirectory.fileNameInDirExists(m_oFileInfo.getDirId(), m_oFileInfo))
          {
            CcSyncFileInfo oFileToDelete = m_oDirectory.getFileInfoByFilename(m_oFileInfo.getDirId(), m_oFileInfo.getName());
            m_oDirectory.fileListRemove(oFileToDelete, false, true);
          }
          if (m_oDirectory.fileListInsert(oResponseFileInfo, true))
          {
            m_oDirectory.queueFinalizeFile(m_uiQueueIndex);
            CcSyncLog::writeDebug("File Successfully uploaded: " + m_oFileInfo.getName(), ESyncLogTarget::Client);
          }
          else
          {
            m_oDirectory.queueIncrementItem(m_uiQueueIndex);
            CcSyncLog::writeError("Inserting to filelist failed: " + m_oFileInfo.getSystemFullPath(), ESyncLogTarget::Client);
          }
          bRet = true;
        }
        else
        {
          m_oDirectory.queueIncrementItem(m_uiQueueIndex);
          CcSyncLog::writeError("Sending file failed: " + m_oFileInfo.getSystemFullPath(), ESyncLogTarget::Client);
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
              m_oDirectory.fileListInsert(oResponseFileInfo, true);
              m_oDirectory.queueFinalizeFile(m_uiQueueIndex);
              CcSyncLog::writeDebug("File Successfully uploaded: " + m_oFileInfo.getName(), ESyncLogTarget::Client);
              bRet = true;
            }
            // fall through
          default:
            m_oDirectory.queueIncrementItem(m_uiQueueIndex);
            CcSyncLog::writeError("Sending file failed: " + m_oFileInfo.getSystemFullPath(), ESyncLogTarget::Client);
            CcSyncLog::writeError("    ErrorMsg: " + m_oCom.getResponse().getErrorMsg(), ESyncLogTarget::Client);
        }
      }
    }
    else
    {
      CcSyncLog::writeError("Sending file failed: " + m_oFileInfo.getSystemFullPath(), ESyncLogTarget::Client);
      CcSyncLog::writeError("    ErrorMsg: " + m_oCom.getResponse().getErrorMsg(), ESyncLogTarget::Client);
      m_oDirectory.queueIncrementItem(m_uiQueueIndex);
    }
  }
  else
  {
    m_oDirectory.queueIncrementItem(m_uiQueueIndex);
    CcSyncLog::writeError("Queued Directory not found: " + m_oFileInfo.getDirPath(), ESyncLogTarget::Client);
    CcSyncLog::writeError("    ErrorMsg: " + m_oCom.getResponse().getErrorMsg(), ESyncLogTarget::Client);
  }
}


double CcSyncWorkerClientUpload::getProgress()
{
  double fProgress = 0.0;
  if (m_oFileInfo.getFileSize() != 0.0)
    fProgress = (static_cast<double>(m_uiReceived) / static_cast<double>(m_oFileInfo.getFileSize())) * 100.0;
  return fProgress;
}

CcString CcSyncWorkerClientUpload::getProgressMessage()
{
  CcString sMessage = "Upload File: ";
  CcString sRate = CcStringUtil::getHumanReadableSizePerSeconds(m_uiReceived, CcKernel::getUpTime() - m_oStartTime);
  sMessage  << m_oFileInfo.getRelativePath() << " (" 
            << CcString::fromNumber(getProgress(), 1, true) 
            << "%, with " << sRate << ")   ";
  return sMessage;
}

void CcSyncWorkerClientUpload::setFileInfo(const CcString& sPathToFile, uint32 uiUserId, uint32 uiGroupId, int64 iModified)
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

bool CcSyncWorkerClientUpload::sendFile()
{
  bool bRet = false;
  CcFile oFile(m_oFileInfo.getSystemFullPath());
  CcCrc32 oCrc;
  if (oFile.open(EOpenFlags::Read | EOpenFlags::ShareRead))
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
        else
        {
          m_uiReceived += uiTransfered;
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
    m_oFileInfo.crc() = oCrc.getValueUint32();
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

}
