/*
 * aAndUsb
 * Copyright (c) 2014-2023 saki t_saki@serenegiant.com
 * Distributed under the terms of the GNU Lesser General Public License (LGPL v3.0) License.
 * License details are in the file license.txt, distributed as part of this software.
 */

#ifndef AANDUSB_IWINDOW_H
#define AANDUSB_IWINDOW_H

// Open GL3を使うかどうか, 1: GL3を使う, 0: 使わない(GL2を使う)
#define USE_GL3 (0)

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
typedef std::function<KeyEvent(const int&/*key*/, const int&/*scancode*/, const int&/*action*/, const int&/*mods*/)> OnKeyEventFunc;

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
class IWindow {
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

	virtual bool poll_events() = 0;
	virtual void init_gl() = 0;
	virtual void terminate_gl() = 0;
	virtual void swap_buffers() = 0;
	virtual void init_gui() = 0;
	virtual void terminate_gui() = 0;
	void internal_resize(const int &width, const int &height);
public:
	IWindow(const int width = 640, const int height = 480, const char *title = "aAndUsb");
	virtual ~IWindow();

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
	 * @return IWindow&
	 */
	inline IWindow &on_key_event(OnKeyEventFunc listener) {
		on_key_event_func = listener;
		return *this;
	}
	inline IWindow &set_on_start(LifeCycletEventFunc callback) {
		on_start = callback;
		return *this;
	}
	inline IWindow &set_on_resume(LifeCycletEventFunc callback) {
		on_resume = callback;
		return *this;
	}
	inline IWindow &set_on_pause(LifeCycletEventFunc callback) {
		on_pause = callback;
		return *this;
	}
	inline IWindow &set_on_stop(LifeCycletEventFunc callback) {
		on_stop = callback;
		return *this;
	}
};

}	// end of namespace serenegiant::app

#endif //AANDUSB_IWINDOW_H
