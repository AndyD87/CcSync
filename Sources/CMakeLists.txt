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
include( ${CMAKE_CURRENT_SOURCE_DIR}/CMakeConfig/Config.cmake )

if(WINDOWS)
  set(CMAKE_INSTALL_PREFIX "C:/Program Files/${CMAKE_PROJECT_NAME}")
endif()

################################################################################
# Configuration is done, setup Project
################################################################################
project("CcSync" VERSION "${CCSYNC_VERSION_CMAKE}")
CcBuildConfigLoad()

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

find_package(CcOS COMPONENTS CcKernel CcSsl CcDocuments)

################################################################################
# Setup CcSync Projects
################################################################################
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/CcSync)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/CcSyncServer)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/CcSyncClient)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/CcSyncClientGui)
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
  #CcLoadWixTools()
  set(CPACK_GENERATOR                 "ZIP")
  #set(CPACK_WIX_UPGRADE_GUID          "750BDD2D-8242-443A-8921-0953FA67A053")
  include(CPack)
endif()