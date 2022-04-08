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
 * @brief     Implemtation of class CcSyncAccountConfigClientCommand
 */
#include "CcSyncGlobals.h"
#include "CcSyncAccountConfig.h"
#include "Xml/CcXmlNode.h"
#include "Xml/CcXmlNodeList.h"
#include "Xml/CcXmlFile.h"
#include "Xml/CcXmlUtil.h"
#include "Json/CcJsonObject.h"
#include "Json/CcJsonArray.h"
#include "CcStringUtil.h"
#include "Network/CcCommonPorts.h"
#include "CcDirectory.h"
#include "Hash/CcSha256.h"
#include "CcSyncClientConfig.h"
#include "CcSyncLog.h"

CcSyncAccountConfig::CcSyncAccountConfig(CcSyncClientConfig* pClientConfig):
  m_pClientConfig(pClientConfig)
{
  init();
  m_bValid = false;
}

CcSyncAccountConfig::CcSyncAccountConfig(const CcJsonObject& pAccountNode)
{
  init();
  parseJson(pAccountNode);
}

CcSyncAccountConfig::CcSyncAccountConfig(const CcString& sUsername, const CcString& sPassword, const CcString& sServer, const CcString& sPort, CcSyncClientConfig *pClientConfig) :
  m_sName(sUsername),
  m_pClientConfig(pClientConfig)
{
  init();
  m_oServer.setHostname(sServer);
  m_oServer.setPort(sPort);
  CcString sPasswordCrypt = sUsername.getLower() + sPassword;
  sPasswordCrypt = CcSha256().generateByteArray(sPasswordCrypt.getByteArray()).getValue().getHexString();
  m_oPassword.setPassword(sPasswordCrypt, EHashType::Sha256);
  m_bValid = true;
}

CcSyncAccountConfig& CcSyncAccountConfig::operator=(const CcSyncAccountConfig& oToCopy)
{
  m_bValid  = oToCopy.m_bValid;
  m_sName   = oToCopy.m_sName;
  m_oPassword = oToCopy.m_oPassword;
  m_sDatabaseFile = oToCopy.m_sDatabaseFile;
  m_oServer = oToCopy.m_oServer;
  m_oDirectoryList = oToCopy.m_oDirectoryList;
  m_pAccountNode = oToCopy.m_pAccountNode;
  m_pClientConfig = oToCopy.m_pClientConfig;
  return *this;
}

CcSyncAccountConfig& CcSyncAccountConfig::operator=(CcSyncAccountConfig&& oToMove)
{
  if (this != &oToMove)
  {
    m_bValid = oToMove.m_bValid;
    m_sName = std::move(oToMove.m_sName);
    m_oPassword = std::move(oToMove.m_oPassword);
    m_sDatabaseFile = std::move(oToMove.m_sDatabaseFile);
    m_oServer = std::move(oToMove.m_oServer);
    m_oDirectoryList = std::move(oToMove.m_oDirectoryList);
    m_pAccountNode = oToMove.m_pAccountNode;
    m_pClientConfig = oToMove.m_pClientConfig;
    oToMove.m_pAccountNode  = nullptr;
    oToMove.m_pClientConfig = nullptr;
  }
  return *this;
}

CcString CcSyncAccountConfig::getAccountDirName()
{
  CcString sRet = m_sName;
  if(m_oServer.getHostname().length() > 0)
  { 
    sRet << +"@" << m_oServer.getHostname();
  }
  return sRet;
}

