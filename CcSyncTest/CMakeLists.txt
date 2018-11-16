################################################################################
# Create Tests only if we are building CcSync
################################################################################
if("${CMAKE_PROJECT_NAME}" STREQUAL "CcSync")

  set(CURRENT_PROJECT CcSyncTest)

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
  
  add_executable( ${CURRENT_PROJECT} ${SOURCE_FILES} )
  
  set_property( TARGET ${CURRENT_PROJECT} PROPERTY FOLDER "Testing")
  
  source_group( "" FILES ${SOURCE_FILES})
  
  target_link_libraries ( ${CURRENT_PROJECT} LINK_PUBLIC CcKernel )

  CcAddTest( ${CURRENT_PROJECT} )
 
endif("${CMAKE_PROJECT_NAME}" STREQUAL "CcSync")
