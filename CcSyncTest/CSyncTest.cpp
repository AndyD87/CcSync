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
 * @brief     Implemtation of class CSyncTest
 */
#include "CSyncTest.h"
#include "CcProcess.h"
#include "CcIODevice.h"
#include "CcByteArray.h"
#include "CcKernel.h"
#include "CcFileSystem.h"
#include "CcDirectory.h"
#include "CTestServer.h"
#include "CTestClient.h"

class CSyncTestPrivate
{
public:
  CcString sTestDir;
  CcString sTestServerDir;
  CcString sTestClient1Dir;
  CcString sTestClient2Dir;
  CcString sServerAppPath;
  CcString sClientAppPath;
  CcSharedPointer<CTestServer> pServer;
  CcSharedPointer<CTestClient> pClient1;
  CcSharedPointer<CTestClient> pClient2;

  static const CcString sAdminName;
  static const CcString sAdminPW;
  static const CcString sServerName;
  static const CcString sServerPort;

};

const CcString CSyncTestPrivate::sAdminName("Admin");
const CcString CSyncTestPrivate::sAdminPW("AdminPW");
const CcString CSyncTestPrivate::sServerName("127.0.0.1");
const CcString CSyncTestPrivate::sServerPort("27499");

CSyncTest::CSyncTest( void ) :
  CcTest("CcSyncTest")
{
  m_pPrivate = new CSyncTestPrivate();
  CCMONITORNEW(m_pPrivate);
  m_pPrivate->sServerAppPath = CcTestFramework::getBinaryDir();
  m_pPrivate->sClientAppPath = CcTestFramework::getBinaryDir();
  m_pPrivate->sServerAppPath.appendPath("CcSyncServer");
  m_pPrivate->sClientAppPath.appendPath("CcSyncClient");
#ifdef WINDOWS
  m_pPrivate->sServerAppPath.append(".exe");
  m_pPrivate->sClientAppPath.append(".exe");
#endif

  appendTestMethod("Test if environment for sync test is successfully created", &CSyncTest::testEnvironment);
  appendTestMethod("Setup TestServer", &CSyncTest::testSetupServer); 
  appendTestMethod("Setup TestClient1", &CSyncTest::testSetupClient1);
  appendTestMethod("Setup TestClient2", &CSyncTest::testSetupClient2);
  appendTestMethod("Start TestServer", &CSyncTest::testStartServer);
  appendTestMethod("Check login TestClient1", &CSyncTest::testCheckLoginClient1);
  appendTestMethod("Check login TestClient2", &CSyncTest::testCheckLoginClient2);
  appendTestMethod("Login TestClient1", &CSyncTest::testLoginClient1);
  appendTestMethod("Login TestClient2", &CSyncTest::testLoginClient2);
  appendTestMethod("Create first Testdir", &CSyncTest::testCreateTestDir);
  appendTestMethod("Write testdata to client 1", &CSyncTest::testWriteTestDataClient1);
  appendTestMethod("Sync TestClient1", &CSyncTest::testSyncClient1);
  appendTestMethod("Sync TestClient2", &CSyncTest::testSyncClient2);
  appendTestMethod("Stop TestServer", &CSyncTest::testStopServer);
}

CSyncTest::~CSyncTest( void )
{
  CCDELETE(m_pPrivate);
}

