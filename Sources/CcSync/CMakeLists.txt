set ( CURRENT_PROJECT CcSync     )
set ( CURRENT_PROJECT_IDE_PATH   )

########
# Add Source Files
########
file (GLOB SOURCE_FILES
      "*.cpp"
      "*.h"
      "private/*.cpp"
      "private/*.h"
      "config/*.xml")

include_directories( ${CMAKE_CURRENT_SOURCE_DIR} )

########
# Update Version Info
########
configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/CcSyncVersion.h.in ${CMAKE_CURRENT_SOURCE_DIR}/CcSyncVersion.h @ONLY)

if(WINDOWS)
  CcSyncGenerateRcFileToCurrentDir(${CURRENT_PROJECT} SOURCE_FILES )
endif()

CcAddLibrary( ${CURRENT_PROJECT} ${CC_LINK_TYPE} ${SOURCE_FILES} )

set_target_properties( ${CURRENT_PROJECT} PROPERTIES FOLDER "${PROJECT_NAME}/${CURRENT_PROJECT_IDE_PATH}")

# To keep Header an Source together, display them in same Directory
# It will only work with IDE's wich are supporting this features
SOURCE_GROUP( ""        FILES ${SOURCE_FILES})
SOURCE_GROUP( "config"  FILES "config/default.Client.xml")
SOURCE_GROUP( "config"  FILES "config/default.Server.xml")
 
target_link_libraries ( ${CURRENT_PROJECT} LINK_PUBLIC
                              CcSsl
                              CcKernel
                              CcDocuments
                              CcSql )

#################
# Post configuration steps
#################
CcSyncTargetIncludeDirs( ${CURRENT_PROJECT} )
CcSyncLibSettings(${CURRENT_PROJECT} TRUE TRUE ${SOURCE_FILES})
