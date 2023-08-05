/*
 * aAndUsb
 * Copyright (c) 2014-2023 saki t_saki@serenegiant.com
 * Distributed under the terms of the GNU Lesser General Public License (LGPL v3.0) License.
 * License details are in the file license.txt, distributed as part of this software.
 */

#ifndef AANDUSB_IWINDOW_H
#define AANDUSB_IWINDOW_H

#include <condition_variable>
#include <cstdlib>
#include <functional>
#include <mutex>
#include <thread>

#include "internal.h"

#include "const.h"
#include "key_event.h"

namespace serenegiant::app {

typedef std::function<void()> LifeCycletEventFunc;
typedef std::function<void()> OnRenderFunc;
// 複数キー同時押しをENTERに割り当てるなどキーイベントを上書きできるようにKeyEventを返す
typedef std::function<KeyEvent(const ImGuiKey&/*key*/, const int&/*scancode*/, const key_action_t&/*action*/, const int&/*mods*/)> OnKeyEventFunc;

/**
 * window関係の処理用ヘルパー基底クラス
 * ライフサイクル：
 * 	コンストラクタ
 * 	　→start→on_start
 *	　　→on_resume→
 *	　　　→on_render→
 *	　　←on_pause←pause←
 *	  ←on_stop←stop←
 * 	　デストラクタ←	
 */
class Window {
private:
	volatile bool running;
	volatile bool resumed;
	mutable std::mutex state_lock;
	std::condition_variable state_cond;
	std::thread renderer_thread;
	GLfloat aspect;
	int fb_width;
	int fb_height;
	LifeCycletEventFunc on_start;
	LifeCycletEventFunc on_resume;
	LifeCycletEventFunc on_pause;
	LifeCycletEventFunc on_stop;
	OnRenderFunc on_render;

	/**
	 * @brief 描画スレッドの実行関数
	 *
	 */
	void renderer_thread_func();
protected:
	OnKeyEventFunc on_key_event_func;

	inline bool set_running(const bool &b) {
		bool prev = running;
		running = b;
		return prev;
	};
	virtual bool poll_events() = 0;
	virtual bool should_close() const = 0;
	virtual void init_gl() = 0;
	virtual void terminate_gl() = 0;
	virtual void swap_buffers() = 0;
	virtual void init_gui() = 0;
	virtual void terminate_gui() = 0;
	void internal_resize(const int &width, const int &height);
public:
	Window(const int width = 640, const int height = 480, const char *title = "aAndUsb");
	virtual ~Window();

	/**
	 * @brief 描画スレッドを開始する
	 *
	 * @return int
	 */
	virtual int start(OnRenderFunc render_func);
	/**
	 * @brief 描画開始(startを呼んだときは自動的に呼ばれる)
	 * 
	 * @return int 
	 */
	virtual int resume();
	/**
	 * @brief 描画待機(stopを呼んだときは自動的に呼ばれる)
	 * 
	 * @return int 
	 */
	virtual int pause();
	/**
	 * @brief 描画スレッドを終了する
	 *
	 * @return int
	 */
	virtual int stop();

	/**
	 * アプリの終了要求をする
	 * エラー時などにEyeAppから呼び出す
	*/
	virtual void terminate() = 0;

	explicit operator bool();
	virtual bool is_valid() const = 0;
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

#endif //AANDUSB_IWINDOW_H
