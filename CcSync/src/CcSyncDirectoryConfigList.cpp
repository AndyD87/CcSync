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
 * @brief     Implemtation of class CcSyncDirectoryConfigList
 */
#include "CcSyncDirectoryConfigList.h"

CcSyncDirectoryConfigList::CcSyncDirectoryConfigList( void )
{
}

CcSyncDirectoryConfigList::~CcSyncDirectoryConfigList( void )
{
}

CcSyncDirectoryConfig * CcSyncDirectoryConfigList::getDirectoryByName(const CcString& sDirectoryName)
{
  for (CcSyncDirectoryConfig& rDirectory : *this)
  {
    if (rDirectory.getName() == sDirectoryName)
      return &rDirectory;
  }
  return nullptr;
}

bool CcSyncDirectoryConfigList::containsDirectory(const CcString& sDirectoryName) const
{
  bool bRet = false;
  for (const CcSyncDirectoryConfig& rDirectory : *this)
  {
    if (rDirectory.getName() == sDirectoryName)
      return true;
  }
  return bRet;
}
