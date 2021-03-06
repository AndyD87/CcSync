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
 * @copyright Andreas Dirmeier (C) 2016
 * @author    Andreas Dirmeier
 * @par       Web: http://coolcow.de
 * @version   0.01
 * @date      2016-04
 * @par       Language   C++ ANSI V3
 * @brief     Implemtation of class CcSyncDbClient
 */
#include "CcSyncDbClient.h"
#include "CcString.h"
#include "CcSqlite.h"
#include "CcSyncGlobals.h"
#include "CcSyncFileInfo.h"
#include "CcSyncFileInfoList.h"
#include "CcKernel.h"
#include "CcDateTime.h"
#include "Hash/CcMd5.h"
#include "CcDirectory.h"
#include "CcSyncLog.h"

CcSyncDbClient::CcSyncDbClient( const CcString& sPath )
{
  openDatabase(sPath);
}

CcSyncDbClient::CcSyncDbClient( const CcSyncDbClient& oToCopy )
{
  operator=(oToCopy);
}

CcSyncDbClient::CcSyncDbClient( CcSyncDbClient&& oToMove )
{
  operator=(std::move(oToMove));
}

CcSyncDbClient::~CcSyncDbClient( void )
{
  if (m_pDatabase != nullptr)
  {
    if (m_uiTransactionCnt > 0)
    {
      m_pDatabase->endTransaction();
    }
    m_pDatabase->close();
  }
}

CcSyncDbClient& CcSyncDbClient::operator=(const CcSyncDbClient& oToCopy)
{
  CCUNUSED(oToCopy);
  return *this;
}

CcSyncDbClient& CcSyncDbClient::operator=(CcSyncDbClient&& oToMove)
{
  if(this != &oToMove)
  {
  }
  return *this;
}

bool CcSyncDbClient::operator==(const CcSyncDbClient& oToCompare) const
{
  bool bRet = false;
  CCUNUSED(oToCompare);
  return bRet;
}

bool CcSyncDbClient::operator!=(const CcSyncDbClient& oToCompare) const
{
  return !operator==(oToCompare);
}

bool CcSyncDbClient::openDatabase(const CcString& sPath)
{
  bool bRet = false;
  CCNEW(m_pDatabase, CcSqlite);
  m_pDatabase->setDatabase(sPath);
  if (m_pDatabase->open())
  {
    bRet = true;
  }
  else
  {
    CcSyncLog::writeError("Unable to open database for " + sPath);
  }
  return bRet;
}

bool CcSyncDbClient::setupDirectory(const CcString& sDirName)
{
  bool bRet = true;
  CcSqlResult oResult; // Temporary Result object
  if (!m_pDatabase->tableExists(sDirName + CcSyncGlobals::Database::DirectoryListAppend))
  {
    CcString sSqlCreateTable = getDbCreateDirectoryList(sDirName);
    oResult = m_pDatabase->query(sSqlCreateTable);
    if (oResult.error())
    {
      CcSyncLog::writeError("Failed to create Table: " + sDirName + CcSyncGlobals::Database::DirectoryListAppend);
    }
  }

  if (!m_pDatabase->tableExists(sDirName + CcSyncGlobals::Database::FileListAppend))
  {
    CcString sSqlCreateTable = getDbCreateFileList(sDirName);
    oResult = m_pDatabase->query(sSqlCreateTable);
    if (oResult.error())
    {
      bRet &= false;
      CcSyncLog::writeError("Failed to create Table: " + sDirName + CcSyncGlobals::Database::FileListAppend);
    }
  }

  if (!m_pDatabase->tableExists(sDirName + CcSyncGlobals::Database::QueueAppend))
  {
    CcString sSqlCreateTable = getDbCreateQueue(sDirName);
    oResult = m_pDatabase->query(sSqlCreateTable);
    if (oResult.error())
    {
      bRet &= false;
      CcSyncLog::writeError("Failed to create Table: " + sDirName + CcSyncGlobals::Database::QueueAppend);
    }
  }

  if (!m_pDatabase->tableExists(sDirName + CcSyncGlobals::Database::HistoryAppend))
  {
    CcString sSqlCreateTable = getDbCreateHistory(sDirName);
    oResult = m_pDatabase->query(sSqlCreateTable);
    if (oResult.error())
    {
      bRet &= false;
      CcSyncLog::writeError("Failed to create Table: " + sDirName + CcSyncGlobals::Database::HistoryAppend);
    }
  }

  CcString sQuery = "SELECT ";
  sQuery << CcSyncGlobals::Database::DirectoryList::Id +
            " FROM " << sDirName + CcSyncGlobals::Database::DirectoryListAppend +
            " WHERE `" << CcSyncGlobals::Database::DirectoryList::DirId << "` IS NULL AND Name = '.'";
  oResult = m_pDatabase->query(sQuery);
  if (!oResult.error())
  {
    CcSyncFileInfo oFile;
    oFile.id() = 1;
    oFile.name() = "root";
    oFile.changed() = CcKernel::getDateTime().getTimestampS();
    directoryListInsert(sDirName, oFile, true);
  }
  else
  {
    bRet &= false;
    CcSyncLog::writeError("Something wents wrong in CcSyncDbClient Constructor");
  }
  return bRet;
}

bool CcSyncDbClient::removeDirectory(const CcString& sDirName)
{
  bool bRet = true;
  CcSqlResult oResult; // Temporary Result object
  if (!m_pDatabase->tableExists(sDirName + CcSyncGlobals::Database::DirectoryListAppend))
  {
    CcString sTableName = sDirName + CcSyncGlobals::Database::DirectoryListAppend;
    CcString sDropTable;
    sDropTable << CcSyncGlobals::Database::DropTable << sTableName << "`";
    oResult = m_pDatabase->query(sDropTable);
    if (oResult.error())
    {
      CcSyncLog::writeError("Failed to delete Table: " + sTableName);
    }
  }

  if (!m_pDatabase->tableExists(sDirName + CcSyncGlobals::Database::FileListAppend))
  {
    CcString sTableName = sDirName + CcSyncGlobals::Database::FileListAppend;
    CcString sDropTable;
    sDropTable << CcSyncGlobals::Database::DropTable << sTableName << "`";
    oResult = m_pDatabase->query(sDropTable);
    if (oResult.error())
    {
      CcSyncLog::writeError("Failed to delete Table: " + sTableName);
    }
  }

  if (!m_pDatabase->tableExists(sDirName + CcSyncGlobals::Database::QueueAppend))
  {
    CcString sTableName = sDirName + CcSyncGlobals::Database::QueueAppend;
    CcString sDropTable;
    sDropTable << CcSyncGlobals::Database::DropTable << sTableName << "`";
    oResult = m_pDatabase->query(sDropTable);
    if (oResult.error())
    {
      CcSyncLog::writeError("Failed to delete Table: " + sTableName);
    }
  }

  if (!m_pDatabase->tableExists(sDirName + CcSyncGlobals::Database::HistoryAppend))
  {
    CcString sTableName = sDirName + CcSyncGlobals::Database::HistoryAppend;
    CcString sDropTable;
    sDropTable << CcSyncGlobals::Database::DropTable << sTableName << "`";
    oResult = m_pDatabase->query(sDropTable);
    if (oResult.error())
    {
      CcSyncLog::writeError("Failed to delete Table: " + sTableName);
    }
  }

  return bRet;
}

