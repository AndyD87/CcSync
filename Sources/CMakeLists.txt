################################################################################
# @Author Andreas Dirmeier
#
# Main File to build CcSync
#
# To Generate a Solution execute a command like:
#   cmake -G "Visual Studio 12"
#
################################################################################
cmake_minimum_required (VERSION 3.0)
set(CMAKE_PROJECT_NAME "CcSync")

include(CMakeConfig/Config.cmake)

if($ENV{TEST_CCOS})
  set(MEMORYMONITOR_ENABLED      TRUE)
  set(MEMORYMONITOR_CHECK_KERNEL TRUE)
else()
  # Enable to check memory usage and leakings
  #set(MEMORYMONITOR_ENABLED      TRUE)
  #set(MEMORYMONITOR_CHECK_KERNEL TRUE)
  #add_definitions(-DTESTING_REDUCE_STRESS)
endif()

# Set Debug build if not BUILD_TYPE selected
#if(NOT CMAKE_BUILD_TYPE)
#  set(CMAKE_BUILD_TYPE Debug)
#endif()

# Enable verbose output if required
#set(CMAKE_VERBOSE_MAKEFILE TRUE CACHE INTERNAL "")

message("-- Default build type: ${CMAKE_BUILD_TYPE}")

################################################################################
# Setup Output Directory if not set
################################################################################
# To be able to overwrite global output root use -DCC_OUTPUT_DIR=/path on configuration
if(NOT CC_OUTPUT_DIR)
  set(CC_OUTPUT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Output")
endif()

################################################################################
# Setup Globals
################################################################################
include( ${CMAKE_CURRENT_SOURCE_DIR}/VERSION.cmake )

if(WINDOWS)
  set(CMAKE_INSTALL_PREFIX "C:/Program Files/${CMAKE_PROJECT_NAME}")
endif()

################################################################################
# Configuration is done, setup Project
################################################################################
if(NOT CC_NO_PROJECT)
  if(${CMAKE_VERSION} VERSION_GREATER_EQUAL 3.12)
    set(PAGE_SUPPORT_NAME  HOMEPAGE_URL) 
    set(PAGE_SUPPORT_VALUE ${CCSYNC_URL})
  else()
    set(PAGE_SUPPORT_NAME  )
    set(PAGE_SUPPORT_VALUE )
  endif()
  project(${CMAKE_PROJECT_NAME} 
          VERSION       ${CCSYNC_VERSION_CMAKE} # Value is defined in VERSION.cmake
          LANGUAGES     ${CCSYNC_LANGUAGES}     # Value is defined in VERSION.cmake
          DESCRIPTION   ${CCSYNC_DESCRIPTION}   # Value is defined in VERSION.cmake
          ${PAGE_SUPPORT_NAME} ${PAGE_SUPPORT_VALUE}  # Value is defined in VERSION.cmake
  )
endif()
CcBuildConfigLoad()

################################################################################
# Setup Debug
################################################################################
if(CMAKE_BUILD_TYPE MATCHES "[Dd][Ee][Bb][Uu][Gg]")
  message("- Build-Type set to Debug")
  set(CCSYNC_BUILD_TYPE "DEBUG" )
  set(CMAKE_VERBOSE_MAKEFILE          ON)
else()
  message("- Build-Type set to Release")
  set(CCSYNC_BUILD_TYPE "RELEASE" )
endif()

################################################################################
# Setup Output Paths
################################################################################
# To be able to overwrite global output root use -DCC_OUTPUT_DIR=/path on configuration
if(NOT CC_OUTPUT_DIR)
  set(CC_OUTPUT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Output")
endif()

################################################################################
# Setup Default Values for CMAKE
################################################################################
CcNoConfigurationDirs()
enable_testing()
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

find_package(CcOS COMPONENTS CcKernel CcSsl CcDocuments CcSql CcTesting CcModule sqlite3 openssl)

################################################################################
# Setup CcSync Projects
################################################################################
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/CcSync)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/CcSyncServer)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/CcSyncClient)
#add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/CcSyncClientGui)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/CcSyncTest)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/Doxygen)


set(CPACK_PACKAGE_DESCRIPTION         "CcSync - Backup System")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Backup system based on CcOS")
set(CPACK_PACKAGE_VENDOR              "Andreas Dirmeier")
set(CPACK_PACKAGE_CONTACT             "coolcow_ccos@yahoo.com")
set(CPACK_PACKAGE_VERSION             ${PROJECT_VERSION})
set(CPACK_PACKAGE_INSTALL_DIRECTORY   ${CMAKE_PROJECT_NAME})
set(CPACK_RESOURCE_FILE_LICENSE       ${CMAKE_CURRENT_SOURCE_DIR}/../Documents/COPYING.LESSER.rtf)

if(LINUX)
  set(CPACK_GENERATOR "DEB")
  set(CPACK_DEBIAN_PACKAGE_MAINTAINER ${CPACK_PACKAGE_VENDOR})
  
  include(CPack)
elseif(WINDOWS)
  set(CPACK_GENERATOR                 "ZIP")
  include(CPack)
endif()