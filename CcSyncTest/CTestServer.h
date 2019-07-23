/*
 * This file is part of CcOS.
 *
 * CcOS is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * CcOS is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with CcOS.  If not, see <http://www.gnu.org/licenses/>.
 **/
/**
 * @page      CcUtil
 * @subpage   CTestServer
 *
 * @page      CTestServer
 * @copyright Andreas Dirmeier (C) 2017
 * @author    Andreas Dirmeier
 * @par       Web:      http://coolcow.de/projects/CcOS
 * @par       Language: C++11
 * @brief     Class CTestServer
 **/
#ifndef _CTestServer_H_
#define _CTestServer_H_

#include "CcBase.h"
#include "CcProcess.h"

/**
 * @brief Class impelmentation
 */
class CTestServer 
{
public:
  /**
   * @brief Constructor
   */
  CTestServer(const CcString& sServerExePath, const CcString& sConfigDir);

  /**
   * @brief Destructor
   */
  virtual ~CTestServer( void );

  void resetArguments();
  void addArgument(const CcString& sArgument)
    { m_oServerProc.addArgument(sArgument); }

  bool serverExists();

  bool createConfiguration(const CcString& sPort, const CcString& sUsername, const CcString& sPassword, const CcString& sPath);

  bool start();
  bool stop();

  CcString readAllData();

private: // Methods
  CcString readUntil(const CcString& sStringEnd, const CcDateTime& oTimeout);
  bool readUntilMatches(const CcString& sStringEnd, const CcDateTime& oTimeout);

private:
  CcProcess m_oServerProc;
  CcString  m_sConfigDir;
};

#endif /* _CTestServer_H_ */
