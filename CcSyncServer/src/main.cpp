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
 * @copyright Andreas Dirmeier (C) 2017
 * @author    Andreas Dirmeier
 * @par       Web: http://coolcow.de
 * @version    0.01
 * @date       2016-04
 * @par        Language   C++ ANSI V3
 * @file     main.cpp
 * @brief    Development default CLI-Application for testing new Implementations
 */

#include "CcBase.h"
#include "CcKernel.h"
#include "CcArguments.h"
#include "CcSyncServer.h"
#include "CcConsole.h"
#include "CcVersion.h"

void printHelp()
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

int main(int argc, char **argv)
{
  int iReturn = 0;
  CcArguments oArguments(argc, argv);
  if (oArguments.size() > 1)
  {
    CcSyncServer oSyncServer;
    // select mode
    if (oArguments[1] == "daemon")
    {
      switch (CcKernel::initService())
      {
        case -1:
          CCERROR("Starting Service failed");
          exit(-1);
        case 0:
          break;
        default:
          CCDEBUG("Service started, close main application.");
          exit(0);
          break;
      }
      iReturn = oSyncServer.exec();
    }
    else if (oArguments[1] == "configure")
    {
      if (oSyncServer.createConfig())
        iReturn = 0;
      else
        iReturn = 1;
    }
    else if (oArguments[1] == "start")
    {
      CcKernel::initCLI();
      iReturn = oSyncServer.exec();
    }
    else if (oArguments[1] == "info")
    {
      CcConsole::writeLine("CcSyncServer Version: " + oSyncServer.getVersion().getVersionString());
      CcConsole::writeLine("CcOS         Version: " + CcKernel::getVersion().getVersionString());
    }
    else
    {
      if (oArguments[1] != "help" &&
          oArguments[1] != "-h")
      {
        CcConsole::writeLine("ERROR, wrong command: " + oArguments[1]);
        iReturn = -1;
      }
      printHelp();
    }
  }
  else
  {
    iReturn = -1;
    printHelp();
  }
  return iReturn;
}
