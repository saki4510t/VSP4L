#ifndef INTERNAL_H_
#define INTERNAL_H_

#if !defined(ENABLE_GLES)
#include <GL/glew.h>
#define GLFW_INCLUDE_ES3
#endif
#include <GLFW/glfw3.h>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"		// フラグメントシェーダー等を使う場合はこっち
// #include "imgui_impl_opengl2.h"	// これは昔の固定パイプラインを使うときのバックエンド

#endif // #ifndef INTERNAL_H_
