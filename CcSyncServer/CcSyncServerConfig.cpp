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
 * @brief     Implemtation of class CcSyncServerConfigClientCommand
 */
#include "CcSyncGlobals.h"
#include "CcSyncServerConfig.h"
#include "Network/CcCommonPorts.h"
#include "CcXml/CcXmlNode.h"
#include "CcXml/CcXmlNodeList.h"
#include "CcXml/CcXmlUtil.h"
#include "CcStringUtil.h"
#include "CcKernel.h"
#include "CcGlobalStrings.h"
#include "CcSyncLog.h"

CcSyncServerConfig::CcSyncServerConfig() :
  m_uiPort(CcSyncGlobals::DefaultPort),
  m_oLocation(this)
{
  // preinit members
  m_sSslKeyFile = CcKernel::getConfigDir();
  m_sSslKeyFile.appendPath(CcSyncGlobals::ConfigDirName);
  m_sSslKeyFile.appendPath(CcSyncGlobals::DefaultKeyFile);
  m_sSslCertFile = CcKernel::getConfigDir();
  m_sSslCertFile.appendPath(CcSyncGlobals::ConfigDirName);
  m_sSslCertFile.appendPath(CcSyncGlobals::DefaultCertFile);
}

void CcSyncServerConfig::addAdminAccount(const CcString& sName, const CcString& sPassword)
{
  CcSyncServerAccount oAccount(sName, sPassword, true);
  m_oAccountList.append(oAccount);
}

void CcSyncServerConfig::addAccount(const CcString& sName, const CcString& sPassword)
{
  CcSyncServerAccount oAccount(sName, sPassword, false);
  m_oAccountList.append(oAccount);
  CcXmlNode& oRootNode = m_oXmlFile.rootNode()[CcSyncGlobals::Server::ConfigTags::Root];
  if (oRootNode.isNotNull())
  {
    CcXmlNode oAccountNode(CcSyncGlobals::Server::ConfigTags::Account);
    CcXmlNode oAccountUsernameNode(CcSyncGlobals::Server::ConfigTags::AccountName, sName);
    CcXmlNode oAccountPasswordNode(CcSyncGlobals::Server::ConfigTags::AccountPassword, oAccount.getPassword().getString());
    oAccountNode.append(oAccountUsernameNode);
    oAccountNode.append(oAccountPasswordNode);
    oRootNode.append(oAccountNode);
  }
  writeConfigFile();
}

void CcSyncServerConfig::setLocation(const CcString& sPath)
{
  m_oLocation.setPath(sPath);
}

bool CcSyncServerConfig::readConfig(const CcString& sConfigFile)
{
  m_bValid = true;
  m_oXmlFile.setFile(sConfigFile);
  m_oXmlFile.readData();
  CcXmlNode& oDocRootNode = m_oXmlFile.rootNode();
  if (oDocRootNode.isNotNull())
  {
    CcXmlNode& oRootNode = oDocRootNode.getNode(CcSyncGlobals::Server::ConfigTags::Root);
    if (oRootNode.isNotNull()             &&
        findServerConfig(oRootNode)       &&
        findLocationsConfig(oRootNode)    &&
        findAccountConfigs(oRootNode))
    {
      CcXmlNode& pPassword = oRootNode.getNode(CcSyncGlobals::Client::ConfigTags::UserPassword);
      if ( pPassword.isNotNull())
      {
        m_oRootPassword = pPassword.getValue();
      }
      m_bValid = true;
    }
    else
    {
      CcSyncLog::writeDebug("Error in configuration", ESyncLogTarget::Server);
    }
  }
  else
  {
    CcSyncLog::writeDebug("No valid config file found", ESyncLogTarget::Server);
  }
  return m_bValid;
}

bool CcSyncServerConfig::writeConfig(const CcString& sConfigFile)
{
  m_oXmlFile.setFile(sConfigFile);
  CcXmlNode oRootNode(CcSyncGlobals::Server::ConfigTags::Root);
  CcXmlNode oPortNode(CcSyncGlobals::Server::ConfigTags::Port, CcString::fromNumber(m_uiPort));
  oRootNode.append(std::move(oPortNode));
  if (m_bSsl)
  {
    CcXmlNode oSslNode(CcSyncGlobals::Server::ConfigTags::Ssl, CcGlobalStrings::True);
    oRootNode.append(std::move(oSslNode));
  }
  else
  {
    CcXmlNode oSslNode(CcSyncGlobals::Server::ConfigTags::Ssl, CcGlobalStrings::True);
    oRootNode.append(std::move(oSslNode));
  }
  for (CcSyncServerAccount& oAccountConfig : m_oAccountList)
  {
    oAccountConfig.writeConfig(oRootNode);
  }
  m_oLocation.writeConfig(oRootNode);

  m_oXmlFile.rootNode().append(oRootNode);
  return writeConfigFile();
}

bool CcSyncServerConfig::writeConfigFile()
{
  return m_oXmlFile.writeData(true);
}

