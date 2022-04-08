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
 * @brief     Implemtation of class CcSyncDirectoryConfig
 */
#include "CcSyncDirectoryConfig.h"
#include "CcStringUtil.h"
#include "CcSyncGlobals.h"
#include "Xml/CcXmlNode.h"
#include "Json/CcJsonObject.h"
#include "CcDirectory.h"
#include "CcSyncAccountConfig.h"
#include "CcKernel.h"
#include "CcUser.h"
#include "CcUserList.h"

CcSyncDirectoryConfig::CcSyncDirectoryConfig(CcSyncAccountConfig* pAccountConfig) :
  m_pAccountConfig(pAccountConfig)
{
}

CcSyncDirectoryConfig::CcSyncDirectoryConfig(const CcString& sName, const CcString& sLocation, CcSyncAccountConfig *pAccountNode):
  m_sName(sName),
  m_sLocation(sLocation),
  m_pAccountConfig(pAccountNode)
{

}

CcSyncDirectoryConfig::CcSyncDirectoryConfig(const CcJsonObject& pJsonNode)
{
  parseJsonNode(pJsonNode);
}

CcSyncDirectoryConfig::CcSyncDirectoryConfig(const CcSyncDirectoryConfig& oToCopy)
{
  operator=(oToCopy);
}

CcSyncDirectoryConfig::CcSyncDirectoryConfig(CcSyncDirectoryConfig&& oToMove)
{
  operator=(std::move(oToMove));
}

CcSyncDirectoryConfig::~CcSyncDirectoryConfig(void)
{
}

CcSyncDirectoryConfig& CcSyncDirectoryConfig::operator=(const CcSyncDirectoryConfig& oToCopy)
{
  m_sLocation = oToCopy.m_sLocation;
  m_sName = oToCopy.m_sName;
  m_sBackupCommand = oToCopy.m_sBackupCommand;
  m_sRestoreCommand = oToCopy.m_sRestoreCommand;
  m_uiUser = oToCopy.m_uiUser;
  m_uiGroup = oToCopy.m_uiGroup;
  m_pAccountConfig = oToCopy.m_pAccountConfig;
  m_pDirectoryNode = oToCopy.m_pDirectoryNode;
  return *this;
}

CcSyncDirectoryConfig& CcSyncDirectoryConfig::operator=(CcSyncDirectoryConfig&& oToMove)
{
  if (this != &oToMove)
  {
    m_sLocation = std::move(oToMove.m_sLocation);
    m_sName = std::move(oToMove.m_sName);
    m_sBackupCommand = std::move(oToMove.m_sBackupCommand);
    m_sRestoreCommand = std::move(oToMove.m_sRestoreCommand);
    m_uiUser  = oToMove.m_uiUser;
    m_uiGroup = oToMove.m_uiGroup;
    m_pAccountConfig = oToMove.m_pAccountConfig;
    m_pDirectoryNode = oToMove.m_pDirectoryNode;
  }
  return *this;
}

bool CcSyncDirectoryConfig::operator==(const CcSyncDirectoryConfig& oToCompare) const
{
  bool bRet = false;
  CCUNUSED(oToCompare);
  return bRet;
}

bool CcSyncDirectoryConfig::operator!=(const CcSyncDirectoryConfig& oToCompare) const
{
  return !operator==(oToCompare);
}

void CcSyncDirectoryConfig::parseXmlNode(CcXmlNode& pXmlNode)
{
  m_pDirectoryNode = &pXmlNode;
  CcXmlNode& rNameNode = pXmlNode[CcSyncGlobals::Client::ConfigTags::Name];
  if (rNameNode.isNotNull())
    m_sName = rNameNode.innerText();
  CcXmlNode& rLocationNode = pXmlNode[CcSyncGlobals::Client::ConfigTags::DirectoryLocation];
  if (rLocationNode.isNotNull())
    m_sLocation.setOsPath(rLocationNode.innerText());
  CcXmlNode& rBackupNode = pXmlNode[CcSyncGlobals::Client::ConfigTags::DirectoryBackupCommand];
  if (rBackupNode.isNotNull())
    m_sBackupCommand = rBackupNode.innerText();
  CcXmlNode& rRestoreNode = pXmlNode[CcSyncGlobals::Client::ConfigTags::DirectoryRestoreCommand];
  if (rRestoreNode.isNotNull())
    m_sRestoreCommand = rRestoreNode.innerText();
  CcXmlNode& rUserNode = pXmlNode[CcSyncGlobals::Client::ConfigTags::DirectoryUser];
  if (rUserNode.isNotNull())
    m_uiUser = this->userIdFromString(rUserNode.innerText());
  CcXmlNode& rGroupNode = pXmlNode[CcSyncGlobals::Client::ConfigTags::DirectoryGroup];
  if (rGroupNode.isNotNull())
    m_uiGroup = this->groupIdFromString(rGroupNode.innerText());
}

