project(mod_cxx)

cmake_minimum_required(VERSION 2.8)

find_package(LibBF HINTS $ENV{MISC_CMAKE_MODULES} REQUIRED)
find_package(CheckForCPP11 HINTS $ENV{MISC_CMAKE_MODULES})

if ( CPP11 )
    message(STATUS "Using C++ 11")
    add_definitions(${CPP11_DEFS})
else()
    message(FATAL_ERROR "Need a C++ 11 compiler!")
endif()

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CMAKE_VERBOSE_MAKEFILE ON)
    set(STRIP_ARGS -R .comment)
else()
    set(STRIP_ARGS -R .comment -R .GCC.command.line --strip-unneeded -s -g)
endif()

add_definitions(-Wno-write-strings -Wall -Wextra -ftrack-macro-expansion=2 -DHIDDEN_VISIBILITY -fvisibility=hidden)

if(${CMAKE_SYSTEM_NAME} STREQUAL "FreeBSD")
    link_directories(/usr/local/lib)
    include_directories(/usr/local/include)
endif()

get_filename_component(OUTPUT_DIRECTORY ./lib ABSOLUTE)

add_subdirectory(src)
