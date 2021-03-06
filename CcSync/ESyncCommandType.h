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
 * @subpage   ESyncCommandType
 *
 * @page      ESyncCommandType
 * @copyright Andreas Dirmeier (C) 2017
 * @author    Andreas Dirmeier
 * @par       Web: http://coolcow.de
 * @version   0.01
 * @date      2016-04
 * @par       Language   C++ ANSI V3
 * @brief     Class ESyncCommandType
 **/
#ifndef _ESyncCommandType_H_
#define _ESyncCommandType_H_

#include "CcBase.h"

/**
 * @brief Class impelmentation
 */
enum class ESyncCommandType : uint16
{
  Unknown                 =      0 ,
  AllOk                           ,
  Crc                             ,
  Close                           ,
  ServerGetInfo          = 0x0100 ,
  ServerAccountCreate             ,
  ServerAccountRescan             ,
  ServerAccountRemove             ,
  ServerStop                      ,
  AccountCreate          = 0x0200 ,
  AccountLogin                    ,
  AccountGetData                  ,
  AccountSetData                  ,
  AccountGetDirectoryList         ,
  AccountGetCommandList           ,
  AccountCreateDirectory          ,
  AccountRights                   ,
  AccountDatabaseUpdateChanged    ,
  AccountRemoveDirectory          ,
  DirectoryGetFileList   = 0x0300 ,
  DirectoryGetFileInfo            ,
  DirectoryGetDirectoryInfo       ,
  DirectoryCreateDirectory        ,
  DirectoryRemoveDirectory        ,
  DirectoryUploadFile             ,
  CommandRemoved                  ,
  DirectoryDownloadFile           ,
  DirectoryRemoveFile             ,
  DirectoryUpdateFileInfo         ,
};

#endif /* _ESyncCommandType_H_ */
