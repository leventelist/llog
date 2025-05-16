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
	exporter_writer.c
	export_window.c
	xml_client.c
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
pkg_check_modules(XML REQUIRED IMPORTED_TARGET libxml-2.0)

find_library(MATH_LIBRARY m REQUIRED)

execute_process(
  COMMAND xmlrpc-c-config client --libs
  OUTPUT_VARIABLE XMLRPC_LIBS
  RESULT_VARIABLE XMLRPC_LIBS_RESULT
)
if(NOT XMLRPC_LIBS_RESULT EQUAL 0)
  message(FATAL_ERROR "Failed to run xmlrpc-c-config client --libs")
endif()

execute_process(
  COMMAND xmlrpc-c-config client --cflags
  OUTPUT_VARIABLE XMLRPC_CFLAGS
  RESULT_VARIABLE XMLRPC_CFLAGS_RESULT
)
if(NOT XMLRPC_CFLAGS_RESULT EQUAL 0)
  message(FATAL_ERROR "Failed to run xmlrpc-c-config client --cflags")
endif()

string(STRIP ${XMLRPC_LIBS} XMLRPC_LIBS)
string(STRIP ${XMLRPC_CFLAGS} XMLRPC_CFLAGS)

target_link_libraries(${PROJECT_NAME} PUBLIC
				PkgConfig::SQLITE
				PkgConfig::GTK
				PkgConfig::GPS
				PkgConfig::XML
				${MATH_LIBRARY}
				${XMLRPC_LIBS}) #Manually add the xmlrpc_client and xmlrpc_util libraries

install (TARGETS ${PROJECT_NAME}
			DESTINATION bin
			COMPONENT bin
			PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ
			GROUP_EXECUTE GROUP_READ
			WORLD_READ WORLD_EXECUTE)
