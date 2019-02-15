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
 * @brief     CcSyncGlobals definitions
 */
#include "CcSyncGlobals.h"
#include "CcVersion.h"
#include "CcSyncVersion.h"

namespace CcSyncGlobals
{
  const CcString sTest           ("Test");
  const CcVersion Version        (CCSYNC_VERSION_MAJOR, CCSYNC_VERSION_MINOR, CCSYNC_VERSION_PATCH, CCSYNC_VERSION_BUILD);
  const CcString ConfigDirName   ("CcSync");
  const size_t MaxReconnections  = 5;
  const uint64 TransferSize      = 10240000;
  const uint64 MaxRequestSize    = TransferSize;
  const uint64 MaxResponseSize   = MaxRequestSize;
  const CcString DefaultCertFile ("SslCertificate.pem");
  const CcString DefaultKeyFile  ("SslPrivateKey.pem");
  const CcString SqliteExtension (".sqlite");
  const CcString XmlExtension    (".xml");
  const uint64 SessionLength     = 1*60*60; // 1 hour
  const uint16 DefaultPort       = CcCommonPorts::CcSync;
  const CcString DefaultPortStr  = CcString::fromNumber(DefaultPort);
  const CcString TemporaryExtension(".~CcSyncTemp~");
  const CcString LockFile(".~CcSyncLock~");

  const CcString IndexName("Id");
  const CcString NameName("Name");
  const CcString SizeName("Size");

  namespace FileInfo
  {
    const CcString& Id     = IndexName;
    const CcString DirId   ("DirId");
    const CcString Name    = NameName;
    const CcString Modified("Modified");
    const CcString CRC     ("CRC");
    const CcString MD5     ("MD5");
    const CcString Size    = SizeName;
    const CcString Changed ("Changed");
    const CcString IsFile  ("IsFile");
    const CcString Attributes("Attributes");
  }

  namespace Database
  {
    const uint64 RootDirId        = 1;
    const CcString CreateTable    ("CREATE TABLE `");
    const CcString DropTable      ("DROP TABLE `");
    const CcString Insert         ("INSERT INTO `");
    const CcString Update         ("UPDATE `");

    const CcString DirectoryListAppend ("_DirList");
    const CcString FileListAppend   ("_FileList");
    const CcString QueueAppend      ("_Queue");
    const CcString HistoryAppend    ("_History");

    namespace FileList
    {
      const CcString& Id       = IndexName;
      const CcString& DirId    = FileInfo::DirId;
      const CcString& Name     = NameName;
      const CcString& Size     = SizeName;
      const CcString Modified  ("Modified");
      const CcString MD5       ("MD5");
      const CcString CRC       ("CRC");
      const CcString Changed   ("Changed");
      const CcString& Attributes = FileInfo::Attributes;
    }

    namespace Queue
    {
      const CcString& Id      = IndexName;
      const CcString QueueId  ("QueueId");
      const CcString Type     ("Type");
      const CcString FileId   ("FileId");
      const CcString& DirId    = FileInfo::DirId;
      const CcString Name     ("Name");
      const CcString& Attributes = FileInfo::Attributes;
      const CcString Attempts ("Attempts");
    }

    namespace DirectoryList
    {
      const CcString& Id         = IndexName;
      const CcString& DirId      = FileInfo::DirId;
      const CcString& Name       = NameName;
      const CcString Modified("Modified");
      const CcString& Attributes = FileInfo::Attributes;
      const CcString ChangedMd5  ("ChangedMd5");
    }

    namespace History
    {
      const CcString& Id       = IndexName;
      const CcString& Type     = Queue::Type;
      const CcString  Path     ("Path");
      const CcString& Name     = FileList::Name;
      const CcString& Size     = FileList::Size;
      const CcString& Modified = FileList::Modified;
      const CcString& Attributes = FileInfo::Attributes;
      const CcString& MD5      = FileList::MD5;
      const CcString& CRC      = FileList::CRC;
      const CcString  Stamp    ("Stamp");
    }

    namespace User
    {
      const CcString& Id      = IndexName;
      const CcString Account    ("Account");
      const CcString Username   ("Username");
      const CcString Session    ("Session");
      const CcString SessionEnd ("SessionEnd");
    }
  }

  namespace Server
  {
    const CcString ConfigFileName ("Server.xml");
    const CcString DatabaseFileName ("Server.sqlite");
    const CcString RootAccountName("Root");
    namespace Database
    {
      const CcString TableNameUser ("User");
    }
    namespace ConfigTags
    {
      const CcString Root  ("CcSyncServer");
      const CcString Port ("Port");
      const CcString Ssl ("Ssl");
      const CcString SslRequired ("SslRequired");
      const CcString SslCert ("SslCert");
      const CcString SslKey  ("SslKey");
      const CcString Account ("Account");
      const CcString AccountAdmin("Admin");
      const CcString AccountName ("Name");
      const CcString AccountPassword ("Password");
      const CcString Location ("Location");
      const CcString LocationPath ("Path");
      const CcString LocationType ("Type");
    }
    namespace Output
    {
      const CcString Started("CcSyncServer started");
    }
  }

