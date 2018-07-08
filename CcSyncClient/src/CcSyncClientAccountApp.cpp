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
 * @file      CcSyncClientAccountApp.h
 * @brief     Class CcSyncClientAccountApp
 *
 *  Implementation of Main Application
 */

#include "CcSyncClientAccountApp.h"
#include "CcKernel.h"
#include "CcAppKnown.h"
#include "CcSyncConsole.h"
#include "CcSyncClientServerApp.h"
#include "CcSyncClientDirectoryApp.h"

namespace AccountStrings
{
  static const CcString Exit("exit");
  static const CcString ExitDesc("logout from account, return to client");
  static const CcString Logout("logout");
  static const CcString LogoutDesc("logout from account, return to client");
  static const CcString List("list");
  static const CcString ListDesc("list all available Directories");
  static const CcString Select("select");
  static const CcString SelectDesc("[dir] select available directory to change settings");
  static const CcString Reset("reset");
  static const CcString ResetDesc("reset current working queue");
  static const CcString Rebuild("rebuild");
  static const CcString RebuildDesc("Clean database and rebuild with server.");
  static const CcString Admin("admin");
  static const CcString AdminDesc("if you are admin on server, open admin console");
  static const CcString Sync("sync");
  static const CcString SyncDesc("synchronise all directories with server");
  static const CcString Verify("verify");
  static const CcString VerifyDesc("Verify database integrity for every directory");
  static const CcString Create("create");
  static const CcString CreateDesc("create new directory on server and local");
  static const CcString Del("del");
  static const CcString DelDesc("[dir] delete an directory from server and local");
  static const CcString AllDirs("alldirs");
  static const CcString AllDirsDesc("Create all sync directories on one location");
  static const CcString Update("update");
  static const CcString UpdateDesc("update databse changed values");
  static const CcString Help("help");
  static const CcString HelpDesc("This help Message");
}

CcSyncClientAccountApp::CcSyncClientAccountApp(CcSyncClient* pSyncClient) :
  m_poSyncClient(pSyncClient)
{
}

CcSyncClientAccountApp::~CcSyncClientAccountApp() 
{
}

