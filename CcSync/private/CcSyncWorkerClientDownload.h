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
 * @subpage   CcSyncWorkerClientDownload
 *
 * @page      CcSyncWorkerClientDownload
 * @copyright Andreas Dirmeier (C) 2017
 * @author    Andreas Dirmeier
 * @par       Web: http://coolcow.de
 * @version   0.01
 * @date      2016-04
 * @par       Language   C++ ANSI V3
 * @brief     Class CcSyncWorkerClientDownload
 **/
#ifndef _CcSyncWorkerClientDownload_H_
#define _CcSyncWorkerClientDownload_H_

#include "CcBase.h"
#include "CcSync.h"
#include "ISyncWorkerBase.h"
#include "CcSyncFileInfo.h"
#include "CcFile.h"

// forward declarations
class CcString;

namespace CcSync
{

/**
 * @brief Class impelmentation
 */
class CcSyncSHARED CcSyncWorkerClientDownload : public ISyncWorkerBase
{
public:
  CcSyncWorkerClientDownload(CcSyncDirectory& oDirectory, CcSyncFileInfo& oFileInfo, uint64 uiQueueIndex, CcSyncClientCom& pSocket);
  virtual void run() override;
  virtual double getProgress() override;
  virtual CcString getProgressMessage() override;
  static void setFileInfo(const CcString& sPathToFile, uint32 uiUserId, uint32 uiGroupId, int64 iModified);

private:
  bool receiveFile(CcFile* pFile);

private: // Member
  uint64 m_uiReceived = 0;
};

}
#endif /* _CcSyncWorkerClientDownload_H_ */
