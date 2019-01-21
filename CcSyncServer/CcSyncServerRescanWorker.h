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
 * @subpage   CcSyncServerRescanWorker
 *
 * @page      CcSyncServerRescanWorker
 * @copyright Andreas Dirmeier (C) 2017
 * @author    Andreas Dirmeier
 * @par       Web: http://coolcow.de
 * @version   0.01
 * @date      2016-04
 * @par       Language   C++ ANSI V3
 * @brief     Class CcSyncServerRescanWorker
 **/
#ifndef _CcSyncServerRescanWorker_H_
#define _CcSyncServerRescanWorker_H_

#include "CcBase.h"
#include "CcSync.h"
#include "CcThreadObject.h"
#include "CcSyncServer.h"
/**
 * @brief Class impelmentation
 */
class CcSyncServerRescanWorker : public CcThreadObject
{
public:
  /**
   * @brief Constructor
   */
  CcSyncServerRescanWorker(CcSyncServer* m_oServer);

  /**
   * @brief Destructor
   */
  virtual ~CcSyncServerRescanWorker( void );

  bool start(bool bDeep);

private:
  void run() override;

private:
  CcSyncServer*   m_oServer   = nullptr;
  bool m_bDeep = false;
};

#endif /* _CcSyncServerRescanWorker_H_ */