void CcSyncClientAccountApp::run()
{
  CcString sSavePrepende = CcSyncConsole::getPrepend();
  CcSyncConsole::setPrepend("[/" + m_poSyncClient->getAccountName() + "]");
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
    else if (oArguments[0].compareInsensitve(AccountStrings::Logout) ||
      oArguments[0].compareInsensitve(AccountStrings::Exit))
    {
      m_poSyncClient->logout();
      CcSyncConsole::writeLine("Logout done, connection closed");
      bCommandlineLoop = false;
    }
    else if (oArguments[0].compareInsensitve(AccountStrings::List))
    {
      CcSyncConsole::writeLine(m_poSyncClient->getAccountInfo());
    }
    else if (oArguments[0].compareInsensitve(AccountStrings::Select))
    {
      if (oArguments.size() > 1)
      {
        CcSyncClientDirectoryApp oDirectory(m_poSyncClient, oArguments[1]);
        oDirectory.exec();
      }
      else
      {
        CcSyncConsole::writeLine("not enough arguments");
      }
    }
    else if (oArguments[0].compareInsensitve(AccountStrings::Rebuild))
    {
      m_poSyncClient->cleanDatabase();
      m_poSyncClient->updateFromRemoteAccount();
      CcSyncConsole::writeLine("Reset Queue");
      m_poSyncClient->resetQueues();
      CcSyncConsole::writeLine("Remote sync: scan");
      m_poSyncClient->doRemoteSyncAll();
      CcSyncConsole::writeLine("Remote sync: do");
      m_poSyncClient->doQueues();
      CcSyncConsole::writeLine("Remote sync: done");
      CcSyncConsole::writeLine("Local sync: scan");
      m_poSyncClient->doLocalSyncAll();
      CcSyncConsole::writeLine("Local sync: do");
      m_poSyncClient->doQueues();
      CcSyncConsole::writeLine("Local sync: done");

    }
    else if (oArguments[0].compareInsensitve(AccountStrings::Reset))
    {
      m_poSyncClient->resetQueues();
    }
    else if (oArguments[0].compareInsensitve(AccountStrings::Admin))
    {
      if (m_poSyncClient->isAdmin())
      {
        CcSyncClientServerApp oServerCtrl(m_poSyncClient);
        oServerCtrl.exec();
      }
      else
      {
        CcSyncConsole::writeLine("You are not admin on server.");
      }
    }
    else if (oArguments[0].compareInsensitve(AccountStrings::Sync))
    {
      m_poSyncClient->updateFromRemoteAccount();
      CcSyncConsole::writeLine("Reset Queue");
      m_poSyncClient->resetQueues();
      CcSyncConsole::writeLine("Remote sync: scan");
      m_poSyncClient->doRemoteSyncAll();
      CcSyncConsole::writeLine("Remote sync: do");
      m_poSyncClient->doQueues();
      CcSyncConsole::writeLine("Remote sync: done");
      CcSyncConsole::writeLine("Local sync: scan");
      m_poSyncClient->doLocalSyncAll();
      CcSyncConsole::writeLine("Local sync: do");
      m_poSyncClient->doQueues();
      CcSyncConsole::writeLine("Local sync: done");
    }
    else if (oArguments[0].compareInsensitve(AccountStrings::Verify))
    {
      verify();
    }
    else if (oArguments[0].compareInsensitve(AccountStrings::Create))
    {
      createDirectory();
    }
    else if (oArguments[0].compareInsensitve(AccountStrings::Del))
    {
      if (oArguments.size() > 1)
      {
        m_poSyncClient->doAccountRemoveDirectory(oArguments[1]);
      }
      else
      {
        CcSyncConsole::writeLine("not enough arguments");
      }
    }
    else if (oArguments[0].compareInsensitve(AccountStrings::AllDirs))
    {
      createAllDirectories();
    }
    else if (oArguments[0].compareInsensitve(AccountStrings::Update))
    {
      m_poSyncClient->doUpdateChanged();
    }
    else if (oArguments[0].compareInsensitve(AccountStrings::Help))
    {
      CcSyncConsole::printHelpLine(AccountStrings::Admin, 20, AccountStrings::AdminDesc);
      CcSyncConsole::printHelpLine(AccountStrings::Create, 20, AccountStrings::CreateDesc);
      CcSyncConsole::printHelpLine(AccountStrings::AllDirs, 20, AccountStrings::AllDirsDesc);
      CcSyncConsole::printHelpLine(AccountStrings::Update, 20, AccountStrings::UpdateDesc);
      CcSyncConsole::printHelpLine(AccountStrings::Exit, 20, AccountStrings::ExitDesc);
      CcSyncConsole::printHelpLine(AccountStrings::Help, 20, AccountStrings::HelpDesc);
      CcSyncConsole::printHelpLine(AccountStrings::List, 20, AccountStrings::ListDesc);
      CcSyncConsole::printHelpLine(AccountStrings::Logout, 20, AccountStrings::LogoutDesc);
      CcSyncConsole::printHelpLine(AccountStrings::Reset, 20, AccountStrings::ResetDesc);
      CcSyncConsole::printHelpLine(AccountStrings::Rebuild, 20, AccountStrings::RebuildDesc);
      CcSyncConsole::printHelpLine(AccountStrings::Select, 20, AccountStrings::SelectDesc);
      CcSyncConsole::printHelpLine(AccountStrings::Sync, 20, AccountStrings::SyncDesc);
      CcSyncConsole::printHelpLine(AccountStrings::Verify, 20, AccountStrings::VerifyDesc);
    }
    else
    {
      CcSyncConsole::writeLine("Unknown Command, run \"help\" to view all commands");
    }
  }
  CcSyncConsole::setPrepend(sSavePrepende);
}

bool CcSyncClientAccountApp::createDirectory()
{
  bool bRet = false;
  CcSyncConsole::writeLine("Create new Directory");
  CcString sName = CcSyncConsole::query("Name");
  CcString sPath;
  sPath.setOsPath(CcSyncConsole::query("Path"));
  if (sName.length() > 0 &&
      sPath.length() > 0)
  {
    return m_poSyncClient->doAccountCreateDirectory(sName, sPath);
  }
  return bRet;
}

bool CcSyncClientAccountApp::createAllDirectories()
{
  bool bRet = false;
  CcString sPath;
  sPath.setOsPath(CcSyncConsole::query("Path"));
  if (sPath.length() > 0)
  {
    CcStringList oDirList = m_poSyncClient->getDirectoryList();
    for (CcString& sDirName : oDirList)
    {
      CcString sDirPath = sPath;
      sDirPath.appendPath(sDirName);
      CcSyncConsole::writeLine("Create new Directory: " + sDirName);
      if (!m_poSyncClient->doAccountCreateDirectory(sDirName, sDirPath))
      {
        CcSyncConsole::writeLine("Failed to create Directory: " + sDirName);
        bRet = false;
        break;
      }
    }
  }
  return bRet;
}

bool CcSyncClientAccountApp::verify()
{
  bool bSuccess = true;
  CcStringList oDirList = m_poSyncClient->getDirectoryList();
  for (CcString& sDirName : oDirList)
  {
    CcSyncConsole::writeLine("Verify Database for Directory: " + sDirName);
    if (!m_poSyncClient->verify(sDirName))
    {
      CcSyncConsole::writeLine("Verification failed for Directory: " + sDirName);
      bSuccess = false;
      break;
    }
  }
  return bSuccess;
}
