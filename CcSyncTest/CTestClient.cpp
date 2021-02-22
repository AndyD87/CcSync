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
 * @file
 * @copyright Andreas Dirmeier (C) 2017
 * @author    Andreas Dirmeier
 * @par       Web:      http://coolcow.de/projects/CcOS
 * @par       Language: C++11
 * @brief     Implemtation of class CTestClient
 */
#include "CTestClient.h"
#include "IIo.h"
#include "CcByteArray.h"
#include "CcTestFramework.h"
#include "CcKernel.h"
#include "CcFile.h"
#include "CcDirectory.h"
#include "CcStringUtil.h"

CTestClient::CTestClient(const CcString& sServerExePath, const CcString& sConfigDir) :
  m_sConfigDir(sConfigDir)
{
  m_oClientProc.setApplication(sServerExePath);
  m_oClientProc.addArgument("--config-dir");
  m_oClientProc.addArgument(sConfigDir);
}

CTestClient::~CTestClient( void )
{
}

void CTestClient::resetArguments(void)
{
  m_oClientProc.clearArguments();
  m_oClientProc.addArgument("--config-dir");
  m_oClientProc.addArgument(m_sConfigDir);
}

bool CTestClient::clientExists(void)
{
  resetArguments();
  m_oClientProc.addArgument("help");
  CcStatus oStatus = m_oClientProc.exec(CcDateTimeFromSeconds(1));
  if (oStatus)
  {
    resetArguments();
    m_oClientProc.addArgument("hel");
    // test if a wrong parameter would faild server
    oStatus = m_oClientProc.exec(CcDateTimeFromSeconds(1));
    if (!oStatus && oStatus != EStatus::TimeoutReached)
    {
      oStatus = true;
    }
    else
    {
      oStatus = false;
    }
  }
  return oStatus;
}

bool CTestClient::addNewServer(const CcString& sServerName, const CcString& sServerPort, const CcString& sUsername, const CcString& sPassword)
{
  CcStatus oStatus;
  resetArguments();
  m_oClientProc.start();
  oStatus = m_oClientProc.waitForRunning(CcDateTimeFromSeconds(1));
  if (oStatus)
  {
    CcTestFramework::writeInfo("  Client started");
    m_oClientProc.pipe().writeLine("new");
    m_oClientProc.pipe().writeLine(sUsername);
    m_oClientProc.pipe().writeLine(sServerName);
    m_oClientProc.pipe().writeLine(sServerPort);
    m_oClientProc.pipe().writeLine(sPassword);

    if (readUntilSucceeded("]:"))
    {
      CcTestFramework::writeInfo("  new command done, exit now");
      m_oClientProc.pipe().writeLine("exit");
      oStatus = m_oClientProc.waitForExit(CcDateTimeFromSeconds(1));
      if (!oStatus)
        CcTestFramework::writeError(CcString("  Failed with") + m_oClientProc.pipe().readAll());
      else
        CcTestFramework::writeInfo("  Account created");
    }
    else
    {
      CcTestFramework::writeError("  Account creation timed out");
      oStatus = EStatus::Error;
    }
  }
  return oStatus;
}

bool CTestClient::login(const CcString& sServerName, const CcString& sUsername)
{
  m_sUsername = sUsername;
  CcStatus oStatus;
  resetArguments();
  m_oClientProc.start();
  oStatus = m_oClientProc.waitForRunning(CcDateTimeFromSeconds(1));
  if (oStatus)
  {
    m_oClientProc.pipe().writeLine("login "+sUsername+"@"+sServerName);
    if(readUntilSucceeded(sUsername+"]:"))
    {
      oStatus = EStatus::AllOk;
    }
    else
    {
      oStatus = EStatus::UserLoginFailed;
    }
    // test if a wrong parameter would faild server
  }
  return oStatus;
}

bool CTestClient::logout()
{
  m_oClientProc.pipe().writeLine("exit");
  m_oClientProc.pipe().writeLine("exit");
  return m_oClientProc.waitForExit(CcDateTimeFromSeconds(1));
}