void CcSyncDbClient::cleanDirectory(const CcString& sDirName)
{
  CcString sSql;
  m_pDatabase->beginTransaction();
  sSql << "DELETE FROM '" << sDirName << CcSyncGlobals::Database::DirectoryListAppend << "'";
  m_pDatabase->query(sSql);
  sSql << "DELETE FROM '" << sDirName << CcSyncGlobals::Database::FileListAppend << "'";
  m_pDatabase->query(sSql);
  sSql << "DELETE FROM '" << sDirName << CcSyncGlobals::Database::QueueAppend << "'";
  m_pDatabase->query(sSql);
  sSql << "DELETE FROM '" << sDirName << CcSyncGlobals::Database::HistoryAppend << "'";
  m_pDatabase->query(sSql);
  setupDirectory(sDirName);
  m_pDatabase->endTransaction();
}

void CcSyncDbClient::beginTransaction()
{
  if (m_uiTransactionCnt == 0)
  {
    m_pDatabase->beginTransaction();
    m_uiTransactionCnt++;
  }
  else
  {
    m_uiTransactionCnt++;
  }
}

void CcSyncDbClient::endTransaction()
{
  if (m_uiTransactionCnt == 1)
  {
    m_pDatabase->endTransaction();
    m_uiTransactionCnt--;
  }
  else if(m_uiTransactionCnt > 0)
  {
    m_uiTransactionCnt--;
  }
}

CcString CcSyncDbClient::getInnerPathById(const CcString& sDirName, uint64 uiDirId)
{
  CcString sPath;
  CcStringList slPathReverseOrder;
  while (uiDirId > 1)
  {
    CcString sQuery = "SELECT ";
    sQuery << "`" << CcSyncGlobals::Database::DirectoryList::DirId << "`,";
    sQuery << "`" << CcSyncGlobals::Database::DirectoryList::Name << "` ";
    sQuery << " FROM `" << sDirName + CcSyncGlobals::Database::DirectoryListAppend << "` ";
    sQuery << "WHERE `" << CcSyncGlobals::Database::DirectoryList::Id << "` = " << CcString::fromNumber(uiDirId);
    CcSqlResult sResult = m_pDatabase->query(sQuery);
    if (sResult.ok() && sResult.size() > 0)
    {
      uiDirId = sResult[0][0].getUint64();
      slPathReverseOrder.append(sResult[0][1].getString());
    }
    else
    {
      sPath.clear();
      break;;
    }
  }
  for (uint64 i = slPathReverseOrder.size(); i > 0; i--)
  {
    sPath.appendPath(slPathReverseOrder[static_cast<size_t>(i) - 1]);
  }
  return sPath;
}

CcSyncFileInfoList CcSyncDbClient::getDirectoryInfoListById(const CcString& sDirName, uint64 uiDirId)
{
  CcSyncFileInfoList oDirectoryList;
  CcString sQuery = "SELECT ";
  sQuery << "`" << CcSyncGlobals::Database::DirectoryList::Id        << "`,";
  sQuery << "`" << CcSyncGlobals::Database::DirectoryList::DirId     << "`,";
  sQuery << "`" << CcSyncGlobals::Database::DirectoryList::Name      << "`,";
  sQuery << "`" << CcSyncGlobals::Database::DirectoryList::Modified  << "`,";
  sQuery << "`" << CcSyncGlobals::Database::DirectoryList::ChangedMd5 << "`";
  sQuery << " FROM `" << sDirName + CcSyncGlobals::Database::DirectoryListAppend << "` "\
    "WHERE `" << CcSyncGlobals::Database::DirectoryList::DirId << "`='" << CcString::fromNumber(uiDirId) << "' ORDER BY `Name`";
  CcSqlResult oSqlDirectoryList = m_pDatabase->query(sQuery);
  for (CcTableRow& oRow : oSqlDirectoryList)
  {
    CcSyncFileInfo oDirectoryInfo;
    oDirectoryInfo.id() = oRow[0].getSize();
    oDirectoryInfo.dirId() = oRow[1].getSize();
    oDirectoryInfo.name() = oRow[2].getString();
    oDirectoryInfo.modified() = oRow[3].getInt64();
    oDirectoryInfo.md5().setHexString(oRow[4].getString());
    oDirectoryInfo.isFile() = false;
    oDirectoryList.append(std::move(oDirectoryInfo));
  }
  return oDirectoryList;
}

CcSyncFileInfoList CcSyncDbClient::getFileInfoListById(const CcString& sDirName, uint64 uiDirId)
{
  CcSyncFileInfoList oFileList;
  CcString sQuery = "SELECT ";
  sQuery << "`" << CcSyncGlobals::Database::FileList::Id        << "`,";
  sQuery << "`" << CcSyncGlobals::Database::FileList::DirId     << "`,";
  sQuery << "`" << CcSyncGlobals::Database::FileList::Size      << "`,";
  sQuery << "`" << CcSyncGlobals::Database::FileList::Name      << "`,";
  sQuery << "`" << CcSyncGlobals::Database::FileList::Modified  << "`,";
  sQuery << "`" << CcSyncGlobals::Database::FileList::CRC       << "`,";
  sQuery << "`" << CcSyncGlobals::Database::FileList::MD5       << "`,";
  sQuery << "`" << CcSyncGlobals::Database::FileList::Changed   << "`";
  sQuery << " FROM `" << sDirName + CcSyncGlobals::Database::FileListAppend << "` "\
    "WHERE `" << CcSyncGlobals::Database::FileList::DirId << "`='" << CcString::fromNumber(uiDirId) << "' ORDER BY `Name`";
  CcSqlResult oSqlFileList = m_pDatabase->query(sQuery);
  for (CcTableRow& oRow : oSqlFileList)
  {
    CcSyncFileInfo oFileInfo;
    oFileInfo.id() = oRow[0].getSize();
    oFileInfo.dirId() = oRow[1].getSize();
    oFileInfo.fileSize() = oRow[2].getUint64();
    oFileInfo.name() = oRow[3].getString();
    oFileInfo.modified() = oRow[4].getInt64();
    oFileInfo.crc() = oRow[5].getUint32();
    oFileInfo.md5() = oRow[6].getString();
    oFileInfo.changed() = oRow[7].getInt64();

    oFileList.append(std::move(oFileInfo));
  }
  return oFileList;
}

CcSyncFileInfo CcSyncDbClient::getDirectoryInfoById(const CcString& sDirName, uint64 uiDirId)
{
  CcSyncFileInfo oDirInfo;
  CcString sQuery = "SELECT ";
  sQuery << "`" << CcSyncGlobals::Database::DirectoryList::Id << "`,";
  sQuery << "`" << CcSyncGlobals::Database::DirectoryList::DirId << "`,";
  sQuery << "`" << CcSyncGlobals::Database::DirectoryList::Name << "`,";
  sQuery << "`" << CcSyncGlobals::Database::DirectoryList::Modified << "`,";
  sQuery << "`" << CcSyncGlobals::Database::DirectoryList::ChangedMd5 << "`";
  sQuery << " FROM `" << sDirName + CcSyncGlobals::Database::DirectoryListAppend << "` "\
    "WHERE `" << CcSyncGlobals::Database::DirectoryList::Id << "`='" << CcString::fromNumber(uiDirId) << "'";
  CcSqlResult oSqlDirectoryList = m_pDatabase->query(sQuery);
  if (oSqlDirectoryList.ok() &&
    oSqlDirectoryList.size() > 0)
  {
    oDirInfo.id() = oSqlDirectoryList[0][0].getSize();
    oDirInfo.dirId() = oSqlDirectoryList[0][1].getSize();
    oDirInfo.name() = oSqlDirectoryList[0][2].getString();
    oDirInfo.modified() = oSqlDirectoryList[0][3].getInt64();
    oDirInfo.md5().setHexString(oSqlDirectoryList[0][4].getString());
    oDirInfo.isFile() = false;
  }
  return oDirInfo;
}

