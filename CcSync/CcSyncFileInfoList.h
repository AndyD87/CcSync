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
 * @subpage   CcSyncFileInfoList
 *
 * @page      CcSyncFileInfoList
 * @copyright Andreas Dirmeier (C) 2017
 * @author    Andreas Dirmeier
 * @par       Web: http://coolcow.de
 * @version   0.01
 * @date      2016-04
 * @par       Language   C++ ANSI V3
 * @brief     Class CcSyncFileInfoList
 **/
#ifndef _CcSyncFileInfoList_H_
#define _CcSyncFileInfoList_H_

#include "CcBase.h"
#include "CcSync.h"
#include "CcSyncFileInfo.h"
#include "CcList.h"

/**
 * @brief Class impelmentation
 */
class CcSyncSHARED CcSyncFileInfoList : public CcList<CcSyncFileInfo>
{
public:
  /**
   * @brief Constructor
   */
  CcSyncFileInfoList()
  {}

  /**
   * @brief Destructor
   */
  ~CcSyncFileInfoList( void )
  {}

  bool containsFile(const CcString& sFilename) const;
  bool containsFile(uint64 uiFileId) const;
  inline bool containsDirectory(const CcString& sDirectoryName) const
    { return containsFile(sDirectoryName); }
  inline bool containsDirectory(uint64 uiDirId) const
    { return containsFile(uiDirId); }
  CcSyncFileInfo& getFile(const CcString& sFilename);
  CcSyncFileInfo& getFile(uint64 uiFileId);
  const CcSyncFileInfo& getFile(const CcString& sFilename) const;
  const CcSyncFileInfo& getFile(uint64 uiFileId) const;
  bool removeFile(const CcString& sFilename);
  bool removeFile(uint64 uiFileId);

  inline const CcSyncFileInfo& getDirectory(const CcString& sDirectoryName) const
    { return getFile(sDirectoryName); }

};

#endif /* _CcSyncFileInfoList_H_ */
