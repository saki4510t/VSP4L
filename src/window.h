/*
 * aAndUsb
 * Copyright (c) 2014-2023 saki t_saki@serenegiant.com
 * Distributed under the terms of the GNU Lesser General Public License (LGPL v3.0) License.
 * License details are in the file license.txt, distributed as part of this software.
 */

#ifndef AANDUSB_WINDOW_H
#define AANDUSB_WINDOW_H

// Open GL3を使うかどうか, 1: GL3を使う, 0: 使わない(GL2を使う)
#define USE_GL3 (0)

#include <condition_variable>
#include <cstdlib>
#include <functional>
#include <mutex>
#include <thread>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"		// フラグメントシェーダー等を使う場合はこっち
// #include "imgui_impl_opengl2.h"	// これは昔の固定パイプラインを使うときのバックエンド

#include "const.h"
#include "key_event.h"

namespace serenegiant::app {

typedef std::function<void(GLFWwindow *win)> LifeCycletEventFunc;
typedef std::function<void(GLFWwindow *win)> OnRenderFunc;
// 複数キー同時押しをENTERに割り当てるなどキーイベントを上書きできるようにKeyEventを返す
typedef std::function<KeyEvent(const int&/*key*/, const int&/*scancode*/, const int&/*action*/, const int&/*mods*/)> OnKeyEventFunc;

/**
 * glfwによるwindow関係の処理用ヘルパークラス
 * あらかじめglfwInitを呼び出してglfwを初期化しておくこと
 * ライフサイクル：
 * Window::initialize
 * 	→コンストラクタ
 * 	　→start→on_start
 *	　　→on_resume→
 *	　　　→on_render→
 *	　　←on_pause←pause←
 *	  ←on_stop←stop←
 * 	　デストラクタ←	
 */
class Window {
private:
	GLFWwindow *window;
	volatile bool running;
	volatile bool resumed;
	mutable std::mutex state_lock;
	std::condition_variable state_cond;
	std::thread renderer_thread;
	GLfloat aspect;
	int fb_width;
	int fb_height;
	GLFWkeyfun prev_key_callback;
	OnKeyEventFunc on_key_event_func;
	LifeCycletEventFunc on_start;
	LifeCycletEventFunc on_resume;
	LifeCycletEventFunc on_pause;
	LifeCycletEventFunc on_stop;
	OnRenderFunc on_render;

	void init_gl();
	void terminate_gl();
	void init_gui();
	void terminate_gui();
	bool poll_events();
	/**
	 * @brief 描画スレッドの実行関数
	 *
	 */
	void renderer_thread_func();
protected:
	static void resize(GLFWwindow *win, int width, int height);
	static void key_callback(GLFWwindow *win, int key, int scancode, int action, int mods);
	void swap_buffers();
public:
	static int initialize();

	Window(const int width = 640, const int height = 480, const char *title = "aAndUsb");
	virtual ~Window();

	/**
	 * @brief 描画スレッドを開始する
	 *
	 * @return int
	 */
	int start(OnRenderFunc render_func);
	/**
	 * @brief 描画開始(startを呼んだときは自動的に呼ばれる)
	 * 
	 * @return int 
	 */
	int resume();
	/**
	 * @brief 描画待機(stopを呼んだときは自動的に呼ばれる)
	 * 
	 * @return int 
	 */
	int pause();
	/**
	 * @brief 描画スレッドを終了する
	 *
	 * @return int
	 */
	int stop();

	explicit operator bool();
	inline bool is_valid() const { return window != nullptr; };
	inline bool is_running() const { return running; };
	inline bool is_resumed() const { return resumed; };
	inline GLfloat get_aspect() const { return aspect; };
	/**
	 * @brief フレームバッファの幅を取得
	 *
	 * @return int
	 */
	inline int width() const { return fb_width; };
	/**
	 * @brief フレームバッファの高さを取得
	 *
	 * @return int
	 */
	inline int height() const { return fb_height; };

	/**
	 * @brief キーイベント発生時のハンドラーを登録
	 *
	 * @param listener
	 * @return Window&
	 */
	inline Window &on_key_event(OnKeyEventFunc listener) {
		on_key_event_func = listener;
		return *this;
	}
	inline Window &set_on_start(LifeCycletEventFunc callback) {
		on_start = callback;
		return *this;
	}
	inline Window &set_on_resume(LifeCycletEventFunc callback) {
		on_resume = callback;
		return *this;
	}
	inline Window &set_on_pause(LifeCycletEventFunc callback) {
		on_pause = callback;
		return *this;
	}
	inline Window &set_on_stop(LifeCycletEventFunc callback) {
		on_stop = callback;
		return *this;
	}
};

}	// end of namespace serenegiant::app

#endif //AANDUSB_WINDOW_H
