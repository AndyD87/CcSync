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
 * @brief     Implemtation of class CcSyncClientConfigClientCommand
 */
#include "CcSyncGlobals.h"
#include "CcSyncClientConfig.h"
#include "CcXml/CcXmlNode.h"
#include "CcXml/CcXmlNodeList.h"
#include "CcStringUtil.h"
#include "Network/CcCommonPorts.h"
#include "CcDirectory.h"
#include "CcStringList.h"

CcSyncClientConfig::CcSyncClientConfig()
{
}

CcSyncClientConfig::~CcSyncClientConfig()
{
}

bool CcSyncClientConfig::create(const CcString& sConfigFile)
{
  m_oXmlFile.setFile(sConfigFile);
  CcXmlNode oRootNode(CcSyncGlobals::Client::ConfigTags::Root);
  m_oXmlFile.rootNode().append(std::move(oRootNode));
  return m_oXmlFile.writeData(true);
}

bool CcSyncClientConfig::readConfig(const CcString& sConfigFile)
{
  bool bRet = false;
  m_oXmlFile.setFile(sConfigFile);
  m_oXmlFile.readData();
  CcXmlNode& oDocRootNode = m_oXmlFile.rootNode();
  if (oDocRootNode.isNotNull())
  {
    CcXmlNode& oRootNode = oDocRootNode.getNode(CcSyncGlobals::Client::ConfigTags::Root);
    if (oRootNode.isNotNull())
    {
      for (CcXmlNode& pAccountNode : oRootNode.nodeList())
      {
        if (pAccountNode.getName() == CcSyncGlobals::Client::ConfigTags::Account)
        {
          CcSyncAccountConfig oAccount(this);
          m_oAccounts.append(std::move(oAccount));
          m_oAccounts.last().parseXml(pAccountNode);
          if (m_oAccounts.last().isValid())
          {
            bRet = true;
          }
        }
      }
    }
  }
  return bRet;
}

bool CcSyncClientConfig::readConfigFromServer(const CcString& sConfigFile, const CcString &sBasePath)
{
  bool bRet = false;
  if (readConfig(sConfigFile))
  {
    for (CcSyncAccountConfig& rAccountConfig : m_oAccounts)
    {
      rAccountConfig.overrideLocations(sBasePath);
    }
    bRet = true;
  }
  return bRet;
}

bool CcSyncClientConfig::addAccount(const CcString& sUsername, const CcString& sPassword, const CcString& sServer, const CcString& sPort)
{
  CcSyncAccountConfig oNewAccount(sUsername, sPassword, sServer, sPort, this);
  m_oAccounts.append(std::move(oNewAccount));
  CcXmlNode& oConfigNode = m_oXmlFile.rootNode()[CcSyncGlobals::Client::ConfigTags::Root];
  if (oConfigNode.isNotNull())
  {
    return m_oAccounts.last().writeConfig(oConfigNode);
  }
  return false;
}

bool CcSyncClientConfig::removeAccount(const CcString & sAccount)
{
  size_t uiIndex = getAccountIndex(sAccount);
  if (uiIndex < m_oAccounts.size())
  {
    CcXmlNodeList& rAccountsNodes = m_oXmlFile.rootNode()[CcSyncGlobals::Client::ConfigTags::Root].nodeList();
    for (size_t j = 0; j < rAccountsNodes.size(); j++)
    {
      CcXmlNode& rAccountNode = rAccountsNodes[j];
      if (rAccountNode.getName() == CcSyncGlobals::Client::ConfigTags::Account)
      {
        CcXmlNode& rNameNode = rAccountNode[CcSyncGlobals::Client::ConfigTags::Name];
        if (rNameNode.isNotNull() && rNameNode.innerText() == m_oAccounts[uiIndex].getName())
        {
          rAccountsNodes.remove(j);
        }
      }
    }
    m_oAccounts.remove(uiIndex);
  }
  return m_oXmlFile.writeData(true);
}

CcSyncAccountConfigHandle CcSyncClientConfig::getAccountConfig(const CcString& sAccountname)
{
  CcSyncAccountConfigHandle pRet = nullptr;
  size_t uiIndex = getAccountIndex(sAccountname);
  if (uiIndex < m_oAccounts.size())
  {
    pRet = &m_oAccounts[uiIndex];
  }
  return pRet;
}

CcSyncAccountConfigHandle CcSyncClientConfig::getFirstAccountConfig()
{
  if (m_oAccounts.size() > 0)
  {
    return &m_oAccounts[0];
  }
  return nullptr;
}

bool CcSyncClientConfig::writeConfigFile()
{
  return m_oXmlFile.writeData(true);
}

size_t CcSyncClientConfig::getAccountIndex(const CcString & sAccount)
{
  size_t uiIndex = SIZE_MAX;
  CcString sUsername; // Extracted Username
  CcString sServer; // Extracted Username
  if (sAccount.contains("@"))
  {
    CcStringList slAccountData = sAccount.split("@");
    if (slAccountData.size() == 2)
    {
      sUsername = slAccountData[0].getLower();
      sServer = slAccountData[1].getLower();
    }
  }
  else
  {
    sUsername = sAccount.getLower();
  }
  if (sUsername.length() > 0)
  {
    for (size_t i = 0; i < m_oAccounts.size(); i++)
    {
      if (m_oAccounts[i].getName().getLower() == sUsername &&
        (sServer.length() == 0 || sServer == m_oAccounts[i].getServer().getHostname().getLower()))
      {
        uiIndex = i;
        break;
      }
    }
  }
  return uiIndex;
}
