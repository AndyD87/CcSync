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
 * @subpage   CcSyncServerDirectory
 *
 * @page      CcSyncServerDirectory
 * @copyright Andreas Dirmeier (C) 2017
 * @author    Andreas Dirmeier
 * @par       Web: http://coolcow.de
 * @version   0.01
 * @date      2016-04
 * @par       Language   C++ ANSI V3
 * @brief     Class CcSyncServerDirectory
 **/
#ifndef _CcSyncServerDirectory_H_
#define _CcSyncServerDirectory_H_

#include "CcBase.h"
#include "CcSync.h"
#include "CcSyncUser.h"

class CcSyncDirectory;

/**
 * @brief Class impelmentation
 */
class CcSyncServerDirectory
{
public:
  /**
   * @brief Constructor
   */
  CcSyncServerDirectory(const CcSyncUser& oUser ):
    m_oUser(oUser)
    {}
  CcSyncServerDirectory(const CcSyncServerDirectory& oToCopy);
  CcSyncServerDirectory(CcSyncServerDirectory&& oToMove);
  ~CcSyncServerDirectory(void);

  CcSyncServerDirectory& operator=(const CcSyncServerDirectory& oToCopy);
  CcSyncServerDirectory& operator=(CcSyncServerDirectory&& oToMove);

  void rescanDirs(bool bDeep);

private: // Methods
  void doQueue(CcSyncDirectory& oCurrentDir);

private:
  CcSyncUser m_oUser;
};

#endif /* _CcSyncServerDirectory_H_ */