CcXmlNode CcSyncAccountConfig::getXmlNode() const
{
  CcXmlNode oAccountNode(CcXmlNode::EType::Node);
  oAccountNode.setName(CcSyncGlobals::Client::ConfigTags::Account);
  {
    CcXmlNode oAccountName(CcXmlNode::EType::Node);
    oAccountName.setName(CcSyncGlobals::Client::ConfigTags::UserName);
    oAccountName.setInnerText(m_sName);
    oAccountNode.append(std::move(oAccountName));

    CcXmlNode oAccountPassword(CcXmlNode::EType::Node);
    oAccountPassword.setName(CcSyncGlobals::Client::ConfigTags::UserPassword);
    oAccountPassword.setInnerText(""); // Hide Password
    oAccountNode.append(std::move(oAccountPassword));

    CcXmlNode oAccountServer(CcXmlNode::EType::Node);
    oAccountServer.setName(CcSyncGlobals::Client::ConfigTags::Server);
    {
      CcXmlNode oAccountServerHost(CcXmlNode::EType::Node);
      oAccountServerHost.setName(CcSyncGlobals::Client::ConfigTags::ServerHost);
      oAccountServerHost.setInnerText(m_oServer.getHostname());
      oAccountServer.append(std::move(oAccountServerHost));

      CcXmlNode oAccountServerPort(CcXmlNode::EType::Node);
      oAccountServerPort.setName(CcSyncGlobals::Client::ConfigTags::ServerPort);
      oAccountServerPort.setInnerText(m_oServer.getPortString());
      oAccountServer.append(std::move(oAccountServerHost));
    }
    oAccountNode.append(std::move(oAccountServer));

    CcXmlNode oAccountDatabaseFile(CcXmlNode::EType::Node);
    oAccountDatabaseFile.setName(CcSyncGlobals::Client::ConfigTags::Database);
    oAccountDatabaseFile.setInnerText(m_sDatabaseFile); // Hide Password
    oAccountNode.append(std::move(oAccountDatabaseFile));

    for (CcSyncDirectoryConfig& oDirConfig : m_oDirectoryList)
    {
      oAccountNode.append(oDirConfig.getXmlNode());
    }
  }
  return oAccountNode;
}

CcJsonObject CcSyncAccountConfig::getJsonNode() const
{
  CcJsonObject oAccountNode;
  oAccountNode.add(CcJsonNode(CcSyncGlobals::Commands::AccountGetData::Name, getName()));
  oAccountNode.add(CcJsonNode(CcSyncGlobals::Commands::AccountGetData::Password, ""));
  oAccountNode.add(CcJsonNode(CcSyncGlobals::Commands::AccountGetData::Database, getDatabaseFilePath()));

  CcJsonObject oServer;
  oServer.add(CcJsonNode(CcSyncGlobals::Commands::AccountGetData::ServerHost, getServer().getHostname()));
  oServer.add(CcJsonNode(CcSyncGlobals::Commands::AccountGetData::ServerPort, getServer().getPort()));
  oAccountNode.add(CcJsonNode(oServer, CcSyncGlobals::Commands::AccountGetData::Server));

  CcJsonArray oDirArray;
  for (CcSyncDirectoryConfig& oDirConfig : getDirectoryList())
  {
    CcJsonObject oDirInfo;
    oDirInfo.add(CcJsonNode(CcSyncGlobals::Commands::AccountGetData::DirName, oDirConfig.getName()));
    oDirInfo.add(CcJsonNode(CcSyncGlobals::Commands::AccountGetData::DirLocation, ""));
    oDirInfo.add(CcJsonNode(CcSyncGlobals::Commands::AccountGetData::DirBCommand, oDirConfig.getBackupCommand()));
    oDirInfo.add(CcJsonNode(CcSyncGlobals::Commands::AccountGetData::DirRCommand, oDirConfig.getRestoreCommand()));
    oDirArray.add(CcJsonNode(oDirInfo, ""));
  }
  oAccountNode.add(CcJsonNode(oDirArray, CcSyncGlobals::Commands::AccountGetData::Directories));
  return oAccountNode;
}

bool CcSyncAccountConfig::writeConfigFile()
{
  if (m_pClientConfig != nullptr)
  {
    return m_pClientConfig->writeConfigFile();
  }
  else
  {
    CcSyncLog::writeError("CcSyncAccountConfig tried to write config file to NULL");
    return false;
  }
}

bool CcSyncAccountConfig::parseXml(CcXmlNode& pAccountNode)
{
  m_pAccountNode = &pAccountNode;
  m_bValid = false;
  if (!pAccountNode.isNull()            &&
      xmlFindServerConfig(pAccountNode) &&
      xmlFindUserConfig(pAccountNode)   )
  {
    xmlFindDirectoriesConfig(pAccountNode);
    xmlFindDatabaseConfig(pAccountNode);
    m_bValid = true;
  }
  return m_bValid;
}

