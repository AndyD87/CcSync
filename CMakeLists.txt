################################################################################
# @Author Andreas Dirmeier
#
# Main File to build all CcOS Components
#
# To Generate a Solution execute the command:
#   cmake -G "Visual Studio 12"
#
################################################################################
cmake_minimum_required (VERSION 3.0)

################################################################################
# Setup Globals
################################################################################
include( ${CMAKE_CURRENT_SOURCE_DIR}/VERSION.cmake )
include( ${CMAKE_CURRENT_SOURCE_DIR}/CMakeConfig/CcMacros.cmake )
include( ${CMAKE_CURRENT_SOURCE_DIR}/CMakeConfig/ProjectMacros.cmake )

if(WINDOWS)
  set(CMAKE_INSTALL_PREFIX "C:/Program Files/CcSync")
endif()

################################################################################
# Configuration is done, setup Project
################################################################################
project("CcSync")

################################################################################
# Setup Debug
################################################################################
if(CMAKE_BUILD_TYPE MATCHES "[Dd][Ee][Bb][Uu][Gg]")
  message("- Build-Type set to ${CMAKE_BUILD_TYPE}")
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

# Set runtime output dir to root/output if no other location was defined
if(NOT CMAKE_RUNTIME_OUTPUT_DIRECTORY)
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CC_OUTPUT_DIR}/bin")
endif()

# Set library output dir to root/output if no other location was defined
if(NOT CMAKE_LIBRARY_OUTPUT_DIRECTORY)
  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CC_OUTPUT_DIR}/lib")
endif()

# Set archive output dir to root/output if no other location was defined
if(NOT CMAKE_ARCHIVE_OUTPUT_DIRECTORY)
  set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CC_OUTPUT_DIR}/lib/static")
endif()

CcNoConfigurationDirs()
enable_testing()
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

################################################################################
# Import CcOS
################################################################################
include(CcOSImport.cmake)

add_subdirectory(CcSync)
add_subdirectory(CcSyncServer)
add_subdirectory(CcSyncClient)
add_subdirectory(CcSyncClientGui)


set(CPACK_PACKAGE_DESCRIPTION         "CcSync - Backup System")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Backup system based on CcOS")
set(CPACK_PACKAGE_VENDOR              "Andreas Dirmeier")
set(CPACK_PACKAGE_CONTACT             "coolcow_ccos@yahoo.com")
set(CPACK_PACKAGE_INSTALL_DIRECTORY   ${CMAKE_PROJECT_NAME})
set(CPACK_RESOURCE_FILE_LICENSE       ${CMAKE_CURRENT_SOURCE_DIR}/Documents/COPYING.LESSER.rtf)

if(LINUX)
  set(CPACK_GENERATOR "DEB")
  set(CPACK_DEBIAN_PACKAGE_MAINTAINER ${CPACK_PACKAGE_VENDOR})
  
  INCLUDE(CPack)
elseif(WINDOWS)
  CcLoadWixTools()

  set(CPACK_GENERATOR                 "WIX")
  set(CPACK_WIX_UPGRADE_GUID          "750BDD2D-8242-443A-8921-0953FA67A053")

  INCLUDE(CPack)
endif()