CcSyncFileInfo CcSyncDbClient::getDirectoryInfoFromSubdir(const CcString& sDirName, uint64 uiDirId, const CcString& sSubDirName)
{
  CcString sQuery = "SELECT ";
  sQuery << "`" << CcSyncGlobals::Database::DirectoryList::Id << "` ";
  sQuery << " FROM `" << sDirName + CcSyncGlobals::Database::DirectoryListAppend << "` "\
    "WHERE `" << CcSyncGlobals::Database::DirectoryList::DirId << "`='" << CcString::fromNumber(uiDirId) << "' " <<
    "AND `" << CcSyncGlobals::Database::DirectoryList::Name << "`='" << CcSqlite::escapeString(sSubDirName) << "'";
  CcSqlResult oResult = m_pDatabase->query(sQuery);
  if (oResult.ok() &&
    oResult.size() > 0)
  {
    return getDirectoryInfoById(sDirName, oResult[0][0].getUint64());
  }
  return CcSyncFileInfo();
}

CcSyncFileInfo CcSyncDbClient::getFileInfoById(const CcString& sDirName, uint64 uiFileId)
{
  CcSyncFileInfo oFileInfo;
  CcString sQuery = "SELECT ";
  sQuery << "`" << CcSyncGlobals::Database::FileList::Id << "`,";
  sQuery << "`" << CcSyncGlobals::Database::FileList::DirId << "`,";
  sQuery << "`" << CcSyncGlobals::Database::FileList::Size << "`,";
  sQuery << "`" << CcSyncGlobals::Database::FileList::Name << "`,";
  sQuery << "`" << CcSyncGlobals::Database::FileList::Modified << "`,";
  sQuery << "`" << CcSyncGlobals::Database::FileList::CRC << "`,";
  sQuery << "`" << CcSyncGlobals::Database::FileList::MD5 << "`,";
  sQuery << "`" << CcSyncGlobals::Database::FileList::Changed << "`";
  sQuery << " FROM `" << sDirName + CcSyncGlobals::Database::FileListAppend << "` "\
            "WHERE `" << CcSyncGlobals::Database::FileList::Id << "`='" << CcString::fromNumber(uiFileId) << "' ORDER BY `Name`";
  CcSqlResult oSqlResult = m_pDatabase->query(sQuery);
  if (oSqlResult.ok() &&
      oSqlResult.size() > 0)
  {
    oFileInfo.id()        = oSqlResult[0][0].getSize();
    oFileInfo.dirId()  = oSqlResult[0][1].getSize();
    oFileInfo.fileSize()      = oSqlResult[0][2].getUint64();
    oFileInfo.name()      = oSqlResult[0][3].getString();
    oFileInfo.modified()  = oSqlResult[0][4].getInt64();
    oFileInfo.crc()       = oSqlResult[0][5].getUint32();
    oFileInfo.md5()       = oSqlResult[0][6].getString();
    oFileInfo.changed()   = oSqlResult[0][7].getInt64();
  }
  return oFileInfo;
}

CcSyncFileInfo CcSyncDbClient::getFileInfoByFilename(const CcString& sDirName, uint64 uiDirId, const CcString& sFileName)
{
  CcSyncFileInfoList oDirectoryList;
  CcString sQuery = "SELECT ";
  sQuery << "`" << CcSyncGlobals::Database::FileList::Id << "` ";
  sQuery << " FROM `" << sDirName + CcSyncGlobals::Database::FileListAppend << "` "\
    "WHERE `" << CcSyncGlobals::Database::FileList::DirId << "`='" << CcString::fromNumber(uiDirId) << "' "\
    "AND `" << CcSyncGlobals::Database::FileList::Name << "`='" << CcSqlite::escapeString(sFileName) << "'";
  CcSqlResult oResult = m_pDatabase->query(sQuery);
  if (oResult.ok() &&
    oResult.size() > 0)
  {
    return getFileInfoById(sDirName, oResult[0][0].getSize());
  }
  return CcSyncFileInfo();
}

bool CcSyncDbClient::queueHasItem(const CcString& sDirName)
{
  bool bRet = false;
  CcString sQuery = "SELECT COUNT(`";
  sQuery << CcSyncGlobals::Database::Queue::Id << "`) AS `count` FROM `" << sDirName + CcSyncGlobals::Database::QueueAppend << "` ";
  sQuery << "WHERE `" << CcSyncGlobals::Database::Queue::QueueId << "` IS NULL ";
  sQuery << "AND `" << CcSyncGlobals::Database::Queue::Attempts << "` < 5 ";
  sQuery << "AND `" << CcSyncGlobals::Database::Queue::DirId << "` IS NOT NULL LIMIT 0,1";
  CcSqlResult oResult = m_pDatabase->query(sQuery);
  if (oResult.ok() &&
    oResult[0][0].getSize() > 0)
  {
    bRet = true;
  }
  return bRet;
}

EBackupQueueType CcSyncDbClient::queueGetNext(const CcString& sDirName, CcSyncFileInfo& oFileInfo, uint64 &uiQueueIndex)
{
  EBackupQueueType eQueueType = EBackupQueueType::Unknown;
  CcString sQuery = "SELECT ";
  sQuery << "`" << CcSyncGlobals::Database::Queue::Id << "`,";
  sQuery << "`" << CcSyncGlobals::Database::Queue::QueueId << "`,";
  sQuery << "`" << CcSyncGlobals::Database::Queue::Name << "`,";
  sQuery << "`" << CcSyncGlobals::Database::Queue::FileId << "`,";
  sQuery << "`" << CcSyncGlobals::Database::Queue::DirId << "`,";
  sQuery << "`" << CcSyncGlobals::Database::Queue::Type << "`";
  sQuery << " FROM `" << sDirName + CcSyncGlobals::Database::QueueAppend << "` ";
  sQuery << "WHERE `" << CcSyncGlobals::Database::Queue::QueueId << "` IS NULL ";
  sQuery << "AND `" << CcSyncGlobals::Database::Queue::Attempts << "` < 5 ";
  sQuery << "AND `" << CcSyncGlobals::Database::Queue::DirId << "` IS NOT NULL LIMIT 0,1";
  CcSqlResult oResult = m_pDatabase->query(sQuery);
  if (oResult.ok() &&
    oResult.size() > 0)
  {
    uiQueueIndex = oResult[0][0].getSize();
    oFileInfo.id() = oResult[0][3].getSize();
    oFileInfo.dirId() = oResult[0][4].getSize();
    oFileInfo.name() = oResult[0][2].getString();
    eQueueType = (EBackupQueueType) oResult[0][5].getUint16();
  }
  return eQueueType;
}

void CcSyncDbClient::queueFinalizeDirectory(const CcString& sDirName, CcSyncFileInfo& oFileInfo, uint64 uiQueueIndex)
{
  // Update Dependent Queues
  CcString sQuery;
  sQuery << "UPDATE `" << sDirName + CcSyncGlobals::Database::QueueAppend << "` SET ";
  sQuery << " `" << CcSyncGlobals::Database::Queue::QueueId << "` = NULL ,";
  sQuery << " `" << CcSyncGlobals::Database::Queue::DirId << "` = " << CcString::fromNumber(oFileInfo.getId()) << " ";
  sQuery << "WHERE `" << CcSyncGlobals::Database::Queue::QueueId << "` = " << CcString::fromNumber(uiQueueIndex);
  CcSqlResult oResult = m_pDatabase->query(sQuery);
  if (oResult.ok())
  {
    // Delete the Queue
    sQuery = "DELETE FROM `";
    sQuery << sDirName + CcSyncGlobals::Database::QueueAppend << "`"\
      "WHERE `" << CcSyncGlobals::Database::Queue::Id << "` = '" << CcString::fromNumber(uiQueueIndex) << "'";
    oResult = m_pDatabase->query(sQuery);
  }
}

