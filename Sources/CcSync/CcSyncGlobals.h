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
 * @subpage   CcSyncGlobals
 * 
 * @page      CcSyncGlobals
 * @copyright Andreas Dirmeier (C) 2017
 * @author    Andreas Dirmeier
 * @par       Web: http://coolcow.de
 * @version   0.01
 * @date      2016-04
 * @par       Language   C++ ANSI V3
 * @brief     Definitions for CcSync
 */
#ifndef _CcSyncGlobals_H_
#define _CcSyncGlobals_H_

#include "CcBase.h"
#include "CcSync.h"
#include "CcString.h"
#include "Network/CcCommonPorts.h"
#include "CcGlobalStrings.h"

class CcVersion;

enum class ESyncRights
{
  None      = 0,
  ReadOnly  = 1,
  User      = 2,
  Account   = 3,
  Admin
};

namespace CcSyncGlobals
{
  extern const CcSyncSHARED CcVersion Version;
  extern const CcSyncSHARED CcString sTest;
  extern const CcSyncSHARED CcString ConfigDirName;
  extern const CcSyncSHARED size_t MaxReconnections;
  extern const CcSyncSHARED uint64 TransferSize;
  extern const CcSyncSHARED uint64 MaxRequestSize;
  extern const CcSyncSHARED uint64 MaxResponseSize;
  extern const CcSyncSHARED CcString DefaultCertFile;
  extern const CcSyncSHARED CcString DefaultKeyFile;
  extern const CcSyncSHARED CcString SqliteExtension;
  extern const CcSyncSHARED CcString XmlExtension;
  extern const CcSyncSHARED uint64 SessionLength;
  extern const CcSyncSHARED uint16 DefaultPort;
  extern const CcSyncSHARED CcString DefaultPortStr;
  extern const CcSyncSHARED CcString TemporaryExtension;
  extern const CcSyncSHARED CcString LockFile;

  extern const CcSyncSHARED CcString IndexName;
  extern const CcSyncSHARED CcString NameName;
  extern const CcSyncSHARED CcString SizeName;

  namespace FileInfo
  {
    extern const CcSyncSHARED CcString& Id;
    extern const CcSyncSHARED CcString DirId;
    extern const CcSyncSHARED CcString Name;
    extern const CcSyncSHARED CcString Modified;
    extern const CcSyncSHARED CcString CRC;
    extern const CcSyncSHARED CcString MD5;
    extern const CcSyncSHARED CcString Size;
    extern const CcSyncSHARED CcString Changed;
    extern const CcSyncSHARED CcString IsFile;
    extern const CcSyncSHARED CcString Attributes;
  }

  namespace Database
  {
    extern const CcSyncSHARED uint64 RootDirId;
    extern const CcSyncSHARED CcString CreateTable;
    extern const CcSyncSHARED CcString DropTable;
    extern const CcSyncSHARED CcString Insert;
    extern const CcSyncSHARED CcString Update;

    extern const CcSyncSHARED CcString DirectoryListAppend;
    extern const CcSyncSHARED CcString FileListAppend;
    extern const CcSyncSHARED CcString QueueAppend;
    extern const CcSyncSHARED CcString HistoryAppend;

    namespace FileList
    {
      extern const CcSyncSHARED CcString& Id;
      extern const CcSyncSHARED CcString& DirId;
      extern const CcSyncSHARED CcString& Name;
      extern const CcSyncSHARED CcString& Size;
      extern const CcSyncSHARED CcString Modified;
      extern const CcSyncSHARED CcString MD5;
      extern const CcSyncSHARED CcString CRC;
      extern const CcSyncSHARED CcString Changed;
      extern const CcSyncSHARED CcString& Attributes;
    }

    namespace Queue
    {
      extern const CcSyncSHARED CcString& Id;
      extern const CcSyncSHARED CcString QueueId;
      extern const CcSyncSHARED CcString Type;
      extern const CcSyncSHARED CcString FileId;
      extern const CcSyncSHARED CcString& DirId;
      extern const CcSyncSHARED CcString Name;
      extern const CcSyncSHARED CcString& Attributes;
      extern const CcSyncSHARED CcString Attempts;
    }

    namespace DirectoryList
    {
      extern const CcSyncSHARED CcString& Id;
      extern const CcSyncSHARED CcString& DirId;
      extern const CcSyncSHARED CcString& Name;
      extern const CcSyncSHARED CcString Modified;
      extern const CcSyncSHARED CcString& Attributes;
      extern const CcSyncSHARED CcString ChangedMd5;
    }
    
    namespace History
    {
      extern const CcSyncSHARED CcString& Id;
      extern const CcSyncSHARED CcString& Type;
      extern const CcSyncSHARED CcString  Path;
      extern const CcSyncSHARED CcString& Name;
      extern const CcSyncSHARED CcString& Size;
      extern const CcSyncSHARED CcString& Modified;
      extern const CcSyncSHARED CcString& Attributes;
      extern const CcSyncSHARED CcString& MD5;
      extern const CcSyncSHARED CcString& CRC;
      extern const CcSyncSHARED CcString  Stamp;
    }

    namespace User
    {
      extern const CcSyncSHARED CcString& Id;
      extern const CcSyncSHARED CcString Account;
      extern const CcSyncSHARED CcString Username;
      extern const CcSyncSHARED CcString Session;
      extern const CcSyncSHARED CcString SessionEnd;
    }
  }

  namespace Server
  {
    extern const CcSyncSHARED CcString ConfigFileName;
    extern const CcSyncSHARED CcString DatabaseFileName;
    extern const CcSyncSHARED CcString RootAccountName;

