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
#include "IIoDevice.h"
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
    m_oClientProc.pipe().writeLine("new");
    m_oClientProc.pipe().writeLine(sUsername);
    m_oClientProc.pipe().writeLine(sServerName);
    m_oClientProc.pipe().writeLine(sServerPort);
    m_oClientProc.pipe().writeLine(sPassword);
    m_oClientProc.pipe().writeLine("exit");
    oStatus = m_oClientProc.waitForExit(CcDateTimeFromSeconds(1));
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
    CcString sData = readUntil(sUsername + "]:");
    // Wait for input
    oStatus = EStatus::UserLoginFailed;

    if(sData.endsWith(sUsername+"]:"))
    {
      oStatus = EStatus::AllOk;
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
  m_oClientProc.pipe().writeLine("sync");
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
  CcString sData = readUntil(m_sUsername + "]:");

  if (CcDirectory::exists(m_sSyncDirs.last()))
  {
    bSuccess = true;
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
  CcString sData = readUntil("[Admin]:");
  if(sData.length() > 0)
  {
    m_oClientProc.pipe().writeLine("stop");
    sData = readUntil("/"+m_sUsername + "]:", CcDateTimeFromSeconds(10));
    if(sData.endsWith("/"+m_sUsername + "]:"))
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

CcString CTestClient::readUntil(const CcString& sStringEnd, const CcDateTime& oTimeout)
{
  CcString sData;
  CcDateTime oCoundDown = oTimeout;
  while (oCoundDown.timestampUs() > 0)
  {
    oCoundDown.addSeconds(-1);
    CcKernel::delayS(1);
    sData += m_oClientProc.pipe().readAll();
    sData.trim();
    if (sData.endsWith(sStringEnd))
    {
      break;
    }
  }
  return sData;
}

bool CTestClient::readUntilSucceeded(const CcString& sStringEnd)
{
  bool bSuccess = false;
  CcString sData = readUntil(sStringEnd);
  if (sData.endsWith(sStringEnd))
  {
    bSuccess = true;
  }
  else
  {
    CcTestFramework::writeError("Last Output:");
    CcTestFramework::writeError(sData);
  }
  return bSuccess;
}
