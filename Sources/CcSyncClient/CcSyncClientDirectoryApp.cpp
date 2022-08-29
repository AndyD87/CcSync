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
 * @copyright Andreas Dirmeier (C) 2017
 * @author    Andreas Dirmeier
 * @par       Web: http://coolcow.de
 * @version   0.01
 * @date      2016-04
 * @par       Language   C++ ANSI V3
 * @file      CcSyncClientDirectoryApp.h
 * @brief     Class CcSyncClientDirectoryApp
 *
 *  Implementation of Main Application
 */

#include "CcSyncClientDirectoryApp.h"
#include "CcKernel.h"
#include "CcAppKnown.h"
#include "CcSyncConsole.h"

namespace DirectoryStrings
{
  static const CcString Exit("exit");
  static const CcString ExitDesc("close this directory");
  static const CcString Help("help");
  static const CcString HelpDesc("this help text");
  static const CcString Info("info");
  static const CcString InfoDesc("settings of this directory");
  static const CcString Lock("lock");
  static const CcString LockDesc("lock directory from synchronisation");
  static const CcString Unlock("unlock");
  static const CcString UnlockDesc("unlock directory for synchronisation");
  static const CcString Sync("sync");
  static const CcString SyncDesc("synchronise this directory with server");
  static const CcString Verify("verify");
  static const CcString VerifyDesc("Verify database integrity");
  static const CcString Set("set");
  static const CcString SetUser("user");
  static const CcString SetGroup("group");
  static const CcString SetDesc("(User|Group) (ID/Name)");
  static const CcString BackupCommand("BackupCommand");
  static const CcString BackupCommandDesc("update the command to execute on client before sync");
  static const CcString RestoreCommand("RestoreCommand");
  static const CcString RestoreCommandDesc("update the command wich can be used to start restoring files");
}

CcSyncClientDirectoryApp::CcSyncClientDirectoryApp(CcSyncClient* pSyncClient, const CcString& sDirectory) :
  m_pSyncClient(pSyncClient),
  m_sDirectory(sDirectory)
{
}

CcSyncClientDirectoryApp::~CcSyncClientDirectoryApp() 
{
}

