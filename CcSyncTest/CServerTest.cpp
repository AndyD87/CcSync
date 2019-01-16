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
 * @brief     Implemtation of class CServerTest
 */
#include "CServerTest.h"
#include "CcProcess.h"
#include "CcIODevice.h"
#include "CcByteArray.h"
#include "CcKernel.h"
#include "CcFileSystem.h"
#include "CcDirectory.h"

CServerTest::CServerTest( void )
{
  CcString sApplicationPath = CcKernel::getWorkingDir();
  CcString sConfigDir = CcKernel::getWorkingDir();
  sConfigDir.appendPath("CServerTest");
  if (CcDirectory::create(sConfigDir, true))
  {
    sApplicationPath.appendPath("CcSyncServer");
#ifdef WINDOWS
    sApplicationPath.append(".exe");
#endif
  }
  m_pServer = new CTestServer(sApplicationPath, sConfigDir);

  appendTestMethod("Test if server can be exectued", &CServerTest::testServerProc);
  appendTestMethod("Test if configure can fail", &CServerTest::testConfigureFailed);
  CCMONITORNEW(m_pServer);
}

CServerTest::~CServerTest( void )
{
  CCDELETE(m_pServer);
}

bool CServerTest::testServerProc()
{
  return m_pServer->serverExists();
}

bool CServerTest::testConfigureFailed()
{
  bool bSuccess = true;
  CcString sApplicationPath = CcKernel::getWorkingDir();
  CcString sConfigDir = CcKernel::getWorkingDir();
  sConfigDir.appendPath("CServerTest");
  if(CcDirectory::create(sConfigDir, true))
  {
    sApplicationPath.appendPath("CcSyncServer");
  #ifdef WINDOWS
    sApplicationPath.append(".exe");
  #endif
    CcProcess oServerRun(sApplicationPath);
    oServerRun.addArgument("configure");
    oServerRun.addArgument("--config-dir");
    oServerRun.addArgument(sConfigDir);
    oServerRun.start();
    if (oServerRun.waitForRunning(CcDateTimeFromSeconds(1)))
    {
      oServerRun.pipe().writeLine("Test");
      oServerRun.pipe().writeLine("test");
      oServerRun.pipe().writeLine("test");
      oServerRun.pipe().writeLine("");
      if (oServerRun.waitForExit(CcDateTimeFromSeconds(1)))
      {
        CCERROR("Succeeded to setup CcSyncServer but it should fail");
        bSuccess = false;
      }
    }
    else
    {
      CCERROR("Succeeded to setup CcSyncServer but it should fail");
      bSuccess = false;
    }
  }
  else
  {
    CCERROR("Failed to create testconfig dir");
    bSuccess = false;
  }
  return bSuccess;
}
