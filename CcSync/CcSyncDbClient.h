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
 * @page      CcSync
 * @subpage   CcSyncDbClient
 *
 * @page      CcSyncDbClient
 * @copyright Andreas Dirmeier (C) 2016
 * @author    Andreas Dirmeier
 * @par       Web: http://coolcow.de
 * @version   0.01
 * @date      2016-04
 * @par       Language   C++ ANSI V3
 * @brief     Class CcSyncDbClient
 **/
#ifndef _CcSyncDbClient_H_
#define _CcSyncDbClient_H_

#include "CcBase.h"
#include "CcSync.h"
#include "CcSharedPointer.h"
#include "CcSqlite.h"
#include "CcSharedPointer.h"

class CcString;
class CcSyncFileInfo;
class CcSyncFileInfoList;
class CcSyncDbClient;

#if WIN32
template class CcSyncSHARED CcSharedPointer<CcSqlite>;
template class CcSharedPointer<CcSyncDbClient>;
#endif

typedef class CcSyncSHARED CcSharedPointer<CcSyncDbClient> CcSyncDbClientPointer;

enum class EBackupQueueType : uint16
{
  Unknown = 0,
  AddDir,
  RemoveDir,
  UpdateDir,
  DownloadDir,
  AddFile,
  RemoveFile,
  UpdateFile,
  DownloadFile,
};

/**
 * @brief Class impelmentation
 */
class CcSyncSHARED CcSyncDbClient 
{
public:
  /**
   * @brief Constructor
   */
  CcSyncDbClient( void )
    { }

  /**
   * @brief Constructor
   */
  CcSyncDbClient( const CcString& sPath );

  /**
   * @brief CopyConstructor
   */
  CcSyncDbClient( const CcSyncDbClient& oToCopy );

  /**
   * @brief MoveConstructor
   */
  CcSyncDbClient( CcSyncDbClient&& oToMove );

  /**
   * @brief Destructor
   */
  virtual ~CcSyncDbClient( void );

  CcSyncDbClient& operator=(const CcSyncDbClient& oToCopy);
  CcSyncDbClient& operator=(CcSyncDbClient&& oToMove);
  bool operator==(const CcSyncDbClient& oToCompare) const;
  bool operator!=(const CcSyncDbClient& oToCompare) const;

  bool openDatabase(const CcString& sPath);
  bool setupDirectory(const CcString& sDirName);
  bool removeDirectory(const CcString& sDirName);

  void cleanDirectory(const CcString& sDirName);

  void beginTransaction();
  void endTransaction();

  CcString getInnerPathById(const CcString& sDirName, uint64 uiDirId);
  CcSyncFileInfoList getDirectoryInfoListById(const CcString& sDirName, uint64 uiDirId);
  CcSyncFileInfoList getFileInfoListById(const CcString& sDirName, uint64 uiDirId);
  CcSyncFileInfo getDirectoryInfoById(const CcString& sDirName, uint64 uiDirId);
  CcSyncFileInfo getDirectoryInfoFromSubdir(const CcString& sDirName, uint64 uiDirId, const CcString& sSubDirName);
  CcSyncFileInfo getFileInfoById(const CcString& sDirName, uint64 uiFileId);
  CcSyncFileInfo getFileInfoByFilename(const CcString& sDirName, uint64 uiDirId, const CcString& sFileName);

  bool queueHasItem(const CcString& sDirName);
  EBackupQueueType queueGetNext(const CcString& sDirName, CcSyncFileInfo& oFileInfo, uint64& uiQueueIndex);
  void queueFinalizeDirectory(const CcString& sDirName, CcSyncFileInfo& oFileInfo, uint64 uiQueueIndex);
  void queueFinalizeFile(const CcString& sDirName, uint64 uiQueueIndex);
  void queueIncrementItem(const CcString& sDirName, uint64 uiQueueIndex);
  void queueReset(const CcString& sDirName);
  void queueResetAttempts(const CcString& sDirName);
  void queueDownloadDirectory(const CcString& sDirName, const CcSyncFileInfo& oFileInfo);
  void queueDownloadFile(const CcString& sDirName, const CcSyncFileInfo& oFileInfo);
  void queueUpdateFile(const CcString& sDirName, const CcSyncFileInfo& oFileInfo);
  uint64 queueInsert(const CcString& sDirName, uint64 uiParentId, EBackupQueueType eQueueType, uint64 uiFileId, uint64 uiDirectoryId, const CcString& sName);

