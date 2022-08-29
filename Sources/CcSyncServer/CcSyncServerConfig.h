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
 * @subpage   CcSyncServerConfig
 *
 * @page      CcSyncServerConfig
 * @copyright Andreas Dirmeier (C) 2017
 * @author    Andreas Dirmeier
 * @par       Web: http://coolcow.de
 * @version   0.01
 * @date      2016-04
 * @par       Language   C++ ANSI V3
 * @brief     Class CcSyncServerConfig
 **/
#ifndef _CcSyncServerConfig_H_
#define _CcSyncServerConfig_H_

#include "CcBase.h"
#include "CcSync.h"
#include "CcString.h"
#include "CcSyncServerAccount.h"
#include "CcSyncServerLocationConfig.h"
#include "CcSyncUser.h"
#include "Xml/CcXmlFile.h"

class CcXmlNode;

/**
 * @brief Class impelmentation
 */
class CcSyncServerConfig
{
public:
  /**
   * @brief Constructor
   */
  CcSyncServerConfig();
  
  uint16 getPort() const
    { return m_uiPort; }
  const CcString& getSslCertFile() const
    { return m_sSslCertFile; }
  const CcString& getSslKeyFile() const
    { return m_sSslKeyFile; }
  const CcSyncServerAccountList& getAccountList() const
    {return m_oAccountList; }
  const CcSyncServerLocationConfig& getLocation() const
    {return m_oLocation; }
  
  void setConfigDir(const CcString& sConfigDir);
  void setPort(uint16 uiPort)
    { m_uiPort = uiPort; }
  void setSslCertFile(const CcString& sSslCertFile)
    { m_sSslCertFile = sSslCertFile; }
  void setSslKeyFile(const CcString& sSslKeyFile)
    { m_sSslKeyFile = sSslKeyFile; }
  void addAccount(const CcString& sName, const CcString&sPassword);
  void addAdminAccount(const CcString& sName, const CcString&sPassword);
  void setLocation(const CcString& sPath);

  bool readConfig(const CcString& sConfigFile);
  bool writeConfig(const CcString& sConfigFile);
  CcSyncServerAccount* findAccount(const CcString& sAccountName) const;
  bool removeAccount(const CcString& sAccountName);

  bool writeConfigFile();

  bool isValid()
    { return m_bValid; }

  CcSyncServerConfig& operator=(const CcSyncServerConfig& oToCopy);
  CcSyncServerConfig& operator=(CcSyncServerConfig&& oToMove);

private:
  bool findServerConfig(CcXmlNode& pNode);
  bool findLocationsConfig(CcXmlNode& pNode);
  bool findAccountConfigs(CcXmlNode& pNode);

private:
  bool     m_bValid = false;
  uint16   m_uiPort;
  bool     m_bSsl = true;
  bool     m_bSslRequired = true;
  CcString m_sConfigDir;
  CcString m_sSslCertFile;
  CcString m_sSslKeyFile;
  CcPassword m_oRootPassword;
  CcSyncServerLocationConfig m_oLocation;
  CcSyncServerAccountList  m_oAccountList;
  CcXmlFile m_oXmlFile;
};

#endif /* _CcSyncServerConfig_H_ */
