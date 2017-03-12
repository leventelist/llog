INCLUDE(FindPkgConfig)
cmake_minimum_required (VERSION 3.0)

project(llog)

set (home_manager_VERSION_MAJOR 2)
set (home_manager_VERSION_MINOR 0)
set (home_manager_VERSION_PATCH 0)

# configure a header file to pass some of the CMake settings
# to the source code
configure_file (
  "${PROJECT_SOURCE_DIR}/llog_Config.h.in"
  "${PROJECT_BINARY_DIR}/llog_Config.h"
  )

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

pkg_check_modules(PC_SQLITE sqlite3)

#this must be prior to 'add_executable'

if (PC_SQLITE_FOUND)
  link_directories("${PC_SQLITE_LIBRARY_DIRS}")
endif ()

add_executable(${PROJECT_NAME} main.c conf.c db_sqlite.c getch.c)

target_link_libraries(${PROJECT_NAME} ${LIBLBUS_NAME})

if (PC_SQLITE_FOUND)
  include_directories("${PC_SQLITE_INCLUDEDIR}")
  target_link_libraries(${PROJECT_NAME} ${PC_SQLITE_LIBRARIES})
endif ()

install (TARGETS ${PROJECT_NAME}
			DESTINATION bin
			COMPONENT bin
			PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ
				GROUP_EXECUTE GROUP_READ
				WORLD_READ WORLD_EXECUTE)

install (FILES hmd.conf
			DESTINATION etc
			COMPONENT config
			RENAME hmd.conf_sample
			PERMISSIONS OWNER_WRITE OWNER_READ
				GROUP_READ
				WORLD_READ)

