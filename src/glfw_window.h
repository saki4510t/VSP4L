/*
 * aAndUsb
 * Copyright (c) 2014-2023 saki t_saki@serenegiant.com
 * Distributed under the terms of the GNU Lesser General Public License (LGPL v3.0) License.
 * License details are in the file license.txt, distributed as part of this software.
 */

#ifndef AANDUSB_WINDOW_H
#define AANDUSB_WINDOW_H

#include <cstdlib>

#include "internal.h"

#include "const.h"
#include "key_event.h"
#include "window.h"

namespace serenegiant::app {

/**
 * glfwによるwindow関係の処理用ヘルパークラス
 * あらかじめglfwInitを呼び出してglfwを初期化しておくこと
 * ライフサイクル：
 * GlfwWindow::initialize
 * 	→コンストラクタ
 * 	　→start→on_start
 *	　　→on_resume→
 *	　　　→on_render→
 *	　　←on_pause←pause←
 *	  ←on_stop←stop←
 * 	　デストラクタ←	
 */
class GlfwWindow : public Window {
private:
	GLFWwindow *window;
	GLFWkeyfun prev_key_callback;
protected:
	static void resize(GLFWwindow *win, int width, int height);
	static void key_callback(GLFWwindow *win, int key, int scancode, int action, int mods);

	void init_gl() override;
	void terminate_gl() override;
	bool poll_events() override;
	void swap_buffers() override;
	void init_gui() override;
	void terminate_gui() override;
public:
	static int initialize();

	GlfwWindow(const int width = 640, const int height = 480, const char *title = "aAndUsb");
	virtual ~GlfwWindow();

	bool is_valid() const override;

	inline GLFWwindow *get_window() { return window; };

	/**
	 * アプリの終了要求をする
	 * エラー時などにEyeAppから呼び出す
	*/
	void terminate() override;
};

}	// end of namespace serenegiant::app

#endif //AANDUSB_WINDOW_H
