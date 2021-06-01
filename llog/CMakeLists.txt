cmake_minimum_required (VERSION 3.13)
project(llog)

find_package(PkgConfig REQUIRED)

#configure the compiler
set(CMAKE_C_FLAGS "-Wall -Wextra -rdynamic -g -O0")
set(CMAKE_C_FLAGS_DEBUG "-O0 -g")
set(CMAKE_C_FLAGS_RELEASE "-O2")

pkg_check_modules(SQLITE REQUIRED IMPORTED_TARGET sqlite3)
pkg_check_modules(GTK REQUIRED IMPORTED_TARGET gtk+-3.0)

add_executable(${PROJECT_NAME} main.c db_sqlite.c main_window.c llog.c)


#set up include directories
include_directories("${PROJECT_BINARY_DIR}")
include_directories(/usr/include/readline)

target_link_libraries(${PROJECT_NAME} PUBLIC
		readline
        PkgConfig::SQLITE
        PkgConfig::GTK)

install (TARGETS ${PROJECT_NAME}
			DESTINATION bin
			COMPONENT bin
			PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ
				GROUP_EXECUTE GROUP_READ
				WORLD_READ WORLD_EXECUTE)

#install (FILES db/create.sh
#			DESTINATION bin
#			COMPONENT config
#			RENAME llog_create
#			PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ
#				GROUP_EXECUTE GROUP_READ
#				WORLD_READ WORLD_EXECUTE)
