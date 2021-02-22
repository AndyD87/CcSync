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
 * @brief     Implemtation of class CcSyncServerAccountClientCommand
 */
#include "CcSyncGlobals.h"
#include "CcSyncServerAccount.h"
#include "CcXml/CcXmlNode.h"
#include "CcXml/CcXmlUtil.h"
#include "CcStringUtil.h"
#include "CcSyncClientConfig.h"
#include "CcSyncDbClient.h"
#include "CcSqlite.h"
#include "CcFile.h"
#include "Hash/CcSha256.h"
#include "CcDirectory.h"
#include "CcSyncLog.h"
#include "CcSyncDbClient.h"
#include "CcSyncConsole.h"

CcSyncServerAccount::CcSyncServerAccount(const CcString& sName, const CcString& sPassword, bool bAdmin) :
  m_bIsAdmin(bAdmin),
  m_sName(sName)
{
  CcString sPasswordCrypt = sName.getLower() + sPassword;
  sPasswordCrypt = CcSha256().generateByteArray(sPasswordCrypt.getByteArray()).getValue().getHexString();
  m_oPassword.setPassword(sPasswordCrypt, EHashType::Sha256);
}

CcSyncServerAccount::CcSyncServerAccount(CcXmlNode& pAccountNode)
{
  CcXmlNode& pTempNode = pAccountNode.getNode(CcSyncGlobals::Server::ConfigTags::AccountName);
  if (pTempNode.isNotNull())
  {
    m_sName = pTempNode.innerText();
    CcXmlNode& pAdmin = pAccountNode.getNode(CcSyncGlobals::Client::ConfigTags::Admin);
    if (pAdmin.isNotNull())
    {
      m_bIsAdmin = CcXmlUtil::getBoolFromNodeValue(pAdmin, false);
    }
    CcXmlNode pPasswordNode = pAccountNode.getNode(CcSyncGlobals::Server::ConfigTags::AccountPassword);
    if (pPasswordNode.isNotNull())
    {
      m_bIsValid = true;
      m_oPassword.setPassword(pPasswordNode.innerText());
    }
  }
}

CcSyncServerAccount::CcSyncServerAccount(const CcSyncServerAccount& oToCopy)
{
  operator=(oToCopy);
}

CcSyncServerAccount::CcSyncServerAccount(CcSyncServerAccount&& oToMove)
{
  operator=(std::move(oToMove));
}

CcSyncServerAccount::~CcSyncServerAccount(void)
{
}

CcSyncServerAccount& CcSyncServerAccount::operator=(const CcSyncServerAccount& oToCopy)
{
  m_sName = oToCopy.m_sName;
  m_oPassword = oToCopy.m_oPassword;
  m_pDatabase = oToCopy.m_pDatabase;
  m_pClientConfig = oToCopy.m_pClientConfig;
  m_bIsAdmin = oToCopy.m_bIsAdmin;
  m_bIsValid = oToCopy.m_bIsValid;
  return *this;
}

CcSyncServerAccount& CcSyncServerAccount::operator=(CcSyncServerAccount&& oToMove)
{
  if (this != &oToMove)
  {
    m_sName = std::move(oToMove.m_sName);
    m_oPassword = std::move(oToMove.m_oPassword);
    m_pClientConfig = oToMove.m_pClientConfig;
    m_pDatabase = oToMove.m_pDatabase;
    m_bIsAdmin = oToMove.m_bIsAdmin;
    m_bIsValid = oToMove.m_bIsValid;
  }
  return *this;
}

CcSyncClientConfigPointer CcSyncServerAccount::clientConfig(const CcString& sClientLocation)
{
  if (m_pClientConfig == nullptr)
  {
    CcSyncClientConfigPointer pClientConfig;
    CCNEW(pClientConfig, CcSyncClientConfig);
    CcString sConfigFilePath(sClientLocation);
    if (CcDirectory::exists(sConfigFilePath) ||
        CcDirectory::create(sConfigFilePath, true))
    {
      sConfigFilePath.appendPath(CcSyncGlobals::Client::ConfigFileName);
      if (CcFile::exists(sConfigFilePath))
      {
        if (pClientConfig->readConfigFromServer(sConfigFilePath, sClientLocation))
        {
          m_pClientConfig = pClientConfig;
        }
      }
      else
      {
        pClientConfig->create(sConfigFilePath);
        pClientConfig->addAccount(m_sName, "", "", "");
        m_pClientConfig = pClientConfig;
      }
    }
    else
    {
      CcSyncLog::writeDebug("Client config dir not found and not created.", ESyncLogTarget::Server);
    }
  }
  return m_pClientConfig;
}

CcSyncDbClientPointer CcSyncServerAccount::database(const CcString& sClientLocation)
{
  if (m_pDatabase == nullptr)
  {
    if (clientConfig(sClientLocation) != nullptr)
    {
      CCNEW(m_pDatabase, CcSyncDbClient);
      m_pDatabase->historyEnable();
      CcString sConfigFilePath(sClientLocation);
      sConfigFilePath.appendPath(CcSyncGlobals::Client::DatabaseFileName);
      if (m_pDatabase->openDatabase(sConfigFilePath))
      {
        return m_pDatabase;
      }
    }
  }
  return m_pDatabase;
}

bool CcSyncServerAccount::writeConfig(CcXmlNode& oNode)
{
  CcXmlNode oAccountNode(CcSyncGlobals::Server::ConfigTags::Account);
  CcXmlNode oAccountNodeName(CcSyncGlobals::Server::ConfigTags::AccountName);
  oAccountNodeName.setInnerText(m_sName);
  CcXmlNode oAccountNodePassword(CcSyncGlobals::Server::ConfigTags::AccountPassword);
  oAccountNodePassword.setInnerText(m_oPassword.getString());
  CcXmlNode oAccountNodeAdmin(CcSyncGlobals::Server::ConfigTags::AccountAdmin);
  oAccountNodeAdmin.setInnerText(CcXmlUtil::getStringFromBool(m_bIsAdmin));
  oAccountNode.append(std::move(oAccountNodeName));
  oAccountNode.append(std::move(oAccountNodePassword));
  oAccountNode.append(std::move(oAccountNodeAdmin));
  oNode.append(std::move(oAccountNode));
  return true;
}