void CcSyncDbClient::queueFinalizeFile(const CcString& sDirName, uint64 uiQueueIndex)
{
  // Update Dependent Queues
  CcString sQuery = "UPDATE `";
  sQuery << sDirName + CcSyncGlobals::Database::QueueAppend << "` SET ";
  sQuery << " `" << CcSyncGlobals::Database::Queue::QueueId << "` = NULL ";
  sQuery << "WHERE `" << CcSyncGlobals::Database::Queue::QueueId << "` = " << CcString::fromNumber(uiQueueIndex);
  CcSqlResult oResult = m_pDatabase->query(sQuery);
  if (oResult.ok())
  {
    // Delete the Queue
    sQuery = "DELETE FROM `";
    sQuery << sDirName + CcSyncGlobals::Database::QueueAppend << "`"\
      "WHERE `" << CcSyncGlobals::Database::Queue::Id << "` = '" << CcString::fromNumber(uiQueueIndex) << "'";
    oResult = m_pDatabase->query(sQuery);
    if(oResult.error())
    {
      CcSyncLog::writeDebug("Finalizing queue failed (Remove).");
    }
  }
  else
  {
    CcSyncLog::writeDebug("Finalizing queue failed (Update).");
  }
}

void CcSyncDbClient::queueIncrementItem(const CcString& sDirName, uint64 uiQueueIndex)
{
  CcString sQuery = "UPDATE `";
  sQuery << sDirName + CcSyncGlobals::Database::QueueAppend << "` SET ";
  sQuery << "`" << CcSyncGlobals::Database::Queue::Attempts << "` = " << CcSyncGlobals::Database::Queue::Attempts << " + 1 ";
  sQuery << "WHERE `" << CcSyncGlobals::Database::Queue::Id << "` = " << CcString::fromNumber(uiQueueIndex);
  CcSqlResult oResult = m_pDatabase->query(sQuery);
}

void CcSyncDbClient::queueReset(const CcString& sDirName)
{
  CcString sQuery = "DELETE FROM  `";
  sQuery << sDirName + CcSyncGlobals::Database::QueueAppend << "`";
  CcSqlResult oResult = m_pDatabase->query(sQuery);
}

void CcSyncDbClient::queueResetAttempts(const CcString& sDirName)
{
  CcString sQuery = "UPDATE `";
  sQuery << sDirName + CcSyncGlobals::Database::QueueAppend << "` SET ";
  sQuery << " " << CcSyncGlobals::Database::Queue::Attempts << " = 0 ";
  CcSqlResult oResult = m_pDatabase->query(sQuery);
}

void CcSyncDbClient::queueDownloadDirectory(const CcString& sDirName, const CcSyncFileInfo& oFileInfo)
{
  CcString sQuery = getDbInsertQueue(sDirName, 0, EBackupQueueType::DownloadDir, oFileInfo.getId(), oFileInfo.getDirId(), oFileInfo.getName());
  CcSqlResult oResult = m_pDatabase->query(sQuery);
  if (oResult.error())
  {
    CcSyncLog::writeError("Adding Download Directory to queue");
  }
}

void CcSyncDbClient::queueDownloadFile(const CcString& sDirName, const CcSyncFileInfo& oFileInfo)
{
  CcString sQuery = getDbInsertQueue(sDirName, 0, EBackupQueueType::DownloadFile, oFileInfo.getId(), oFileInfo.getDirId(), oFileInfo.getName());
  CcSqlResult oResult = m_pDatabase->query(sQuery);
  if (oResult.error())
  {
    CcSyncLog::writeError("Adding Download File to queue");
  }
}

void CcSyncDbClient::directoryListUpdateChanged(const CcString& sDirName, uint64 uiDirId)
{
  CcMd5 oMd5;
  CcByteArray oHashData;
  union
  {
    uint64 uiValue;
    char  pcBytes[sizeof(uint64)];
  }UUint64ToByteArray;
  union
  {
    int64 iValue;
    char  pcBytes[sizeof(int64)];
  }UInt64ToByteArray;
  CcSyncFileInfoList oDirectoryInfoList = getDirectoryInfoListById(sDirName, uiDirId);
  CcSyncFileInfoList oFileInfoList = getFileInfoListById(sDirName, uiDirId);
  // FileList is processed, start removing unfound files in database
  for (const CcSyncDirInfo& oBackupFileInfo : oDirectoryInfoList)
  {
    oHashData.append(oBackupFileInfo.getMd5());
    UUint64ToByteArray.uiValue = oBackupFileInfo.getId();
    oHashData.append(UUint64ToByteArray.pcBytes, sizeof(UUint64ToByteArray.uiValue));
    UUint64ToByteArray.uiValue = oBackupFileInfo.getDirId();
    oHashData.append(UUint64ToByteArray.pcBytes, sizeof(UUint64ToByteArray.uiValue));
    oHashData.append(oBackupFileInfo.getName().getCharString(), oBackupFileInfo.getName().length());
    oHashData.append(oBackupFileInfo.getAttributes().getCharString(), oBackupFileInfo.getAttributes().length());
  }
  for (const CcSyncFileInfo& oBackupFileInfo : oFileInfoList)
  {
    oHashData.append(oBackupFileInfo.getMd5());
    UUint64ToByteArray.uiValue = oBackupFileInfo.getId();
    oHashData.append(UUint64ToByteArray.pcBytes, sizeof(UUint64ToByteArray.uiValue));
    UUint64ToByteArray.uiValue = oBackupFileInfo.getDirId();
    oHashData.append(UUint64ToByteArray.pcBytes, sizeof(UUint64ToByteArray.uiValue));
    UInt64ToByteArray.iValue = oBackupFileInfo.getChanged();
    oHashData.append(UInt64ToByteArray.pcBytes, sizeof(UInt64ToByteArray.iValue));
    UInt64ToByteArray.iValue = oBackupFileInfo.getCrc();
    oHashData.append(UInt64ToByteArray.pcBytes, sizeof(UInt64ToByteArray.iValue));
    UInt64ToByteArray.iValue = oBackupFileInfo.getModified();
    oHashData.append(UInt64ToByteArray.pcBytes, sizeof(UInt64ToByteArray.iValue));
    oHashData.append(oBackupFileInfo.getName().getCharString(), oBackupFileInfo.getName().length());
    oHashData.append(oBackupFileInfo.getAttributes().getCharString(), oBackupFileInfo.getAttributes().length());
  }
  oMd5.generate(oHashData);
  CcSyncDirInfo oDirInfo = getDirectoryInfoById(sDirName, uiDirId);
  CcString sTableName = sDirName + CcSyncGlobals::Database::DirectoryListAppend;
  CcString sQuery(CcSyncGlobals::Database::Update);
  sQuery << sTableName << "` SET `" << CcSyncGlobals::Database::DirectoryList::ChangedMd5 << "` = '" << oMd5.getValue().getHexString() << "'";
  sQuery << " WHERE `" << CcSyncGlobals::Database::DirectoryList::Id << "` = " << CcString::fromNumber(uiDirId);
  CcSqlResult oResult = m_pDatabase->query(sQuery);
  if (oResult.ok())
  {
    if (oDirInfo.getDirId() != 0)
      directoryListUpdateChanged(sDirName, oDirInfo.getDirId());
  }
  else
  {
    CcSyncLog::writeError("Unexpected Error on updating directory timestamps: " + CcString::fromNumber(uiDirId));
  }
}

