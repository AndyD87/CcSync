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
 * @brief     Implemtation of class CcSyncConsole
 */
#include "CcSyncConsole.h"
#include "CcConsole.h"
#include "CcString.h"

CcString CcSyncConsole::s_sPrepend = "[Client]";

void CcSyncConsole::writeLine(const CcString& sMessage)
{
  CcConsole::writeLine(sMessage);
}

void CcSyncConsole::writeString(const CcString& sMessage)
{
  CcConsole::writeString(sMessage);
}

size_t CcSyncConsole::clientQuery(CcString& sRead)
{
  return query(s_sPrepend, sRead);
}

size_t CcSyncConsole::query(const CcString& sQuery, CcString& sRead)
{
  CcConsole::writeString(sQuery + ": ");
  return CcConsole::readLine(sRead);
}

size_t CcSyncConsole::query(const CcString& sQuery, const CcString& sDefault, CcString& sRead)
{
  CcConsole::writeString(sQuery + ": ");
  size_t uiRead = CcConsole::readLine(sRead);
  if (sRead.trim().length() == 0)
  {
    sRead = sDefault;
    uiRead = sDefault.length();
  }
  return uiRead;
}

size_t CcSyncConsole::queryHidden(const CcString& sQuery, CcString& sRead)
{
  CcConsole::writeString(sQuery + ": ");
  return CcConsole::readLineHidden(sRead);
}

void CcSyncConsole::printHelpLine(const CcString& sCommand, size_t iNr, const CcString& sDescription)
{
  CcSyncConsole::writeString(sCommand);
  for (size_t i = sCommand.length(); i < iNr; i++)
    CcSyncConsole::writeString(" ");
  CcSyncConsole::writeLine(sDescription);
}

CcString& CcSyncConsole::getPrepend()
{
  return s_sPrepend;
}

void CcSyncConsole::setPrepend(const CcString& sPrepend)
{
  s_sPrepend = sPrepend;
}
