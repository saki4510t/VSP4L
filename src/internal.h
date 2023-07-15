#ifndef INTERNAL_H_
#define INTERNAL_H_

#include <memory>

// common
#include "glutils.h"

#define GLFW_INCLUDE_ES3
#define GLFW_EXPOSE_NATIVE_WAYLAND
#define GLFW_EXPOSE_NATIVE_EGL

#if defined(EYEAPP_ENABLE_GLFW)
    #include <GLFW/glfw3.h>
    #include <GLFW/glfw3native.h>
    #include "imgui_impl_glfw.h"
#else
    #include <wayland-client.h>
    #include <wayland-cursor.h>
    #include <wayland-egl.h>
    // FIXME waylandを自前でアクセスするimguiのバックエンドが必要
#endif

#include "imgui.h"
#include "imgui_impl_opengl3.h"		// フラグメントシェーダー等を使う場合はこっち
// #include "imgui_impl_opengl2.h"	// これは昔の固定パイプラインを使うときのバックエンド

#endif // #ifndef INTERNAL_H_
