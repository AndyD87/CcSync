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
 * @subpage   CcSyncClient
 *
 * @page      CcSyncClient
 * @copyright Andreas Dirmeier (C) 2017
 * @author    Andreas Dirmeier
 * @par       Web: http://coolcow.de
 * @version   0.01
 * @date      2016-04
 * @par       Language   C++ ANSI V3
 * @brief     Class CcSyncClientApp
 *
 *  Example GUI-Application with Menue Structure and Application Loader
 */
#ifndef _SyncClient_H_
#define _SyncClient_H_

#include "CcBase.h"
#include "CcApp.h"
#include "CcSyncClient.h"
#include "CcArguments.h"

enum class ESyncClientMode
{
  Unknown = 0,
  Cli = 1,
  Once,
  Daemon,
  Create,
  Dirs,
  Help
};

class CcSyncClientApp: public CcApp 
{
public:
  CcSyncClientApp(const CcArguments& oArguments);
  virtual ~CcSyncClientApp();

  virtual CcVersion getVersion() const override;

  void onStop() override;
  void run() override;
  void runDaemon();
  void runCli();
  void runOnce();
  void runCreate();
  void runDirs();
  void runHelp();
  bool createConfig(const CcString& sConfigDir);
  bool createAccount();
  bool editAccount(const CcString& sAccount);
private:
  ESyncClientMode m_eMode = ESyncClientMode::Cli;
  CcString        m_sConfigDir;
  CcArguments     m_oArguments;
  CcSyncClient*   m_poSyncClient = nullptr;
};

#endif /* _SyncClient_H_ */
