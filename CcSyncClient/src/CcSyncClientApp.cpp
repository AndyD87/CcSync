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
 * @file      CcSyncClientApp.h
 * @brief     Class CcSyncClientApp
 *
 *  Implementation of Main Application
 */

#include "CcSyncClientApp.h"
#include "CcKernel.h"
#include "CcAppKnown.h"
#include "CcSyncConsole.h"
#include "CcSyncClientAccountApp.h"
#include "CcGlobalStrings.h"

namespace Strings
{
  static const CcString Exit        ("exit");
  static const CcString ExitDesc    ("exit client and close application");
  static const CcString Help        ("help");
  static const CcString HelpDesc    ("This help Message");
  static const CcString List        ("list");
  static const CcString ListDesc    ("list all accounts.");
  static const CcString Login       ("login");
  static const CcString LoginDesc   ("[Username[@Server]] Login to an account");
  static const CcString Edit        ("edit");
  static const CcString EditDesc    ("[Username[@Server]] select an account counfig and edit it");
  static const CcString New         ("new");
  static const CcString NewDesc     ("create new account.");
  static const CcString Del         ("del");
  static const CcString DelDesc     ("[Username[@Server]] select an account counfig and delete it");
}

CcSyncClientApp::CcSyncClientApp(const CcArguments& oArguments) :
  CcApp(CcAppKnown::CcSyncClientName, CcAppKnown::CcSyncClientUuid),
  m_oArguments(oArguments)
{
}

CcSyncClientApp::~CcSyncClientApp() {
}

void CcSyncClientApp::run()
{
  if (m_oArguments.size() > 1)
  {
    if (m_oArguments[1].compareInsensitve("cli"))
    {
      m_eMode = ESyncClientMode::Cli;
    }
    else if (m_oArguments[1].compareInsensitve("once"))
    {
      m_eMode = ESyncClientMode::Once;
    }
    else if (m_oArguments[1].compareInsensitve("daemon"))
    {
      m_eMode = ESyncClientMode::Daemon;
    }
    else if (m_oArguments[1].compareInsensitve("help"))
    {
      m_eMode = ESyncClientMode::Help;
    }
  }
  switch (m_eMode)
  {
    case ESyncClientMode::Cli:
      runCli();
      break;
    case ESyncClientMode::Once:
      runOnce();
      break;
    case ESyncClientMode::Daemon:
      switch (CcKernel::initService())
      {
        case -1:
          CCERROR("Starting Service failed");
          setExitCode(-1);
          break;
        case 0:
          runDaemon();
          break;
        default:
          CCDEBUG("Service started, close main application.");
          setExitCode(0);
          break;
      }
      break;
    case ESyncClientMode::Help:
      runHelp();
      break;
  default:
    break;
  }
}