CcSyncServerAccount* CcSyncServerConfig::findAccount(const CcString& sAccountName) const
{
  for (CcSyncServerAccount& rAccount : m_oAccountList)
  {
    if (rAccount.getName() == sAccountName)
      return &rAccount;
  }
  return nullptr;
}

bool CcSyncServerConfig::removeAccount(const CcString& sAccountName)
{
  bool bRet = false;
  size_t uiIndex = 0;
  for (CcSyncServerAccount& rAccount : m_oAccountList)
  {
    if (rAccount.getName() == sAccountName)
      break;
    uiIndex++;
  }
  if (uiIndex < m_oAccountList.size())
  {
    m_oAccountList.remove(uiIndex);
    CcXmlNode& oRootNode = m_oXmlFile.rootNode()[CcSyncGlobals::Server::ConfigTags::Root];
    if (oRootNode.isNotNull())
    {
      uiIndex = 0;
      for (CcXmlNode& rAccountNode : oRootNode)
      {
        if (rAccountNode.getName() == CcSyncGlobals::Server::ConfigTags::Account)
        {
          CcXmlNode& rNameNode = rAccountNode.getNode(CcSyncGlobals::Server::ConfigTags::AccountName);
          if (rNameNode.isNotNull() &&
              rNameNode.getValue().compareInsensitve(sAccountName))
          {
            break;
          }
        }
        uiIndex++;
      }
      if (uiIndex < oRootNode.size())
      {
        oRootNode.remove(uiIndex);
        bRet = writeConfigFile();
      }
    }
  }
  return bRet;
}


CcSyncServerConfig& CcSyncServerConfig::operator=(const CcSyncServerConfig& oToCopy)
{
  m_bValid = oToCopy.m_bValid;
  m_bSsl = oToCopy.m_bSsl;
  m_bSslRequired = oToCopy.m_bSslRequired;
  m_oXmlFile = oToCopy.m_oXmlFile;
  return *this;
}

CcSyncServerConfig& CcSyncServerConfig::operator=(CcSyncServerConfig&& oToMove)
{
  if (this != &oToMove)
  {
    m_bValid = oToMove.m_bValid;
    m_bSsl = oToMove.m_bSsl;
    m_bSslRequired = oToMove.m_bSslRequired;
    m_oXmlFile = std::move(oToMove.m_oXmlFile);
  }
  return *this;
}

bool CcSyncServerConfig::findServerConfig(CcXmlNode& pNode)
{
  bool bRet = false;
  CcXmlNode& pTempNode = pNode.getNode(CcSyncGlobals::Server::ConfigTags::Port);
  if (pTempNode.isNotNull())
  {
    m_uiPort = pTempNode.getValue().toUint16(&bRet);
    CcXmlNode& pTempNode1 = pNode.getNode(CcSyncGlobals::Server::ConfigTags::Ssl);
    if (pTempNode1.isNotNull())
    {
      m_bSsl = CcStringUtil::getBoolFromStirng(pTempNode1.getValue());
    }
    CcXmlNode& pTempNode2 = pNode.getNode(CcSyncGlobals::Server::ConfigTags::SslRequired);
    if (pTempNode2.isNotNull())
    {
      m_bSslRequired = CcStringUtil::getBoolFromStirng(pTempNode2.getValue());
    }
    CcXmlNode& pTempNode3 = pNode.getNode(CcSyncGlobals::Server::ConfigTags::SslCert);
    if (pTempNode3.isNotNull())
    {
      m_sSslCertFile = pTempNode3.getValue();
    }
    CcXmlNode& pTempNode4 = pNode.getNode(CcSyncGlobals::Server::ConfigTags::SslKey);
    if (pTempNode4.isNotNull())
    {
      m_sSslKeyFile = pTempNode4.getValue();
    }
  }
  else
  {
    CcSyncLog::writeError("Server configuration not found in CcSyncClient configuration file", ESyncLogTarget::Server);
  }
  return bRet;
}

bool CcSyncServerConfig::findAccountConfigs(CcXmlNode& pNode)
{
  bool bRet = false;
  for (CcXmlNode& pAccountNode : pNode)
  {
    if (pAccountNode.getName() == CcSyncGlobals::Server::ConfigTags::Account)
    {
      bRet = true;
      CcSyncServerAccount oAccountConfig(pAccountNode);
      if (oAccountConfig.isValid())
      {
        CcString sUserPath = m_oLocation.getPath();
        sUserPath.appendPath(oAccountConfig.getName());
        m_oAccountList.append(oAccountConfig);
      }
    }
  }
  return bRet;
}

bool CcSyncServerConfig::findLocationsConfig(CcXmlNode& pNode)
{
  bool bRet = true;
  CcXmlNode pLocationNode = pNode.getNode(CcSyncGlobals::Server::ConfigTags::Location);
  if (pLocationNode.isNotNull())
  {
    m_oLocation.readConfig(pLocationNode);
  }
  else
  {
    CCWARNING("Locations not found in CcSyncClient configuration file");
  }
  return bRet;
}

