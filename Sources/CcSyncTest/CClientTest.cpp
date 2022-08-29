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
 * @brief     Implemtation of class CClientTest
 */
#include "CClientTest.h"
#include "CcProcess.h"
#include "IIo.h"
#include "CcByteArray.h"
#include "CcKernel.h"
#include "CcFileSystem.h"
#include "CcDirectory.h"

CClientTest::CClientTest( void ) :
  CcTest("CClientTest")
{
  CcString sApplicationPath = CcTestFramework::getBinaryDir();
  CcString sConfigDir = CcTestFramework::getTemporaryDir();
  sConfigDir.appendPath("CClientTest");
  if (CcDirectory::create(sConfigDir, true))
  {
    sApplicationPath.appendPath("CcSyncClient" CC_DEBUG_EXTENSION);
#ifdef WINDOWS
    sApplicationPath.append(".exe");
#endif
  }
  CCNEW(m_pClient, CTestClient, sApplicationPath, sConfigDir);

  appendTestMethod("Test if client can be exectued", &CClientTest::testClientExists);
  appendTestMethod("Test if sync server is waiting for input", &CClientTest::testClientWaitInput);
}

CClientTest::~CClientTest( void )
{
  CCDELETE(m_pClient);
}

bool CClientTest::testClientExists()
{
  return m_pClient->clientExists();
}

bool CClientTest::testClientWaitInput()
{
  bool bSuccess = false;
  CcString sApplicationPath = CcTestFramework::getBinaryDir();
  CcString sConfigDir = CcTestFramework::getTemporaryDir();
  sConfigDir.appendPath("CClientTest");
  if(CcDirectory::create(sConfigDir, true))
  {
    sApplicationPath.appendPath("CcSyncClient" CC_DEBUG_EXTENSION);
  #ifdef WINDOWS
    sApplicationPath.append(".exe");
  #endif
    CcProcess oServerRun(sApplicationPath);
    oServerRun.addArgument("--config-dir");
    oServerRun.addArgument(sConfigDir);
    oServerRun.start();
    if (oServerRun.waitForRunning(CcDateTimeFromSeconds(1)))
    {
      if (oServerRun.waitForExit(CcDateTimeFromSeconds(1)))
      {
        CcTestFramework::writeError("Succeeded to setup CcSyncClient but it should fail");
      }
      else
      {
        bSuccess = true;
      }
    }
    else
    {
      CcTestFramework::writeError("Succeeded to setup CcSyncServer but it should fail");
    }
  }
  else
  {
    CcTestFramework::writeError("Failed to create testconfig dir");
  }
  return bSuccess;
}