bool CcSyncAccountConfig::parseJson(const CcJsonObject& pAccountNode)
{
  // set default port. it will be overwritten if config exists
  m_oServer.setPort(CcSyncGlobals::DefaultPort);
  m_bValid = false;
  if (  jsonFindServerConfig(pAccountNode) &&
        jsonFindUserConfig(pAccountNode) &&
        jsonFindCommandsConfig(pAccountNode) &&
        jsonFindDirectoriesConfig(pAccountNode) &&
        jsonFindDatabaseConfig(pAccountNode))
  {
    m_bValid = true;
  }
  return m_bValid;
}

bool CcSyncAccountConfig::writeConfig(CcXmlNode& pParentNode)
{
  CcXmlNode oAccountNode(CcSyncGlobals::Client::ConfigTags::Account);
  CcXmlNode oAccountNodeName(CcSyncGlobals::Client::ConfigTags::Name);
  oAccountNodeName.setInnerText(m_sName);
  CcXmlNode oAccountNodePassword(CcSyncGlobals::Client::ConfigTags::UserPassword);
  oAccountNodePassword.setInnerText(m_oPassword.getString());
  CcXmlNode oAccountNodeServer(CcSyncGlobals::Client::ConfigTags::Server);
  CcXmlNode oAccountNodeServerHost(CcSyncGlobals::Client::ConfigTags::ServerHost);
  oAccountNodeServerHost.setInnerText(m_oServer.getHostname());
  CcXmlNode oAccountNodeServerPort(CcSyncGlobals::Client::ConfigTags::ServerPort);
  oAccountNodeServerPort.setInnerText(m_oServer.getPortString());
  oAccountNodeServer.append(std::move(oAccountNodeServerHost));
  oAccountNodeServer.append(std::move(oAccountNodeServerPort));
  oAccountNode.append(std::move(oAccountNodeServer));
  oAccountNode.append(std::move(oAccountNodeName));
  oAccountNode.append(std::move(oAccountNodePassword));
  pParentNode.append(std::move(oAccountNode));
  m_pAccountNode = &pParentNode.getLastAddedNode();
  return writeConfigFile();
}

bool CcSyncAccountConfig::checkLogin(const CcString sUsername, const CcPassword oPassword)
{
  bool bRet = false;
  if (m_sName == sUsername &&
      m_oPassword == oPassword)
  {
    bRet = true;
  }
  return bRet;
}

bool CcSyncAccountConfig::changePassword(const CcString& sPassword)
{
  bool bRet = false;
  if (m_pAccountNode != nullptr)
  {
    CcXmlNode& rPasswordNode = m_pAccountNode->getNode(CcSyncGlobals::Client::ConfigTags::UserPassword);
    if (rPasswordNode.isNotNull())
    {
      CcString sPasswordCrypt = m_sName.getLower() + sPassword;
      sPasswordCrypt = CcSha256().generateByteArray(sPasswordCrypt.getByteArray()).getValue().getHexString();
      m_oPassword.setPassword(sPasswordCrypt, EHashType::Sha256);
      rPasswordNode.setInnerText(sPasswordCrypt);
      m_oPassword.setPassword(sPasswordCrypt);
      bRet = writeConfigFile();
    }
  }
  return bRet;
}

bool CcSyncAccountConfig::changeHostname(const CcString& sHostname)
{
  bool bRet = false;
  if (m_pAccountNode != nullptr)
  {
    CcXmlNode& rServerNode = m_pAccountNode->getNode(CcSyncGlobals::Client::ConfigTags::Server);
    if (rServerNode.isNotNull())
    {
      CcXmlNode& rHostnameNode = rServerNode.getNode(CcSyncGlobals::Client::ConfigTags::ServerHost);
      if (rHostnameNode.isNotNull())
      {
        rHostnameNode.setInnerText(sHostname);
        m_oServer.setHostname(sHostname);
        bRet = writeConfigFile();
      }
    }
  }
  return bRet;
}

bool CcSyncAccountConfig::changePort(const CcString& sPort)
{
  bool bRet = false;
  if (m_pAccountNode != nullptr)
  {
    CcXmlNode& rServerNode = m_pAccountNode->getNode(CcSyncGlobals::Client::ConfigTags::Server);
    if (rServerNode.isNotNull())
    {
      CcXmlNode& rPortNode = rServerNode.getNode(CcSyncGlobals::Client::ConfigTags::ServerPort);
      if (rPortNode.isNotNull())
      {
        rPortNode.setInnerText(sPort);
        m_oServer.setPort(sPort);
        bRet = writeConfigFile();
      }
    }
  }
  return bRet;
}

