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
 * @brief     Implemtation of class CTestServer
 */
#include "CTestServer.h"
#include "CcSyncTestGlobals.h"
#include "CcIODevice.h"
#include "CcTestFramework.h"
#include "CcByteArray.h"

CTestServer::CTestServer(const CcString& sServerExePath, const CcString& sConfigDir) :
  m_sConfigDir(sConfigDir)
{
  m_oServerProc.setApplication(sServerExePath);
  m_oServerProc.addArgument("--config-dir");
  m_oServerProc.addArgument(sConfigDir);
}

CTestServer::~CTestServer( void )
{
}

void CTestServer::resetArguments(void)
{
  m_oServerProc.clearArguments();
  m_oServerProc.addArgument("--config-dir");
  m_oServerProc.addArgument(m_sConfigDir);
}

bool CTestServer::serverExists(void)
{
  resetArguments();
  m_oServerProc.addArgument("help");
  CcStatus oStatus = m_oServerProc.exec(CcSyncTestGlobals::DefaultSyncTimeout);
  if (oStatus)
  {
    resetArguments();
    // test if a wrong parameter would faild server
    m_oServerProc.addArgument("hel");
    oStatus = m_oServerProc.exec(CcSyncTestGlobals::DefaultSyncTimeout);
    if (!oStatus && oStatus != EStatus::TimeoutReached)
    {
      oStatus = true;
    }
    else
    {
      oStatus = false;
    }
  }
  else
  {
    CcTestFramework::writeError("Help request failed, response:");
    CcTestFramework::writeError(CcString("  ") + m_oServerProc.pipe().readAll());
  }
  return oStatus;
}

bool CTestServer::createConfiguration(const CcString& sPort, const CcString& sUsername, const CcString& sPassword, const CcString& sPath)
{
  bool bSuccess = false;
  resetArguments();
  addArgument("configure");
  m_oServerProc.start();
  if (m_oServerProc.waitForRunning(CcDateTimeFromSeconds(1)))
  {
    CcString sAllData;
    do
    {
      sAllData += m_oServerProc.pipe().readAll();
    } while(!sAllData.contains("Administrator"));
    m_oServerProc.pipe().writeLine(sUsername);
    m_oServerProc.pipe().writeLine(sPassword);
    m_oServerProc.pipe().writeLine(sPassword);
    m_oServerProc.pipe().writeLine(sPort);
    m_oServerProc.pipe().writeLine(sPath);
    if (m_oServerProc.waitForExit(CcDateTimeFromSeconds(1)))
    {
      CcString sNewData = m_oServerProc.pipe().readAll();
      bSuccess = m_oServerProc.getExitCode();
    }
  }
  return bSuccess;
}

bool CTestServer::start()
{
  bool bStarted = false;
  resetArguments();
  addArgument("start");
  m_oServerProc.start();
  if(m_oServerProc.waitForState(EThreadState::Running, CcDateTimeFromSeconds(1)))
  {
    if(m_oServerProc.waitForState(EThreadState::Stopped, CcDateTimeFromSeconds(1)) == EStatus::TimeoutReached)
    {
      bStarted = true;
    }
  }
  return bStarted;
}

bool CTestServer::stop()
{
  m_oServerProc.stop();
  return m_oServerProc.waitForState(EThreadState::Stopped, CcDateTimeFromSeconds(1));
}
