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
 * @file      CcSyncClientApp.h
 * @brief     Class CcSyncClientApp
 *
 *  Implementation of Main Application
 */
#include "CcSyncVersion.h"
#include "CcSyncClientApp.h"
#include "CcKernel.h"
#include "CcAppKnown.h"
#include "CcSyncConsole.h"
#include "CcSyncClientAccountApp.h"
#include "CcGlobalStrings.h"
#include "CcSyncGlobals.h"
#include "CcVersion.h"
#include "CcDirectory.h"
#include "CcGroupList.h"
#include "CcUserList.h"

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

CcVersion CcSyncClientApp::getVersion() const
{
  return CcSyncGlobals::Version;
}

void CcSyncClientApp::run()
{
  bool bParamOk = true;
  if (m_oArguments.size() > 1)
  {
    // Parse options
    for (size_t iArg = 1; iArg < m_oArguments.size(); iArg++)
    {
      if (m_oArguments[iArg] == "--config-dir")
      {
        m_oArguments.remove(iArg);
        if (iArg<m_oArguments.size())
        {
          m_sConfigDir = m_oArguments[iArg];
          m_oArguments.remove(iArg);
          iArg--;
          if (!CcDirectory::exists(m_sConfigDir))
          {
            bParamOk = false;
            CcSyncConsole::writeLine("--config-dir requires an existing valid path");
          }
        }
        else
        {
          bParamOk = false;
          CcSyncConsole::writeLine("--config-dir requires an additional paramter");
        }
      }
    }
  }
  if (bParamOk == false)
  {
    setExitCode(EStatus::CommandInvalidParameter);
  }
  else if (m_oArguments.size() > 1)
  {
    if (m_oArguments[1].compareInsensitve("cli"))
    {
      m_eMode = ESyncClientMode::Cli;
    }
    else if (m_oArguments[1].compareInsensitve("once"))
    {
      m_eMode = ESyncClientMode::Once;
    }
    else if (m_oArguments[1].compareInsensitve("create"))
    {
      m_eMode = ESyncClientMode::Create;
    }
    else if (m_oArguments[1].compareInsensitve("dirs"))
    {
      m_eMode = ESyncClientMode::Dirs;
    }
    else if (m_oArguments[1].compareInsensitve("groupid"))
    {
      m_eMode = ESyncClientMode::GroupId;
    }
    else if (m_oArguments[1].compareInsensitve("userid"))
    {
      m_eMode = ESyncClientMode::UserId;
    }
    else if (m_oArguments[1].compareInsensitve("help") ||
             m_oArguments[1].compareInsensitve("-h") ||
             m_oArguments[1].compareInsensitve("/h"))
    {
      m_eMode = ESyncClientMode::Help;
    }
    else
    {
      m_eMode = ESyncClientMode::Unknown;
    }
    if(m_eMode != ESyncClientMode::Unknown)
    {
      // Remove first arguments
      m_oArguments.remove(0, 2);
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
    case ESyncClientMode::Help:
      runHelp();
      break;
    case ESyncClientMode::Create:
      runCreate();
      break;
    case ESyncClientMode::Dirs:
      runDirs();
      break;
    case ESyncClientMode::GroupId:
      runGroupId();
      break;
    case ESyncClientMode::UserId:
      runUserId();
      break;
    default:
      setExitCode(EStatus::CommandInvalidParameter);
      break;
  }
}

void CcSyncClientApp::onStop()
{
  if(m_poSyncClient)
  {
    m_poSyncClient->logout();
  }
}

void CcSyncClientApp::runDaemon()
{
  m_poSyncClient = CcSyncClient::create(m_sConfigDir, false);
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
  m_poSyncClient = nullptr;
}

void CcSyncClientApp::runCli()
{
  bool bSuccess = true;
  m_poSyncClient = CcSyncClient::create(m_sConfigDir, false);
  if (!m_poSyncClient->isConfigAvailable())
  {
    bSuccess = createConfig(m_sConfigDir);
  }
  if (bSuccess)
  {
    bool bCommandlineLoop = true;
    while (bCommandlineLoop)
    {
      CcString sCommandLine;
      if (CcSyncConsole::clientQuery(sCommandLine) != SIZE_MAX)
      {
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
      else
      {
        bCommandlineLoop = false;
      }
    }
  }
  else
  {
    CCDEBUG("Error in Directory Locations, stop progress");
    setExitCode(EStatus::FSDirNotFound);
  }
  CcSyncClient::remove(m_poSyncClient);
  m_poSyncClient = nullptr;
}

void CcSyncClientApp::runOnce()
{
  m_poSyncClient = CcSyncClient::create(m_sConfigDir, false);
  CcStringList slAccounts;
  if(m_oArguments.size() > 0)
  {
    slAccounts.append(m_oArguments[0]);
  }
  else
  {
    slAccounts = m_poSyncClient->getAccountList();
  }
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
  m_poSyncClient = nullptr;
}

void CcSyncClientApp::runCreate()
{
  m_poSyncClient = CcSyncClient::create(m_sConfigDir, true);
  if(m_oArguments[0].contains("@"))
  {
    if (m_poSyncClient->selectAccount(m_oArguments[0]))
    {
      CcSyncConsole::writeLine("Account already available");
    }
    else
    {
      CcStringList oList = m_oArguments[0].split("@");
      if(oList.size() == 2)
      {
        CcString sPort;
        CcString sServer;
        if(oList[1].contains(":"))
        {
          CcStringList oServer = oList[1].split(":");
          if(oServer.size() == 2)
          {
            sServer = oServer[0];
            sPort = oServer[1];
          }
          else
          {
            CcSyncConsole::writeLine("Invalid server given");
            setExitCode(EStatus::CommandInvalidParameter);
          }
        }
        else
        {
          sServer = oList[1];
          sPort = CcSyncGlobals::DefaultPortStr;
        }
        CcString sPassword;
        if(m_oArguments.size() > 1)
        {
          sPassword = m_oArguments[1];
        }
        else
        {
          CcSyncConsole::queryHidden("Password", sPassword);
        }
        if (sPassword.length() >= 8)
        {
          if (m_poSyncClient->addAccount(oList[0], sPassword, sServer, sPort))
          {
            CcSyncConsole::writeLine("Account successfully created");
          }
          else
          {
            CcSyncConsole::writeLine("Failed to create account");
            setExitCode(EStatus::ConfigError);
          }
        }
        else
        {
          CcSyncConsole::writeLine("Password requires at least 8 signs");
        }
      }
    }
  }
  else
  {
    CcSyncConsole::writeLine("No username and server set");
    setExitCode(EStatus::CommandInvalidParameter);
  }
  CcSyncClient::remove(m_poSyncClient);
  m_poSyncClient = nullptr;
}

void CcSyncClientApp::runDirs()
{
  m_poSyncClient = CcSyncClient::create(m_sConfigDir);
  if(m_oArguments.size() == 2)
  {
    CcString sAccount = m_oArguments[0];
    if (m_poSyncClient->selectAccount(sAccount) &&
        m_poSyncClient->login()                 &&
        m_poSyncClient->updateFromRemoteAccount())
    {
      CcString sPath = m_oArguments[1].getOsPath();
      if(!CcDirectory::exists(sPath) &&
         !CcDirectory::create(sPath))
      {
        CcSyncConsole::writeLine("Path not found and not createable");
        setExitCode(EStatus::FSDirNotFound);
      }
      else
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
            setExitCode(EStatus::FSDirCreateFailed);
            break;
          }
        }
      }
    }
    else
    {
      CcSyncConsole::writeLine("Login to account failed");
      setExitCode(EStatus::UserAccessDenied);
    }
  }
  else
  {
    CcSyncConsole::writeLine("Error Invalid paramters given");
    setExitCode(EStatus::InvalidHandle);
  }
  CcSyncClient::remove(m_poSyncClient);
  m_poSyncClient = nullptr;
}

