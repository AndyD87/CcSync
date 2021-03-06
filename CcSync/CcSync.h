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
 * @title     CcSync
 * @copyright Andreas Dirmeier (C) 2017
 * @author    Andreas Dirmeier
 * @par       Web: http://coolcow.de
 * @version   0.01
 * @date      2016-04
 * @par       Language   C++ ANSI V3
 */

#ifdef _MSC_VER
# ifndef CcSyncSHARED
#   ifdef CcSync_EXPORTS
 //    Cmake definition for shared build is set
#     define CcSyncSHARED __declspec(dllexport)
#   elif defined CC_STATIC
 //    CCOS will be build as static library no im-/export
#     define CcSyncSHARED
#   else
 //    if no definition found, we are on importing as dll
#     define CcSyncSHARED __declspec(dllimport)
#   endif
# endif
#else
# define CcSyncSHARED
#endif
