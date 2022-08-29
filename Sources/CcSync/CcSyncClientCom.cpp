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
 * @brief     Implemtation of class CcSyncClient
 */
#include "CcSyncClientCom.h"
#include "Json/CcJsonDocument.h"
#include "Json/CcJsonObject.h"
#include "CcDateTime.h"

bool CcSyncClientCom::connect(const CcUrl& oConnect)
{
  bool bRet = false;
  if (isConnected())
  {
    bRet = true;
  }
  else
  {
    CCNEWTYPE(pSocket, CcSslSocket);
    m_oSocket = pSocket;
    if (static_cast<CcSslSocket*>(m_oSocket.getRawSocket())->initClient())
    {
      if (m_oSocket.connect(oConnect.getHostname(), oConnect.getPortString()))
      {
        // reset counter;
        m_uiReconnections = 0;
        m_oSocket.setTimeout(CcDateTimeFromSeconds(30));
        bRet = true;
      }
      else
      {
        m_uiReconnections++;
        m_oSocket.close();
      }
    }
    else
    {
      m_uiReconnections++;
      m_oSocket.close();
    }
  }
  return bRet;
}

void CcSyncClientCom::close()
{
  m_oSocket.close();
}


void CcSyncClientCom::reconnect()
{
  close();
  while (m_uiReconnections < CcSyncGlobals::MaxReconnections &&
        !connect())
  {
    if (login())
      break;
    else
      close();
  }
}

bool CcSyncClientCom::login()
{
  bool bLogin = false;
  getRequest().setAccountLogin(m_sSession);
  if (sendRequestGetResponse())
  {
    if (!getResponse().hasError())
    {
      bLogin = true;
    }
  }
  return bLogin;
}

bool CcSyncClientCom::sendRequestGetResponse()
{
  bool bRet = false;
  CcJsonDocument oJsonDoc(m_oRequest.getData());
  if (connect())
  {
    m_oResponse.clear();
    if (m_oSocket.writeArray(oJsonDoc.getDocument()))
    {
      if (m_oRequest.getCommandType() != ESyncCommandType::Close)
      {
        CcString sRead;
        size_t uiReadSize = 0;
        CcByteArray oLastRead(static_cast<size_t>(CcSyncGlobals::MaxResponseSize));
        do
        {
          uiReadSize = m_oSocket.readArray(oLastRead, false);
          if (uiReadSize > 0 && uiReadSize <= CcSyncGlobals::MaxResponseSize)
          {
            sRead.append(oLastRead, 0, uiReadSize);
          }
        } while (uiReadSize <= CcSyncGlobals::MaxResponseSize &&
          sRead.length() <= CcSyncGlobals::MaxResponseSize  &&
          CcJsonDocument::isValidData(sRead) == false);

        if (uiReadSize > CcSyncGlobals::MaxResponseSize)
        {
          CcSyncLog::writeError("Read from socket failed, try reconnection", ESyncLogTarget::Client);
          CcSyncLog::writeError("Request:", ESyncLogTarget::Client);
          CcSyncLog::writeError(oJsonDoc.getDocument(), ESyncLogTarget::Client);
          CcSyncLog::writeError("Response size: " + CcString::fromSize(sRead.length()), ESyncLogTarget::Client);
          CcSyncLog::writeError("Response 24 signs:", ESyncLogTarget::Client);
          CcSyncLog::writeError(sRead.substr(0, 24), ESyncLogTarget::Client);
        }
        else if (sRead.length() > CcSyncGlobals::MaxResponseSize)
        {
          CcSyncLog::writeError("Incoming data exceed maximum", ESyncLogTarget::Client);
          CcSyncLog::writeError("Request:", ESyncLogTarget::Client);
          CcSyncLog::writeError(oJsonDoc.getDocument(), ESyncLogTarget::Client);
          CcSyncLog::writeError("Response 24 signs:", ESyncLogTarget::Client);
          CcSyncLog::writeError(sRead.substr(0, 24), ESyncLogTarget::Client);
          reconnect();
        }
        else
        {
          m_oResponse.parseData(sRead);
          if (m_oResponse.getCommandType() == m_oRequest.getCommandType())
          {
            if (m_oResponse.hasError() == false)
            {
              bRet = true;
            }
          }
          else
          {
            CcSyncLog::writeError("Wrong server response, try reconnect.", ESyncLogTarget::Client);
            CcJsonDocument oDoc(m_oRequest.getData());
            CCDEBUG(oDoc.getDocument());
            CCDEBUG(CcString(sRead));
            reconnect();
          }
        }
      }
    }
    else
    {
      CcSyncLog::writeError("Get response failed, try reconnect.", ESyncLogTarget::Client);
      reconnect();
    }
  }
  return bRet;
}

bool CcSyncClientCom::isConnected()
{
  return m_oSocket.isValid() && m_uiReconnections < CcSyncGlobals::MaxReconnections;
}
