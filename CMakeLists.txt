cmake_minimum_required (VERSION 3.20)

project("hijacker")

set(PROJECT_ROOT "${CMAKE_CURRENT_SOURCE_DIR}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_ROOT}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_ROOT}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_ROOT}/lib)	# static libs are archive

add_subdirectory(crt)
add_subdirectory(daemon)
add_subdirectory(daemon_shim)
add_subdirectory(libhijacker)
add_subdirectory(spawner)
add_subdirectory(homebrew)

# this was too complicated
#include(FetchContent)
#FetchContent_Declare(
#  NidResolver
#  GIT_REPOSITORY https://github.com/astrelsky/libNidResolver.git
#  OVERRIDE_FIND_PACKAGE
#)
#FetchContent_MakeAvailable(NidResolver)

add_subdirectory(libNidResolver)