  namespace Client
  {
    const CcString ConfigFileName   ("Client.xml");
    const CcString DatabaseFileName ("Client.sqlite");
    namespace ConfigTags
    {
      const CcString Root ("CcSyncClient");
      const CcString Name ("Name");
      const CcString User ("User");
      const CcString UserName ("Name");
      const CcString UserPassword ("Password");

      const CcString Admin("Admin");
      const CcString Account ("Account");

      const CcString Directory("Directory");
      const CcString& DirectoryName    = Name;
      const CcString DirectoryLocation ("Location");
      const CcString DirectoryBackupCommand ("BackupCommand");
      const CcString DirectoryRestoreCommand("RestoreCommand");
      const CcString DirectoryUser("User");
      const CcString DirectoryGroup("Group");

      const CcString Command ("Command");
      const CcString CommandExecutable ("Executable");
      const CcString CommandParameters ("Parameters");
      const CcString CommandWorkingDirectory ("WorkingDirectory");
      const CcString CommandTimeMask ("TimeMask");

      const CcString Server ("Server");
      const CcString ServerHost ("Host");
      const CcString ServerPort ("Port");

      const CcString Database ("Database");
    }
  }

  namespace Commands
  {
    namespace Crc
    {
      const CcString Crc ("Crc");
    }

    namespace ServerGetInfo
    {

    }

    namespace AccountCreate
    {

    }

    namespace AccountLogin
    {
      const CcString& Account    = Database::User::Account;
      const CcString& Username   = Database::User::Username;
      const CcString Password    ("Password");
      const CcString& Session    = Database::User::Session;
    }

    namespace AccountRights
    {
      const CcString Right       ("Right");
    }

    namespace AccountGetData
    {
      const CcString& Account     = Database::User::Account;
      const CcString& Name        = Client::ConfigTags::Name;
      const CcString& Password    = Client::ConfigTags::UserPassword;
      const CcString& Server      = Client::ConfigTags::Server;
      const CcString& ServerHost  = Client::ConfigTags::ServerHost;
      const CcString& ServerPort  = Client::ConfigTags::ServerPort;
      const CcString& Database    = Client::ConfigTags::UserPassword;

      const CcString& Directories = Client::ConfigTags::Directory;
      const CcString& DirName     = Client::ConfigTags::DirectoryName;
      const CcString& DirLocation = Client::ConfigTags::DirectoryLocation;
      const CcString& DirBCommand = Client::ConfigTags::DirectoryBackupCommand;
      const CcString& DirRCommand = Client::ConfigTags::DirectoryRestoreCommand;
    }
    namespace AccountSetData
    {

    }
    namespace AccountGetDirectoryList
    {
    }
    namespace AccountCreateDirectory
    {
      const CcString DirectoryName("DirectoryName");
    }
    namespace UserGetCommandList
    {

    }
    namespace DirectoryGetFileList
    {
      const CcString DirectoryName    ("DirectoryName");
      const CcString& Id           = FileInfo::Id;
      const CcString FilesNode     ("Files");
      const CcString DirsNode      ("Directories");
    }
    namespace DirectoryGetFileInfo
    {
      const CcString& Id           = FileInfo::Id;
    }
    namespace DirectoryCreateDirectory
    {
      const CcString& DirectoryName = DirectoryGetFileList::DirectoryName;
    }
    namespace DirectoryRemoveDirectory
    {
      const CcString& DirectoryName = DirectoryGetFileList::DirectoryName;
    }
    namespace DirectoryUploadFile
    {
      const CcString& DirectoryName = DirectoryGetFileList::DirectoryName;
    }
    namespace DirectoryGetDirectoryInfo
    {
      const CcString& DirectoryName = DirectoryGetFileList::DirectoryName;
      const CcString& Id         = FileInfo::Id;
    }
    namespace DirectoryDownloadFile
    {
      const CcString& DirectoryName = DirectoryGetFileList::DirectoryName;
      const CcString& Id         = FileInfo::Id;
    }

    namespace ServerAccountCreate
    {
      const CcString& AccountName = Server::ConfigTags::AccountName;
      const CcString& Password    = Server::ConfigTags::AccountPassword;
    }

    namespace ServerAccountRescan
    {
    const CcString Deep("Deep");
    }

    const CcString Command    ("Command");
    const CcString Session    ("Session");
    const CcString Result     ("Result");
    const CcString ErrorCode  ("Error");
    const CcString ErrorMsg   ("ErrorMsg");
  }
}
