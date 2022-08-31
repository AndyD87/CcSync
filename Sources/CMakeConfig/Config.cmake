set(THIRDPARTY_DIR ${CMAKE_CURRENT_LIST_DIR}/../ThirdParty)

if(NOT EXISTS ${CMAKE_CURRENT_LIST_DIR}/CcBuildConfig/CcBuildConfig.cmake)
  execute_process(COMMAND git submodule init "${CMAKE_CURRENT_LIST_DIR}/CcBuildConfig"
          WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR})
  execute_process(COMMAND git submodule update "${CMAKE_CURRENT_LIST_DIR}/CcBuildConfig"
          WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR})
endif()
include(${CMAKE_CURRENT_LIST_DIR}/CcBuildConfig/CcBuildConfig.cmake)

################################################################################
# Load Macros from CcOS
################################################################################
include(${CMAKE_CURRENT_LIST_DIR}/ProjectMacros.cmake )
