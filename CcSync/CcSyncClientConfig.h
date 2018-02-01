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
 * @subpage   CcSyncClientConfig
 *
 * @page      CcSyncClientConfig
 * @copyright Andreas Dirmeier (C) 2017
 * @author    Andreas Dirmeier
 * @par       Web: http://coolcow.de
 * @version   0.01
 * @date      2016-04
 * @par       Language   C++ ANSI V3
 * @brief     Class CcSyncClientConfig
 **/
#ifndef _CcSyncClientConfig_H_
#define _CcSyncClientConfig_H_

#include "CcBase.h"
#include "CcSync.h"
#include "CcSyncAccountConfig.h"
#include "CcXml/CcXmlFile.h"
#include "CcSharedPointer.h"

class CcSyncClientConfig;
#if WIN32
template class CcSharedPointer<CcSyncClientConfig>;
#endif

typedef class CcSyncSHARED CcSharedPointer<CcSyncClientConfig> CcSyncClientConfigPointer;

/**
 * @brief Class impelmentation
 */
class CcSyncSHARED CcSyncClientConfig
{
public:
  /**
   * @brief Constructor
   */
  CcSyncClientConfig();

  /**
   * @brief Destructor
   */
  ~CcSyncClientConfig();

  bool create(const CcString& sConfigFile);
  bool readConfig(const CcString& sConfigFile);
  bool readConfigFromServer(const CcString& sConfigFile, const CcString &sBasePath);
  bool addAccount(const CcString& sUsername, const CcString& sPassword, const CcString& sServer, const CcString& sPort);
  bool removeAccount(const CcString& sAccount);
  CcSyncAccountConfigHandle getAccountConfig(const CcString& sAccountname);
  CcSyncAccountConfigHandle getFirstAccountConfig();
  const CcSyncAccountConfigList& getAccountList()
    {return m_oAccounts;}

  bool writeConfigFile();
  
private: // Methods
  size_t getAccountIndex(const CcString& sAccountname);
private:
  CcXmlFile               m_oXmlFile;
  CcSyncAccountConfigList m_oAccounts;
};

#endif /* _CcSyncClientConfig_H_ */
