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
 * @brief     Implemtation of class CcSyncResponse
 */
#include "CcSyncResponse.h"
#include "CcJson/CcJsonDocument.h"
#include "CcJson/CcJsonNode.h"
#include "CcJson/CcJsonArray.h"
#include "CcJson/CcJsonObject.h"
#include "CcSyncGlobals.h"
#include "CcByteArray.h"
#include "CcSyncAccountConfig.h"

CcSyncResponse::CcSyncResponse( void )
{
}

CcSyncResponse::CcSyncResponse( const CcSyncResponse& oToCopy )
{
  operator=(oToCopy);
}

CcSyncResponse::CcSyncResponse( CcSyncResponse&& oToMove )
{
  operator=(std::move(oToMove));
}

CcSyncResponse::~CcSyncResponse( void )
{
}

CcSyncResponse& CcSyncResponse::operator=(const CcSyncResponse& oToCopy)
{
  CCUNUSED(oToCopy);
  return *this;
}

CcSyncResponse& CcSyncResponse::operator=(CcSyncResponse&& oToMove)
{
  if(this != &oToMove)
  {
  }
  return *this;
}

bool CcSyncResponse::operator==(const CcSyncResponse& oToCompare) const
{
  bool bRet = false;
  CCUNUSED(oToCompare);
  return bRet;
}

bool CcSyncResponse::operator!=(const CcSyncResponse& oToCompare) const
{
  return !operator==(oToCompare);
}

bool CcSyncResponse::parseData(const CcString& oData)
{
  bool bRet = false;
  m_oData.clear();
  m_bHasAdditionalData = false;
  m_eType = ESyncCommandType::Unknown;
  CcJsonDocument oJsonDoc;
  if (oJsonDoc.parseDocument(oData))
  {
    m_oData = oJsonDoc.getJsonData().getJsonObject();
    if (getTypeFromData())
    {
      bRet = true;
    }
  }
  return bRet;
}

void CcSyncResponse::init(ESyncCommandType eCommandType)
{
  m_oData.clear();
  m_bHasAdditionalData = false;
  m_oData.add(CcJsonNode("Command", (uint16) eCommandType));
  m_eType = eCommandType;
}

bool CcSyncResponse::hasError()
{
  bool bRet = false;
  CcJsonNode& rErrorValue = m_oData[CcSyncGlobals::Commands::ErrorCode];
  if (rErrorValue.isValue() &&
      rErrorValue.getValue().isInt())
  {
    CcStatus oStatus = rErrorValue.getValue().getUint32();
    bRet = !oStatus;
  }
  return bRet;
}

CcStatus CcSyncResponse::getError()
{
  CcStatus oError = m_oData[CcSyncGlobals::Commands::ErrorCode].getValue().getUint32();
  return oError;
}

CcString CcSyncResponse::getErrorMsg()
{
  CcJsonNode& oErrorMsg = m_oData[CcSyncGlobals::Commands::ErrorMsg];
  CcString sError;
  if (oErrorMsg.isValue())
    sError = oErrorMsg.getValue().getString();
  return sError;
}

CcByteArray CcSyncResponse::getBinary()
{
  CcJsonDocument oJsonDoc(m_oData);
  CcByteArray oRet = oJsonDoc.getDocument();
  return oRet;
}

void CcSyncResponse::setLogin(const CcString& sUserToken)
{
  m_oData.add(CcJsonNode(CcSyncGlobals::Commands::AccountLogin::Session, sUserToken));
}

void CcSyncResponse::setAccountRight(ESyncRights eRights)
{
  init(ESyncCommandType::AccountRights);
  m_oData.add(CcJsonNode(CcSyncGlobals::Commands::AccountRights::Right, static_cast<int>(eRights)));
}

ESyncRights CcSyncResponse::getAccountRight() const
{
  ESyncRights eRights = ESyncRights::None;
  CcJsonNode oJsonData = m_oData[CcSyncGlobals::Commands::AccountRights::Right];
  if (oJsonData.isValue())
  {
    eRights = static_cast<ESyncRights>(oJsonData.getValue().getInt());
  }
  return eRights;
}

void CcSyncResponse::setResult(bool uiResult)
{
  m_oData.add(CcJsonNode(CcSyncGlobals::Commands::Result, uiResult));
}