bool CcSyncAccountConfig::addAccountDirectory(const CcString& sDirectoryName, const CcString& sDirectoryPath)
{
  for (CcSyncDirectoryConfig& oDirConfig : directoryList())
  {
    if (oDirConfig.getName() == sDirectoryName)
    {
      if (oDirConfig.getLocation() != sDirectoryPath)
      {
        return oDirConfig.setLocation(sDirectoryPath);
      }
      return true;
    }
  }
  CcSyncDirectoryConfig oNewConfig(sDirectoryName, sDirectoryPath, this);
  oNewConfig.writeConfig(*m_pAccountNode);
  m_oDirectoryList.append(std::move(oNewConfig));
  return writeConfigFile();
}

bool CcSyncAccountConfig::removeAccountDirectory(const CcString& sDirectoryName)
{
  bool bRet = false;
  size_t uiIndex = 0;
  for (CcSyncDirectoryConfig& oDirConfig : m_oDirectoryList)
  {
    if (oDirConfig.getName() == sDirectoryName)
    {
      break;
    }
    uiIndex++;
  }
  if (uiIndex < m_oDirectoryList.size())
  {
    m_oDirectoryList.remove(uiIndex);
    uiIndex = 0;
    for (CcXmlNode& rDirNode : *m_pAccountNode)
    {
      if (rDirNode.getName() == CcSyncGlobals::Client::ConfigTags::Directory)
      {
        CcXmlNode rNameNode = rDirNode.getNode(CcSyncGlobals::Client::ConfigTags::DirectoryName);
        if (rNameNode.isNotNull()  &&
            rNameNode.innerText().compareInsensitve(sDirectoryName))
        {
          break;
        }
      }
      uiIndex++;
    }
    if (uiIndex < m_pAccountNode->size())
    {
      m_pAccountNode->remove(uiIndex);
      bRet = writeConfigFile();
    }
  }
  return bRet;
}

void CcSyncAccountConfig::overrideLocations(const CcString& sBasePath)
{
  for (CcSyncDirectoryConfig& oDirectoryConfig : m_oDirectoryList)
  {
    CcString sPath(sBasePath);
    sPath.appendPath(oDirectoryConfig.getName());
    oDirectoryConfig.overrideLocation(sPath);
  }
}

void CcSyncAccountConfig::init()
{
  // set default port. it will be overwritten if config exists
  m_oServer.setPort(CcSyncGlobals::DefaultPort);
  // set default Database location, will be overwritten if config is set
  m_sDatabaseFile << "Client" << CcSyncGlobals::SqliteExtension;
}

bool CcSyncAccountConfig::xmlFindServerConfig(CcXmlNode& pNode)
{
  bool bRet = false;
  CcXmlNode& pServerNode = pNode.getNode(CcSyncGlobals::Client::ConfigTags::Server);
  if (!pServerNode.isNull())
  {
    CcXmlNode& pHostNode = pServerNode[CcSyncGlobals::Client::ConfigTags::ServerHost];
    CcXmlNode& pPortNode = pServerNode[CcSyncGlobals::Client::ConfigTags::ServerPort];
    if (!pHostNode.isNull() &&
      !pPortNode.isNull())
    {
      bRet = true;
      m_oServer.setHostname(pHostNode.innerText());
      m_oServer.setPort(pPortNode.innerText());
    }
    else
    {
      CcSyncLog::writeError("Server configuration failure in CcSyncClient configuration file");
    }
  }
  else
  {
    CcSyncLog::writeError("Server configuration not found in CcSyncClient configuration file");
  }
  return bRet;
}

bool CcSyncAccountConfig::xmlFindUserConfig(CcXmlNode& pNode)
{
  bool bRet = false;
  CcXmlNode& pNameNode = pNode.getNode(CcSyncGlobals::Client::ConfigTags::UserName);
  CcXmlNode& pPassword = pNode.getNode(CcSyncGlobals::Client::ConfigTags::UserPassword);
  if (pNameNode.isNotNull() &&
      pPassword.isNotNull())
  {
    bRet = true;
    m_sName = pNameNode.innerText();
    m_oPassword.setPassword(pPassword.innerText(), EHashType::Sha256);
  }
  return bRet;
}

