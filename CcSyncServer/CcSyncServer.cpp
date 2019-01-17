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
 * @brief     Implemtation of class CcSyncServer
 */
#include "CcSyncServer.h"
#include "CcSyncGlobals.h"
#include "CcKernel.h"
#include "CcDirectory.h"
#include "Network/CcSocket.h"
#include "CcSyncServerWorker.h"
#include "CcSyncClientConfig.h"
#include "CcSslControl.h"
#include "CcSslSocket.h"
#include "CcFile.h"
#include "Hash/CcMd5.h"
#include "CcSyncConsole.h"
#include "Network/CcCommonPorts.h"
#include "CcSyncVersion.h"
#include "CcVersion.h"
#include "CcConsole.h"

CcSyncServer::CcSyncServer(int pArgc, char **ppArgv) : 
  m_oArguments(pArgc, ppArgv)
{
}

CcSyncServer::CcSyncServer(const CcSyncServer& oToCopy):
  CcApp(oToCopy)
{
  operator=(oToCopy);
}

CcSyncServer::CcSyncServer(CcSyncServer&& oToMove)
{
  operator=(std::move(oToMove));
}

CcSyncServer::~CcSyncServer(void)
{
}

void CcSyncServer::run()
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
  // select mode
  else if (m_oArguments.size() > 1)
  {
    if (m_oArguments[1] == "daemon")
    {
      switch (CcKernel::initService())
      {
        case -1:
          CCERROR("Starting Service failed");
          setExitCode(EStatus::Error);
        case 0:
          break;
        default:
          CCDEBUG("Service started, close main application.");
          setExitCode(EStatus::Success);
          break;
      }
    }
    else if (m_oArguments[1] == "configure")
    {
      createConfig();
    }
    else if (m_oArguments[1] == "start")
    {
      CcKernel::initCLI();
    }
    else if (m_oArguments[1] == "info")
    {
      CcConsole::writeLine("CcSyncServer Version: " + getVersion().getVersionString());
      CcConsole::writeLine("CcOS         Version: " + CcKernel::getVersion().getVersionString());
    }
    else
    {
      if (m_oArguments[1] != "help" &&
        m_oArguments[1] != "-h")
      {
        setExitCode(EStatus::CommandUnknownParameter);
        CcConsole::writeLine("ERROR, wrong command: " + m_oArguments[1]);
      }
      runHelp();
    }
  }
  else
  {
    setExitCode(EStatus::CommandRequiredParameter);
    runHelp();
  }
}

void CcSyncServer::runHelp()
{
  CcConsole::writeLine("Usage of CcSyncServer:");
  CcConsole::writeLine("  CcSyncServer <command>");
  CcConsole::writeLine("");
  CcConsole::writeLine("Commands");
  CcConsole::writeLine("  daemon      Start server as background application");
  CcConsole::writeLine("  start       Start server and keep running until closed");
  CcConsole::writeLine("  configure   Change or create CcSyncServer settings");
  CcConsole::writeLine("  info        Get information about this server");
  CcConsole::writeLine("  help|-h     Get information about this server");
}