void CcSyncClientApp::runDaemon()
{
  m_poSyncClient = CcSyncClient::create();
  CcStringList slAccounts = m_poSyncClient->getAccountList();
  while (getThreadState() == EThreadState::Running)
  {
    for (CcString& sAccount : slAccounts)
    {
      if (m_poSyncClient->selectAccount(sAccount))
      {
        // Try to login to server
        if (m_poSyncClient->login())
        {
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
        else
        {
          CCDEBUG("Unable to login  "+sAccount+" to server");
        }
      }
    }
    CcKernel::delayS(60);
  }
  CcSyncClient::remove(m_poSyncClient);
}

void CcSyncClientApp::runCli()
{
  bool bSuccess = true;
  m_poSyncClient = CcSyncClient::create();
  if (!m_poSyncClient->isConfigAvailable())
  {
    bSuccess = createConfig();
  }
  if (bSuccess)
  {
    bool bCommandlineLoop = true;
    while (bCommandlineLoop)
    {
      CcString sCommandLine = CcSyncConsole::clientQuery();
      CcArguments oArguments(sCommandLine);
      if (oArguments.size() == 0)
      {
        continue;
      }
      else if (oArguments[0].compareInsensitve(Strings::New))
      {
        if (!createAccount())
        {
          CcSyncConsole::writeLine("Account creation failed.");
        }
      }
      else if (oArguments[0].compareInsensitve(Strings::Edit))
      {
        if (oArguments.size() > 1)
        {
          if (!editAccount(oArguments[1]))
          {
            CcSyncConsole::writeLine("Account editing failed.");
          }
        }
        else
        {
          CcSyncConsole::writeLine("no account given, please type \"help\"");
        }
      }
      else if (oArguments[0].compareInsensitve(Strings::Del))
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
      else if (oArguments[0].compareInsensitve(Strings::List))
      {
        for (CcString& sAccount : m_poSyncClient->getAccountList())
        {
          CcSyncConsole::writeLine(sAccount);
        }
      }
      else if (oArguments[0].compareInsensitve(Strings::Login))
      {
        if (oArguments.size() > 1)
        {
          if (m_poSyncClient->selectAccount(oArguments[1]))
          {
            CcSyncConsole::writeLine("Account selected: " + m_poSyncClient->getAccountName());
          }
          else
          {
            CcSyncConsole::writeLine("No account found");
          }
        }
        else
        {
          if (m_poSyncClient->selectAccount(""))
          {
            CcSyncConsole::writeLine("First account selcted: " + m_poSyncClient->getAccountName());
          }
          else
          {
            CcSyncConsole::writeLine("No account found");
          }
        }
        if (m_poSyncClient->login())
        {
          CcSyncConsole::writeLine("Login succeeded");
          CcSyncClientAccountApp oAccount(m_poSyncClient);
          oAccount.exec();
        }
        else
        {
          CcSyncConsole::writeLine("Login failed");
        }
      }
      else if (oArguments[0].compareInsensitve(Strings::Exit))
      {
        CcSyncConsole::writeLine("Bye :)");
        bCommandlineLoop = false;
      }
      else if (oArguments[0].compareInsensitve(Strings::Help))
      {
        CcSyncConsole::writeLine("Client commands:");
        CcSyncConsole::printHelpLine("  " + Strings::Help, 30, Strings::HelpDesc);
        CcSyncConsole::printHelpLine("  " + Strings::Exit, 30, Strings::ExitDesc);

        CcSyncConsole::writeLine(CcGlobalStrings::Empty);
        CcSyncConsole::writeLine("Manage Accounts:");
        CcSyncConsole::printHelpLine("  " + Strings::Login, 30, Strings::LoginDesc);
        CcSyncConsole::printHelpLine("  " + Strings::List, 30, Strings::ListDesc);
        CcSyncConsole::printHelpLine("  " + Strings::New, 30, Strings::NewDesc);
        CcSyncConsole::printHelpLine("  " + Strings::Edit, 30, Strings::EditDesc);
        CcSyncConsole::printHelpLine("  " + Strings::Del, 30, Strings::DelDesc);
      }
      else
      {
        CcSyncConsole::writeLine("Unknown Command, run \"help\" to view all commands");
      }
    }
  }
  else
  {
    CCDEBUG("Error in Directory Locations, stop progress");
    setExitCode((int32)EStatus::FSDirNotFound);
  }
  CcSyncClient::remove(m_poSyncClient);
}

void CcSyncClientApp::runOnce()
{
  m_poSyncClient = CcSyncClient::create();
  CcStringList slAccounts = m_poSyncClient->getAccountList();
  for (CcString& sAccount : slAccounts)
  {
    CcSyncConsole::writeLine("Select Account: " + sAccount);
    if (m_poSyncClient->selectAccount(sAccount))
    {
      // Try to login to server
      if (m_poSyncClient->login())
      {
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
      else
      {
        CCDEBUG("Unable to login  " + sAccount + " to server");
      }
    }
  }
  CcSyncClient::remove(m_poSyncClient);
}

void CcSyncClientApp::runHelp()
{

}

bool CcSyncClientApp::createConfig()
{
  if (m_poSyncClient != nullptr)
    return m_poSyncClient->createConfig();
  return false;
}

bool CcSyncClientApp::createAccount()
{
  bool bSuccess = false;
  if (m_poSyncClient != nullptr)
  {
    CcSyncConsole::writeLine("Setup new Account");
    CcString sAccount = CcSyncConsole::query("Name");
    CcString sServer = CcSyncConsole::query("Server");
    CcString sPort = CcSyncConsole::query("Port [27500]", CcSyncGlobals::DefaultPortStr);
    if (m_poSyncClient->selectAccount(sAccount + "@" + sServer))
    {
      CcString sAnswer = CcSyncConsole::query("Account already existing, change password? [y/N]", "n");
      if (sAnswer.toLower()[0] == 'y')
      {
        CcString sPassword = CcSyncConsole::queryHidden("Password");
        bSuccess = m_poSyncClient->getCurrentAccountConfig()->changePassword(sPassword);
      }
    }
    else
    {
      CcString sPassword = CcSyncConsole::queryHidden("Password");
      bSuccess = m_poSyncClient->addAccount(sAccount, sPassword, sServer, sPort);
    }
  }
  return bSuccess;
}

bool CcSyncClientApp::editAccount(const CcString& sAccount)
{
  bool bSuccess = false;
  if (m_poSyncClient != nullptr)
  {
    if (m_poSyncClient->selectAccount(sAccount))
    {
      bSuccess = true;
      CcSyncConsole::writeLine("Edit Account");
      CcString sOldServer = m_poSyncClient->getCurrentAccountConfig()->getServer().getHostname();
      CcString sServer = CcSyncConsole::query("Server ["+ sOldServer +"]", m_poSyncClient->getCurrentAccountConfig()->getServer().getHostname());
      if (sServer != sOldServer)
      {
        bSuccess &= m_poSyncClient->changeHostname(sServer);
      }
      else
      {
        CcSyncConsole::writeLine("  not changed");
      }
      if (bSuccess)
      {
        CcString sPort = CcSyncConsole::query("Port [" + m_poSyncClient->getCurrentAccountConfig()->getServer().getPortString() + "]", m_poSyncClient->getCurrentAccountConfig()->getServer().getPortString());
        if (sPort != m_poSyncClient->getCurrentAccountConfig()->getServer().getPortString())
        {
          bSuccess &= m_poSyncClient->getCurrentAccountConfig()->changePort(sPort);
        }
        else
        {
          CcSyncConsole::writeLine("  not changed");
        }
        if (!bSuccess &&
            sServer != sOldServer)
        {
          bSuccess &= m_poSyncClient->changeHostname(sOldServer);
        }
      }
      if (bSuccess)
      {
        CcString sAnswer = CcSyncConsole::query("Change password? [y/N]", "n");
        if (sAnswer.toLower()[0] == 'y')
        {
          CcString sPassword = CcSyncConsole::queryHidden("Password");
          bSuccess &= m_poSyncClient->getCurrentAccountConfig()->changePassword(sPassword);
        }
        else
        {
          CcSyncConsole::writeLine("  not changed");
        }
      }
    }
    else
    {
      CcSyncConsole::writeLine("Failed to select account: " + sAccount);
    }
  }
  return bSuccess;
}