    namespace Database
    {
      extern const CcSyncSHARED CcString TableNameUser;
    }
    namespace ConfigTags
    {
      extern const CcSyncSHARED CcString Root;
      extern const CcSyncSHARED CcString Port;
      extern const CcSyncSHARED CcString Ssl;
      extern const CcSyncSHARED CcString SslRequired;
      extern const CcSyncSHARED CcString SslCert;
      extern const CcSyncSHARED CcString SslKey;
      extern const CcSyncSHARED CcString Account;
      extern const CcSyncSHARED CcString AccountAdmin;
      extern const CcSyncSHARED CcString AccountName;
      extern const CcSyncSHARED CcString AccountPassword;
      extern const CcSyncSHARED CcString Location;
      extern const CcSyncSHARED CcString LocationPath;
      extern const CcSyncSHARED CcString LocationType;
    }
    namespace Output
    {
      extern const CcSyncSHARED CcString Started;
    }
  }

  namespace Client
  {
    extern const CcSyncSHARED CcString ConfigFileName;
    extern const CcSyncSHARED CcString DatabaseFileName;
    namespace ConfigTags
    {
      extern const CcSyncSHARED CcString Root;
      extern const CcSyncSHARED CcString Name;
      extern const CcSyncSHARED CcString User;
      extern const CcSyncSHARED CcString UserName;
      extern const CcSyncSHARED CcString UserPassword;

      extern const CcSyncSHARED CcString Admin;
      extern const CcSyncSHARED CcString Account;

      extern const CcSyncSHARED CcString Directory;
      extern const CcSyncSHARED CcString& DirectoryName;
      extern const CcSyncSHARED CcString DirectoryLocation;
      extern const CcSyncSHARED CcString DirectoryBackupCommand;
      extern const CcSyncSHARED CcString DirectoryRestoreCommand;
      extern const CcSyncSHARED CcString DirectoryUser;
      extern const CcSyncSHARED CcString DirectoryGroup;

      extern const CcSyncSHARED CcString Command;
      extern const CcSyncSHARED CcString CommandExecutable;
      extern const CcSyncSHARED CcString CommandParameters;
      extern const CcSyncSHARED CcString CommandWorkingDirectory;
      extern const CcSyncSHARED CcString CommandTimeMask;

      extern const CcSyncSHARED CcString Server;
      extern const CcSyncSHARED CcString ServerHost;
      extern const CcSyncSHARED CcString ServerPort;

      extern const CcSyncSHARED CcString Database;
    }
  }

  namespace Commands
  {
    namespace Crc
    {
      extern const CcSyncSHARED CcString Crc;
    }

    namespace ServerGetInfo
    {
      
    }

    namespace AccountCreate
    {
      
    }

    namespace AccountLogin
    {
      extern const CcSyncSHARED CcString& Account;
      extern const CcSyncSHARED CcString& Username;
      extern const CcSyncSHARED CcString Password;
      extern const CcSyncSHARED CcString& Session;
    }

    namespace AccountRights
    {
      extern const CcSyncSHARED CcString Right;
    }

    namespace AccountGetData
    {
      extern const CcSyncSHARED CcString& Account;
      extern const CcSyncSHARED CcString& Name;
      extern const CcSyncSHARED CcString& Password;
      extern const CcSyncSHARED CcString& Server;
      extern const CcSyncSHARED CcString& ServerHost;
      extern const CcSyncSHARED CcString& ServerPort;
      extern const CcSyncSHARED CcString& Database;

      extern const CcSyncSHARED CcString& Directories;
      extern const CcSyncSHARED CcString& DirName;
      extern const CcSyncSHARED CcString& DirLocation;
      extern const CcSyncSHARED CcString& DirBCommand;
      extern const CcSyncSHARED CcString& DirRCommand;
    }
    namespace AccountSetData
    {

    }
    namespace AccountGetDirectoryList
    {
    }
    namespace AccountCreateDirectory
    {
      extern const CcSyncSHARED CcString DirectoryName;
    }
    namespace UserGetCommandList
    {

    }
    namespace DirectoryGetFileList
    {
      extern const CcSyncSHARED CcString DirectoryName;
      extern const CcSyncSHARED CcString& Id;
      extern const CcSyncSHARED CcString FilesNode;
      extern const CcSyncSHARED CcString DirsNode;
    }
    namespace DirectoryGetFileInfo
    {
      extern const CcSyncSHARED CcString& Id;
    }
    namespace DirectoryCreateDirectory
    {
      extern const CcSyncSHARED CcString& DirectoryName;
    }
    namespace DirectoryRemoveDirectory
    {
      extern const CcSyncSHARED CcString& DirectoryName;
    }
    namespace DirectoryUploadFile
    {
      extern const CcSyncSHARED CcString& DirectoryName;
    }
    namespace DirectoryGetDirectoryInfo
    {
      extern const CcSyncSHARED CcString& DirectoryName;
      extern const CcSyncSHARED CcString& Id;
    }
    namespace DirectoryDownloadFile
    {
      extern const CcSyncSHARED CcString& DirectoryName;
      extern const CcSyncSHARED CcString& Id;
    }

    namespace ServerAccountCreate
    {
      extern const CcSyncSHARED CcString& AccountName;
      extern const CcSyncSHARED CcString& Password;
    }

    namespace ServerAccountRescan
    {
    extern const CcSyncSHARED CcString Deep;
    }

    extern const CcSyncSHARED CcString Command;
    extern const CcSyncSHARED CcString Session;
    extern const CcSyncSHARED CcString Result;
    extern const CcSyncSHARED CcString ErrorCode;
    extern const CcSyncSHARED CcString ErrorMsg;
  }
}

#endif /* _CcSyncGlobals_H_ */