void CcSyncServer::runServer()
{
  if(m_sConfigDir.length() == 0)
  {
    m_sConfigDir = CcKernel::getConfigDir();
  }
  m_sDatabaseFile = m_sConfigDir;
  CcString sConfigFile;
  sConfigFile.appendPath(CcSyncGlobals::ConfigDirName);
  if (!CcDirectory::exists(sConfigFile))
  {
    CcSyncLog::writeInfo("No configuration Directory on default config-directory");
    CcDirectory::create(sConfigFile, true);
  }
  else
  {
    if (setupDatabase())
    {
      sConfigFile.appendPath(CcSyncGlobals::Server::ConfigFileName);
      m_oConfig.readConfig(sConfigFile);
    }
    else
      CcSyncLog::writeError("No database file available");
  }
  if(CcFile::exists(m_oConfig.getSslCertFile()) == false ||
     CcFile::exists(m_oConfig.getSslKeyFile()) == false)
  {
    CcSslControl::createCert(m_oConfig.getSslCertFile(), m_oConfig.getSslKeyFile());
  }
  m_oSocket = new CcSslSocket();
  static_cast<CcSslSocket*>(m_oSocket.getRawSocket())->initServer();
  if (m_oSocket.bind(m_oConfig.getPort())                   &&
      static_cast<CcSslSocket*>(m_oSocket.getRawSocket())->loadKey(m_oConfig.getSslKeyFile()) &&
      static_cast<CcSslSocket*>(m_oSocket.getRawSocket())->loadCertificate(m_oConfig.getSslCertFile()))
  {
    CcSyncLog::writeDebug("Server is listening on: " + CcString::fromNumber(m_oConfig.getPort()));
    while (getThreadState() == EThreadState::Running)
    {
      if (m_oSocket.listen())
      {
        CcSocketAbstract* oTemp = m_oSocket.accept();
        if (oTemp != nullptr)
        {
          CcSyncLog::writeDebug("Server recognized an incomming connection, starting thread");
          CcSyncServerWorker* oNewWorker = new CcSyncServerWorker(this, oTemp); 
          CCMONITORNEW(oNewWorker);
          oNewWorker->start();
        }
        else
        {
          CcSyncLog::writeError("Error on accepting connection");
        }
      }
      else
      {
        CcSyncLog::writeError("Error on listening");
      }
    }
    CcSyncLog::writeDebug("Server is going down");
  }
  else
  {
    CcSyncLog::writeError("Unable to bind Socket");
  }
}

CcSyncServer& CcSyncServer::operator=(const CcSyncServer& oToCopy)
{
  m_oConfig     = oToCopy.m_oConfig;
  return *this;
}

CcSyncServer& CcSyncServer::operator=(CcSyncServer&& oToMove)
{
  if (this != &oToMove)
  {
    m_oConfig     = std::move(oToMove.m_oConfig);
  }
  return *this;
}

CcSyncUser CcSyncServer::loginUser(const CcString& sAccount, const CcString & sUserName, const CcString & sPassword)
{
  CcString sToken;
  for (const CcSyncServerAccount& rAccount : m_oConfig.getAccountList())
  {
    if (rAccount.getName().compareInsensitve(sAccount))
    {
      // @todo: Check for Additional Users
      if (rAccount.getPassword().getString() == sPassword )
      {
        sToken << rAccount.getName() << CcString::fromNumber(CcKernel::getDateTime().getTimestampUs());
        CcMd5 oTokenGenerator;
        oTokenGenerator.generate(sToken);
        sToken = oTokenGenerator.getHexString();
        if (m_oDatabase.userExistsInDatabase(sAccount, sUserName))
        {
          if (false == m_oDatabase.updateUser(rAccount.getName(), rAccount.getName(), sToken))
          {
            sToken = "";
          }
          else
          {
            break;
          }
        }
        else
        {
          if (false == m_oDatabase.insertUser(rAccount.getName(), rAccount.getName(), sToken))
          {
            sToken = "";
          }
          else
          {
            break;
          }
        }
      }
    }
  }
  return getUserByToken(sToken);
}