void CcSyncResponse::setError(CcStatus uiErrorCode, const CcString& sErrorMsg)
{
  m_oData.add(CcJsonNode(CcSyncGlobals::Commands::ErrorCode, uiErrorCode.getErrorUint()));
  m_oData.add(CcJsonNode(CcSyncGlobals::Commands::ErrorMsg, sErrorMsg));
}

void CcSyncResponse::addAccountInfo(const CcSyncAccountConfig& oAccountConfig)
{
  m_oData.add(CcJsonNode(oAccountConfig.getJsonNode(), CcSyncGlobals::Commands::AccountGetData::Account));
}

CcSyncAccountConfig CcSyncResponse::getAccountConfig()
{
  CcSyncAccountConfig oAccountConfig;
  CcJsonNode& rAccountNode = m_oData[CcSyncGlobals::Commands::AccountGetData::Account];
  if (rAccountNode.isObject())
  {
    oAccountConfig.parseJson(rAccountNode.getJsonObject());
  }
  return oAccountConfig;
}

CcString CcSyncResponse::getSession()
{
  CcString sSession;
  if (m_oData.contains(CcSyncGlobals::Commands::AccountLogin::Session))
  {
    sSession = m_oData[CcSyncGlobals::Commands::AccountLogin::Session].getValue().getString();
  }
  return sSession;
}

void CcSyncResponse::addDirectoryDirectoryInfoList(const CcSyncFileInfoList& oDirectoryInfoList, const CcSyncFileInfoList& oFileInfoList)
{
  CcJsonNode oDirectoriesNode(EJsonDataType::Array);
  oDirectoriesNode.setName(CcSyncGlobals::Commands::DirectoryGetFileList::DirsNode);
  for (CcSyncFileInfo& oFileInfo : oDirectoryInfoList)
  {
    oDirectoriesNode.array().add(CcJsonNode( oFileInfo.getJsonObject(), ""));
  }
  m_oData.append(std::move(oDirectoriesNode));

  CcJsonNode oFilesNode(EJsonDataType::Array);
  oFilesNode.setName(CcSyncGlobals::Commands::DirectoryGetFileList::FilesNode);
  for (CcSyncFileInfo& oFileInfo : oFileInfoList)
  {
    oFilesNode.array().add(CcJsonNode(oFileInfo.getJsonObject(),""));
  }
  m_oData.append(std::move(oFilesNode));
}

bool CcSyncResponse::hasFileInfo()
{
  return false;
}

void CcSyncResponse::addFileInfo(const CcSyncFileInfo& oFileInfo)
{
  m_oData.append(std::move(oFileInfo.getJsonObject()));
}

CcSyncFileInfo CcSyncResponse::getFileInfo()
{
  CcSyncFileInfo oFileInfo;
  oFileInfo.fromJsonObject(m_oData);
  return oFileInfo;
}

bool CcSyncResponse::getDirectoryDirectoryInfoList(CcSyncFileInfoList& oDirectoryInfoList, CcSyncFileInfoList& oFileInfoList)
{
  bool bRet = false;
  if (m_oData.contains(CcSyncGlobals::Commands::DirectoryGetFileList::DirsNode))
  {
    CcJsonArray& oJsonArray = m_oData[CcSyncGlobals::Commands::DirectoryGetFileList::DirsNode].array();
    for (CcJsonNode& oJsonData : oJsonArray)
    {
      CcJsonObject& oJsonFileArray = oJsonData.object();
      CcSyncFileInfo oFileInfo;
      oFileInfo.fromJsonObject(oJsonFileArray);
      oDirectoryInfoList.append(std::move(oFileInfo));
    }
  }
  if (m_oData.contains(CcSyncGlobals::Commands::DirectoryGetFileList::FilesNode))
  {
    CcJsonArray& oJsonArray = m_oData[CcSyncGlobals::Commands::DirectoryGetFileList::FilesNode].array();
    for (CcJsonNode& oJsonData : oJsonArray)
    {
      CcJsonObject& oJsonFileArray = oJsonData.object();
      CcSyncFileInfo oFileInfo;
      oFileInfo.fromJsonObject(oJsonFileArray);
      oFileInfoList.append(std::move(oFileInfo));
    }
  }
  return bRet;
}

bool CcSyncResponse::getTypeFromData()
{
  CcJsonNode& oValue = m_oData[CcSyncGlobals::Commands::Command];
  if (oValue.isValue())
  {
    m_eType = (ESyncCommandType) oValue.value().getUint16();
    return true;
  }
  return false;
}
