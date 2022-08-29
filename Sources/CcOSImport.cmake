set(CCOS_DIR ${CMAKE_CURRENT_LIST_DIR}/CcOS)  

if(NOT EXISTS ${CCOS_DIR})
execute_process(COMMAND git submodule init "${CCOS_DIR}"
				WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR})
execute_process(COMMAND git submodule update "${CCOS_DIR}"
				WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR})
endif()

set(CCOS_NO_PROJECT TRUE)
set(CCOS_BUILDLEVEL                       1     )

add_subdirectory(${CCOS_DIR}/Sources)
