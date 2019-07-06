INCLUDE(FindPkgConfig)
cmake_minimum_required (VERSION 3.0)

project(llog)


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

pkg_check_modules(EXTLIBS sqlite3)

#this must be prior to 'add_executable'

if (EXTLIBS_FOUND)
  link_directories("${EXTLIBS_LIBRARY_DIRS}")
endif ()

add_executable(${PROJECT_NAME} main.c conf.c db_sqlite.c getch.c)

target_link_libraries(${PROJECT_NAME} "readline")

if (EXTLIBS_FOUND)
  include_directories("${EXTLIBS_INCLUDEDIR}")
  target_link_libraries(${PROJECT_NAME} ${EXTLIBS_LIBRARIES} "readline")
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

