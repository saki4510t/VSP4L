# Sets the minimum version of CMake required to build your native library.
# This ensures that a certain set of CMake features is available to
# your build.

cmake_minimum_required(VERSION 3.8)

# Specifies a library name, specifies whether the library is STATIC or
# SHARED, and provides relative paths to the source code. You can
# define multiple libraries by adding multiple add.library() commands,
# and CMake builds them for you. When you build your app, Gradle
# automatically packages shared libraries with your APK.

set(CMAKE_VERBOSE_MAKEFILE on)
set(lib_src_DIR ${CMAKE_CURRENT_SOURCE_DIR})

add_subdirectory(${lib_src_DIR}/core)
add_subdirectory(${lib_src_DIR}/pipeline)
add_subdirectory(${lib_src_DIR}/v4l2)
