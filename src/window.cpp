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
#if USE_GL3
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
//	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);	// OpenGL 3.2
//	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
//	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);	// OpenGL3以降で前方互換プロファイルを使う
#else
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
#endif
	glfwWindowHint(GLFW_RESIZABLE, GL_TRUE);	// マウス等でリサイズ可能
//	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	// 終了時の処理登録
	atexit(glfwTerminate);

	RETURN(0, int);
}

Window::Window(const int width, const int height, const char *title)
:	window(glfwCreateWindow(width, height, title, nullptr/*monitor*/, nullptr/*share*/)),
	running(false), resumed(false),
	renderer_thread(),
	aspect(640 / 480.0f), fb_width(width), fb_height(height),
	prev_key_callback(nullptr),
	on_key_event_func(nullptr),
	on_start(nullptr), on_stop(nullptr), on_render(nullptr)
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
Window::~Window() {
	ENTER();

	if (window) {
		stop();
		glfwDestroyWindow(window);
		window = nullptr;
	}

	EXIT();
}

/**
 * @brief 描画スレッドを開始する
 *
 * @return int
 */
int Window::start(OnRenderFunc render_func) {
	ENTER();

	if (!running) {
		running = true;
		on_render = render_func;
    	renderer_thread = std::thread([this] { renderer_thread_func(); });
		resume();
	}

	RETURN(0, int);
}

/**
 * @brief 描画開始(startを呼んだときは自動的に呼ばれる)
 *
 * @return int
 */
int Window::resume() {
	ENTER();

	if (running && !resumed) {
 		std::lock_guard<std::mutex> lock(state_lock);
		resumed = true;
		state_cond.notify_one();
	}

	RETURN(0, int);
}

/**
 * @brief 描画待機(stopを呼んだときは自動的に呼ばれる)
 *
 * @return int
 */
int Window::pause() {
	ENTER();

	resumed = false;

	RETURN(0, int);
}

/**
 * @brief 描画スレッドを終了する
 *
 * @return int
 */
/*public*/
int Window::stop() {
	ENTER();

	if (running) {
		pause();
		running = false;
		state_cond.notify_one();
	}
    if (renderer_thread.joinable()
		&& (std::this_thread::get_id() != renderer_thread.get_id())) {
		// デッドロックしないように描画スレッド以外から呼び出されたときのみjoinを呼び出す
        renderer_thread.join();
    }

	RETURN(0, int);
}

/*public*/
Window::operator bool() {
	return poll_events();
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
		self->fb_width = fbWidth;
		self->fb_height = fbHeight;
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
bool Window::poll_events() {
	// イベントを確認
//	glfwWaitEvents(); // こっちはイベントが起こるまで実行をブロックする
	// glfwWaitEventsTimeout(0.010); // イベントが起こるかタイム・アウトするまで実行をブロック, glfw3.2以降
	glfwPollEvents(); // イベントをチェックするが実行をブロックしない
	// ウィンドウを閉じる必要がなければ true を返す
	return window && !glfwWindowShouldClose(window);
}

/**
 * @brief 描画スレッドの実行関数
 *
 */
/*private,@WorkerThread*/
void Window::renderer_thread_func() {
    ENTER();

	init_gl();

	if (on_start) {
		on_start();
	}

	for ( ; running ; ) {
		for ( ; running && !resumed ; ) {
			// リジュームしていないときは待機する
 			std::unique_lock<std::mutex> lock(state_lock);
			const auto result = state_cond.wait_for(lock, std::chrono::seconds(3));
			if ((result == std::cv_status::timeout)) {
				continue;
			}
		}

		if (LIKELY(running)) {
			if (on_resume) {
				on_resume();
			}

			// 描画ループ
			// メインスレッドでイベント処理しているのでここではpoll_eventsを呼び出さないように変更
			for ( ; running && resumed /*&& poll_events()*/; ) {
				const auto start = systemTime();
				on_render();
				// ダブルバッファーをスワップ
				swap_buffers();
				// フレームレート調整
				const auto t = (systemTime() - start) / 1000L;
				if (t < 12000) {
					// 60fpsだと16.6msだけど少し余裕をみて最大12ms待機する
					usleep(12000 - t);
				}
			}

			if (on_pause) {
				on_pause();
			}
		}
	}

	running = false;
	if (on_stop) {
		on_stop();
	}

	terminate_gl();

	LOGD("finished");

	EXIT();
}

/*private,@WorkerThread*/
void Window::init_gl() {
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

/*private,@WorkerThread*/
void Window::terminate_gl() {
	ENTER();

	if (prev_key_callback) {
		// 念のためにもとに戻しておく
		glfwSetKeyCallback(window, prev_key_callback);
	}

	terminate_gui();

	EXIT();
}

/*private,@WorkerThread*/
void Window::init_gui() {
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

/*private,@WorkerThread*/
void Window::terminate_gui() {
	ENTER();

	// Cleanup
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	EXIT();
}

}	// end of namespace serenegiant::app
