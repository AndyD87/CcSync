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
 * @page      CcApps
 * @subpage   CcSyncClientServer
 *
 * @page      CcSyncClientServer
 * @copyright Andreas Dirmeier (C) 2017
 * @author    Andreas Dirmeier
 * @par       Web: http://coolcow.de
 * @version   0.01
 * @date      2016-04
 * @par       Language   C++ ANSI V3
 * @brief     Class CcSyncClientServerApp
 *
 *  Example GUI-Application with Menue Structure and Application Loader
 */
#ifndef _SyncClientServer_H_
#define _SyncClientServer_H_

#include "CcBase.h"
#include "CcApp.h"
#include "CcSyncClient.h"
#include "CcArguments.h"

class CcSyncClientServerApp : public CcApp
{
public:
  CcSyncClientServerApp(CcSyncClient* m_poSyncClient);
  ~CcSyncClientServerApp();

  void help();
  bool createAccount();

  virtual void run() override;

private:
  CcSyncClient* m_poSyncClient = NULL;
};

#endif /* _SyncClientServer_H_ */
