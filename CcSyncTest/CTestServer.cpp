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
#include "IIo.h"
#include "CcTestFramework.h"
#include "CcKernel.h"
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
  if (m_oServerProc.waitForRunning(CcDateTimeFromSeconds(10)))
  {
    if (!readUntilSucceeded("Administrator:")) 
		CcTestFramework::writeError("No Administrator prompt");
    else
    {
      m_oServerProc.pipe().writeLine(sUsername);
      if (!readUntilSucceeded(":")) 
		  CcTestFramework::writeError("No Username prompt");
      else
      {
        m_oServerProc.pipe().writeLine(sPassword);
        if (!readUntilSucceeded(":")) 
			CcTestFramework::writeError("No Password prompt");
        else
        {
          m_oServerProc.pipe().writeLine(sPassword);
          if (!readUntilSucceeded(":")) 
			  CcTestFramework::writeError("No Password repeat prompt");
          else
          {
            m_oServerProc.pipe().writeLine(sPort);
            CcKernel::delayMs(1000);
            if (!readUntilSucceeded(":")) 
				CcTestFramework::writeError("No Port prompt");
            else
            {
              m_oServerProc.pipe().writeLine(sPath);
              m_oServerProc.pipe().writeLine("y");
              if (m_oServerProc.waitForExit(CcSyncTestGlobals::DefaultSyncTimeout))
              {
                bSuccess = m_oServerProc.getExitCode();
              }
              else
              {
                CcTestFramework::writeError("Server didn't stop as expected:");
                CcTestFramework::writeError(m_oServerProc.pipe().readAll());
                m_oServerProc.stop();
              }
            }
          }
        }
      }
    }
  }
  else
  {
    CcTestFramework::writeError("Server didn't stop as expected:");
    CcTestFramework::writeError(m_oServerProc.pipe().readAll());
  }
  return bSuccess;
}

bool CTestServer::start()
{
  bool bStarted = false;
  resetArguments();
  addArgument("start");
  m_oServerProc.start();
  if(m_oServerProc.waitForState(EThreadState::Running))
  {
    if(readUntilSucceeded(CcSyncTestGlobals::Server::ServerStarted))
    {
      CcKernel::sleep(500);
      bStarted = true;
    }
  }
  return bStarted;
}

bool CTestServer::stop()
{
  m_oServerProc.stop();
  bool bSuccess = m_oServerProc.waitForState(EThreadState::Stopped, 
                                             CcSyncTestGlobals::DefaultSyncTimeout
  );
  CcKernel::sleep(500);
  return bSuccess;
}

CcString CTestServer::readAllData()
{
  return m_oServerProc.pipe().readAll();
}

CcString CTestServer::readWithTimeout(const CcString& sStringEnd, CcStatus& oStatus, const CcDateTime& oTimeout)
{
  CcString sData;
  CcDateTime oCountDown = oTimeout;
  while (oCountDown.timestampUs() > 0)
  {
    oCountDown.addMSeconds(-100);
    CcKernel::delayMs(100);
    sData += m_oServerProc.pipe().readAll();
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

bool CTestServer::readUntilSucceeded(const CcString& sStringEnd, CcStatus* oStatus)
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
