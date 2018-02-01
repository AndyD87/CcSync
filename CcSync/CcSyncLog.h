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
 * @page      CcSync
 * @subpage   CcSyncLog
 *
 * @page      CcSyncLog
 * @copyright Andreas Dirmeier (C) 2017
 * @author    Andreas Dirmeier
 * @par       Web: http://coolcow.de
 * @version   0.01
 * @date      2016-04
 * @par       Language   C++ ANSI V3
 * @brief     Class CcSyncLog
 **/
#ifndef _CcSyncLog_H_
#define _CcSyncLog_H_

#include "CcBase.h"
#include "CcSync.h"
#include "CcString.h"
#include "CcSyncGlobals.h"

// forward declarations
class CcString;

enum class ESyncLogTarget
{
  Client,
  Server,
  Common
};

enum class ESyncLogLevel
{
  All = 0,
  Verbose,
  Debug,
  Info,
  Warning,
  Error
};

/**
 * @brief Class impelmentation
 */
class CcSyncSHARED CcSyncLog
{
public:
  static void writeMessage(const CcString& sMessage, ESyncLogTarget eTarget = ESyncLogTarget::Common);
  static void writeDebug(const CcString& sMessage, ESyncLogTarget eTarget = ESyncLogTarget::Common);
  static void writeInfo(const CcString& sMessage, ESyncLogTarget eTarget = ESyncLogTarget::Common);
  static void writeWarning(const CcString& sMessage, ESyncLogTarget eTarget = ESyncLogTarget::Common);
  static void writeError(const CcString& sMessage, ESyncLogTarget eTarget = ESyncLogTarget::Common);
  static void setLogLevel(ESyncLogLevel eNewLevel)
    {s_eLogLevel = eNewLevel;}
  static void disableConsoleOutput()
    {s_bConsoleOutput = false;}

private:
  static bool initPaths();
  static void writeFile(const CcString& sPath, const CcString sMessage);
private:
  static bool s_bLogsAvailable;
  static bool s_bConsoleOutput;
  static CcString s_sLocation;
  static CcString s_sLocationClient;
  static CcString s_sLocationServer;
  static CcString s_sLocationCommon;
  static ESyncLogLevel s_eLogLevel;
};

#endif /* _CcSyncLog_H_ */