  bool directoryListRemove(const CcString& sDirName, const CcSyncFileInfo& oFileInfo, bool bDoUpdateParents = true);
  bool directoryListUpdate(const CcString& sDirName, const CcSyncFileInfo& oFileInfo);
  bool directoryListUpdateId(const CcString& sDirName, uint64 uiDirectoryId, const CcSyncFileInfo& oFileInfo);
  bool directoryListExists(const CcString& sDirName, uint64 uiDirectoryId);
  bool directoryListSubDirExists(const CcString& sDirName, uint64 uiParentDirId, const CcString& sName);
  bool directoryListEmpty(const CcString& sDirName, uint64 uiDirectoryId);
  bool directoryListInsert(const CcString& sDirName, CcSyncFileInfo& oFileInfo);
  void directoryListUpdateChanged(const CcString& sDirName, uint64 uiDirId);
  void directoryListUpdateChangedAll(const CcString& sDirName, uint64 uiDirId = 1);
  void directoryListSearchDouble(const CcString& sDirName, uint64 uiDirId = 1);
  void directoryListSearchTemporary(const CcString& sDirName);

  bool fileListInsert(const CcString& sDirName, CcSyncFileInfo& oFileInfo);
  bool fileListUpdate(const CcString& sDirName, const CcSyncFileInfo& oFileInfo);
  bool fileListRemove(const CcString& sDirName, const CcSyncFileInfo& oFileInfo, bool bDoUpdateParents = true);
  bool fileListExists(const CcString& sDirName, uint64 uiFileId);
  bool fileListFileIdExists(const CcString& sDirName, uint64 uiDirId, const CcSyncFileInfo& oFileInfo);
  bool fileListFileNameExists(const CcString& sDirName, uint64 uiDirId, const CcSyncFileInfo& oFileInfo);
  void fileListSearchTemporary(const CcString& sDirName);

  bool historyInsert(const CcString& sDirName, EBackupQueueType eQueueType, const CcSyncFileInfo& oFileInfo);
  inline void historyEnable()
    { m_bEnableHistory = true;}
  inline void diableDisable()
    { m_bEnableHistory = false;}
  inline bool historyEnabled()
    { return m_bEnableHistory; }

private: // Methods
  CcString getDbCreateDirectoryList(const CcString& sDirName);
  CcString getDbCreateFileList(const CcString& sDirName);
  CcString getDbCreateQueue(const CcString& sDirName);
  CcString getDbCreateHistory(const CcString& sDirName);
  CcString getDbInsertDirectoryList(const CcString& sDirName, const CcSyncFileInfo& oInfo);
  CcString getDbInsertFileList(const CcString& sDirName, const CcSyncFileInfo& oInfo);
  CcString getDbInsertQueue(const CcString& sDirName, uint64 uiParentId, EBackupQueueType eQueueType, uint64 uiFileId, uint64 uiDirectoryId, const CcString& sName);
  CcString getDbInsertHistory(const CcString& sDirName, EBackupQueueType eQueueType, const CcSyncFileInfo& oFileInfo);
  CcString getDbUpdateFileList(const CcString& sDirName, const CcSyncFileInfo& oInfo);
private:
  CcSharedPointer<CcSqlite> m_pDatabase;
  size_t m_uiTransactionCnt = 0;
  bool m_bEnableHistory = true;
};

#endif /* _CcSyncDbClient_H_ */
