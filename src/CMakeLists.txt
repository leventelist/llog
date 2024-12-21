cmake_minimum_required(VERSION 3.10)

# Set the project name
project(llog)

# Add the source files
set(SOURCES
  main.c
  db_sqlite.c
  llog.c
  main_window.c
  position.c
  preferences_window.c
  conf.c
)

# Add the executable
add_executable(llog ${SOURCES})

# Include directories
include_directories(${CMAKE_SOURCE_DIR} ${CMAKE_BINARY_DIR}/src)

configure_file(llog_config.h.in llog_config.h)

find_package(PkgConfig REQUIRED)

#configure the compiler
set(CMAKE_C_FLAGS "-Wall -Wextra -rdynamic -g -O0")
set(CMAKE_C_FLAGS_DEBUG "-O0 -g")
set(CMAKE_C_FLAGS_RELEASE "-O2")

pkg_check_modules(SQLITE REQUIRED IMPORTED_TARGET sqlite3)
pkg_check_modules(GTK REQUIRED IMPORTED_TARGET gtk4)
pkg_check_modules(GPS REQUIRED IMPORTED_TARGET libgps)
find_library(MATH_LIBRARY m REQUIRED)

target_link_libraries(${PROJECT_NAME} PUBLIC
				PkgConfig::SQLITE
				PkgConfig::GTK
				PkgConfig::GPS
				${MATH_LIBRARY})

install (TARGETS ${PROJECT_NAME}
			DESTINATION bin
			COMPONENT bin
			PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ
			GROUP_EXECUTE GROUP_READ
			WORLD_READ WORLD_EXECUTE)
