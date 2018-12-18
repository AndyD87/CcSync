set(CCOS_DIR ${CMAKE_CURRENT_LIST_DIR}/CcOS)  

macro(CcOSLoad)
  if(NOT EXISTS ${CCOS_DIR}/CcOS.cmake)
    execute_process(COMMAND git submodule init "${CCOS_DIR}"
                    WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR})
    execute_process(COMMAND git submodule update "${CCOS_DIR}"
                    WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR})
  endif()
  
  if(DEFINED CCOS_BOARD)
    CcOSLoadMacros()
    message("- Load Board")
    add_subdirectory(${CCOS_DIR}/${CCOS_BOARD} )
  endif()
endmacro()

macro(CcOSLoadMacros)
  include(${CCOS_DIR}/CMakeConfig/CcMacros.cmake)
endmacro()

macro(CcOSLoadProjects)
  set(CCOS_BUILDLEVEL 1)
  set(CCOS_CCGUI_ACTIVE 4)
  set(CCOS_CCUTIL_CCDOCUMENTS_ACTIVE 4)
  set(CCOS_CCUTIL_CCSQL_ACTIVE 4)
  set(CCOS_CCNETWORK_CCSSL_ACTIVE 4)
  set(CCOS_THIRDPARTY_SQLITE3_ACTIVE 4)
  include(${CCOS_DIR}/CcOS.cmake)
endmacro()