bool CTestClient::sync()
{
  CcTestFramework::writeInfo("  send sync command");
  m_oClientProc.pipe().writeLine("sync");
  CcTestFramework::writeInfo("  wait for command done");
  return readUntilSucceeded(m_sUsername + "]:");
}

bool CTestClient::checkLogin(const CcString& sServerName, const CcString& sUsername)
{
  CcStatus oStatus(EStatus::LoginFailed);
  if (login(sServerName, sUsername))
  {
    oStatus = logout();
  }
  return oStatus;
}

bool CTestClient::createSyncDirectory(const CcString& sDirectoryPath)
{
  bool bSuccess = false;
  m_oClientProc.pipe().writeLine("create");
  m_oClientProc.pipe().writeLine("TestDir");
  m_sSyncDirs.append(m_sConfigDir.appendPath("CcSync").appendPath(sDirectoryPath));
  m_oClientProc.pipe().writeLine(m_sSyncDirs.last());
  if (readUntilSucceeded(m_sUsername + "]:"))
  {
  if (CcDirectory::exists(m_sSyncDirs.last()))
  {
    bSuccess = true;
  }
  }
  return bSuccess;
}

bool CTestClient::createDirectory(const CcString & sDirectoryPath)
{
  CcString sDir = m_sSyncDirs.last();
  sDir.appendPath(sDirectoryPath);
  return CcDirectory::create(sDir, true, true);
}

bool CTestClient::createFile(const CcString & sPathInDir, const CcString &sContent)
{
  bool bSuccess = false;
  CcString sDir = m_sSyncDirs.last();
  sDir.appendPath(sPathInDir);
  if (CcDirectory::create(CcStringUtil::getDirectoryFromPath(sDir), true, false))
  {
    CcFile oFile(sDir);
    if (oFile.open(EOpenFlags::Write))
    {
      bSuccess = oFile.writeString(sContent);
      oFile.close();
    }
  }
  else
  {
    CcTestFramework::writeError("Failed to create Directory: "+ sPathInDir);
  }
  return bSuccess;
}

bool CTestClient::serverShutdown()
{
  bool bSuccess = false;
  m_oClientProc.pipe().writeLine("admin");
  if(readUntilSucceeded("[Admin]:"))
  {
    m_oClientProc.pipe().writeLine("stop");
  if (readUntilSucceeded("/" + m_sUsername + "]:"))
    {
      m_oClientProc.pipe().writeLine("exit");
      m_oClientProc.pipe().readAll();
      bSuccess = true;
    }
    else
    {
      CcTestFramework::writeError("No response from server on exit");
    }
  }
  else
  {
    CcTestFramework::writeError("Failed to login as admin");
  }
  return bSuccess;
}

CcString CTestClient::readWithTimeout(const CcString& sStringEnd, CcStatus& oStatus, const CcDateTime& oTimeout)
{
  CcString sData;
  CcDateTime oCountDown = oTimeout;
  CcString sWaitForever = CcKernel::getEnvironmentVariable("WAIT_FOREVER");

  while (oCountDown.timestampUs() > 0 ||
         sWaitForever.length())
  {
    oCountDown.addSeconds(-1);
    CcKernel::delayS(1);
    sData += m_oClientProc.pipe().readAll();
    sData.trim();
    if (sData.endsWith(sStringEnd))
    {
      break;
    }
  }
  if (oCountDown <= 0)
  {
    oStatus = EStatus::TimeoutReached;
  }
  else
  {
    oStatus = true;
  }
  return sData;
}

bool CTestClient::readUntilSucceeded(const CcString& sStringEnd, CcStatus* oStatus)
{
  CcStatus oLocalStatus;
  if(!oStatus) oStatus = &oLocalStatus;
  CcString sRead = readWithTimeout(sStringEnd, *oStatus);
  if (*oStatus == EStatus::TimeoutReached)
  {
    CcTestFramework::writeError("Read timed out");
  }
  else if (sRead.endsWith(sStringEnd))
  {
    *oStatus = true;
  }
  else
  {
    CcTestFramework::writeError("Last Output:");
    CcTestFramework::writeError(sRead);
  }
  return *oStatus;
}