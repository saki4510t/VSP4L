cmake_minimum_required(VERSION 3.8)

project(`BOVProj_EyeApp)

set(CMAKE_VERBOSE_MAKEFILE on)
set(lib_src_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(target EyeApp)

# キャッシュ変数
option(ENABLE_GLES "Use OpenGL|ES instead of OpenGL" ON)

# キャッシュ変数、glfwのドキュメント・テスト・サンプルのビルドを無効化
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
# キャッシュ変数、glfwにwaylandサポート込みでビルドする
set(GLFW_BUILD_WAYLAND ON CACHE BOOL "" FORCE)

# C/C++用設定
set(c_cpp_flags "${c_cpp_flags} -DHAVE_PTHREADS -fPIC")
set(c_cpp_flags "${c_cpp_flags} -O3 -fstrict-aliasing")
set(c_cpp_flags "${c_cpp_flags} -Wno-parentheses -Wno-switch -Wno-extern-c-compat")
set(c_cpp_flags "${c_cpp_flags} -Wno-empty-body -Wno-deprecated-register -Wno-multichar")
set(c_cpp_flags "${c_cpp_flags} -Wreturn-type")
set(c_cpp_flags "${c_cpp_flags} -Wno-write-strings")

# C用の設定
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-incompatible-pointer-types")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${c_cpp_flags}")
#デバッグ用に警告を出す設定
#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wreorder")

# C++用の設定
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${c_cpp_flags}")
# 例外を有効にする
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fexceptions")
# RTTI(実行時型情報)を有効にする
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -frtti")
#デバッグ用に警告を出す設定
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wreorder")

# png, libjpeg-turbo, libyuvはpkg-configを使ってインストールされているものを使う
# load the module to use pkg-config
find_package(PkgConfig)
if (ENABLE_GLES)
# search library with pkg-config and name its results "GLES"
pkg_check_modules(LIBGLES REQUIRED glesv2)
add_definitions("-DENABLE_GLES")
else()
# search library with pkg-config and name its results "GL"
pkg_check_modules(LIBGL REQUIRED gl)
# # search library with pkg-config and name its results "GLEW"
pkg_check_modules(LIBGLEW REQUIRED glew)
endif()

# search library with pkg-config and name its results "LIBPNG"
pkg_check_modules(LIBPNG REQUIRED libpng)
# search library with pkg-config and name its results "LIBJPEG_TURBO"
pkg_check_modules(LIBJPEG REQUIRED libjpeg)
pkg_check_modules(LIBJPEG_TURBO REQUIRED libturbojpeg)
# search library with pkg-config and name its results "LIBYUV"
pkg_check_modules(LIBYUV REQUIRED libyuv)
# search library with pkg-config and name its results "LIBUDEV"
pkg_check_modules(LIBUDEV REQUIRED libudev)

# サブモジュールとして取り込んだglfwのソースをビルドする
add_subdirectory(${lib_src_DIR}/glfw)

# サブモジュールとしてsrc下に取り込んだimguiをスタティックライブラリーとしてビルドする
add_library(imgui STATIC
    imgui/imgui.cpp
    imgui/imgui_demo.cpp
    imgui/imgui_draw.cpp
    imgui/imgui_tables.cpp
    imgui/imgui_widgets.cpp
    imgui/backends/imgui_impl_glfw.cpp
    imgui/backends/imgui_impl_opengl3.cpp

    # imgui/backends/imgui_impl_opengl2.cpp
)

target_compile_definitions(imgui PRIVATE
    AVOID_TABLES
    #ログ出力設定
    NDEBUG            # LOG_ALLを無効にする・assertを無効にする場合
    LOG_NDEBUG        # デバッグメッセージを出さないようにする時
#   USE_LOGALL		# define USE_LOGALL macro to enable all debug string
#   DISABLE_IMPORTGL  # when static link OpenGL|ES library
)

target_include_directories(imgui PUBLIC
    ./imgui/
    ./imgui/backends/
)

add_subdirectory(${lib_src_DIR}/aandusb)
add_subdirectory(${lib_src_DIR}/common)

add_executable(${target}
    const.cpp
    eye_app.cpp
    key_dispatcher.cpp
    main.cpp
    osd.cpp
    settings.cpp
    window.cpp
)

# ターゲットアプリのビルド設定
target_compile_definitions(${target} PRIVATE
    #ログ出力設定
    NDEBUG            # LOG_ALLを無効にする・assertを無効にする場合
    LOG_NDEBUG        # デバッグメッセージを出さないようにする時
#   USE_LOGALL		  # define USE_LOGALL macro to enable all debug string
#   DISABLE_IMPORTGL  # when static link OpenGL|ES library
)

target_include_directories(${target} PRIVATE
    ../
    ./aandusb
    ./glfw/include
    # ${LIBGLEW_INCLUDE_DIRS}
    ${LIBGL_INCLUDE_DIRS}
    ${LIBGLES_INCLUDE_DIRS}
)

target_link_libraries(${target} PRIVATE
    # ${LIBGLEW_LIBRARIES}
    ${LIBGL_LIBRARIES}
    ${LIBGLES_LIBRARIES}
    ${LIBUDEV_LIBRARIES}
    ${LIBPNG_LIBRARIES}
    ${LIBJPEG_TURBO_LIBRARIES}
    ${LIBYUV_LIBRARIES}
    pthread
    glfw
    imgui
    aandusb_core
    aandusb_pipeline
    common_static
)

message("==================== All variables (EyeApp)  ====================")
get_cmake_property(_variableNames CACHE_VARIABLES)
list (SORT _variableNames)
foreach (_variableName ${_variableNames})
    message(STATUS "${_variableName}=${${_variableName}}")
endforeach()
message("==================== End of all variables (EyeApp)  ====================")