CcSyncUser CcSyncServer::getUserByToken(const CcString& sToken)
{
  CcSyncUser oUser;
  CcString sUsername;
  CcString sAccount;
  if (m_oDatabase.getUserByToken(sToken, sAccount, sUsername))
  {
    CcSyncServerAccount* pAccount = m_oConfig.findAccount(sUsername);
    if (pAccount != nullptr)
    {
      CcString sClientPath = m_oConfig.getLocation().getPath();
      sClientPath.appendPath(pAccount->getName());
      CcSyncClientConfigPointer pClientConfig = pAccount->clientConfig(sClientPath);
      if (pClientConfig != nullptr)
      {
        CcSyncAccountConfigHandle pAccountConfig = pClientConfig->getAccountConfig(sAccount);
        if (pAccountConfig != nullptr)
        {
          ESyncRights eRights = ESyncRights::Account;
          if (pAccount->isAdmin())
          {
            eRights = ESyncRights::Admin;
          }
          CcSyncDbClientPointer pClientDatabase = pAccount->database(sClientPath);
          if (pClientDatabase != nullptr)
          {
            oUser = CcSyncUser(sToken, pClientConfig, pAccountConfig, pClientDatabase, eRights);
          }
          else
          {
            CcSyncLog::writeDebug("User Database not found at " + sClientPath);
          }
        }
        else
        {
          CcSyncLog::writeDebug("User not found in config " + sClientPath);
        }
      }
      else
      {
        CcSyncLog::writeDebug("User config not found at " + sClientPath);
      }
    }
    else
    {
      CcSyncLog::writeDebug("User not found in Database " + sAccount);
    }
  }
  return oUser;
}

CcSyncUser CcSyncServer::getUserByName(const CcString& sName)
{
  CcSyncUser oUser;
  CcSyncServerAccount* pAccount = m_oConfig.findAccount(sName);
  if (pAccount != nullptr)
  {
    CcString sClientPath = m_oConfig.getLocation().getPath();
    sClientPath.appendPath(pAccount->getName());
    CcSyncClientConfigPointer pClientConfig = pAccount->clientConfig(sClientPath);
    if (pClientConfig != nullptr)
    {
      CcSyncAccountConfigHandle pAccountConfig = pClientConfig->getAccountConfig(sName);
      if (pAccountConfig != nullptr)
      {
        ESyncRights eRights = ESyncRights::Account;
        if (pAccount->isAdmin())
        {
          eRights = ESyncRights::Admin;
        }
        CcSyncDbClientPointer pClientDatabase = pAccount->database(sClientPath);
        if (pClientDatabase != nullptr)
        {
          oUser = CcSyncUser("", pClientConfig, pAccountConfig, pClientDatabase, eRights);
        }
        else
        {
          CcSyncLog::writeDebug("User Database not found at " + sClientPath);
        }
      }
      else
      {
        CcSyncLog::writeDebug("User not found in config " + sClientPath);
      }
    }
    else
    {
      CcSyncLog::writeDebug("User config not found at " + sClientPath);
    }
  }
  return oUser;
}

CcVersion CcSyncServer::getVersion() const
{
  return CcSyncGlobals::Version;
}

