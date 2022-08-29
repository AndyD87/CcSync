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
 * @subpage   CcSyncDirectory
 *
 * @page      CcSyncDirectory
 * @copyright Andreas Dirmeier (C) 2017
 * @author    Andreas Dirmeier
 * @par       Web: http://coolcow.de
 * @version   0.01
 * @date      2016-04
 * @par       Language   C++ ANSI V3
 * @brief     Class CcSyncDirectory
 **/
#ifndef _CcSyncDirectory_H_
#define _CcSyncDirectory_H_

#include "CcBase.h"
#include "CcSync.h"
#include "CcSyncDirectoryConfig.h"
#include "CcSyncDbClient.h"

// forward declarations
class CcDateTime;
class CcFileInfo;
class CcSqlite;
class CcSyncFileInfo;
class CcSyncFileInfoList;
class CcString;
enum class EBackupQueueType : uint16;

/**
 * @brief Class impelmentation
 */
class CcSyncSHARED CcSyncDirectory
{
public:

  CcSyncDirectory(void) :
    m_pDatabase(nullptr),
    m_pConfig(nullptr)
    {}

  /**
   * @brief Constructor
   */
  CcSyncDirectory( const CcSyncDirectory& oToCopy);

  /**
   * @brief Destructor
   */
  virtual ~CcSyncDirectory( void );

  CcSyncDirectory& operator=(const CcSyncDirectory& oToCopy);

  /**
   * @brief Compare two items
   * @param oToCompare: Item to compare to
   * @return true if they are the same, otherwis false
   */
  inline bool operator==(const CcSyncDirectory oToCompare) const
    { return ((&m_pDatabase == &oToCompare.m_pDatabase && &m_pConfig == &oToCompare.m_pConfig));}

  /**
   * @brief Compare two items
   * @param oToCompare: Item to compare to
   * @return true if they are not same, otherwis false
   */
  inline bool operator!=(const CcSyncDirectory oToCompare) const
    { return !operator==(oToCompare); }

  void init(CcSyncDbClientPointer& oDatabase, CcSyncDirectoryConfig* oConfig);

  void scan(bool bDeepSearch);
  bool validate();

  bool queueHasItems();
  EBackupQueueType queueGetNext(CcSyncFileInfo& oFileInfo, uint64& uiQueueIndex);
  void queueFinalizeDirectory(CcSyncFileInfo& oFileInfo, uint64 uiQueueIndex);
  void queueFinalizeFile(uint64 uiQueueIndex);
  void queueIncrementItem(uint64 uiQueueIndex);
  void queueReset();
  void queueResetAttempts();
  void queueDownloadDirectory(const CcSyncFileInfo& oFileInfo);
  void queueDownloadFile(const CcSyncFileInfo& oFileInfo);

  const CcString& getName() const;
  bool getInnerPathById(CcSyncFileInfo& oFileInfo);
  bool getFullDirPathById(CcSyncFileInfo& oFileInfo);

  bool fileListInsert(CcSyncFileInfo& oFileInfo, bool bDoUpdateParents);
  bool fileListRemove(CcSyncFileInfo& oFileInfo, bool bDoUpdateParents, bool bKeepFile);
  bool fileListCreate(CcSyncFileInfo& oFileInfo, bool bDoUpdateParents);
  bool fileListExists(uint64 uiFileId);

  bool directoryListCreate(CcSyncFileInfo& oFileInfo, bool bDoUpdateParents);
  bool directoryListUpdate(const CcSyncFileInfo& oFileInfo);
  bool directoryListUpdateId(uint64 uiDirectoryId, const CcSyncFileInfo& oFileInfo);
  bool directoryListRemove(CcSyncFileInfo& oFileInfo, bool bDoUpdateParents);
  bool directoryListExists(uint64 uiDirectoryId);
  bool directoryListSubDirExists(uint64 uiParentDirId, const CcString& sName);
  bool directoryListEmpty(uint64 uiDirectoryId);
  bool directoryListInsert(CcSyncFileInfo& oFileInfo, bool bDoUpdateParents);
  void directoryListUpdateChanged(uint64 uiDirId);

  bool fileIdInDirExists(uint64 uiDirectoryId, const CcSyncFileInfo& oFileInfo);
  bool fileNameInDirExists(uint64 uiDirectoryId, const CcSyncFileInfo& oFileInfo);
  const CcString& getLocation() const
    { return m_pConfig->getLocation(); }
  CcSyncFileInfoList getDirectoryInfoListById(uint64 uiDirId);
  CcSyncFileInfoList getFileInfoListById(uint64 uiDirId);
  CcSyncFileInfo getDirectoryInfoById(uint64 uiDirId);
  CcSyncFileInfo getDirectoryInfoFromSubdir(uint64 uiDirId, const CcString& sSubDirName);
  CcSyncFileInfo getFileInfoById(uint64 uiFileId);
  CcSyncFileInfo getFileInfoByFilename(uint64 uiDirId, const CcString& sFileName);
  CcString getTemporaryDir();
  CcString getHistoryDir();
  uint32 getUserId();
  uint32 getGroupId();
  bool isLocked();

private: // methods
  void scanSubDir(uint64 uiDbIndex, const CcString& sPath, bool bDeepSearch);
  void queueCreateDir(uint64 uiDependent, uint64 uiQueueId, const CcString& sParentPath, const CcFileInfo& oFileInfo);
  void queueUpdateDir(const CcSyncFileInfo& oFileInfo);
  uint64 queueRemoveDir(uint64 uiDependent, const CcSyncFileInfo& oFileInfo);
  uint64 queueRemoveFile(uint64 uiDependent, const CcSyncFileInfo& oFileInfo);
  void queueUploadFile(uint64 uiDependent, uint64 uiDirectoryId, const CcFileInfo& oDirectoryInfo);
  bool historyInsert(EBackupQueueType eQueueType, const CcSyncFileInfo& oFileInfo);

private:
  CcSyncDbClientPointer   m_pDatabase;
  CcSyncDirectoryConfig*  m_pConfig   = nullptr;
  uint64 m_uiRootId = 1;
};

#endif /* _CcSyncDirectory_H_ */
