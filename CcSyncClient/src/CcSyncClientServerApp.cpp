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
 * @copyright Andreas Dirmeier (C) 2017
 * @author    Andreas Dirmeier
 * @par       Web: http://coolcow.de
 * @version   0.01
 * @date      2016-04
 * @par       Language   C++ ANSI V3
 * @file      CcSyncClientServerApp.h
 * @brief     Class CcSyncClientServerApp
 *
 *  Implementation of Main Application
 */

#include "CcSyncClientServerApp.h"
#include "CcKernel.h"
#include "CcAppKnown.h"
#include "CcSyncConsole.h"

namespace ServerStrings
{
  static const CcString Exit("exit");
  static const CcString ExitDesc("close this directory");
  static const CcString Help("help");
  static const CcString HelpDesc("this help text");
  static const CcString Rescan("rescan");
  static const CcString RescanDesc("[deep] Start rescan on server, if deep is specified force verifying files by crc");
  static const CcString New("new");
  static const CcString NewDesc("Create new Account on Server");
  static const CcString Del("del");
  static const CcString DelDesc("[Username] delete an account from server");
}

CcSyncClientServerApp::CcSyncClientServerApp(CcSyncClient* pSyncClient) :
  m_poSyncClient(pSyncClient)
{
}

CcSyncClientServerApp::~CcSyncClientServerApp() {
}

void CcSyncClientServerApp::help()
{
  size_t iSize = 10;
  CcSyncConsole::printHelpLine(ServerStrings::Help, iSize, ServerStrings::HelpDesc);
  CcSyncConsole::printHelpLine(ServerStrings::Exit, iSize, ServerStrings::ExitDesc);
  CcSyncConsole::printHelpLine(ServerStrings::Rescan, iSize, ServerStrings::RescanDesc);
  CcSyncConsole::printHelpLine(ServerStrings::New, iSize, ServerStrings::NewDesc);
  CcSyncConsole::printHelpLine(ServerStrings::Del, iSize, ServerStrings::DelDesc);
}

bool CcSyncClientServerApp::createAccount()
{
  bool bSuccess = false;
  if (m_poSyncClient != nullptr)
  {
    CcSyncConsole::writeLine("Setup new Configuration");
    CcString sAccount = CcSyncConsole::query("Account");
    CcString sPassword = CcSyncConsole::queryHidden("Password");
    bSuccess = m_poSyncClient->addRemoteAccount(sAccount, sPassword);
  }
  return bSuccess;
}

void CcSyncClientServerApp::run()
{
  CcString sSavePrepende = CcSyncConsole::getPrepend();
  CcSyncConsole::setPrepend("[Admin]");

  bool bCommandlineLoop = true;
  while (bCommandlineLoop &&
    m_poSyncClient->isLoggedIn())
  {
    CcString sCommandLine = CcSyncConsole::clientQuery();
    CcArguments oArguments(sCommandLine);
    if (oArguments.size() == 0)
    {
      continue;
    }
    else if (oArguments[0].compareInsensitve(ServerStrings::Exit))
    {
      bCommandlineLoop = false;
    }
    else if (oArguments[0].compareInsensitve(ServerStrings::New))
    {
      if (createAccount())
      {
        CcSyncConsole::writeLine("Account successfully created");
        CcSyncConsole::writeLine("");
      }
      else
      {
        CcSyncConsole::writeLine("Account failed to create");
        CcSyncConsole::writeLine("");
      }
    }
    else if (oArguments[0].compareInsensitve(ServerStrings::Del))
    {
      if (oArguments.size() > 1)
      {
        if (m_poSyncClient->removeAccount(oArguments[1]))
        {
          CcSyncConsole::writeLine("Account successfully removed");
        }
        else
        {
          CcSyncConsole::writeLine("Failed to delete Account.");
        }
      }
      else
      {
        CcSyncConsole::writeLine("no account given, please type \"help\"");
      }
    }
    else if (oArguments[0].compareInsensitve(ServerStrings::Rescan))
    {
      m_poSyncClient->serverRescan();
    }
    else if (oArguments[0].compareInsensitve(ServerStrings::Help))
    {
      help();
    }
    else
    {
      CcSyncConsole::writeLine("Unknown Command, run \"help\" to view all commands");
    }
  }
  CcSyncConsole::setPrepend(sSavePrepende);
}