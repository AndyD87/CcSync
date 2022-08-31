set ( CURRENT_PROJECT CcSyncServer )
set ( CURRENT_PROJECT_IDE_PATH     )

########
# Add Source Files
########
file (GLOB SOURCE_FILES
      "*.c"
      "*.cpp"
      "*.h")

include_directories( ${CMAKE_CURRENT_SOURCE_DIR} )

if(WINDOWS)
  CcSyncGenerateRcFileToCurrentDir(${CURRENT_PROJECT} SOURCE_FILES )
endif()

CcAddExecutable( ${CURRENT_PROJECT} ${SOURCE_FILES})

set_target_properties( ${CURRENT_PROJECT} PROPERTIES FOLDER "${PROJECT_NAME}/${CURRENT_PROJECT_IDE_PATH}")

# To keep Header an Source together, display them in same Directory
# It will only work with IDE's wich are supporting this features
SOURCE_GROUP( ""        FILES ${SOURCE_FILES})

target_link_libraries ( ${CURRENT_PROJECT} LINK_PUBLIC
                              CcKernel
                              CcSync
                              CcSql )

#################
# Post configuration steps
#################
CcSyncSetInstall( ${CURRENT_PROJECT} )
