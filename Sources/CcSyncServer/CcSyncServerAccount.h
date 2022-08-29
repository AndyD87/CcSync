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
 * @subpage   CcSyncServerAccount
 *
 * @page      CcSyncServerAccount
 * @copyright Andreas Dirmeier (C) 2017
 * @author    Andreas Dirmeier
 * @par       Web: http://coolcow.de
 * @version   0.01
 * @date      2016-04
 * @par       Language   C++ ANSI V3
 * @brief     Class CcSyncServerAccount
 **/
#ifndef _CcSyncServerAccount_H_
#define _CcSyncServerAccount_H_

#include "CcBase.h"
#include "CcSync.h"
#include "CcList.h"
#include "CcString.h"
#include "CcPassword.h"
#include "CcSyncDbClient.h"
#include "CcSyncClientConfig.h"

class CcXmlNode;
class CcSyncServerAccount;

typedef class CcList<CcSyncServerAccount> CcSyncServerAccountList;

/**
 * @brief Class impelmentation
 */
class CcSyncServerAccount
{
public:
  /**
   * @brief Constructor
   */
  CcSyncServerAccount()
    {}
  CcSyncServerAccount(const CcString& sName, const CcString& sPassword, bool bAdmin);
  CcSyncServerAccount(CcXmlNode& pAccountNode);
  CcSyncServerAccount(const CcSyncServerAccount& oToCopy);
  CcSyncServerAccount(CcSyncServerAccount&& oToMove);
  ~CcSyncServerAccount(void);

  CcSyncServerAccount& operator=(const CcSyncServerAccount& oToCopy);
  CcSyncServerAccount& operator=(CcSyncServerAccount&& oToMove);

  bool operator==(const CcSyncServerAccount&) const
    {return false;}

  inline bool isValid() const
    { return m_bIsValid; }
  inline bool isAdmin()const
    { return m_bIsAdmin; }

  CcSyncClientConfigPointer clientConfig(const CcString& sClientLocation);
  CcSyncDbClientPointer database(const CcString& sClientLocation);
  bool writeConfig(CcXmlNode& oParent);

  inline const CcString& getName() const
    { return m_sName; }
  inline const CcPassword& getPassword() const
    { return m_oPassword; }

private:
  bool                  m_bIsAdmin = false;
  CcString              m_sName;
  CcPassword            m_oPassword;
  CcSyncClientConfigPointer   m_pClientConfig = nullptr;
  CcSyncDbClientPointer       m_pDatabase = nullptr;
  bool                        m_bIsValid = false;
};

#endif /* _CcSyncServerAccount_H_ */
