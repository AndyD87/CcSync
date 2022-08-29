
set ( CURRENT_PROJECT CcSyncClientGui)

########
# Add Source Files
########
file (GLOB SOURCE_FILES
      "*.c"
      "*.cpp")

########
# Add Source Files
########
file (GLOB HEADER_FILES
      "*.h")

include_directories( ${CMAKE_CURRENT_SOURCE_DIR} )

# To keep Header an Source together, display them in same Directory
# It will only work with IDE's wich are supporting this features
SOURCE_GROUP( ""        FILES ${SOURCE_FILES})

if(WINDOWS)
  CcSyncGenerateRcFileToCurrentDir(${CURRENT_PROJECT} SOURCE_FILES )
endif()
CcAddGuiExecutable( ${CURRENT_PROJECT} ${SOURCE_FILES} ${HEADER_FILES} )

# To keep Header an Source together, display them in same Directory
# It will only work with IDE's wich are supporting this features
SOURCE_GROUP( ""        FILES ${SOURCE_FILES})

target_link_libraries ( ${CURRENT_PROJECT} LINK_PUBLIC
                              CcGui
                              CcKernel
                              CcSync )
                              

#################
# Post configuration steps
#################
CcSyncSetInstall( ${CURRENT_PROJECT} )
