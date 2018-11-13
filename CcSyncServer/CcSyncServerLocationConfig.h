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
 * @subpage   CcSyncServerLocationConfig
 *
 * @page      CcSyncServerLocationConfig
 * @copyright Andreas Dirmeier (C) 2017
 * @author    Andreas Dirmeier
 * @par       Web: http://coolcow.de
 * @version   0.01
 * @date      2016-04
 * @par       Language   C++ ANSI V3
 * @brief     Class CcSyncServerLocationConfig
 **/
#ifndef _CcSyncServerLocationConfig_H_
#define _CcSyncServerLocationConfig_H_

#include "CcBase.h"
#include "CcSync.h"
#include "CcList.h"
#include "CcString.h"

class CcXmlNode;
class CcSyncServerConfig;
class CcSyncServerLocationConfig;
enum class EBackupLocation;

/**
 * @brief Class impelmentation
 */
class CcSyncServerLocationConfig
{
public:
  /**
   * @brief Constructor
   */
  CcSyncServerLocationConfig(CcSyncServerConfig* m_pAccountConfig);
  CcSyncServerLocationConfig(const CcString& sPath, CcSyncServerConfig* m_pAccountConfig);
  CcSyncServerLocationConfig(const CcSyncServerLocationConfig& oToCopy);
  CcSyncServerLocationConfig(CcSyncServerLocationConfig&& oToMove);

  CcSyncServerLocationConfig& operator=(const CcSyncServerLocationConfig& oToCopy);
  CcSyncServerLocationConfig& operator=(CcSyncServerLocationConfig&& oToMove);

  bool readConfig(CcXmlNode& pLocationNode);
  bool writeConfig(CcXmlNode& pLocationNode);
  const CcString& getPath() const
    { return m_sPath; }
  void setPath(const CcString& sPath)
    { m_sPath = sPath; }

  bool writeConfigFile();

private:

private:
  CcString  m_sPath;
  CcXmlNode*            m_pLocationNode  = nullptr;
  CcSyncServerConfig*   m_pServerConfig = nullptr;
};

#endif /* _CcSyncServerLocationConfig_H_ */