bool CcSyncServer::createConfig()
{
  bool bSuccess = true;
  if(m_sConfigDir.length() == 0)
  {
    m_sConfigDir = CcKernel::getConfigDir();
  }
  CcString sConfigFile = m_sConfigDir;
  m_sDatabaseFile = m_sConfigDir;
  sConfigFile.appendPath(CcSyncGlobals::ConfigDirName);
  if (!CcDirectory::exists(sConfigFile))
  {
    CcSyncLog::writeInfo("No configuration Directory on default location");
    if (!CcDirectory::create(sConfigFile, true))
    {
      CcSyncConsole::writeLine("Configuration Directory could not be created");
      bSuccess = false;
      setExitCode(EStatus::FSDirNotFound);
    }
  }
  if (bSuccess)
  {
    sConfigFile.appendPath(CcSyncGlobals::Server::ConfigFileName);
    if (CcFile::exists(sConfigFile))
    {
      bSuccess = m_oConfig.readConfig(sConfigFile);
      if(!bSuccess)
      {
        setExitCode(EStatus::ConfigReadFailed);
      }
    }
    else
    {
      CcString sDefaultLocation = CcKernel::getDataDir();
      sDefaultLocation.appendPath("CcSync/Server");
      m_oConfig.setLocation(sDefaultLocation);
      m_oConfig.setPort(CcCommonPorts::CcSync);
    }
  }
  if (bSuccess)
  {
    CcString sAdmin;
    CcString sAdminPw;
    CcString sLocation;
    uint16 uiPort = 0;
    // Read admin name
    bool bAnswered = false;
    while (bAnswered == false)
    {
      sAdmin = CcSyncConsole::query("Administrator");
      if (sAdmin == "")
      {
        CcSyncConsole::writeLine("Name is required, please retry");
      }
      else
      {
        bAnswered = true;
      }
    }
    bAnswered = false;
    while (bAnswered == false)
    {
      sAdminPw = CcSyncConsole::query("Password");
      CcString sAdminPwTest = CcSyncConsole::query("Repeat");
      if (sAdminPw == "" || sAdminPw != sAdminPwTest)
      {
        CcSyncConsole::writeLine("Passwords not matching or empty, please retry");
      }
      else
      {
        bAnswered = true;
      }
    }
    bAnswered = false;
    while (bAnswered == false)
    {

      CcString sPort = CcSyncConsole::query("Port [" + CcString::fromNumber(m_oConfig.getPort()) + "]", CcString::fromNumber(m_oConfig.getPort()));
      uiPort = sPort.trim().toUint16(&bAnswered);
      if (bAnswered == false||
          uiPort == 0)
      {
        CcSyncConsole::writeLine("Port must be a number between 0 and 65536");
      }
      else
      {
        bAnswered = true;
      }
    }
    bAnswered = false;
    while (bAnswered == false)
    {
      sLocation.setOsPath(CcSyncConsole::query("Location [" + m_oConfig.getLocation().getPath() + "]", m_oConfig.getLocation().getPath()));
      if (!CcDirectory::exists(sLocation))
      {
        CcString sAnswer = CcSyncConsole::query("Location not existing, create? [Y/n]", "y");
        if (sAnswer[0] == 'y' || sAnswer[0] == 'Y')
        {
          bAnswered = true;
          if (!CcDirectory::create(sLocation, true))
          {
            CcSyncConsole::writeLine("Failed to create location, please retry");
            bAnswered = false;
            continue;
          }
        }
        else if (sAnswer[0] == 'n' || sAnswer[0] == 'N')
        {
          CcSyncConsole::writeLine("Location will be configured, please create it later.");
          bAnswered = true;
          continue;
        }
        else
        {
          CcSyncConsole::writeLine("Answer not verfied, please retry");
          continue;
        }
      }
      else
      {
        bAnswered = true;
      }
    }
    if (bSuccess)
    {
      m_oConfig.setLocation(sLocation);
      m_oConfig.setPort(uiPort);
      createAccount(sAdmin, sAdminPw, true);
      m_oConfig.writeConfig(sConfigFile);
    }
  }
  return bSuccess;
}

bool CcSyncServer::setupDatabase()
{
  m_sDatabaseFile = CcKernel::getDataDir();
  m_sDatabaseFile.appendPath(CcSyncGlobals::ConfigDirName);
  if (CcDirectory::exists(m_sDatabaseFile) ||
      CcDirectory::create(m_sDatabaseFile, true))
  {
    m_sDatabaseFile.appendPath(CcSyncGlobals::Server::DatabaseFileName);
    return m_oDatabase.openDatabase(m_sDatabaseFile);
  }
  else
  {
    CcSyncLog::writeError("Location for database not available: " + m_sDatabaseFile);
    return false;
  }
}

bool CcSyncServer::createAccount(const CcString& sUsername, const CcString& sPassword, bool bAdmin)
{
  bool bRet = true;
  if(bAdmin)
    m_oConfig.addAdminAccount(sUsername, sPassword);
  else
    m_oConfig.addAccount(sUsername, sPassword);
  const CcSyncServerLocationConfig& oLocation = m_oConfig.getLocation();
  CcString sPath = oLocation.getPath();
  sPath.appendPath(sUsername);
  if (CcDirectory::exists(sPath) ||
      CcDirectory::create(sPath, true))
  {
    CcSyncLog::writeInfo("Accountpath created " + sPath);
  }
  else
  {
    bRet = false;
  }
  return bRet;
}

bool CcSyncServer::removeAccount(const CcString& sUsername)
{
  return m_oConfig.removeAccount(sUsername);;
}
