#ifndef INTERNAL_H_
#define INTERNAL_H_

#include <memory>

#define DEBUG_GL_CHECK			// GL関数のデバッグメッセージを表示する時
// common
#include "glutils.h"

#define GLFW_INCLUDE_ES3
#define GLFW_EXPOSE_NATIVE_EGL
#define GLFW_NATIVE_INCLUDE_NONE

#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#include "imgui_impl_glfw.h"

#include "imgui.h"
#include "imgui_impl_opengl3.h"		// フラグメントシェーダー等を使う場合はこっち
// #include "imgui_impl_opengl2.h"	// これは昔の固定パイプラインを使うときのバックエンド

#include "const.h"

#endif // #ifndef INTERNAL_H_
