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
 * @page      CcSync
 * @subpage   CcSyncAccountConfig
 *
 * @page      CcSyncAccountConfig
 * @copyright Andreas Dirmeier (C) 2017
 * @author    Andreas Dirmeier
 * @par       Web: http://coolcow.de
 * @version   0.01
 * @date      2016-04
 * @par       Language   C++ ANSI V3
 * @brief     Class CcSyncAccountConfig
 **/
#ifndef _CcSyncAccountConfig_H_
#define _CcSyncAccountConfig_H_

#include "CcBase.h"
#include "CcSync.h"
#include "CcUrl.h"
#include "CcString.h"
#include "CcPassword.h"
#include "CcHandle.h"
#include "CcSyncDirectoryConfigList.h"

class CcXmlNode;
class CcJsonObject;
class CcSyncClientConfig;
class CcSyncAccountConfig;

/**
 * @brief Class impelmentation
 */
class CcSyncSHARED CcSyncAccountConfig
{
public:
  /**
   * @brief Constructor
   */
  CcSyncAccountConfig(CcSyncClientConfig* pClientConfig = nullptr);
  
  /**
   * @brief Constructor
   */
  CcSyncAccountConfig(const CcSyncAccountConfig& oToCopy)
    { operator=(oToCopy);}
  
  /**
   * @brief Constructor
   */
  CcSyncAccountConfig(CcSyncAccountConfig&& oToMove)
    { operator=(std::move(oToMove));}
  
  /**
   * @brief Constructor
   */
  CcSyncAccountConfig(const CcJsonObject& pAccountNode);

  CcSyncAccountConfig(const CcString& sUsername, const CcString& sPassword, const CcString& sServer, const CcString& sPort, CcSyncClientConfig *pClientConfig);

  /**
   * @brief compare two items
   * @param oToCompare: Item to compare to
   * @return true if they are the same
   */
  inline bool operator==(const CcSyncAccountConfig& oToCompare) const
    { return m_sName == oToCompare.m_sName; }

  /**
   * @brief compare two items
   * @param oToCompare: Item to compare to
   * @return true if they are not same
   */
  inline bool operator!=(const CcSyncAccountConfig& oToCompare) const
    { return m_sName != oToCompare.m_sName; }

  CcSyncAccountConfig& operator=(const CcSyncAccountConfig& oToCopy);
  CcSyncAccountConfig& operator=(CcSyncAccountConfig&& oToMove);

  bool parseXml(CcXmlNode& pAccountNode);
  bool parseJson(const CcJsonObject& pAccountNode);
  bool writeConfig(CcXmlNode& pParentNode);
  bool checkLogin(const CcString sUsername, const CcPassword oPassword);
  bool changePassword(const CcString& sPassword);
  bool changeHostname(const CcString& sHostName);
  bool changePort(const CcString& sPort);

  bool addAccountDirectory(const CcString& sDirectoryName, const CcString& sDirectoryPath);
  bool removeAccountDirectory(const CcString& sDirectoryName);
  void overrideLocations(const CcString& sBasePath);
  
  bool isValid()
    { return m_bValid; }

  const CcUrl& getServer() const
    { return m_oServer; }
  
  CcSyncDirectoryConfigList& directoryList()
    { return m_oDirectoryList; }

  
  void setDatabaseFilePath(const CcString& sFilePath)
    { m_sDatabaseFile = sFilePath; }
  
  const CcString& getName() const
    { return m_sName; }
  CcString getAccountDirName();
  const CcPassword& getPassword() const
    { return m_oPassword; }
  const CcString& getDatabaseFilePath() const
    { return m_sDatabaseFile; }
  const CcSyncDirectoryConfigList& getDirectoryList() const
    { return m_oDirectoryList; }
  CcXmlNode getXmlNode() const;
  CcJsonObject getJsonNode() const;

  bool writeConfigFile();

private:
  void init();
  bool xmlFindServerConfig(CcXmlNode& pNode);
  bool xmlFindUserConfig(CcXmlNode& pNode);
  bool xmlFindDirectoriesConfig(CcXmlNode& pNode);
  bool xmlFindDatabaseConfig(CcXmlNode& pNode);
  bool jsonFindServerConfig(const CcJsonObject& pNode);
  bool jsonFindUserConfig(const CcJsonObject& pNode);
  bool jsonFindDirectoriesConfig(const CcJsonObject& pNode);
  bool jsonFindCommandsConfig(const CcJsonObject& pNode);
  bool jsonFindDatabaseConfig(const CcJsonObject& pNode);

private:
  bool        m_bValid = false;
  CcString    m_sName;
  CcPassword  m_oPassword;
  CcUrl       m_oServer;
  CcString    m_sDatabaseFile;
  CcSyncDirectoryConfigList m_oDirectoryList;
  CcXmlNode*            m_pAccountNode  = nullptr;
  CcSyncClientConfig*   m_pClientConfig = nullptr;
};

#ifdef WIN32
template class CcSyncSHARED CcList<CcSyncAccountConfig>;
template class CcSyncSHARED CcHandle<CcSyncAccountConfig>;
#endif

typedef CcList<CcSyncAccountConfig> CcSyncAccountConfigList;
typedef class CcHandle<CcSyncAccountConfig> CcSyncAccountConfigHandle;

#endif /* _CcSyncAccountConfig_H_ */
