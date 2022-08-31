################################################################################
# Create Tests only if we are building CcSync
################################################################################
if("${CMAKE_PROJECT_NAME}" STREQUAL "CcSync")

  set ( CURRENT_PROJECT CcSyncTest )
  set ( CURRENT_PROJECT_IDE_PATH   Testing)

  ##############################################################################
  # Add Source Files
  ##############################################################################
  file (GLOB SOURCE_FILES
        "*.c"
        "*.cpp"
        "*.h")
  
  include_directories( ${CMAKE_CURRENT_SOURCE_DIR} )
  
  if(WINDOWS)
    CcSyncGenerateRcFileToCurrentDir(${CURRENT_PROJECT} SOURCE_FILES )
  endif()
  
  CcAddExecutable( ${CURRENT_PROJECT} ${SOURCE_FILES} )

  set_target_properties( ${CURRENT_PROJECT} PROPERTIES FOLDER "${PROJECT_NAME}/${CURRENT_PROJECT_IDE_PATH}")
  
  source_group( "" FILES ${SOURCE_FILES})
  
  target_link_libraries ( 
    ${CURRENT_PROJECT} LINK_PUBLIC 
    CcKernel 
    CcTesting 
  )

  CcAddTest( ${CURRENT_PROJECT} )
 
endif("${CMAKE_PROJECT_NAME}" STREQUAL "CcSync")
