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
#include "glfw_window.h"

namespace serenegiant::app {

EglWindow::EglWindow(const int width, const int height, const char *title)
:	Window(width, height, title),
	window(glfwCreateWindow(width, height, title, nullptr/*monitor*/, nullptr/*share*/)),
	prev_key_callback(nullptr)
{
	ENTER();

	if (window) {
		// コールバック内からWindowインスタンスを参照できるようにポインタを渡しておく
		glfwSetWindowUserPointer(window, this);
		// ウインドウサイズが変更されたときのコールバックをセット
		glfwSetWindowSizeCallback(window, resize);
	}

	EXIT();
}

/*public*/
EglWindow::~EglWindow() {
	ENTER();

	if (window) {
		stop();
		glfwDestroyWindow(window);
		window = nullptr;
	}

	EXIT();
}

bool EglWindow::is_valid() const {
	return window != nullptr;
};

/*public*/
void EglWindow::swap_buffers() {
	if (window) {
		glfwMakeContextCurrent(window);
		glfwSwapBuffers(window);
	}
}

/*static, protected*/
void EglWindow::resize(GLFWwindow *win, int width, int height) {
    ENTER();

	// フレームバッファのサイズを調べる
	int fbWidth, fbHeight;
	glfwGetFramebufferSize(win, &fbWidth, &fbHeight);
	// フレームバッファ全体をビューポートに設定する
	glViewport(0, 0, fbWidth, fbHeight);

	// このインスタンスの this ポインタを得る
	auto self = reinterpret_cast<EglWindow *>(glfwGetWindowUserPointer(win));
	if (self) {
		// このインスタンスが保持する縦横比を更新する
		self->internal_resize(fbWidth, fbHeight);
	}

	EXIT();
}

/*static, protected*/
void EglWindow::key_callback(GLFWwindow *win, int key, int scancode, int action, int mods) {
	ENTER();

	// このインスタンスの this ポインタを得る
	auto self = reinterpret_cast<EglWindow *>(glfwGetWindowUserPointer(win));
	if (self && self->on_key_event_func) {
		// コールバックが設定されていればそれを呼び出す
		const auto event = self->on_key_event_func(key, scancode, action, mods);
		if ((event.key != GLFW_KEY_UNKNOWN) && self->prev_key_callback) {
			// ここでprev_key_callbackを呼び出せばimgui自体のキーコールバックが呼ばれる
			// キーが有効な場合のみprev_key_callbackを呼び出す
			self->prev_key_callback(self->window, event.key, event.scancode, event.action, event.mods);
		}
	}

	EXIT();
}

/*private*/
bool EglWindow::poll_events() {
	// イベントを確認
//	glfwWaitEvents(); // こっちはイベントが起こるまで実行をブロックする
	// glfwWaitEventsTimeout(0.010); // イベントが起こるかタイム・アウトするまで実行をブロック, glfw3.2以降
	glfwPollEvents(); // イベントをチェックするが実行をブロックしない
	// ウィンドウを閉じる必要がなければ true を返す
	return window && !glfwWindowShouldClose(window);
}

/*protected,@WorkerThread*/
void EglWindow::init_gl() {
	ENTER();

	if (window) {
		// 作成したウィンドウへOpenGLで描画できるようにする
		glfwMakeContextCurrent(window);
		// ダブルバッファの入れ替えタイミングを指定, 垂直同期のタイミングを待つ
		glfwSwapInterval(1);

		// 作成したウインドウを初期化
		resize(window, width(), height());
		// IMGUIでのGUI描画用に初期化する
		init_gui();

		// XXX init_gui(imguiの初期化後)にキーイベントコールバックをセットすることで
		//     imguiのキーコールバックを無効にする
		// キー入力イベントコールバックをセット
		prev_key_callback = glfwSetKeyCallback(window, key_callback);
		LOGD("prev_key_callback=%p", prev_key_callback);
	}

	EXIT();
}

/*protected,@WorkerThread*/
void EglWindow::terminate_gl() {
	ENTER();

	if (prev_key_callback) {
		// 念のためにもとに戻しておく
		glfwSetKeyCallback(window, prev_key_callback);
	}

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
    ImGui_ImplGlfw_InitForOpenGL(window, true);
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
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	EXIT();
}

}	// end of namespace serenegiant::app
