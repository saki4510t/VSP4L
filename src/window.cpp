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

#include "utilbase.h"

#include "window.h"

namespace serenegiant::app {

static void glfw_error_callback(int error, const char* description) {
    LOGE("GLFW Error %d: %s", error, description);
}

int Window::initialize() {
	ENTER();

	glfwSetErrorCallback(glfw_error_callback);
	// GLFW の初期化
	if (glfwInit() == GL_FALSE) {
		// 初期化に失敗した処理
		LOGE("Can't initialize GLFW");
		RETURN(1, int);
	}
	// OpenGL Version 3.2 Core Profile を選択する XXX ここで指定するとエラーはなさそうなのに映像が表示できなくなる
//	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);	// OpenGL 3.2
//	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
//	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);	// OpenGL3以降で前方互換プロファイルを使う
	glfwWindowHint(GLFW_RESIZABLE, GL_TRUE);	// マウス等でリサイズ可能
//	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	// 終了時の処理登録
	atexit(glfwTerminate);
	RETURN(0, int);
}

Window::Window(
	const int width, const int height,
	const char *title)
:	window(glfwCreateWindow(width, height, title, nullptr/*monitor*/, nullptr/*share*/)),
	aspect(640 / 480.0f),
	on_key_event_func(nullptr)
{
	ENTER();

	if (window) {
		// 作成したウィンドウへOpenGLで描画できるようにする
		glfwMakeContextCurrent(window);
		// GLEW を初期化する
		glewExperimental = GL_TRUE;
		if (glewInit() != GLEW_OK) {
			// GLEW の初期化に失敗した
			LOGE("Can't initialize GLEW");
			EXIT();
		}
	
		// ダブルバッファの入れ替えタイミングを指定, 垂直同期のタイミングを待つ
		glfwSwapInterval(1);

		// コールバック内からWindowインスタンスを参照できるようにポインタを渡しておく
		glfwSetWindowUserPointer(window, this);
		// ウインドウサイズが変更されたときのコールバックをセット
		glfwSetWindowSizeCallback(window, resize);
		// キー入力イベントコールバックをセット
		glfwSetKeyCallback(window, key_callback);
		// 作成したウインドウを初期化
		resize(window, width, height);
		// IMGUIでのGUI描画用に初期化する
		init_gui();
	}

	EXIT();
}

/*public*/
Window::~Window() {
	ENTER();

	if (window) {
		terminate_gui();
		glfwDestroyWindow(window);
		window = nullptr;
	}

	EXIT();
}

/*public*/
Window::operator bool() {
	// イベントを確認
//	glfwWaitEvents(); // こっちはイベントが起こるまで実行をブロックする
	// glfwWaitEventsTimeout(0.010); // イベントが起こるかタイム・アウトするまで実行をブロック, glfw3.2以降
	glfwPollEvents(); // イベントをチェックするが実行をブロックしない
	// ウィンドウを閉じる必要がなければ true を返す
	return window && !glfwWindowShouldClose(window);
}

/*public*/
void Window::swap_buffers() {
	if (window) {
		glfwMakeContextCurrent(window);
		glfwSwapBuffers(window);
	}
}

/*static, protected*/
void Window::resize(GLFWwindow *win, int width, int height) {
    ENTER();

	// フレームバッファのサイズを調べる
	int fbWidth, fbHeight;
	glfwGetFramebufferSize(win, &fbWidth, &fbHeight);
	// フレームバッファ全体をビューポートに設定する
	glViewport(0, 0, fbWidth, fbHeight);

	// このインスタンスの this ポインタを得る
	auto self = reinterpret_cast<Window *>(glfwGetWindowUserPointer(win));
	if (self) {
		// このインスタンスが保持する縦横比を更新する
		self->aspect = width / (float)height;
	}

	EXIT();
}

/*static, protected*/
void Window::key_callback(GLFWwindow *win, int key, int scancode, int action, int mods) {
	ENTER();
	
	// このインスタンスの this ポインタを得る
	auto self = reinterpret_cast<Window *>(glfwGetWindowUserPointer(win));
	if (self && self->on_key_event_func) {
		// コールバックが設定されていればそれを呼び出す
		self->on_key_event_func(key, scancode, action, mods);
	}

	EXIT();
}

/*private*/
void Window::init_gui() {
	ENTER();

// Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    // ImGuiIO& io = ImGui::GetIO(); (void)io;
    // io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    // io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsLight();

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL2_Init();

	EXIT();	
}

/*private*/
void Window::terminate_gui() {
	ENTER();

	// Cleanup
	ImGui_ImplOpenGL2_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	EXIT();
}

}	// end of namespace serenegiant::app
