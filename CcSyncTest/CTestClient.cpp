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
#include "CcIODevice.h"
#include "CcByteArray.h"
#include "CcTestFramework.h"
#include "CcKernel.h"

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

bool CTestClient::checkLogin(const CcString& sServerName, const CcString& sUsername)
{
  CcStatus oStatus;
  resetArguments();
  m_oClientProc.start();
  oStatus = m_oClientProc.waitForRunning(CcDateTimeFromSeconds(1));
  if (oStatus)
  {
    m_oClientProc.pipe().writeLine("login "+sUsername+"@"+sServerName);
    CcString sData;
    CcDateTime oCountDown  = CcDateTimeFromSeconds(5);
    // Wait for input
    oStatus = EStatus::UserLoginFailed;
    while(oCountDown.timestampUs() > 0)
    {
      oCountDown.addSeconds(-1);
      CcKernel::delayS(1);
      sData += m_oClientProc.pipe().readAll();
      sData.trim();
      if(sData.endsWith(sUsername+"]:"))
      {
        break;
      }
    }
    if(sData.endsWith(sUsername+"]:"))
    {
      m_oClientProc.pipe().writeLine("exit");
      m_oClientProc.pipe().writeLine("exit");
      oStatus = m_oClientProc.waitForExit(CcDateTimeFromSeconds(1));
    }
    // test if a wrong parameter would faild server
  }
  return oStatus;
}
