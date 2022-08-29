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
 * @file
 * @copyright Andreas Dirmeier (C) 2017
 * @author    Andreas Dirmeier
 * @par       Web: http://coolcow.de
 * @version   0.01
 * @date      2016-04
 * @par       Language   C++ ANSI V3
 * @brief     Implemtation of class CcSyncLog
 */
#include "CcSyncLog.h"
#include "CcConsole.h"
#include "CcString.h"
#include "CcKernel.h"
#include "CcFile.h"
#include "CcDirectory.h"
#include "CcSyncConsole.h"

bool CcSyncLog::s_bLogsAvailable = false;
bool CcSyncLog::s_bConsoleOutput = true;
CcString CcSyncLog::s_sLocation;
CcString CcSyncLog::s_sLocationClient;
CcString CcSyncLog::s_sLocationServer;
CcString CcSyncLog::s_sLocationCommon;
#ifdef DEBUG
ESyncLogLevel CcSyncLog::s_eLogLevel = ESyncLogLevel::Debug;
#else
ESyncLogLevel CcSyncLog::s_eLogLevel = ESyncLogLevel::Warning;
#endif

bool CcSyncLog::initPaths()
{
  if(s_bLogsAvailable == false)
  {
    s_sLocation = CcKernel::getUserDataDir();
    s_sLocation.appendPath("CcSync");
    s_sLocation.appendPath("Logs");
    s_sLocationClient = s_sLocation;
    s_sLocationServer = s_sLocation;
    s_sLocationCommon = s_sLocation;
    s_sLocationClient.appendPath("Client.log");
    s_sLocationServer.appendPath("Server.log");
    s_sLocationCommon.appendPath("Common.log");
    s_bLogsAvailable = true;
  }
  return s_bLogsAvailable;
}

void CcSyncLog::writeMessage(const CcString& sMessage, ESyncLogTarget eTarget)
{
  if (initPaths())
  {
    switch (eTarget)
    {
      case ESyncLogTarget::Client:
        writeFile(s_sLocationClient, sMessage);
        break;
      case ESyncLogTarget::Server:
        writeFile(s_sLocationServer, sMessage);
        break;
      case ESyncLogTarget::Common:
        writeFile(s_sLocationCommon, sMessage);
        break;
      default:
        break;
    }
  }
  if (s_bConsoleOutput)
  {
    CcSyncConsole::writeLine(sMessage);
  }
}

void CcSyncLog::writeDebug(const CcString& sMessage, ESyncLogTarget eTarget)
{
  if (s_eLogLevel <= ESyncLogLevel::Debug)
  {
    CcString sMessageWrite;
    sMessageWrite << "[dbg ] " << sMessage;
    writeMessage(sMessageWrite, eTarget);
  }
}

void CcSyncLog::writeInfo(const CcString& sMessage, ESyncLogTarget eTarget)
{
  if (s_eLogLevel <= ESyncLogLevel::Debug)
  {
    CcString sMessageWrite;
    sMessageWrite << "[info] " << sMessage;
    writeMessage(sMessageWrite, eTarget);
  }
}

void CcSyncLog::writeWarning(const CcString& sMessage, ESyncLogTarget eTarget)
{
  if (s_eLogLevel <= ESyncLogLevel::Warning)
  {
    CcString sMessageWrite;
    sMessageWrite << "[warn] " << sMessage;
    writeMessage(sMessageWrite, eTarget);
  }
}

void CcSyncLog::writeError(const CcString& sMessage, ESyncLogTarget eTarget)
{
  if (s_eLogLevel <= ESyncLogLevel::Error)
  {
    CcString sMessageWrite;
    sMessageWrite << "[err ] " << sMessage;
    writeMessage(sMessageWrite, eTarget);
  }
}

void CcSyncLog::writeFile(const CcString& sPath, const CcString sMessage)
{
  CcFile oFile(sPath);
  bool bSuccess = false;
  if (oFile.exists())
  {
    bSuccess = oFile.open(EOpenFlags::Append);
  }
  else
  {
    if (CcDirectory::exists(s_sLocation) ||
      CcDirectory::create(s_sLocation, true))
    {
      bSuccess = oFile.open(EOpenFlags::Write);
    }
  }
  if (bSuccess)
  {
    oFile.writeLine(sMessage);
    oFile.close();
  }
}