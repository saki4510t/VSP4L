/*
 * aAndUsb
 * Copyright (c) 2014-2023 saki t_saki@serenegiant.com
 * Distributed under the terms of the GNU Lesser General Public License (LGPL v3.0) License.
 * License details are in the file license.txt, distributed as part of this software.
 */

#if 1    // set 0 if you need debug log, otherwise set 1
	#ifndef LOG_NDEBUG
		#define LOG_NDEBUG
	#endif
	#undef USE_LOGALL
#else
	#define USE_LOGALL
	#define USE_LOGD
	#undef LOG_NDEBUG
	#undef NDEBUG
#endif

#include <chrono>
#include <signal.h>

#include "utilbase.h"
// common
#include "times.h"
// app
#include "egl_window.h"

namespace serenegiant::app {

//--------------------------------------------------------------------------------
int EglWindow::initialize() {
	ENTER();
	RETURN(0, int);
}

EglWindow::EglWindow(const int &width, const int &height, const char *title)
:	Window(width, height, title),
	display(nullptr)
{
	ENTER();

	EXIT();
}

/*public*/
EglWindow::~EglWindow() {
	ENTER();

	display.reset();

	EXIT();
}

bool EglWindow::is_valid() const {
	return display != nullptr;
};

/**
 * アプリの終了要求をする
 * エラー時などにEyeAppから呼び出す
*/
/*public*/
void GlfwWindow::terminate() {
    ENTER();

	// FIXME 未実装

    EXIT();
}

//--------------------------------------------------------------------------------
/*protected*/
void EglWindow::swap_buffers() {
	// ENTER();

	// FIXME 未実装

	// EXIT();
}

/*protected*/
bool EglWindow::poll_events() {
	// イベントを確認
	int result = 0;
	if (LIKELY(display)) {
		result = display->dispatch();
	}

	return true; // !result && display;
}

/*protected,@WorkerThread*/
void EglWindow::init_gl() {
	ENTER();

	display = std::make_unique<wayland::WaylandDisplay>();
	init_egl();
	display->create_window(width(), height());

	// IMGUIでのGUI描画用に初期化する
	init_gui();

	EXIT();
}

/*protected,@WorkerThread*/
void EglWindow::terminate_gl() {
	ENTER();

	terminate_gui();
	release_egl();
	display.reset();

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

//--------------------------------------------------------------------------------
/*private*/
void EglWindow::init_egl() {
	ENTER();

	static const EGLint context_attribs[] = {
		EGL_CONTEXT_CLIENT_VERSION, 2,
		EGL_NONE
	};

	EGLint config_attribs[] = {
		EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
		EGL_RED_SIZE, 1,
		EGL_GREEN_SIZE, 1,
		EGL_BLUE_SIZE, 1,
		EGL_ALPHA_SIZE, 1,
		EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
		EGL_NONE
	};

	EGLint major, minor, n;
	EGLBoolean ret;

	wayland::egl_t &egl = display->egl();
	egl.display = eglGetDisplay((EGLNativeDisplayType)display->display());
	assert(egl.display);

	ret = eglInitialize(egl.display, &major, &minor);
	assert(ret == EGL_TRUE);
	ret = eglBindAPI(EGL_OPENGL_ES_API);
	assert(ret == EGL_TRUE);

	ret = eglChooseConfig(egl.display,
		config_attribs, &egl.config, 1, &n);
	assert(ret && n == 1);

	egl.context = eglCreateContext(egl.display, egl.config,
		EGL_NO_CONTEXT, context_attribs);

	EXIT();
}

/*private*/
void EglWindow::release_egl() {
	ENTER();

	eglTerminate(display->egl().display);
	eglReleaseThread();

	EXIT();
}

}	// end of namespace serenegiant::app