bool CSyncTest::testEnvironment()
{
  bool bSuccess = false;
  m_pPrivate->sTestDir = CcTestFramework::getTemporaryDir();
  m_pPrivate->sTestDir.appendPath("CSyncTest");
  if (CcDirectory::create(m_pPrivate->sTestDir, true))
  {
    m_pPrivate->sTestServerDir = m_pPrivate->sTestDir;
    m_pPrivate->sTestServerDir.appendPath("CTestServer");
    m_pPrivate->sTestClient1Dir = m_pPrivate->sTestDir;
    m_pPrivate->sTestClient1Dir.appendPath("CTestClient1");
    m_pPrivate->sTestClient2Dir = m_pPrivate->sTestDir;
    m_pPrivate->sTestClient2Dir.appendPath("CTestClient2");
    if (CcDirectory::create(m_pPrivate->sTestServerDir, true) &&
      CcDirectory::create(m_pPrivate->sTestClient1Dir, true) &&
      CcDirectory::create(m_pPrivate->sTestClient2Dir, true))
    {
      if (CcFile::exists(m_pPrivate->sServerAppPath))
      {
        m_pPrivate->pServer = new CTestServer(m_pPrivate->sServerAppPath, m_pPrivate->sTestServerDir);
        if (CcFile::exists(m_pPrivate->sClientAppPath))
        {
          m_pPrivate->pClient1 = new CTestClient(m_pPrivate->sClientAppPath, m_pPrivate->sTestClient1Dir);
          m_pPrivate->pClient2 = new CTestClient(m_pPrivate->sClientAppPath, m_pPrivate->sTestClient2Dir);
          bSuccess = true;
        }
        else
        {
          CcTestFramework::writeError("CcSyncServer not found: " + m_pPrivate->sServerAppPath);
        }
      }
      else
      {
        CcTestFramework::writeError("CcSyncServer not found: " + m_pPrivate->sServerAppPath);
      }
    }
    else
    {
      CcTestFramework::writeError("Failed to create test directories");
    }
  }
  else
  {
    CcTestFramework::writeError("Failed to create CSyncTest directory");
  }
  return bSuccess;
}

bool CSyncTest::testSetupServer()
{
  bool bSuccess = true;
  bSuccess = m_pPrivate->pServer->createConfiguration(
    m_pPrivate->sServerPort,
    m_pPrivate->sAdminName,
    m_pPrivate->sAdminPW,
    m_pPrivate->sTestServerDir
  );
  return bSuccess;
}

bool CSyncTest::testSetupClient1()
{
  bool bSuccess = true;
  bSuccess = m_pPrivate->pClient1->addNewServer(
    m_pPrivate->sServerName,
    m_pPrivate->sServerPort,
    m_pPrivate->sAdminName,
    m_pPrivate->sAdminPW
  );
  return bSuccess;
}

bool CSyncTest::testSetupClient2()
{
  bool bSuccess = true;
  bSuccess = m_pPrivate->pClient2->addNewServer(
    m_pPrivate->sServerName,
    m_pPrivate->sServerPort,
    m_pPrivate->sAdminName,
    m_pPrivate->sAdminPW
  );
  return bSuccess;
}

bool CSyncTest::testStartServer()
{
  return m_pPrivate->pServer->start();
}

bool CSyncTest::testStopServer()
{
  return m_pPrivate->pServer->stop();
}

bool CSyncTest::testCheckLoginClient1()
{
  bool bSuccess = m_pPrivate->pClient1->checkLogin(
        m_pPrivate->sServerName,
        m_pPrivate->sAdminName);
  return bSuccess;
}

bool CSyncTest::testCheckLoginClient2()
{
  bool bSuccess = m_pPrivate->pClient2->checkLogin(
        m_pPrivate->sServerName,
        m_pPrivate->sAdminName);
  return bSuccess;
}

bool CSyncTest::testLoginClient1()
{
  bool bSuccess = m_pPrivate->pClient1->login(
    m_pPrivate->sServerName,
    m_pPrivate->sAdminName);
  return bSuccess;
}

bool CSyncTest::testLoginClient2()
{
  bool bSuccess = m_pPrivate->pClient2->login(
    m_pPrivate->sServerName,
    m_pPrivate->sAdminName);
  return bSuccess;
}

bool CSyncTest::testSyncClient1()
{
  bool bSuccess = m_pPrivate->pClient1->sync();
  return bSuccess;
}

bool CSyncTest::testSyncClient2()
{
  bool bSuccess = m_pPrivate->pClient2->sync();
  return bSuccess;
}

bool CSyncTest::testCreateTestDir()
{
  bool bSuccess = m_pPrivate->pClient1->createSyncDirectory("TestDir");
  if (bSuccess)
  {
    bSuccess = m_pPrivate->pClient2->createSyncDirectory("TestDir");
  }
  return bSuccess;
}

bool CSyncTest::testWriteTestDataClient1()
{
  bool bSuccess = m_pPrivate->pClient1->createDirectory("TestDir1");
  if (bSuccess)
  {
    bSuccess = m_pPrivate->pClient1->createFile("TestDir2/File.test", "TestContent");
  }
  return bSuccess;
}
