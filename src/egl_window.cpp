/*
 * aAndUsb
 * Copyright (c) 2014-2023 saki t_saki@serenegiant.com
 * Distributed under the terms of the GNU Lesser General Public License (LGPL v3.0) License.
 * License details are in the file license.txt, distributed as part of this software.
 */

#if 0    // set 0 if you need debug log, otherwise set 1
#ifndef LOG_NDEBUG
	#define LOG_NDEBUG
	#endif
	#undef USE_LOGALL
#else
//	#define USE_LOGALL
	#define USE_LOGD
	#undef LOG_NDEBUG
	#undef NDEBUG
#endif

#include <chrono>

#include "utilbase.h"
// common
#include "times.h"
// app
#include "egl_window.h"

namespace serenegiant::app {

int GlfwWindow::initialize() {
	ENTER();
	RETURN(0, int);
}

EglWindow::EglWindow(const int width, const int height, const char *title)
:	Window(width, height, title)
{
	ENTER();

	EXIT();
}

/*public*/
EglWindow::~EglWindow() {
	ENTER();

	EXIT();
}

bool EglWindow::is_valid() const {
	return true;
};

/*public*/
void EglWindow::swap_buffers() {
}

/*private*/
bool EglWindow::poll_events() {
	// イベントを確認
	return true;
}

/*protected,@WorkerThread*/
void EglWindow::init_gl() {
	ENTER();

	// IMGUIでのGUI描画用に初期化する
	init_gui();

	EXIT();
}

/*protected,@WorkerThread*/
void EglWindow::terminate_gl() {
	ENTER();

	terminate_gui();

	EXIT();
}

/*protected,@WorkerThread*/
void EglWindow::init_gui() {
	ENTER();

// Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
    io.IniFilename = nullptr;	// 設定ファイルへの読み書きを行わない

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsLight();

    // Setup Platform/Renderer backends
    // ImGui_ImplGlfw_InitForOpenGL(window, true);
#if USE_GL3
    ImGui_ImplOpenGL3_Init("#version 130");
#else
	ImGui_ImplOpenGL3_Init("#version 100");
#endif

	EXIT();
}

/*protected,@WorkerThread*/
void EglWindow::terminate_gui() {
	ENTER();

	// Cleanup
	ImGui_ImplOpenGL3_Shutdown();
	// ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	EXIT();
}

}	// end of namespace serenegiant::app
