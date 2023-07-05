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

file(GLOB src_common
    *.cpp
)

add_library(aandusb_core STATIC
    ${src_common}
)

target_compile_definitions(aandusb_core PRIVATE
    AVOID_TABLES
    #ログ出力設定
    NDEBUG            # LOG_ALLを無効にする・assertを無効にする場合
    LOG_NDEBUG        # デバッグメッセージを出さないようにする時
#   USE_LOGALL		# define USE_LOGALL macro to enable all debug string
)

target_include_directories(aandusb_core PRIVATE
    ${LIBJPEG_TURBO_INCLUDE_DIRS}
    ../../libyuv/include/
)

target_include_directories(aandusb_core PUBLIC
    ../../
    ../../common/
    ../../aandusb/
)

target_link_libraries(aandusb_core PRIVATE
    ${LIBUDEV_LIBRARIES}
    ${LIBJPEG_LIBRARIES}
    ${LIBJPEG_TURBO_LIBRARIES}
    yuv
    common_static
)

target_link_libraries(aandusb_core PUBLIC
)
