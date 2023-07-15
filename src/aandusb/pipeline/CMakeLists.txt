# Sets the minimum version of CMake required to build your native library.
# This ensures that a certain set of CMake features is available to
# your build.

cmake_minimum_required(VERSION 3.8)

# Specifies a library name, specifies whether the library is STATIC or
# SHARED, and provides relative paths to the source code. You can
# define multiple libraries by adding multiple add.library() commands,
# and CMake builds them for you. When you build your app, Gradle
# automatically packages shared libraries with your APK.

file(GLOB src_common
    *.cpp
)

add_library(aandusb_pipeline STATIC
    ${src_common}
)

target_compile_definitions(aandusb_pipeline PRIVATE
    -DAVOID_TABLES
    #ログ出力設定
    -DNDEBUG            # LOG_ALLを無効にする・assertを無効にする場合
    -DLOG_NDEBUG        # デバッグメッセージを出さないようにする時
#   -DUSE_LOGALL		# define USE_LOGALL macro to enable all debug string
#   -DDISABLE_IMPORTGL  # when static link OpenGL|ES library
)

target_include_directories(aandusb_pipeline PRIVATE
)

target_link_libraries(aandusb_pipeline PUBLIC
)

target_link_libraries(aandusb_pipeline PRIVATE
    common_static
    aandusb_core
)
