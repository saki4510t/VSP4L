// Vision Support Program for Linux ... EyeApp
// Copyright (C) 2023-2024 saki t_saki@serenegiant.com
// 
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

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
