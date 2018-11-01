cmake_minimum_required (VERSION 3.0)
project(llog)


#set prefix
if (${CMAKE_HOST_SYSTEM_NAME} STREQUAL "Linux")
	set (CMAKE_INSTALL_PREFIX "/usr/local")
elseif (${CMAKE_HOST_SYSTEM_NAME} STREQUAL "FreeBSD")
	set (CMAKE_INSTALL_PREFIX "/opt")
else ()
	set (CMAKE_INSTALL_PREFIX "/usr/local")
endif ()


set (llog_VERSION_MAJOR 1)
set (llog_VERSION_MINOR 0)
set (llog_VERSION_PATCH 0)

# configure a header file to pass some of the CMake settings
# to the source code
#configure_file (
#  "${PROJECT_SOURCE_DIR}/${PROJECT_NAME}_Config.h.in"
#  "${PROJECT_BINARY_DIR}/${PROJECT_NAME}_Config.h"
#  )

#set up include directories
include_directories("${PROJECT_BINARY_DIR}")

#configure the compiler
set(CMAKE_C_FLAGS "-Wall -Wextra")
set(CMAKE_C_FLAGS_DEBUG "-O0 -g")
set(CMAKE_C_FLAGS_RELEASE "-O2")

message("Host is: ${CMAKE_HOST_SYSTEM_NAME}")
message("Kernel is: ${CMAKE_HOST_SYSTEM}")
message("Configuring for build type: ${CMAKE_BUILD_TYPE}")


message("Prefix is: ${CMAKE_INSTALL_PREFIX}")


#set(LIBLBUS_DIR "liblbus")
#set(LIBLBUS_NAME "lbuslib")
#add_subdirectory(${LIBLBUS_DIR})
#include_directories(${LIBLBUS_DIR})

add_executable(${PROJECT_NAME} main.c conf.c  csv.c  getch.c)

target_link_libraries(${PROJECT_NAME} readline)


install (TARGETS ${PROJECT_NAME}
			DESTINATION bin
			COMPONENT bin
			PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ
				GROUP_EXECUTE GROUP_READ
				WORLD_READ WORLD_EXECUTE)