void CcSyncDirectoryConfig::parseJsonNode(const CcJsonObject& rJsonNode)
{
  const CcJsonNode& pTempNode = rJsonNode[CcSyncGlobals::Client::ConfigTags::Name];
  if (pTempNode.isValue())
      m_sName = pTempNode.getValue().getString();
  const CcJsonNode& pLocationNode = rJsonNode[CcSyncGlobals::Client::ConfigTags::DirectoryLocation];
  if (pLocationNode.isValue())
  {
    m_sLocation.setOsPath(pLocationNode.getValue().getString());
  }
  const CcJsonNode& pBCNode = rJsonNode[CcSyncGlobals::Client::ConfigTags::DirectoryBackupCommand];
  if (pBCNode.isValue())
    m_sBackupCommand = pBCNode.getValue().getString();
  const CcJsonNode& pRCNode = rJsonNode[CcSyncGlobals::Client::ConfigTags::DirectoryRestoreCommand];
  if (pRCNode.isValue())
    m_sRestoreCommand = pRCNode.getValue().getString();
}

bool CcSyncDirectoryConfig::writeConfig(CcXmlNode& pXmlNode)
{
  CcXmlNode oDirectoryNode(CcSyncGlobals::Client::ConfigTags::Directory);
  CcXmlNode oDirectoryNameNode(CcSyncGlobals::Client::ConfigTags::DirectoryName);
  oDirectoryNameNode.setInnerText(m_sName);
  CcXmlNode oDirectoryLocationNode(CcSyncGlobals::Client::ConfigTags::DirectoryLocation);
  oDirectoryLocationNode.setInnerText(m_sLocation);
  oDirectoryNode.append(std::move(oDirectoryNameNode));
  oDirectoryNode.append(std::move(oDirectoryLocationNode));
  CcXmlNode oBackupNode(CcSyncGlobals::Client::ConfigTags::DirectoryBackupCommand);
  oBackupNode.setInnerText(m_sBackupCommand);
  CcXmlNode oRestoreNode(CcSyncGlobals::Client::ConfigTags::DirectoryRestoreCommand);
  oRestoreNode.setInnerText(m_sRestoreCommand);
  CcXmlNode oUserNode(CcSyncGlobals::Client::ConfigTags::DirectoryUser);
  oUserNode.setInnerText(CcString::fromNumber(m_uiUser));
  CcXmlNode oGroupNode(CcSyncGlobals::Client::ConfigTags::DirectoryGroup);
  oGroupNode.setInnerText(CcString::fromNumber(m_uiGroup));
  oDirectoryNode.append(std::move(oBackupNode));
  oDirectoryNode.append(std::move(oRestoreNode));
  oDirectoryNode.append(std::move(oUserNode));
  oDirectoryNode.append(std::move(oGroupNode));
  pXmlNode.append(std::move(oDirectoryNode));
  m_pDirectoryNode = &pXmlNode.getLastAddedNode();
  return writeConfigFile();
}

bool CcSyncDirectoryConfig::writeConfigFile()
{
  if (m_pAccountConfig != nullptr)
  {
    return m_pAccountConfig->writeConfigFile();
  }
  return false;
}

bool CcSyncDirectoryConfig::setLocation(const CcString& sLocation)
{
  m_sLocation = sLocation;
  if (m_pDirectoryNode != nullptr)
  {
    CcXmlNode& rLocationNode = (*m_pDirectoryNode)[CcSyncGlobals::Client::ConfigTags::DirectoryLocation];
    if (rLocationNode.isNotNull())
      rLocationNode.setInnerText(m_sLocation);
  }
  return writeConfigFile();
}

bool CcSyncDirectoryConfig::setBackupCommand(const CcString& sBackupCommand)
{
  m_sBackupCommand = sBackupCommand;
  if (m_pDirectoryNode != nullptr)
  {
    CcXmlNode& rBackupCommandNode = (*m_pDirectoryNode)[CcSyncGlobals::Client::ConfigTags::DirectoryBackupCommand];
    if (rBackupCommandNode.isNotNull())
      rBackupCommandNode.setInnerText(m_sBackupCommand);
    else
    {
      CcXmlNode oNewBackupNode(CcSyncGlobals::Client::ConfigTags::DirectoryBackupCommand);
      oNewBackupNode.setInnerText(sBackupCommand);
      m_pDirectoryNode->append(oNewBackupNode);
    }
  }
  return writeConfigFile();
}

