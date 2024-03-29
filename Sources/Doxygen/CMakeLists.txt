################################################################################
# Build doxygen documentation if doxygen is available
################################################################################
set(CURRENT_PROJECT "${PROJECT_NAME}Doxygen")

find_package(Doxygen)

if(Doxygen_FOUND)
  # Support cmake find doxygen too
  if(TARGET Doxygen::doxygen)
    get_target_property(Doxygen_EXECUTABLE Doxygen::doxygen IMPORTED_LOCATION)
    if(NOT EXISTS "${Doxygen_EXECUTABLE}")
        message(FATAL_ERROR "Invalid doxygen location ${Doxygen_EXECUTABLE}")
    endif()
  endif()

  set(WARNINGS_FORMAT "\$file:\$line: \$text") # default format
  if(QT_QMAKE_EXECUTABLE)
    # do nothing
  elseif(DEFINED MSVC)
    set(WARNINGS_FORMAT "\$file(\$line): \$text") # Visual Studio warning format
  endif()

  configure_file( ${CMAKE_CURRENT_LIST_DIR}/config.in.txt
                  ${CMAKE_CURRENT_BINARY_DIR}/config.txt
                  @ONLY
  )

  set( SOURCE_FILES
    ${CMAKE_CURRENT_LIST_DIR}/config.in.txt
    ${CMAKE_CURRENT_LIST_DIR}/doxygen.h
    ${CMAKE_CURRENT_LIST_DIR}/MarkDown/Mainpage.md
  )

  add_custom_target(
    ${CURRENT_PROJECT}
    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/config.txt
    SOURCES ${SOURCE_FILES}
    COMMAND ${Doxygen_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/config.txt
    #COMMAND ${CMAKE_COMMAND} -E touch ${WARNINGS_FILE}
    #COMMAND ${PRINT_COMMAND} ${WARNINGS_FILE}
    WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
  )
  set_property( TARGET ${CURRENT_PROJECT} PROPERTY FOLDER "CMakePredefinedTargets")
endif()
