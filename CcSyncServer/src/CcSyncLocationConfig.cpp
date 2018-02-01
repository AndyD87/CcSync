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
 * @par       Web: http://coolcow.de
 * @version   0.01
 * @date      2016-04
 * @par       Language   C++ ANSI V3
 * @brief     Implemtation of class CcSyncServerLocationConfigClientCommand
 */
#include "CcSyncGlobals.h"
#include "CcSyncServerLocationConfig.h"
#include "CcXml/CcXmlNode.h"
#include "CcXml/CcXmlUtil.h"
#include "CcStringUtil.h"
#include "CcSyncLog.h"
#include "CcSyncServerConfig.h"

CcSyncServerLocationConfig::CcSyncServerLocationConfig(CcSyncServerConfig* pAccountConfig) :
  m_pServerConfig(pAccountConfig)
{

}

CcSyncServerLocationConfig::CcSyncServerLocationConfig(const CcString& sPath, CcSyncServerConfig* pAccountConfig) :
  m_sPath(sPath),
  m_pServerConfig(pAccountConfig)
{

}

CcSyncServerLocationConfig::CcSyncServerLocationConfig(const CcSyncServerLocationConfig& oToCopy)
{
  operator=(oToCopy);
}

CcSyncServerLocationConfig::CcSyncServerLocationConfig(CcSyncServerLocationConfig&& oToMove)
{
  operator=(std::move(oToMove));
}

bool CcSyncServerLocationConfig::readConfig(CcXmlNode& pLocationNode)
{
  bool bRet = false;
  CcXmlNode& pTempNode = pLocationNode.getNode(CcSyncGlobals::Server::ConfigTags::LocationPath);
  if (pTempNode.isNotNull())
  {
    m_sPath = pTempNode.getValue();
    bRet = true;
  }
  m_pLocationNode = &pLocationNode;
  return bRet;
}

bool CcSyncServerLocationConfig::writeConfig(CcXmlNode& rParentNode)
{
  CcXmlNode oLocationNode("Location");
  CcXmlNode oLocationNodeName("Path", m_sPath);
  oLocationNode.append(std::move(oLocationNodeName));
  rParentNode.append(std::move(oLocationNode));
  m_pLocationNode = &rParentNode.getLastAddedNode();
  return writeConfigFile();
}

CcSyncServerLocationConfig& CcSyncServerLocationConfig::operator=(const CcSyncServerLocationConfig& oToCopy)
{
  m_sPath = oToCopy.m_sPath;
  m_pServerConfig = oToCopy.m_pServerConfig;
  m_pLocationNode = oToCopy.m_pLocationNode;
  return *this;
}

CcSyncServerLocationConfig& CcSyncServerLocationConfig::operator=(CcSyncServerLocationConfig&& oToMove)
{
  if (this != &oToMove)
  {
    m_sPath = std::move(oToMove.m_sPath);
    m_pServerConfig = oToMove.m_pServerConfig;
    m_pLocationNode = oToMove.m_pLocationNode;
    oToMove.m_pServerConfig = nullptr;
    oToMove.m_pLocationNode = nullptr;
  }
  return *this;
}

bool CcSyncServerLocationConfig::writeConfigFile()
{
  if (m_pServerConfig != nullptr)
  {
    return m_pServerConfig->writeConfigFile();
  }
  else
  {
    CcSyncLog::writeError("CcSyncAccountConfig tried to write config file to NULL");
    return false;
  }
}
