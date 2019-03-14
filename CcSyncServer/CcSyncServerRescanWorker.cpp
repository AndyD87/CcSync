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
 * @brief     Implemtation of class CcSyncServerRescanWorker
 */
#include "CcSyncServerRescanWorker.h"
#include "CcSyncServerDirectory.h"

CcSyncServerRescanWorker::CcSyncServerRescanWorker(CcSyncServer* oServer) :
  m_oServer(oServer)
{
}

CcSyncServerRescanWorker::~CcSyncServerRescanWorker(void)
{
}

bool CcSyncServerRescanWorker::start(bool bDeep)
{
  bool bRet = false;
  if (!isInProgress())
  {
    bRet = true;
    m_bDeep = bDeep;
    IThread::start();
  }
  return bRet;
}

void CcSyncServerRescanWorker::run()
{
  for (const CcSyncServerAccount& oAccount : m_oServer->config().getAccountList())
  {
    CcSyncUser oUser = m_oServer->getUserByName(oAccount.getName());
    CcSyncServerDirectory oServerDir(oUser);
    oServerDir.rescanDirs(m_bDeep);
  }
}
