
set ( CURRENT_PROJECT CcSync)

########
# Add Source Files
########
file (GLOB SOURCE_FILES
      "src/*.c"
      "src/*.cpp"
      "config/*.xml"
      "*.h")

include_directories( ${CMAKE_CURRENT_SOURCE_DIR} )

add_library( ${CURRENT_PROJECT} ${CCOS_LINK_TYPE} ${SOURCE_FILES} )

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
CcSyncSetInstall( ${CURRENT_PROJECT} )
#CcSyncSetInstallConfig ( ${CURRENT_PROJECT} )