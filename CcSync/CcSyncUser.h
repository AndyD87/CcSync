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
 * @subpage   CcSyncUser
 *
 * @page      CcSyncUser
 * @copyright Andreas Dirmeier (C) 2017
 * @author    Andreas Dirmeier
 * @par       Web: http://coolcow.de
 * @version   0.01
 * @date      2016-04
 * @par       Language   C++ ANSI V3
 * @brief     Class CcSyncUser
 **/
#ifndef _CcSyncUser_H_
#define _CcSyncUser_H_

#include "CcBase.h"
#include "CcSync.h"
#include "CcSyncGlobals.h"
#include "CcSyncClientConfig.h"
#include "CcSyncDbClient.h"

class CcXmlNode;
class CcSyncUser;
class CcSyncAccountConfig;

/**
 * @brief Class impelmentation
 */
class CcSyncSHARED CcSyncUser
{
public:
  CcSyncUser()
    {}

  CcSyncUser(const CcSyncUser& oToCopy)
    { operator=(oToCopy); }
  
  CcSyncUser(CcSyncUser&& oToMove)
    { operator=(std::move(oToMove)); }

  /**
   * @brief Constructor
   */
  CcSyncUser(CcString sToken, CcSyncClientConfigPointer& pClientConfig, CcSyncAccountConfigHandle& pAccountConfig, CcSyncDbClientPointer& pDatabase, ESyncRights eRights) :
    m_sToken(sToken),
    m_pClientConfig(pClientConfig),
    m_pAccountConfig(pAccountConfig),
    m_pDatabase(pDatabase),
    m_eRights(eRights),
    m_bIsValid(true)
  {}

  ~CcSyncUser(void);

  bool isValid() const
    { return m_bIsValid; }

  const CcString& getToken() const
    { return m_sToken; }
  CcSyncClientConfigPointer& getClientConfig()
    { return m_pClientConfig; }
  CcSyncAccountConfigHandle& getAccountConfig()
    { return m_pAccountConfig; }
  CcSyncDbClientPointer& getDatabase()
    { return m_pDatabase; }
  ESyncRights getRights() const
    { return m_eRights; }

  CcSyncUser& operator=(const CcSyncUser& oToCopy);
  CcSyncUser& operator=(CcSyncUser&& oToMove);
private:
  CcString                  m_sToken;
  CcSyncClientConfigPointer m_pClientConfig = nullptr;
  CcSyncAccountConfigHandle m_pAccountConfig = nullptr;
  CcSyncDbClientPointer     m_pDatabase = nullptr;
  ESyncRights               m_eRights = ESyncRights::None;
  bool                      m_bIsValid = false;
};

#endif /* _CcSyncUser_H_ */
