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
 * @subpage   ISyncWorkerBase
 *
 * @page      ISyncWorkerBase
 * @copyright Andreas Dirmeier (C) 2017
 * @author    Andreas Dirmeier
 * @par       Web: http://coolcow.de
 * @version   0.01
 * @date      2016-04
 * @par       Language   C++ ANSI V3
 * @brief     Class ISyncWorkerBase
 **/
#ifndef _ISyncWorkerBase_H_
#define _ISyncWorkerBase_H_

#include "CcBase.h"
#include "CcSync.h"
#include "IThread.h"

#include "CcSyncClientCom.h"
#include "CcSyncDirectory.h"
#include "CcSyncFileInfo.h"

// forward declarations
class CcString;

namespace CcSync
{

/**
 * @brief Class impelmentation
 */
class CcSyncSHARED ISyncWorkerBase : public IThread
{
public:
  ISyncWorkerBase(CcSyncDirectory& oDirectory, CcSyncFileInfo& oFileInfo, uint64 uiQueueIndex, CcSyncClientCom& pSocket);
  virtual double getProgress() = 0;
  virtual CcString getProgressMessage() = 0;

protected:
  CcSyncClientCom&  m_oCom;
  CcSyncDirectory&  m_oDirectory;
  CcSyncFileInfo&   m_oFileInfo;
  uint64            m_uiQueueIndex;
};

}
#endif /* _ISyncWorkerBase_H_ */
