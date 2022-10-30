cmake_minimum_required(VERSION 3.24)
project(n64-controller)

###############################################################################
#
#  DirectInput
#

FIND_PATH( DIRECTINPUT_INCLUDE_DIR dinput.h
           PATHS
           ${DIRECTINPUT_ROOT_DIR}/Include
)

FIND_LIBRARY( DIRECTINPUT_LIBRARY dinput7.lib dinput8.lib
              PATHS
              ${DIRECTINPUT_ROOT_DIR}/lib/x86
)

FIND_LIBRARY( DIRECTINPUT_GUID_LIBRARY dxguid.lib
              PATHS
              ${DIRECTINPUT_ROOT_DIR}/lib/x86
)

SET( DIRECTINPUT_LIBRARIES
     ${DIRECTINPUT_LIBRARY}
     ${DIRECTINPUT_GUID_LIBRARY}
)

IF ( DIRECTINPUT_INCLUDE_DIR AND DIRECTINPUT_LIBRARIES )
    SET( DIRECTINPUT_FOUND TRUE )
ENDIF ( DIRECTINPUT_INCLUDE_DIR AND DIRECTINPUT_LIBRARIES )

###############################################################################
#
#  ViGEmClient
#

add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/external/ViGEmClient)

###############################################################################
#
#  Project
#

set(SOURCES
	${CMAKE_CURRENT_LIST_DIR}/src/main.cpp
)

add_executable(${PROJECT_NAME} ${SOURCES})
target_link_libraries(${PROJECT_NAME}
	PRIVATE
		${DIRECTINPUT_LIBRARIES}
		ViGEmClient
)
target_include_directories(${PROJECT_NAME}
	PRIVATE
		${DIRECTINPUT_INCLUDE_DIR}
)
set_target_properties(${PROJECT_NAME} PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)