void CcSyncClientApp::runGroupId()
{
  m_poSyncClient = CcSyncClient::create(m_sConfigDir);
  if(m_oArguments.size() == 2)
  {
    CcString sAccount = m_oArguments[0];
    if (m_poSyncClient->selectAccount(sAccount) &&
        m_poSyncClient->login()                 &&
        m_poSyncClient->updateFromRemoteAccount())
    {
      bool bOk = false;
      uint32 uiId = m_oArguments[1].toUint32(&bOk);
      if(!bOk)
      {
        CcGroupList oGroups = CcKernel::getGroupList();
        bOk = oGroups.containsGroup(m_oArguments[1]);
        if(!bOk)
        {
          CcSyncConsole::writeLine("No matching group found");
          setExitCode(EStatus::CommandInvalidParameter);
        }
        else
        {
          uiId = oGroups.findGroup(m_oArguments[1]).getId();
        }
      }
      CCUNUSED(uiId);
      if(bOk)
      {
        CcStringList oDirList = m_poSyncClient->getDirectoryList();
        for (CcString& sDirName : oDirList)
        {
          if (!m_poSyncClient->updateDirectorySetGroup(sDirName, m_oArguments[1]))
          {
            CcSyncConsole::writeLine("Failed to set id for dir: " + sDirName);
            setExitCode(EStatus::UserNotFound);
            break;
          }
        }
      }
    }
    else
    {
      CcSyncConsole::writeLine("Login to account failed");
      setExitCode(EStatus::UserAccessDenied);
    }
  }
  else
  {
    CcSyncConsole::writeLine("Error Invalid paramters given");
    setExitCode(EStatus::InvalidHandle);
  }
  CcSyncClient::remove(m_poSyncClient);
  m_poSyncClient = nullptr;
}