void CcSyncDbClient::directoryListUpdateChangedAll(const CcString& sDirName, uint64 uiDirId)
{
  CcSyncFileInfoList oDirectoryList = getDirectoryInfoListById(sDirName, uiDirId);
  for (CcSyncDirInfo oDirInfo : oDirectoryList)
  {
    directoryListUpdateChangedAll(sDirName, oDirInfo.getId());
  }
  directoryListUpdateChanged(sDirName, uiDirId);
}

void CcSyncDbClient::directoryListSearchDouble(const CcString& sDirName, uint64 uiDirId)
{
  CcSyncFileInfoList oDirectoryList = getDirectoryInfoListById(sDirName, uiDirId);
  if (oDirectoryList.size() > 0)
  {
    for (CcSyncDirInfo oDirInfo : oDirectoryList)
    {
      CcString sQuery = "SELECT ";
      sQuery << "`" << CcSyncGlobals::Database::DirectoryList::Id << "`";
      sQuery << " FROM `" << sDirName + CcSyncGlobals::Database::DirectoryListAppend << "` "\
                "WHERE `" << CcSyncGlobals::Database::DirectoryList::DirId << "`='" << CcString::fromNumber(uiDirId) << "' AND "
                " `" << CcSyncGlobals::Database::DirectoryList::Name << "`='" << m_pDatabase->escapeString(oDirInfo.getName()) << "'";
      CcSqlResult oResult = m_pDatabase->query(sQuery);
      if (oResult.size() == 0)
      {
        CCDEBUG("Impossible situation oO");
      }
      else if (oResult.size() > 1)
      {
        CCDEBUG("Double Entries found for :" + oDirInfo.getName());
        for (size_t i = 0; i < oResult.size() - 1; i++)
        {
          CcSyncFileInfo oRemoveDirInfo;
          oRemoveDirInfo.id() = oResult[i][0].getUint64();
          directoryListRemove(sDirName, oRemoveDirInfo, false);
        }
        // start rescan and instantly close
        directoryListSearchDouble(sDirName, uiDirId);
        return;
      }
      else
      {
        directoryListSearchDouble(sDirName, oDirInfo.getId());
      }
    }
  }
}

void CcSyncDbClient::directoryListSearchTemporary(const CcString& sDirName)
{
  CcString sQuery = "SELECT ";
  sQuery << "`" << CcSyncGlobals::Database::DirectoryList::Id << "`";
  sQuery << " FROM `" << sDirName + CcSyncGlobals::Database::DirectoryListAppend << "` "\
            " WHERE `" << CcSyncGlobals::Database::DirectoryList::Name << "` LIKE '" << CcSyncGlobals::TemporaryExtension << "'";
  CcSqlResult oResult = m_pDatabase->query(sQuery);
  for (CcTableRow& oRow : oResult)
  {
    directoryListRemove(sDirName, getDirectoryInfoById(sDirName, oRow[0].getUint64()), false);
  }
}

bool CcSyncDbClient::fileListFileIdExists(const CcString& sDirName, uint64 uiDirId, const CcSyncFileInfo& oFileInfo)
{
  bool bRet = false;
  CcString sQuery = "SELECT ";
  sQuery << "`" << CcSyncGlobals::Database::FileList::Id << "` ";
  sQuery << " FROM `" << sDirName + CcSyncGlobals::Database::FileListAppend << "` "\
    "WHERE `" << CcSyncGlobals::Database::FileList::DirId << "`='" << CcString::fromNumber(uiDirId) << "' "\
    "AND `" << CcSyncGlobals::Database::FileList::Id << "`='" << CcString::fromNumber(oFileInfo.getId()) << "'";
  CcSqlResult oSqlDirectoryList = m_pDatabase->query(sQuery);
  if (oSqlDirectoryList.ok() &&
    oSqlDirectoryList.size() > 0)
  {
    bRet = true;
  }
  return bRet;
}

bool CcSyncDbClient::fileListFileNameExists(const CcString& sDirName, uint64 uiDirId, const CcSyncFileInfo& oFileInfo)
{
  bool bRet = false;
  CcSyncFileInfoList oDirectoryList;
  CcString sQuery = "SELECT ";
  sQuery << "`" << CcSyncGlobals::Database::FileList::Id << "` ";
  sQuery << " FROM `" << sDirName + CcSyncGlobals::Database::FileListAppend << "` "\
    "WHERE `" << CcSyncGlobals::Database::FileList::DirId << "`='" << CcString::fromNumber(uiDirId) << "' "\
    "AND `" << CcSyncGlobals::Database::FileList::Name << "`='" << CcSqlite::escapeString(oFileInfo.getName()) << "'";
  CcSqlResult oSqlDirectoryList = m_pDatabase->query(sQuery);
  if (oSqlDirectoryList.ok() &&
    oSqlDirectoryList.size() > 0)
  {
    bRet = true;
  }
  return bRet;
}

void CcSyncDbClient::fileListSearchTemporary(const CcString& sDirName)
{
  CcString sQuery = "SELECT ";
  sQuery << "`" << CcSyncGlobals::Database::DirectoryList::Id << "`";
  sQuery << " FROM `" << sDirName + CcSyncGlobals::Database::FileListAppend << "` "\
            " WHERE `" << CcSyncGlobals::Database::FileList::Name << "` LIKE '" << CcSyncGlobals::TemporaryExtension << "'";
  CcSqlResult oResult = m_pDatabase->query(sQuery);
  for (CcTableRow& oRow : oResult)
  {
    fileListRemove(sDirName, getFileInfoById(sDirName, oRow[0].getUint64()), true);
  }
}


bool CcSyncDbClient::historyInsert(const CcString& sDirName, EBackupQueueType eQueueType, const CcSyncFileInfo& oFileInfo)
{
  if (m_bEnableHistory)
  {
    CcString sQuery = getDbInsertHistory(sDirName, eQueueType, oFileInfo);
    CcSqlResult oResult = m_pDatabase->query(sQuery);
    if (oResult.error())
    {
      CcSyncLog::writeDebug("Error on adding data to history.");
    }
    return oResult.ok();
  }
  else
  {
    return true;
  }
}

uint64 CcSyncDbClient::queueInsert(const CcString& sDirName, uint64 uiParentId, EBackupQueueType eQueueType, uint64 uiFileId, uint64 uiDirectoryId, const CcString& sName)
{
  CcString sQuery = getDbInsertQueue(sDirName, uiParentId, eQueueType, uiFileId, uiDirectoryId, sName);
  CcSqlResult oResult = m_pDatabase->query(sQuery);
  if (oResult.ok())
  {
    return oResult.getLastInsertId();
  }
  CcSyncLog::writeError("Adding Download File to queue");
  return 0;
}

bool CcSyncDbClient::directoryListRemove(const CcString& sDirName, const CcSyncFileInfo& oFileInfo, bool bDoUpdateParents)
{
  bool bRet = false;
  CcString sQuery;
  sQuery << "DELETE FROM `" << sDirName + CcSyncGlobals::Database::DirectoryListAppend << "`"\
            "WHERE `" << CcSyncGlobals::Database::DirectoryList::Id << "` = '" << CcString::fromNumber(oFileInfo.getId()) << "'";
  CcSqlResult oResult = m_pDatabase->query(sQuery);
  if (oResult.ok())
  {
    bRet = true;
    if (bDoUpdateParents)
      directoryListUpdateChanged(sDirName, oFileInfo.getId());
  }
  else
  {
    CcSyncLog::writeError("Remove from DirectoryList failed:");
    CcSyncLog::writeError("    File: " + oFileInfo.getName());
    CcSyncLog::writeError("    Reason: " + oResult.getErrorMessage());
  }
  return bRet;
}

