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
 * @subpage   CcSyncClientCom
 *
 * @page      CcSyncClientCom
 * @copyright Andreas Dirmeier (C) 2017
 * @author    Andreas Dirmeier
 * @par       Web: http://coolcow.de
 * @version   0.01
 * @date      2016-04
 * @par       Language   C++ ANSI V3
 * @brief     Class CcSyncClientCom
 **/
#ifndef _CcSyncClientCom_H_
#define _CcSyncClientCom_H_

#include "CcBase.h"
#include "CcSync.h"
#include "CcUrl.h"
#include "Network/CcSocket.h"
#include "CcSyncRequest.h"
#include "CcSyncResponse.h"
#include "CcSslSocket.h"

/**
 * @brief Class impelmentation
 */
class CcSyncSHARED CcSyncClientCom
{
public:
  CcSyncClientCom() = default;
  ~CcSyncClientCom() = default;

  bool connect(const CcUrl& oConnect);
  bool connect()
  { return connect(m_oUrl); }
  void close();
  void reconnect();
  bool sendRequestGetResponse();

  CcSocket& getSocket()
  { return m_oSocket; }
  CcString&        getSession()
  {return m_sSession;}
  CcSyncResponse&  getResponse()
  {return m_oResponse;}
  CcSyncRequest&   getRequest()
  {return m_oRequest;}
  bool isConnected();

  void setUrl(const CcUrl& oConnect)
  { m_oUrl = oConnect; }

private:
  CcUrl           m_oUrl;
  CcSocket        m_oSocket;
  CcString        m_sSession;
  CcSyncResponse  m_oResponse;
  CcSyncRequest   m_oRequest;
  size_t          m_uiReconnections = 0;
};

#endif /* _CcSyncClientCom_H_ */
