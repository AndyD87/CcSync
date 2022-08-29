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
 * @subpage   CcSyncWorkerClientUpload
 *
 * @page      CcSyncWorkerClientUpload
 * @copyright Andreas Dirmeier (C) 2017
 * @author    Andreas Dirmeier
 * @par       Web: http://coolcow.de
 * @version   0.01
 * @date      2016-04
 * @par       Language   C++ ANSI V3
 * @brief     Class CcSyncWorkerClientUpload
 **/
#ifndef _CcSyncWorkerClientUpload_H_
#define _CcSyncWorkerClientUpload_H_

#include "CcBase.h"
#include "CcSync.h"
#include "ISyncWorkerBase.h"
#include "CcSyncFileInfo.h"
#include "CcFile.h"
#include "CcDateTime.h"

// forward declarations
class CcString;

namespace CcSync
{

/**
 * @brief Class impelmentation
 */
class CcSyncSHARED CcSyncWorkerClientUpload : public ISyncWorkerBase
{
public:
  CcSyncWorkerClientUpload(CcSyncDirectory& oDirectory, CcSyncFileInfo& oFileInfo, uint64 uiQueueIndex, CcSyncClientCom& pSocket);
  virtual void run() override;
  virtual double getProgress() override;
  virtual CcString getProgressMessage() override;
  static void setFileInfo(const CcString& sPathToFile, uint32 uiUserId, uint32 uiGroupId, int64 iModified);

private:
  bool sendFile();

private: // Member
  uint64 m_uiReceived = 0;
  CcDateTime m_oStartTime;
};

}
#endif /* _CcSyncWorkerClientUpload_H_ */
