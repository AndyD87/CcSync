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
 * @page      CcSync
 * @subpage   CcSyncConsole
 *
 * @page      CcSyncConsole
 * @copyright Andreas Dirmeier (C) 2017
 * @author    Andreas Dirmeier
 * @par       Web: http://coolcow.de
 * @version   0.01
 * @date      2016-04
 * @par       Language   C++ ANSI V3
 * @brief     Class CcSyncConsole
 **/
#ifndef _CcSyncConsole_H_
#define _CcSyncConsole_H_

#include "CcBase.h"
#include "CcSync.h"
#include "CcSyncGlobals.h"

// forward declarations
class CcString;

/**
 * @brief Class impelmentation
 */
class CcSyncSHARED CcSyncConsole
{
public:
  static void writeLine(const CcString& sMessage);
  static void writeString(const CcString& sString);
  static void printHelpLine(const CcString& sCommand, size_t iNr, const CcString& sDescription);
  static CcString clientQuery();
  static CcString query(const CcString& sQuery);
  static CcString query(const CcString& sQuery, const CcString& sDefault);
  static CcString queryHidden(const CcString& sQuery);
  static CcString& getPrepend();
  static void setPrepend(const CcString& sPrepend);
private:
  static CcString s_sPrepend;
};

#endif /* _CcSyncConsole_H_ */