bool CcSyncDbClient::directoryListExists(const CcString& sDirName, uint64 uiDirId)
{
  bool bRet = false;
  CcSyncFileInfoList oDirectoryList;
  CcString sQuery = "SELECT ";
  sQuery << "`" << CcSyncGlobals::Database::DirectoryList::Id << "` ";
  sQuery << " FROM `" << sDirName + CcSyncGlobals::Database::DirectoryListAppend << "` "\
    "WHERE `" << CcSyncGlobals::Database::DirectoryList::Id << "`='" << CcString::fromNumber(uiDirId) << "'";
  CcSqlResult oSqlDirectoryList = m_pDatabase->query(sQuery);
  if (oSqlDirectoryList.ok() &&
    oSqlDirectoryList.size() > 0)
  {
    bRet = true;
  }
  return bRet;
}

bool CcSyncDbClient::directoryListSubDirExists(const CcString& sDirName, uint64 uiParentDirId, const CcString& sName)
{
  bool bRet = false;
  CcSyncFileInfoList oDirectoryList;
  CcString sQuery = "SELECT ";
  sQuery << "`" << CcSyncGlobals::Database::DirectoryList::Id << "` ";
  sQuery << " FROM `" << sDirName + CcSyncGlobals::Database::DirectoryListAppend << "` "\
    "WHERE `" << CcSyncGlobals::Database::DirectoryList::DirId << "`='" << CcString::fromNumber(uiParentDirId) << "' " <<
    "AND `" << CcSyncGlobals::Database::DirectoryList::Name << "`='" << CcSqlite::escapeString(sName) << "'";
  CcSqlResult oSqlDirectoryList = m_pDatabase->query(sQuery);
  if (oSqlDirectoryList.ok() &&
    oSqlDirectoryList.size() > 0)
  {
    bRet = true;
  }
  return bRet;
}

bool CcSyncDbClient::directoryListEmpty(const CcString& sDirName, uint64 uiDirId)
{
  bool bRet = false;
  CcSyncFileInfoList oDirectoryList = getDirectoryInfoListById(sDirName, uiDirId);
  if (oDirectoryList.size() == 0)
  {
    CcSyncFileInfoList oFileList = getFileInfoListById(sDirName, uiDirId);
    if (oDirectoryList.size() == 0)
    {
      bRet = true;
    }
  }
  return bRet;
}

bool CcSyncDbClient::directoryListInsert(const CcString& sDirName, CcSyncFileInfo& oFileInfo, bool bDoUpdateParents)
{
  bool bRet = false;
  CcString sQuery = getDbInsertDirectoryList(sDirName, oFileInfo);
  CcSqlResult oResult = m_pDatabase->query(sQuery);
  if (oResult.ok())
  {
    oFileInfo.id() = oResult.getLastInsertId();
    bRet = historyInsert(sDirName, EBackupQueueType::CreateDir, oFileInfo);
    if (bRet == false)
    {
      fileListRemove(sDirName, oFileInfo);
    }
    else if(bDoUpdateParents)
    {
      directoryListUpdateChanged(sDirName, oFileInfo.id());
    }
  }
  return bRet;
}

bool CcSyncDbClient::directoryListUpdate(const CcString& sDirName, const CcSyncFileInfo& oFileInfo)
{
  CcString sTableName = sDirName + CcSyncGlobals::Database::DirectoryListAppend;
  CcString sQuery(CcSyncGlobals::Database::Update);
  sQuery << sTableName << "` SET ";
  sQuery << "`" << CcSyncGlobals::Database::DirectoryList::Name << "` = '" << CcSqlite::escapeString(oFileInfo.getName()) << "', ";
  sQuery << "`" << CcSyncGlobals::Database::DirectoryList::Modified << "` = '" << CcString::fromNumber(oFileInfo.getModified()) << "'";
  sQuery << " WHERE `" << CcSyncGlobals::Database::DirectoryList::Id << "` = " << CcString::fromNumber(oFileInfo.getId());
  CcSqlResult oResult = m_pDatabase->query(sQuery);
  directoryListUpdateChanged(sDirName, oFileInfo.getId());
  return oResult.ok();
}


bool CcSyncDbClient::directoryListUpdateId(const CcString& sDirName, uint64 uiDirectoryId, const CcSyncFileInfo& oFileInfo)
{
  CcString sTableName = sDirName + CcSyncGlobals::Database::DirectoryListAppend;
  CcString sQuery(CcSyncGlobals::Database::Update);
  sQuery << sTableName << "` SET ";
  sQuery << "`" << CcSyncGlobals::Database::DirectoryList::Id << "` = '" << CcString::fromNumber(oFileInfo.getId()) << "', ";
  sQuery << "`" << CcSyncGlobals::Database::DirectoryList::Name << "` = '" << CcSqlite::escapeString(oFileInfo.getName()) << "', ";
  sQuery << "`" << CcSyncGlobals::Database::DirectoryList::Modified << "` = '" << CcString::fromNumber(oFileInfo.getModified()) << "'";
  sQuery << " WHERE `" << CcSyncGlobals::Database::DirectoryList::Id << "` = " << CcString::fromNumber(uiDirectoryId);
  CcSqlResult oResult = m_pDatabase->query(sQuery);
  directoryListUpdateChanged(sDirName, oFileInfo.getId());
  return oResult.ok();
}

bool CcSyncDbClient::fileListInsert(const CcString& sDirName, CcSyncFileInfo& oFileInfo, bool bDoUpdateParents)
{
  bool bRet = false;
  CcString sQuery = getDbInsertFileList(sDirName, oFileInfo);
  CcSqlResult oResult = m_pDatabase->query(sQuery);
  if (oResult.ok())
  {
    oFileInfo.id() = oResult.getLastInsertId();
    bRet = historyInsert(sDirName, EBackupQueueType::AddFile, oFileInfo);
    if (bRet == false)
    {
      fileListRemove(sDirName, oFileInfo);
    }
    if(bDoUpdateParents)
    {
      directoryListUpdateChanged(sDirName, oFileInfo.getDirId());
    }
  }
  else
  {
    CcSyncLog::writeError("Insert to filelist failed:");
    CcSyncLog::writeError("    File:   " + oFileInfo.getName());
    CcSyncLog::writeError("    Reason: " + oResult.getErrorMessage());
  }
  return bRet;
}

bool CcSyncDbClient::fileListRemove(const CcString& sDirName, const CcSyncFileInfo& oFileInfo, bool bDoUpdateParents)
{
  bool bRet = false;
  CcString sQuery = "DELETE FROM `";
  sQuery << sDirName + CcSyncGlobals::Database::FileListAppend << "`"\
            "WHERE `" << CcSyncGlobals::Database::FileList::Id << "` = '" << CcString::fromNumber(oFileInfo.getId()) << "'";
  CcSqlResult oResult = m_pDatabase->query(sQuery);
  if (oResult.ok())
  {
    bRet = true;
    if (bDoUpdateParents)
      directoryListUpdateChanged(sDirName, oFileInfo.getDirId());
  }
  else
  {
    CcSyncLog::writeError("Remove from filelist failed:");
    CcSyncLog::writeError("    File:   " + oFileInfo.getName());
    CcSyncLog::writeError("    Reason: " + oResult.getErrorMessage());
  }
  return bRet;
}