bool CcSyncDirectoryConfig::setRestoreCommand(const CcString& sRestoreCommand)
{
  m_sRestoreCommand = sRestoreCommand;
  if (m_pDirectoryNode != nullptr)
  {
    CcXmlNode& rRestoreCommandNode = (*m_pDirectoryNode)[CcSyncGlobals::Client::ConfigTags::DirectoryRestoreCommand];
    if (rRestoreCommandNode.isNotNull())
      rRestoreCommandNode.setInnerText(m_sRestoreCommand);
    else
    {
      CcXmlNode oNewRestoreNode(CcSyncGlobals::Client::ConfigTags::DirectoryRestoreCommand);
      oNewRestoreNode.setInnerText(sRestoreCommand);
      m_pDirectoryNode->append(oNewRestoreNode);
    }
  }
  return writeConfigFile();
}

bool CcSyncDirectoryConfig::setUser(const CcString& sUser)
{
  m_uiUser = userIdFromString(sUser);
  if(m_uiUser != UINT32_MAX)
  {
    if (m_pDirectoryNode != nullptr)
    {
      CcXmlNode& rUserNode = (*m_pDirectoryNode)[CcSyncGlobals::Client::ConfigTags::DirectoryUser];
      if (rUserNode.isNotNull())
        rUserNode.setInnerText(CcString::fromNumber(m_uiUser));
      else
      {
        CcXmlNode oUserNode(CcSyncGlobals::Client::ConfigTags::DirectoryUser);
        oUserNode.setInnerText(CcString::fromNumber(m_uiUser));
        m_pDirectoryNode->append(oUserNode);
      }
    }
    return writeConfigFile();
  }
  else
  {
    return false;
  }
}

bool CcSyncDirectoryConfig::setGroup(const CcString& sGroup)
{
  m_uiGroup = groupIdFromString(sGroup);
  if(m_uiGroup != UINT32_MAX)
  {
    if (m_pDirectoryNode != nullptr)
    {
      CcXmlNode& rGroupNode = (*m_pDirectoryNode)[CcSyncGlobals::Client::ConfigTags::DirectoryGroup];
      if (rGroupNode.isNotNull())
        rGroupNode.setInnerText(CcString::fromNumber(m_uiGroup));
      else
      {
        CcXmlNode oGroupNode(CcSyncGlobals::Client::ConfigTags::DirectoryGroup);
        oGroupNode.setInnerText(CcString::fromNumber(m_uiGroup));
        m_pDirectoryNode->append(oGroupNode);
      }
    }
    return writeConfigFile();
  }
  else
  {
    return false;
  }
}

CcXmlNode CcSyncDirectoryConfig::getXmlNode()
{
  CcXmlNode oDirectoryNode(CcXmlNode::EType::Node);
  oDirectoryNode.setName(CcSyncGlobals::Client::ConfigTags::Directory);
  {
    CcXmlNode oDirectoryName(CcXmlNode::EType::Node);
    oDirectoryName.setName(CcSyncGlobals::Client::ConfigTags::DirectoryName);
    oDirectoryName.setInnerText(m_sName);
    oDirectoryNode.append(std::move(oDirectoryName));

    CcXmlNode oDirectoryLocation(CcXmlNode::EType::Node);
    oDirectoryLocation.setName(CcSyncGlobals::Client::ConfigTags::DirectoryLocation);
    oDirectoryLocation.setInnerText(m_sName);
    oDirectoryNode.append(std::move(oDirectoryLocation));
  }
  return oDirectoryNode;
}

uint32 CcSyncDirectoryConfig::userIdFromString(const CcString& sUser)
{
  bool bOk = false;
  uint32 uiUserId = sUser.toUint32(&bOk);
  if(!bOk)
  {
    CcUser* hUser = CcKernel::getUserList().findUser(sUser);
    if (hUser != nullptr)
    {
      return hUser->getId();
    }
    else
    {
      return UINT32_MAX;
    }
  }
  return uiUserId;
}

uint32 CcSyncDirectoryConfig::groupIdFromString(const CcString& sGroup)
{
  bool bOk = false;
  uint32 uiGroupId = sGroup.toUint32(&bOk);
  if(!bOk)
  {
    const CcGroup& hGroup = CcKernel::getGroupList().findGroup(sGroup);
    if (!CCISNULLREF(hGroup))
    {
      return hGroup.getId();
    }
    else
    {
      return UINT32_MAX;
    }
  }
  return uiGroupId;
}