void CcSyncClientApp::runUserId()
{
  m_poSyncClient = CcSyncClient::create(m_sConfigDir);
  if(m_oArguments.size() == 2)
  {
    CcString sAccount = m_oArguments[0];
    if (m_poSyncClient->selectAccount(sAccount) &&
        m_poSyncClient->login()                 &&
        m_poSyncClient->updateFromRemoteAccount())
    {
      bool bOk = false;
      uint32 uiId = m_oArguments[1].toUint32(&bOk);
      if(!bOk)
      {
        CcUserList oUsers = CcKernel::getUserList();
        bOk = oUsers.containsUser(m_oArguments[1]);
        if(!bOk)
        {
          CcSyncConsole::writeLine("No matching User found");
          setExitCode(EStatus::CommandInvalidParameter);
        }
        else
        {
          uiId = oUsers.findUser(m_oArguments[1])->getId();
        }
      }
      CCUNUSED(uiId);
      if(bOk)
      {
        CcStringList oDirList = m_poSyncClient->getDirectoryList();
        for (CcString& sDirName : oDirList)
        {
          if (!m_poSyncClient->updateDirectorySetUser(sDirName, m_oArguments[1]))
          {
            CcSyncConsole::writeLine("Failed to set id for dir: " + sDirName);
            setExitCode(EStatus::UserNotFound);
            break;
          }
        }
      }
    }
    else
    {
      CcSyncConsole::writeLine("Login to account failed");
      setExitCode(EStatus::UserAccessDenied);
    }
  }
  else
  {
    CcSyncConsole::writeLine("Error Invalid paramters given");
    setExitCode(EStatus::InvalidHandle);
  }
  CcSyncClient::remove(m_poSyncClient);
  m_poSyncClient = nullptr;
}

