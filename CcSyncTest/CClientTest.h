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
 * @page      CcSyncTest
 * @subpage   CClientTest
 *
 * @page      CClientTest
 * @copyright Andreas Dirmeier (C) 2017
 * @author    Andreas Dirmeier
 * @par       Web:      http://coolcow.de/projects/CcOS
 * @par       Language: C++11
 * @brief     Class CClientTest
 **/
#ifndef _CClientTest_H_
#define _CClientTest_H_

#include "CcBase.h"
#include "CcTest.h"
#include "CTestClient.h"

/**
 * @brief Class impelmentation
 */
class CClientTest : public CcTest<CClientTest>
{
public:
  /**
   * @brief Constructor
   */
  CClientTest( void );

  /**
   * @brief Destructor
   */
  virtual ~CClientTest( void );

private:
  bool testClientExists();
  bool testClientWaitInput();

private: // Member
  CTestClient * m_pClient = nullptr;
};

#endif /* _CClientTest_H_ */
