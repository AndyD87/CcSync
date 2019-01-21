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
 * @brief     Implemtation of class CcSyncLocation
 */
#include "CcSyncFileInfo.h"
#include "CcSyncGlobals.h"
#include "CcDirectory.h"
#include "CcFile.h"
#include "CcFileInfo.h"
#include "CcStringUtil.h"
#include "CcJson/CcJsonObject.h"
#include "Hash/CcCrc32.h"

bool CcSyncFileInfo::operator==(const CcSyncFileInfo& oToCompare) const
{
  if (oToCompare.getId()    == m_uiId &&
      oToCompare.getDirId() == m_uiDirId &&
      oToCompare.getName()  == m_sFilename)
  {
    if (getIsFile())
    {
      if (oToCompare.m_oLastModified == m_oLastModified && 
          oToCompare.m_oChanged == m_oChanged &&
          oToCompare.m_uiFileSize == m_uiFileSize )
      {
        return true;
      }
    }
    else
    {
      // check the md5 changed value in directories
      if (oToCompare.getMd5() == getMd5())
        return true;
    }
  }
  else
  {
    return false;
  }
  return false;
}

bool CcSyncFileInfo::operator==(const CcFileInfo& oToCompare) const
{
  bool bRet = false;
  if (oToCompare.getName() == m_sFilename)
  {
    if (getIsFile())
    {
      if (oToCompare.getModified().getTimestampS() == m_oLastModified &&
          oToCompare.getFileSize() == m_uiFileSize)
      {
        bRet = true;
      }
      else
      {
        bRet = false;
      }
    }
    else
    {
      bRet = true;
    }
  }
  else
  {
    bRet = false;
  }
  return bRet;
}

bool CcSyncFileInfo::fromSystemFile(bool bWithCrc)
{
  bool bRet = false;
  m_sSystemFullPath = m_sSystemRootPath;
  m_sSystemFullPath.appendPath(m_sDirPath);
  m_sSystemFullPath.appendPath(getName());
  CcFile oFile(m_sSystemFullPath);
  if (oFile.exists())
  {
    bRet = true;
    CcFileInfo oFileInfo = oFile.getInfo();
    m_uiFileSize    = oFileInfo.getFileSize();
    m_oLastModified = oFileInfo.getModified().getTimestampS();
    m_bIsFile       = oFileInfo.isFile();
    m_sAttributes   = oFileInfo.getAttributesString();
    if(m_bIsFile && bWithCrc)
      m_oCrc = CcFile::getCrc32(m_sSystemFullPath).getValueUint32();
  }
  return bRet;
}

bool CcSyncFileInfo::fromSystemDirectory()
{
  bool bRet = false;
  m_sSystemFullPath = m_sSystemRootPath;
  m_sSystemFullPath.appendPath(m_sDirPath);
  m_sSystemFullPath.appendPath(getName());
  CcFile oDir(m_sSystemFullPath);
  if (oDir.isDir())
  {
    m_oLastModified = oDir.getInfo().getModified().getTimestampS();
    m_sAttributes = oDir.getInfo().getAttributesString();
    bRet = true;
  }
  return bRet;
}

bool CcSyncFileInfo::fromJsonObject(const CcJsonObject& oJsonData)
{
  bool bRet = false;
  const CcJsonData& oIdNode = oJsonData[CcSyncGlobals::FileInfo::Id];
  if (oIdNode.isValue())
    id() = oIdNode.getValue().getSize();

  const CcJsonData& oDirIdNode = oJsonData[CcSyncGlobals::FileInfo::DirId];
  if (oDirIdNode.isValue())
    dirId() = oDirIdNode.getValue().getSize();

  const CcJsonData& oNameNode = oJsonData[CcSyncGlobals::FileInfo::Name];
  if (oNameNode.isValue())
    name() = oNameNode.getValue().getString();

  const CcJsonData& oSizeNode = oJsonData[CcSyncGlobals::FileInfo::Size];
  if (oSizeNode.isValue())
    fileSize() = oSizeNode.getValue().getUint64();

  const CcJsonData& oModifiedNode = oJsonData[CcSyncGlobals::FileInfo::Modified];
  if (oModifiedNode.isValue())
    modified() = oModifiedNode.getValue().getUint64();

  const CcJsonData& oAttributesNode = oJsonData[CcSyncGlobals::FileInfo::Attributes];
  if (oAttributesNode.isValue())
    attributes() = oAttributesNode.getValue().getString();

  const CcJsonData& oCRCNode = oJsonData[CcSyncGlobals::FileInfo::CRC];
  if (oCRCNode.isValue())
    crc() = oCRCNode.getValue().getUint32();

  const CcJsonData& oMD5Node = oJsonData[CcSyncGlobals::FileInfo::MD5];
  if (oMD5Node.isValue())
    md5().setHexString(oMD5Node.getValue().getString());

  const CcJsonData& oChangedNode = oJsonData[CcSyncGlobals::FileInfo::Changed];
  if (oChangedNode.isValue())
    changed() = oChangedNode.getValue().getUint64();

  const CcJsonData& oIsFileNode = oJsonData[CcSyncGlobals::FileInfo::IsFile];
  if (oIsFileNode.isValue())
    isFile() = oIsFileNode.getValue().getBool();

  return bRet;
}

CcJsonObject CcSyncFileInfo::getJsonObject() const
{
  CcJsonObject oFileData;
  if(getId() != 0)
    oFileData.add(CcJsonData(CcSyncGlobals::FileInfo::Id, getId()));

  if (getDirId() != 0)
    oFileData.add(CcJsonData(CcSyncGlobals::FileInfo::DirId, getDirId()));

  if (getName().length() != 0)
    oFileData.add(CcJsonData(CcSyncGlobals::FileInfo::Name, getName()));

  if (getFileSize() != 0)
    oFileData.add(CcJsonData(CcSyncGlobals::FileInfo::Size, getFileSize()));

  if (getModified() != 0)
    oFileData.add(CcJsonData(CcSyncGlobals::FileInfo::Modified, getModified()));

  if (getAttributes().length() != 0)
    oFileData.add(CcJsonData(CcSyncGlobals::FileInfo::Attributes, getAttributes()));

  if (getCrc() != 0)
    oFileData.add(CcJsonData(CcSyncGlobals::FileInfo::CRC, getCrc()));

  if (getMd5().size() > 0)
    oFileData.add(CcJsonData(CcSyncGlobals::FileInfo::MD5, getMd5().getHexString()));

  if (getChanged() != 0)
    oFileData.add(CcJsonData(CcSyncGlobals::FileInfo::Changed, getChanged()));

  if (getIsFile() == false)
    oFileData.add(CcJsonData(CcSyncGlobals::FileInfo::IsFile, getIsFile()));

  return oFileData;
}

const CcString& CcSyncFileInfo::getSystemFullPath()
{
  if (m_sSystemFullPath.length() > 0)
  {
    return m_sSystemFullPath;
  }
  else
  {
    m_sSystemFullPath = m_sSystemRootPath;
    m_sSystemFullPath.appendPath(m_sDirPath);
    m_sSystemFullPath.appendPath(m_sFilename);
    return m_sSystemFullPath;
  }
}

CcString CcSyncFileInfo::getSystemDirPath()
{
  CcString sPath = m_sSystemRootPath;
  sPath.appendPath(m_sDirPath);
  return sPath;
}

CcString CcSyncFileInfo::getRelativePath()
{
  CcString sPath = m_sDirPath;
  sPath.appendPath(m_sFilename);
  return sPath;
}
