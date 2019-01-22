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
 * @brief     Implemtation of class CcSyncServerDirectoryClientCommand
 */
#include "CcSyncGlobals.h"
#include "CcSyncServerDirectory.h"
#include "CcFile.h"
#include "CcDirectory.h"
#include "CcFileInfo.h"
#include "CcFileInfoList.h"
#include "CcSyncFileInfo.h"
#include "CcSyncFileInfoList.h"
#include "CcSyncDirectory.h"

CcSyncServerDirectory::CcSyncServerDirectory(const CcSyncServerDirectory& oToCopy)
{
  operator=(oToCopy);
}

CcSyncServerDirectory::CcSyncServerDirectory(CcSyncServerDirectory&& oToMove)
{
  operator=(std::move(oToMove));
}

CcSyncServerDirectory::~CcSyncServerDirectory(void)
{
}

CcSyncServerDirectory& CcSyncServerDirectory::operator=(const CcSyncServerDirectory& oToCopy)
{
  m_oUser = oToCopy.m_oUser;
  return *this;
}

CcSyncServerDirectory& CcSyncServerDirectory::operator=(CcSyncServerDirectory&& oToMove)
{
  if (this != &oToMove)
  {
    m_oUser = std::move(oToMove.m_oUser);
  }
  return *this;
}

void CcSyncServerDirectory::rescanDirs(bool bDeep)
{
  for (CcSyncDirectoryConfig& oDirectoryConfig : m_oUser.getAccountConfig()->directoryList())
  {
    CcSyncDirectory m_oDirectory;
    m_oDirectory.init(m_oUser.getDatabase(), &oDirectoryConfig);
    m_oUser.getDatabase()->directoryListUpdateChangedAll(m_oDirectory.getName());
    m_oDirectory.scan(bDeep);
    doQueue(m_oDirectory);
  }
}

void CcSyncServerDirectory::doQueue(CcSyncDirectory& oCurrentDir)
{
  m_oUser.getDatabase()->beginTransaction();
  oCurrentDir.queueResetAttempts();
  while (oCurrentDir.queueHasItems())
  {
    CcSyncFileInfo oFileInfo;
    uint64 uiQueueIndex;
    EBackupQueueType eQueueType = oCurrentDir.queueGetNext(oFileInfo, uiQueueIndex);
    switch (eQueueType)
    {
      case EBackupQueueType::AddDir:
        if (oCurrentDir.directoryListCreate(oFileInfo))
        {
          oCurrentDir.queueFinalizeDirectory(oFileInfo, uiQueueIndex);
        }
        else
        {
          oCurrentDir.queueIncrementItem(uiQueueIndex);
        }
        break;
      case EBackupQueueType::RemoveDir:
        if (oCurrentDir.directoryListRemove(oFileInfo))
        {
          oCurrentDir.queueFinalizeFile(uiQueueIndex);
        }
        else
        {
          oCurrentDir.queueIncrementItem(uiQueueIndex);
        }
        break;
      case EBackupQueueType::UpdateDir:
        if (oCurrentDir.directoryListUpdate(oFileInfo))
        {
          oCurrentDir.queueFinalizeDirectory(oFileInfo, uiQueueIndex);
        }
        else
        {
          oCurrentDir.queueIncrementItem(uiQueueIndex);
        }
        break;
      case EBackupQueueType::DownloadDir:
        CCDEBUG("DownloadDir should not happen on local sync");
        break;
      case EBackupQueueType::AddFile:
      {
        oCurrentDir.getFullDirPathById(oFileInfo);
        oFileInfo.fromSystemFile(true);
        if (oCurrentDir.fileListCreate(oFileInfo))
        {
          oCurrentDir.queueFinalizeDirectory(oFileInfo, uiQueueIndex);
        }
        else
        {
          oCurrentDir.queueIncrementItem(uiQueueIndex);
        }
        break;
      }
      case EBackupQueueType::RemoveFile:
        if (oCurrentDir.fileListRemove(oFileInfo, true, false))
        {
          oCurrentDir.queueFinalizeDirectory(oFileInfo, uiQueueIndex);
        }
        else
        {
          oCurrentDir.queueIncrementItem(uiQueueIndex);
        }
        break;
      case EBackupQueueType::UpdateFile:
      {
        oCurrentDir.getFullDirPathById(oFileInfo);
        oFileInfo.fromSystemFile(true);
        if (oCurrentDir.fileListUpdate(oFileInfo, false, true))
        {
          oCurrentDir.queueFinalizeDirectory(oFileInfo, uiQueueIndex);
        }
        else
        {
          oCurrentDir.queueIncrementItem(uiQueueIndex);
        }
        break;
      }
      case EBackupQueueType::DownloadFile:
        CCDEBUG("DownloadFile should not happen on local sync");
        break;
      default:
        oCurrentDir.queueIncrementItem(uiQueueIndex);
    }
  }
  m_oUser.getDatabase()->endTransaction();
}