bool CcSyncAccountConfig::xmlFindDirectoriesConfig(CcXmlNode& pNode)
{
  bool bRet = false;
  for (CcXmlNode& pDirectoryNode : pNode)
  {
    if (pDirectoryNode.getName() == CcSyncGlobals::Client::ConfigTags::Directory)
    {
      bRet = true;
      CcSyncDirectoryConfig oDirectory(this);
      m_oDirectoryList.append(std::move(oDirectory));
      m_oDirectoryList.last().parseXmlNode(pDirectoryNode);
    }
  }
  return bRet;
}

bool CcSyncAccountConfig::xmlFindDatabaseConfig(CcXmlNode& pNode)
{
  bool bRet = true;
  CcXmlNode& pCommandsNode = pNode.getNode(CcSyncGlobals::Client::ConfigTags::Database);
  if (pCommandsNode.isNotNull() && pCommandsNode.innerText() != "")
  {
    m_sDatabaseFile = pCommandsNode.innerText();
  }
  return bRet;
}

bool CcSyncAccountConfig::jsonFindServerConfig(const CcJsonObject& pNode)
{
  bool bRet = false;
  const CcJsonNode& pServerNode = pNode[CcSyncGlobals::Client::ConfigTags::Server];
  if (pServerNode.isObject())
  {
    const CcJsonNode& pHostNode = pServerNode[CcSyncGlobals::Client::ConfigTags::ServerHost];
    const CcJsonNode& pPortNode = pServerNode[CcSyncGlobals::Client::ConfigTags::ServerPort];
    if (pHostNode.isValue() &&
        pPortNode.isValue())
    {
      bRet = true;
      m_oServer.setHostname(pHostNode.getValue().getString());
      m_oServer.setPort(pPortNode.getValue().getUint16());
    }
    else
    {
      CcSyncLog::writeError("Server configuration failure in CcSyncClient configuration file");
    }
  }
  else
  {
    CcSyncLog::writeError("Server configuration not found in CcSyncClient configuration file");
  }
  return bRet;
}

bool CcSyncAccountConfig::jsonFindUserConfig(const CcJsonObject& pNode)
{
  bool bRet = true;
  const CcJsonNode& pNameNode = pNode[CcSyncGlobals::Client::ConfigTags::UserName];
  const CcJsonNode& pPassword = pNode[CcSyncGlobals::Client::ConfigTags::UserPassword];
  if (pNameNode.isValue() &&
      pPassword.isValue())
  {
    bRet = true;
    m_sName = pNameNode.getValue().getString();
    m_oPassword = pPassword.getValue().getString();
  }
  // @todo: load additional users if available

  //// @todo: load additional users if available
  return bRet;
}

bool CcSyncAccountConfig::jsonFindDirectoriesConfig(const CcJsonObject& pNode)
{
  bool bRet = false;
  const CcJsonNode& oDirectoryNodes = pNode[CcSyncGlobals::Client::ConfigTags::Directory];
  if (oDirectoryNodes.isArray())
  {
    for (const CcJsonNode& pDirectoryNode : oDirectoryNodes.getJsonArray())
    {
      bRet = true;
      CcSyncDirectoryConfig oDirectory(pDirectoryNode.getJsonObject());
      m_oDirectoryList.append(std::move(oDirectory));
    }
  }
  return bRet;
}

bool CcSyncAccountConfig::jsonFindCommandsConfig(const CcJsonObject& pNode)
{
  bool bRet = true;
  CCUNUSED(pNode);
  //// @todo: move to each directory
  return bRet;
}

bool CcSyncAccountConfig::jsonFindDatabaseConfig(const CcJsonObject& pNode)
{
  bool bRet = true;
  const CcJsonNode& oDatabaseNodes = pNode[CcSyncGlobals::Client::ConfigTags::Database];
  if (oDatabaseNodes.isValue() && oDatabaseNodes.getValue().getString() != "")
  {
    m_sDatabaseFile = oDatabaseNodes.getValue().getString();
  }
  return bRet;
}
