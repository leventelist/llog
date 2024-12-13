cmake_minimum_required (VERSION 3.13)
project(llog)

set(SHARE_DIR ${CMAKE_INSTALL_PREFIX}/share/${PROJECT_NAME})
set(DESKTOP_FILE_DIR ${CMAKE_INSTALL_PREFIX}/share/applications)
set(ICONS_FILE_DIR ${CMAKE_INSTALL_PREFIX}/share/icons)

set(GLADE_FILE_NAME llog.glade)
set(GLADE_FILE_PATH ${SHARE_DIR}/${GLADE_FILE_NAME})

set(DB_FILE_NAME llog.sql)
set(DB_FILE_PATH ${SHARE_DIR}/${DB_FILE_NAME})

configure_file(llog_Config.h.in llog_Config.h)

find_package(PkgConfig REQUIRED)

#configure the compiler
set(CMAKE_C_FLAGS "-Wall -Wextra -rdynamic -g -O0")
set(CMAKE_C_FLAGS_DEBUG "-O0 -g")
set(CMAKE_C_FLAGS_RELEASE "-O2")

pkg_check_modules(SQLITE REQUIRED IMPORTED_TARGET sqlite3)
pkg_check_modules(GTK REQUIRED IMPORTED_TARGET gtk4)
pkg_check_modules(GPS REQUIRED IMPORTED_TARGET libgps)

add_executable(${PROJECT_NAME} main.c db_sqlite.c main_window.c llog.c conf.c
																preferences_window.c position.c)

#set up include directories
include_directories("${PROJECT_BINARY_DIR}")

target_link_libraries(${PROJECT_NAME} PUBLIC
				PkgConfig::SQLITE
				PkgConfig::GTK
				PkgConfig::GPS)

install (TARGETS ${PROJECT_NAME}
			DESTINATION bin
			COMPONENT bin
			PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ
			GROUP_EXECUTE GROUP_READ
			WORLD_READ WORLD_EXECUTE)

install (FILES db/create_llog.sh
			DESTINATION bin
			COMPONENT bin
			PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ
			GROUP_EXECUTE GROUP_READ
			WORLD_READ WORLD_EXECUTE)

install (FILES gui/${GLADE_FILE_NAME} gui/llog.png db/${DB_FILE_NAME}
			DESTINATION ${SHARE_DIR}
			COMPONENT config
			PERMISSIONS OWNER_WRITE OWNER_READ
			GROUP_READ WORLD_READ)

install (FILES gui/llog.desktop
			DESTINATION ${DESKTOP_FILE_DIR}
			COMPONENT config
			PERMISSIONS OWNER_WRITE OWNER_READ
			GROUP_READ WORLD_READ)

install (FILES gui/llog.png
			DESTINATION ${ICONS_FILE_DIR}
			COMPONENT config
			PERMISSIONS OWNER_WRITE OWNER_READ
			GROUP_READ WORLD_READ)
