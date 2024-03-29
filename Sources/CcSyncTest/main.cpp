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
 * @brief    Development default CLI-Application for testing new Implementations
 */

#include "CcBase.h"
#include "CcKernel.h"
#include "CcConsole.h"
#include "CcTestFramework.h"

#include "CServerTest.h"
#include "CClientTest.h"
#include "CSyncTest.h"

#include "CcProcess.h"

// Application entry point. 
int main(int argc, char **argv)
{
  int iReturn = 0;
  int iNumberOfTests = 1;

  do
  {
    CcTestFramework::init(argc, argv);
    CcConsole::writeLine("Start: CcSyncTest");

    CcTestFramework_addTest(CServerTest);
    CcTestFramework_addTest(CClientTest);
    CcTestFramework_addTest(CSyncTest);

    CcTestFramework::runTests();
  } while((iReturn = CcTestFramework::deinit()) == 0 && --iNumberOfTests);
  return iReturn;
}