void CcSyncClientApp::runHelp()
{
  CcSyncConsole::writeLine(CcString("Version: ") + CCSYNC_VERSION_STRING + "");
  CcSyncConsole::writeLine("Usage:");
  CcSyncConsole::writeLine("  []");
  CcSyncConsole::writeLine("    Start interactive mode");
  CcSyncConsole::writeLine("  once [<Username>@<Server>]");
  CcSyncConsole::writeLine("    Synchronize all directories of available accounts, or selected");
  CcSyncConsole::writeLine("  create <Username>@<Server>");
  CcSyncConsole::writeLine("    Create local account and connect to server");
  CcSyncConsole::writeLine("  dirs <Username>@<Server> <Path>");
  CcSyncConsole::writeLine("    Create all directories from client in path");
  CcSyncConsole::writeLine("  userid <Username>@<Server> <ID>");
  CcSyncConsole::writeLine("    Set user id for all directories, names will be resolved");
  CcSyncConsole::writeLine("  groupid <Username>@<Server> <ID>");
  CcSyncConsole::writeLine("    Set group id for all directories, names will be resolved");
}

bool CcSyncClientApp::createConfig(const CcString& sConfigDir)
{
  if (m_poSyncClient != nullptr)
    return m_poSyncClient->createConfig(sConfigDir);
  return false;
}

bool CcSyncClientApp::createAccount()
{
  bool bSuccess = false;
  if (m_poSyncClient != nullptr)
  {
    CcSyncConsole::writeLine("Setup new Account");
    CcString sAccount;
    CcSyncConsole::query("Name", sAccount);
    CcString sServer ;
    CcSyncConsole::query("Server", sServer);
    CcString sPort   ;
    CcSyncConsole::query("Port [27500]", CcSyncGlobals::DefaultPortStr, sPort);
    if (m_poSyncClient->selectAccount(sAccount + "@" + sServer))
    {
      CcString sAnswer;
      CcSyncConsole::query("Account already existing, change password? [y/N]", "n", sAnswer);
      if (sAnswer.toLower()[0] == 'y')
      {
        CcString sPassword;
        CcSyncConsole::queryHidden("Password", sPassword);
        if (sPassword.length() >= 8)
        {
          bSuccess = m_poSyncClient->getCurrentAccountConfig()->changePassword(sPassword);
        }
        else
        {
          CcSyncConsole::writeLine("Password requires at least 8 signs");
        }
      }
    }
    else
    {
      CcString sPassword;
      CcSyncConsole::queryHidden("Password", sPassword);
      if (sPassword.length() >= 8)
      {
        bSuccess = m_poSyncClient->addAccount(sAccount, sPassword, sServer, sPort);
      }
      else
      {
        CcSyncConsole::writeLine("Password requires at least 8 signs");
      }
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
      CcString sServer;
      CcSyncConsole::query("Server [" + sOldServer + "]", m_poSyncClient->getCurrentAccountConfig()->getServer().getHostname(), sServer);
      if (sServer.length() > 0  && sServer != sOldServer)
      {
        bSuccess &= m_poSyncClient->changeHostname(sServer);
      }
      else if (sServer.length())
      {
        CcSyncConsole::writeLine("  No server set, not changed");
      }
      else
      {
        CcSyncConsole::writeLine("  not changed");
      }
      if (bSuccess)
      {
        CcString sPort;
        CcSyncConsole::query("Port [" + m_poSyncClient->getCurrentAccountConfig()->getServer().getPortString() + "]", m_poSyncClient->getCurrentAccountConfig()->getServer().getPortString(), sPort);
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
        CcString sAnswer;
        CcSyncConsole::query("Change password? [y/N]", "n", sAnswer);
        if (sAnswer.toLower()[0] == 'y')
        {
          CcString sPassword;
          CcSyncConsole::queryHidden("Password", sPassword);
          if (sPassword.length() >= 8)
          {
            bSuccess &= m_poSyncClient->getCurrentAccountConfig()->changePassword(sPassword);
          }
          else
          {
            CcSyncConsole::writeLine("Password requires at least 8 signs");
          }
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