bool CcSyncDbClient::fileListExists(const CcString& sDirName, uint64 uiFileId)
{
  bool bRet = false;
  CcSyncFileInfoList oDirectoryList;
  CcString sQuery = "SELECT ";
  sQuery << "`" << CcSyncGlobals::Database::FileList::Id << "` ";
  sQuery << " FROM `" << sDirName + CcSyncGlobals::Database::FileListAppend << "` "\
    "WHERE `" << CcSyncGlobals::Database::FileList::Id << "`='" << CcString::fromNumber(uiFileId) << "'";
  CcSqlResult oSqlDirectoryList = m_pDatabase->query(sQuery);
  if (oSqlDirectoryList.ok() &&
    oSqlDirectoryList.size() > 0)
  {
    bRet = true;
  }
  return bRet;
}

CcString CcSyncDbClient::getDbCreateDirectoryList(const CcString& sDirName)
{
  CcString sRet;
  CcString sTableName = sDirName + CcSyncGlobals::Database::DirectoryListAppend;
  sRet << CcSyncGlobals::Database::CreateTable << sTableName << "` (";
  sRet << "`" << CcSyncGlobals::Database::DirectoryList::Id << "` INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,";
  sRet << "`" << CcSyncGlobals::Database::DirectoryList::DirId << "` INTEGER NULL,";
  sRet << "`" << CcSyncGlobals::Database::DirectoryList::Name << "` TEXT,";
  sRet << "`" << CcSyncGlobals::Database::DirectoryList::Modified << "` TIMESTAMP DEFAULT 0,";
  sRet << "`" << CcSyncGlobals::Database::DirectoryList::Attributes << "` VARCHAR(10) NOT NULL,";
  sRet << "`" << CcSyncGlobals::Database::DirectoryList::ChangedMd5 << "` CHAR(32)  DEFAULT '00000000000000000000000000000000', ";
  sRet << "FOREIGN KEY(`" << CcSyncGlobals::Database::DirectoryList::DirId << "`) REFERENCES " << sDirName + CcSyncGlobals::Database::DirectoryListAppend + 
          "(`" << CcSyncGlobals::Database::DirectoryList::Id << "`) ON UPDATE CASCADE ON DELETE CASCADE); \r\n";
  sRet << "CREATE INDEX Index_" << sTableName << "_" << CcSyncGlobals::Database::DirectoryList::DirId +
          " ON `" << sTableName << "`(`" << CcSyncGlobals::Database::DirectoryList::DirId << "`);\r\n";
  return sRet;
}

CcString CcSyncDbClient::getDbCreateFileList(const CcString& sDirName)
{
  CcString sRet(CcSyncGlobals::Database::CreateTable);
  CcString sTableName = sDirName + CcSyncGlobals::Database::FileListAppend;
  sRet << sTableName << "` (";
  sRet << "`" << CcSyncGlobals::Database::FileList::Id << "` INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,";
  sRet << "`" << CcSyncGlobals::Database::FileList::DirId << "` INTEGER NOT NULL,";
  sRet << "`" << CcSyncGlobals::Database::FileList::Name << "` TEXT NOT NULL,";
  sRet << "`" << CcSyncGlobals::Database::FileList::Size << "` UNSIGNED BIG INT DEFAULT 0,";
  sRet << "`" << CcSyncGlobals::Database::FileList::Modified << "` TIMESTAMP DEFAULT 0,";
  sRet << "`" << CcSyncGlobals::Database::FileList::Attributes << "` VARCHAR(10) NOT NULL,";
  sRet << "`" << CcSyncGlobals::Database::FileList::MD5 << "` CHAR(32) DEFAULT '00000000000000000000000000000000',";
  sRet << "`" << CcSyncGlobals::Database::FileList::CRC << "` UNSIGNED INT DEFAULT 0,";
  sRet << "`" << CcSyncGlobals::Database::FileList::Changed << "` TIMESTAMP DEFAULT 0,";
  sRet << "FOREIGN KEY(`" << CcSyncGlobals::Database::FileList::DirId << "`) REFERENCES " << sDirName + CcSyncGlobals::Database::DirectoryListAppend + 
          "(`" << CcSyncGlobals::Database::DirectoryList::Id << "`) ON UPDATE CASCADE ON DELETE CASCADE);\r\n";
  sRet << "CREATE INDEX Index_" << sTableName << "_" << CcSyncGlobals::Database::FileList::DirId +
          " ON `" << sTableName << "`(`" << CcSyncGlobals::Database::FileList::DirId << "`);\r\n";
  return sRet;
}

CcString CcSyncDbClient::getDbCreateQueue(const CcString& sDirName)
{
  CcString sRet(CcSyncGlobals::Database::CreateTable);
  CcString sTableName = sDirName + CcSyncGlobals::Database::QueueAppend;
  sRet << sTableName << "` (";
  sRet << "`" << CcSyncGlobals::Database::Queue::Id        << "` INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,";
  sRet << "`" << CcSyncGlobals::Database::Queue::QueueId   << "` INTEGER NULL,";
  sRet << "`" << CcSyncGlobals::Database::Queue::Type      << "` INTEGER NOT NULL,";
  sRet << "`" << CcSyncGlobals::Database::Queue::FileId    << "` INTEGER NULL,";
  sRet << "`" << CcSyncGlobals::Database::Queue::DirId     << "` INTEGER NULL,";
  sRet << "`" << CcSyncGlobals::Database::Queue::Name      << "` TEXT NULL,";
  sRet << "`" << CcSyncGlobals::Database::Queue::Attempts  << "` INTEGER DEFAULT 0,";
  sRet << "FOREIGN KEY(`" << CcSyncGlobals::Database::Queue::QueueId << "`) REFERENCES `" << sDirName + CcSyncGlobals::Database::QueueAppend + 
          "`(`" << CcSyncGlobals::Database::Queue::Id << "`) ON UPDATE CASCADE ON DELETE CASCADE);\r\n";
  sRet << "CREATE INDEX Index_" << sTableName << "_" << CcSyncGlobals::Database::Queue::QueueId +
    " ON `" << sTableName << "`(`" << CcSyncGlobals::Database::Queue::QueueId << "`);\r\n";
  return sRet;
}

CcString CcSyncDbClient::getDbCreateHistory(const CcString& sDirName)
{
  CcString sRet(CcSyncGlobals::Database::CreateTable);
  sRet << sDirName + CcSyncGlobals::Database::HistoryAppend << "` (";
  sRet << "`" << CcSyncGlobals::Database::History::Id        << "` INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,";
  sRet << "`" << CcSyncGlobals::Database::History::Type      << "` INTEGER,";
  sRet << "`" << CcSyncGlobals::Database::History::Path      << "` TEXT,";
  sRet << "`" << CcSyncGlobals::Database::History::Name      << "` TEXT NOT NULL,";
  sRet << "`" << CcSyncGlobals::Database::History::Size      << "` UNSIGNED BIG INT DEFAULT 0,";
  sRet << "`" << CcSyncGlobals::Database::History::Modified  << "` TIMESTAMP DEFAULT 0,";
  sRet << "`" << CcSyncGlobals::Database::History::Attributes<< "` VARCHAR(10) NOT NULL,";
  sRet << "`" << CcSyncGlobals::Database::History::MD5       << "` VARCHAR(32) NULL,";
  sRet << "`" << CcSyncGlobals::Database::History::CRC       << "` UNSIGNED INT DEFAULT 0,";
  sRet << "`" << CcSyncGlobals::Database::History::Stamp     << "` TIMESTAMP DEFAULT 0);";
  return sRet;
}

