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

# Enable to check memory usage and leakings
#set(MEMORYMONITOR_ENABLED TRUE)

# Set Debug build if not BUILD_TYPE selected
#if(NOT CMAKE_BUILD_TYPE)
#  set(CMAKE_BUILD_TYPE Debug)
#endif()

# Enable verbose output if required
#set(CMAKE_VERBOSE_MAKEFILE TRUE CACHE INTERNAL "")

message("-- Default build type: ${CMAKE_BUILD_TYPE}")

################################################################################
# Setup Globals
################################################################################
include( ${CMAKE_CURRENT_SOURCE_DIR}/VERSION.cmake )
include( ${CMAKE_CURRENT_SOURCE_DIR}/CMakeConfig/ProjectMacros.cmake )
include( ${CMAKE_CURRENT_SOURCE_DIR}/CcOSImport.cmake )

if(WINDOWS)
  set(CMAKE_INSTALL_PREFIX "C:/Program Files/CcSync")
endif()

################################################################################
# Setup Cache directory if not yet defined
################################################################################
if(NOT DEFINED CC_CACHE_DIR)
  set( CC_CACHE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Cache)
  if(NOT EXISTS ${CC_CACHE_DIR})
    file(MAKE_DIRECTORY ${CC_CACHE_DIR})
  endif()
endif()

################################################################################
# Load CcOS
################################################################################
CcOSLoad()
CcOSLoadMacros()

################################################################################
# Configuration is done, setup Project
################################################################################
project("CcSync" VERSION "${CCSYNC_VERSION_CMAKE}")

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

################################################################################
# Import CcOS
################################################################################
CcOSLoadProjects()

################################################################################
# Setup CcSync Projects
################################################################################
add_subdirectory(CcSync)
add_subdirectory(CcSyncServer)
add_subdirectory(CcSyncClient)
add_subdirectory(CcSyncClientGui)
add_subdirectory(CcSyncTest)

################################################################################
# Update dokumentation
################################################################################
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/Documents/Doxygen)


set(CPACK_PACKAGE_DESCRIPTION         "CcSync - Backup System")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Backup system based on CcOS")
set(CPACK_PACKAGE_VENDOR              "Andreas Dirmeier")
set(CPACK_PACKAGE_CONTACT             "coolcow_ccos@yahoo.com")
set(CPACK_PACKAGE_VERSION             ${PROJECT_VERSION})   
set(CPACK_PACKAGE_INSTALL_DIRECTORY   ${CMAKE_PROJECT_NAME})
set(CPACK_RESOURCE_FILE_LICENSE       ${CMAKE_CURRENT_SOURCE_DIR}/Documents/COPYING.LESSER.rtf)

if(LINUX)
  set(CPACK_GENERATOR "DEB")
  set(CPACK_DEBIAN_PACKAGE_MAINTAINER ${CPACK_PACKAGE_VENDOR})
  
  include(CPack)
elseif(WINDOWS)
  CcLoadWixTools()
  set(CPACK_GENERATOR                 "WIX")
  set(CPACK_WIX_UPGRADE_GUID          "750BDD2D-8242-443A-8921-0953FA67A053")
  include(CPack)
endif()