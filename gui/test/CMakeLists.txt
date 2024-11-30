cmake_minimum_required(VERSION 3.10)

# Set the project name
project(Gtk4Test C)

# Find the GTK4 package
find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK4 REQUIRED gtk4)

# Add the executable
add_executable(gtk4test gtk4test.c)

# Include GTK4 headers
target_include_directories(gtk4test PRIVATE ${GTK4_INCLUDE_DIRS})

# Link GTK4 libraries
target_link_libraries(gtk4test PRIVATE ${GTK4_LIBRARIES})

# Add compiler flags
target_compile_options(gtk4test PRIVATE ${GTK4_CFLAGS_OTHER})