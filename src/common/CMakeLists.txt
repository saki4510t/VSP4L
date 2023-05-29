# Sets the minimum version of CMake required to build your native library.
# This ensures that a certain set of CMake features is available to
# your build.

cmake_minimum_required(VERSION 3.8)

# Specifies a library name, specifies whether the library is STATIC or
# SHARED, and provides relative paths to the source code. You can
# define multiple libraries by adding multiple add.library() commands,
# and CMake builds them for you. When you build your app, Gradle
# automatically packages shared libraries with your APK.

# 同じファイルを複数回ビルドしないようにオブジェクトライブラリーを生成させる
add_library(objlib OBJECT
    binutils.cpp
    charutils.cpp
    eglbase.cpp
    gloffscreen.cpp
    gltexture.cpp
    glrenderer.cpp
    glutils.cpp
    handler.cpp
    image_helper.cpp
    json_helper.cpp
    matrix.cpp
    times.cpp
)

target_compile_definitions(objlib PRIVATE
    AVOID_TABLES
    #ログ出力設定
    NDEBUG            # LOG_ALLを無効にする・assertを無効にする場合
    LOG_NDEBUG        # デバッグメッセージを出さないようにする時
#   USE_LOGALL		# define USE_LOGALL macro to enable all debug string
#   DISABLE_IMPORTGL  # when static link OpenGL|ES library
)

target_include_directories(objlib PUBLIC
    ./
    ../
    ../rapidjson/include/
)

add_library(common_static STATIC $<TARGET_OBJECTS:objlib>)

target_include_directories(common_static PUBLIC
    ./
    ../
    ../rapidjson/include/
)

target_link_libraries(common_static PRIVATE
    atomic
    ${OPENGL_LIB}
    ${HARDWARE_BUFFER_NDK_LIB}
    ${LIBGLEW_LIBRARIES}
    ${LIBGL_LIBRARIES}
)

target_link_libraries(common_static PUBLIC
)

add_library(common SHARED $<TARGET_OBJECTS:objlib>)

target_include_directories(common PUBLIC
    ./
)

target_link_libraries(common PRIVATE
    atomic
    ${OPENGL_LIB}
    ${HARDWARE_BUFFER_NDK_LIB}
    ${libs_platform}
)

target_link_libraries(common PUBLIC
)

if (AANDUSB_OUTPUT_VALS)
    message("==================== All variables (common)  ====================")
    get_cmake_property(_variableNames CACHE_VARIABLES)
    list (SORT _variableNames)
    foreach (_variableName ${_variableNames})
        message(STATUS "${_variableName}=${${_variableName}}")
    endforeach()
    message("==================== End of all variables (common)  ====================")
endif()
