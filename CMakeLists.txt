cmake_minimum_required(VERSION 3.25)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_COMPILER "/usr/bin/clang++")

project(moba-environment VERSION 1.0.0)

configure_file(config.h.in config.h)

#FIND_PACKAGE(glib-2.0)
#find_package(PkgConfig)
#find_package(GTK2 2.6 REQUIRED COMPONENTS glib-2.0)
#pkg_check_modules(GLIB_PKG glib-2.0)

add_executable(
    moba-environment

    src/bridge.cpp
    src/main.cpp
    src/msgloop.cpp
    src/statuscontrol.cpp
    src/monitor.cpp
)

install(TARGETS moba-environment)

find_path(GLIB_INCLUDE_DIR NAMES glib.h PATH_SUFFIXES glib-2.0)

target_include_directories(moba-environment PUBLIC "${PROJECT_BINARY_DIR}")

target_link_libraries(moba-environment ncurses)
target_link_libraries(moba-environment mobacommon)
target_link_libraries(moba-environment z)
target_link_libraries(moba-environment ${CMAKE_SOURCE_DIR}/modules/lib-msghandling/libmoba-lib-msghandling.a)

include_directories(${CMAKE_SOURCE_DIR}/modules/lib-msghandling/src)
