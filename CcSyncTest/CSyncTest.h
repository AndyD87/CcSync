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
 * @subpage   CSyncTest
 *
 * @page      CSyncTest
 * @copyright Andreas Dirmeier (C) 2017
 * @author    Andreas Dirmeier
 * @par       Web:      http://coolcow.de/projects/CcOS
 * @par       Language: C++11
 * @brief     Class CSyncTest
 **/
#ifndef _CSyncTest_H_
#define _CSyncTest_H_

#include "CcBase.h"
#include "CcTest.h"

class CSyncTestPrivate;

/**
 * @brief Class impelmentation
 */
class CSyncTest : public CcTest<CSyncTest>
{
public:
  /**
   * @brief Constructor
   */
  CSyncTest( void );

  /**
   * @brief Destructor
   */
  virtual ~CSyncTest( void );

private:
  bool testEnvironment();
  bool testSetupServer();
  bool testSetupClient1();
  bool testSetupClient2();
  bool testStartServer();
  bool testStopServer();
  bool testCheckLoginClient1();
  bool testCheckLoginClient2();

  bool testLoginClient1();
  bool testCreateTestDir();

private: // Member
  CSyncTestPrivate* m_pPrivate = nullptr;
};

#endif /* _CSyncTest_H_ */