CcString CcSyncDbClient::getDbInsertDirectoryList(const CcString& sDirName, const CcSyncFileInfo& oInfo)
{
  CcString sId, sDirId;
  if (oInfo.getId() == 0)
  {
    sId = "NULL"; // for auto increment
  }
  else
  {
    sId = CcString::fromNumber(oInfo.getId());
  }
  if (oInfo.getDirId() == 0)
  {
    sDirId = "NULL"; // for auto increment
  }
  else
  {
    sDirId = CcString::fromNumber(oInfo.getDirId());
  }
  CcString sRet(CcSyncGlobals::Database::Insert);
  sRet << sDirName + CcSyncGlobals::Database::DirectoryListAppend << "` (`Id`,";
  sRet << "`" << CcSyncGlobals::Database::DirectoryList::DirId << "`,";
  sRet << "`" << CcSyncGlobals::Database::DirectoryList::Name << "`,";
  sRet << "`" << CcSyncGlobals::Database::DirectoryList::Modified << "`,";
  sRet << "`" << CcSyncGlobals::Database::DirectoryList::Attributes << "`,";
  sRet << "`" << CcSyncGlobals::Database::DirectoryList::ChangedMd5 << "`) ";
  sRet << "VALUES (";
  sRet << sId << ",";
  sRet << sDirId << ",";
  sRet << "'" << CcSqlite::escapeString(oInfo.getName()) << "',";
  sRet << "'" << CcString::fromNumber(oInfo.getModified()) << "',";
  sRet << "'" << CcSqlite::escapeString(oInfo.getAttributes()) << "',";
  sRet << "'" << oInfo.getMd5().getHexString() << "'";
  sRet << ")";
  return sRet;
}

CcString CcSyncDbClient::getDbInsertFileList(const CcString& sDirName, const CcSyncFileInfo& oInfo)
{
  CcString sId = "NULL";
  if (oInfo.getId() != 0)
    sId = CcString::fromNumber(oInfo.getId());

  CcString sRet(CcSyncGlobals::Database::Insert);
  sRet << sDirName + CcSyncGlobals::Database::FileListAppend;

  // Columns
  sRet << "` ( ";
  sRet << "`" << CcSyncGlobals::Database::FileList::Id << "`,";
  sRet << "`" << CcSyncGlobals::Database::FileList::DirId << "`,";
  sRet << "`" << CcSyncGlobals::Database::FileList::Name << "`,";
  sRet << "`" << CcSyncGlobals::Database::FileList::Size << "`,";
  sRet << "`" << CcSyncGlobals::Database::FileList::Modified << "`,";
  sRet << "`" << CcSyncGlobals::Database::FileList::Attributes << "`,";
  sRet << "`" << CcSyncGlobals::Database::FileList::CRC << "`,";
  sRet << "`" << CcSyncGlobals::Database::FileList::MD5 << "`,";
  sRet << "`" << CcSyncGlobals::Database::FileList::Changed << "`) ";

  // Values
  sRet << "VALUES (";
  sRet << sId << ",";
  sRet << CcString::fromNumber(oInfo.getDirId()) << ",";
  sRet << "'" << CcSqlite::escapeString(oInfo.getName()) << "',";
  sRet << CcString::fromNumber(oInfo.getFileSize()) << ",";
  sRet << CcString::fromNumber(oInfo.getModified()) << ",";
  sRet << "'" << CcSqlite::escapeString(oInfo.getAttributes()) << "',";
  sRet << CcString::fromNumber(oInfo.getCrc()) << ",";
  sRet << "'" << oInfo.getMd5().getHexString() << "',";
  sRet << CcString::fromNumber(oInfo.getChanged());
  sRet << ")";
  return sRet;
}

CcString CcSyncDbClient::getDbInsertQueue(const CcString& sDirName, uint64 uiParentId, EBackupQueueType eQueueType, uint64 uiFileId, uint64 uiDirId, const CcString& sName)
{
  CcString sParentId;
  CcString sDirId;
  CcString sFileId;
  CcString sQueueType = CcString::fromNumber((int)eQueueType);
  if (uiParentId == 0)
  {
    sParentId = "NULL";
  }
  else
  {
    sParentId = CcString::fromNumber(uiParentId);
  }
  if (uiDirId == 0)
  {
    sDirId = "0";
  }
  else
  {
    sDirId = CcString::fromNumber(uiDirId);
  }
  if (uiFileId == 0)
  {
    sFileId = "0";
  }
  else
  {
    sFileId = CcString::fromNumber(uiFileId);
  }
  CcString sRet(CcSyncGlobals::Database::Insert);
  sRet << sDirName + CcSyncGlobals::Database::QueueAppend << "` (";
  sRet << "`" << CcSyncGlobals::Database::Queue::Id << "`,";
  sRet << "`" << CcSyncGlobals::Database::Queue::QueueId << "`,";
  sRet << "`" << CcSyncGlobals::Database::Queue::Type << "`,";
  sRet << "`" << CcSyncGlobals::Database::Queue::FileId << "`,";
  sRet << "`" << CcSyncGlobals::Database::Queue::DirId << "`,";
  sRet << "`" << CcSyncGlobals::Database::Queue::Name << "`,";
  sRet << "`" << CcSyncGlobals::Database::Queue::Attempts << "`";
  sRet << ") ";

  sRet << "VALUES (";
  sRet << "NULL,";
  sRet << sParentId  << ",";
  sRet << sQueueType << ",";
  sRet << sFileId    << ",";
  sRet << sDirId  << ",";
  sRet << "'" << CcSqlite::escapeString(sName) << "',";
  sRet << "0";
  sRet << ")";
  return sRet;
}

CcString CcSyncDbClient::getDbInsertHistory(const CcString& sDirName, EBackupQueueType eQueueType, const CcSyncFileInfo& oFileInfo)
{
  CcString sTargetId;
  CcString sDirId;
  CcString sQueueType = CcString::fromNumber((int)eQueueType);
  if (oFileInfo.getId() == 0)
  {
    sTargetId = "NULL";
  }
  else
  {
    sTargetId = CcString::fromNumber(oFileInfo.getId());
  }
  CcString sRet(CcSyncGlobals::Database::Insert);
  sRet << sDirName + CcSyncGlobals::Database::HistoryAppend;

  // Columns
  sRet << "` ( ";
  sRet << "`" << CcSyncGlobals::Database::History::Id << "`,";
  sRet << "`" << CcSyncGlobals::Database::History::Type << "`,";
  sRet << "`" << CcSyncGlobals::Database::History::Path << "`,";
  sRet << "`" << CcSyncGlobals::Database::History::Name << "`,";
  sRet << "`" << CcSyncGlobals::Database::History::Size << "`,";
  sRet << "`" << CcSyncGlobals::Database::History::Modified << "`,";
  sRet << "`" << CcSyncGlobals::Database::History::Attributes << "`,";
  sRet << "`" << CcSyncGlobals::Database::History::CRC << "`,";
  sRet << "`" << CcSyncGlobals::Database::History::MD5 << "`,";
  sRet << "`" << CcSyncGlobals::Database::History::Stamp << "`) ";

  // Values
  sRet << "VALUES (";
  sRet << "NULL,";
  sRet << CcString::fromNumber((uint16) eQueueType) << ",";
  sRet << "'" << CcSqlite::escapeString(oFileInfo.getDirPath()) << "',";
  sRet << "'" << CcSqlite::escapeString(oFileInfo.getName()) << "',";
  sRet << CcString::fromNumber(oFileInfo.getFileSize()) << ",";
  sRet << CcString::fromNumber(oFileInfo.getModified()) << ",";
  sRet << "'" << CcSqlite::escapeString(oFileInfo.getAttributes()) << "',";
  sRet << CcString::fromNumber(oFileInfo.getCrc()) << ",";
  sRet << "'" << oFileInfo.getMd5().getHexString() << "',";
  sRet << CcString::fromNumber(oFileInfo.getChanged());
  sRet << ")";
  return sRet;
}