void CcSyncClientDirectoryApp::run()
{
  CcString sSavePrepende = CcSyncConsole::getPrepend();
  CcString sPrependName = "[/" + m_pSyncClient->getAccountName() + "/" + m_sDirectory + "]";
  CcSyncConsole::setPrepend(sPrependName);

  bool bCommandlineLoop = true;
  while (bCommandlineLoop &&
    m_pSyncClient->isLoggedIn())
  {
    CcString sCommandLine;
    if (CcSyncConsole::clientQuery(sCommandLine) != SIZE_MAX)
    {
      CcArguments oArguments;
      oArguments.parse(sCommandLine);
      if (oArguments.size() == 0)
      {
        continue;
      }
      else if (oArguments[0].compareInsensitve(DirectoryStrings::Info))
      {
        CcString sDirInfo = m_pSyncClient->getDirectoryInfo(m_sDirectory);
        CcSyncConsole::writeLine(sDirInfo);
      }
      else if (oArguments[0].compareInsensitve(DirectoryStrings::BackupCommand))
      {
        setBackupCommand();
      }
      else if (oArguments[0].compareInsensitve(DirectoryStrings::RestoreCommand))
      {
        setRestoreCommand();
      }
      else if (oArguments[0].compareInsensitve(DirectoryStrings::Sync))
      {
        CcSyncConsole::writeLine("Reset Queue");
        m_pSyncClient->resetQueue(m_sDirectory);
        CcSyncConsole::writeLine("Remote sync: scan");
        m_pSyncClient->doRemoteSync(m_sDirectory);
        CcSyncConsole::writeLine("Remote sync: do");
        m_pSyncClient->doQueue(m_sDirectory);
        CcSyncConsole::writeLine("Remote sync: done");
        CcSyncConsole::writeLine("Local sync: scan");
        m_pSyncClient->doLocalSync(m_sDirectory);
        CcSyncConsole::writeLine("Local sync: do");
        m_pSyncClient->doQueue(m_sDirectory);
        CcSyncConsole::writeLine("Local sync: done");
      }
      else if (oArguments[0].compareInsensitve(DirectoryStrings::Verify))
      {
        verify();
      }
      else if (oArguments[0].compareInsensitve(DirectoryStrings::Set))
      {
        if (oArguments.size() != 3)
        {
          CcSyncConsole::writeLine("Wrong number of Arguments:");
          CcSyncConsole::writeLine(DirectoryStrings::SetDesc);
        }
        else
        {
          if (oArguments[1].compareInsensitve(DirectoryStrings::SetUser))
          {
            m_pSyncClient->updateDirectorySetUser(m_sDirectory, oArguments[2]);
          }
          else if (oArguments[1].compareInsensitve(DirectoryStrings::SetGroup))
          {
            m_pSyncClient->updateDirectorySetGroup(m_sDirectory, oArguments[2]);
          }
        }
      }
      else if (oArguments[0].compareInsensitve(DirectoryStrings::Lock))
      {
        if (m_pSyncClient->setDirectoryLock(m_sDirectory))
          CcSyncConsole::writeLine("Directory locked");
        else
          CcSyncConsole::writeLine("Failed to lock Directory");
      }
      else if (oArguments[0].compareInsensitve(DirectoryStrings::Unlock))
      {
        if (m_pSyncClient->setDirectoryUnlock(m_sDirectory))
          CcSyncConsole::writeLine("Directory unlocked");
        else
          CcSyncConsole::writeLine("Failed to unlock Directory");
      }
      else if (oArguments[0].compareInsensitve(DirectoryStrings::Exit))
      {
        bCommandlineLoop = false;
      }
      else if (oArguments[0].compareInsensitve(DirectoryStrings::Help))
      {
        help();
      }
      else
      {
        CcSyncConsole::writeLine("Unknown Command, run \"help\" to view all commands");
      }
    }
    else
    {
      bCommandlineLoop = false;
    }
  }

  CcSyncConsole::setPrepend(sSavePrepende);
}

void CcSyncClientDirectoryApp::help()
{
  size_t uiLength = 15;
  CcSyncConsole::printHelpLine(DirectoryStrings::Exit, uiLength, DirectoryStrings::ExitDesc);
  CcSyncConsole::printHelpLine(DirectoryStrings::Help, uiLength, DirectoryStrings::HelpDesc);
  CcSyncConsole::printHelpLine(DirectoryStrings::Info, uiLength, DirectoryStrings::InfoDesc);
  CcSyncConsole::printHelpLine(DirectoryStrings::Sync, uiLength, DirectoryStrings::SyncDesc);
  CcSyncConsole::printHelpLine(DirectoryStrings::Verify, uiLength, DirectoryStrings::VerifyDesc);
  CcSyncConsole::printHelpLine(DirectoryStrings::BackupCommand, uiLength, DirectoryStrings::BackupCommandDesc);
  CcSyncConsole::printHelpLine(DirectoryStrings::RestoreCommand, uiLength, DirectoryStrings::RestoreCommandDesc);
  CcSyncConsole::printHelpLine(DirectoryStrings::Set, uiLength, DirectoryStrings::SetDesc);
}

bool CcSyncClientDirectoryApp::setBackupCommand()
{
  CcSyncConsole::writeLine("Create new Directory");
  CcString sCommand;
  CcSyncConsole::query("Command", sCommand);
  if (sCommand.length())
  {
    m_pSyncClient->updateDirectoryBackupCommand(m_sDirectory, sCommand);
  }
  return false;
}

bool CcSyncClientDirectoryApp::setRestoreCommand()
{
  CcSyncConsole::writeLine("Create new Directory");
  CcString sCommand;
  CcSyncConsole::query("Command", sCommand);
  if (sCommand.length())
  {
    m_pSyncClient->updateDirectoryRestoreCommand(m_sDirectory, sCommand);
  }
  return false;
}

bool CcSyncClientDirectoryApp::verify()
{
  CcSyncConsole::writeLine("Verify Database");
  return m_pSyncClient->verify(m_sDirectory);
}
