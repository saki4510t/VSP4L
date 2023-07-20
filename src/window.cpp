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

Window::Window(const int width, const int height, const char *title)
:	running(false), resumed(false),
	renderer_thread(),
	aspect(640 / 480.0f), fb_width(width), fb_height(height),
	on_key_event_func(nullptr),
	on_start(nullptr), on_stop(nullptr), on_render(nullptr)
{
	ENTER();
	EXIT();
}

/*public*/
Window::~Window() {
	ENTER();
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

void Window::internal_resize(const int &width, const int &height) {
	ENTER();

	aspect = width / (float)height;
	fb_width = width;
	fb_height = height;

	EXIT();
}

}	// end of namespace serenegiant::app
