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
 * EglWindow::initialize
 * 	→コンストラクタ
 * 	　→start→on_start
 *	　　→on_resume→
 *	　　　→on_render→
 *	　　←on_pause←pause←
 *	  ←on_stop←stop←
 * 	　デストラクタ←	
 */
class EglWindow : public Window {
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

	EglWindow(const int width = 640, const int height = 480, const char *title = "aAndUsb");
	virtual ~EglWindow();

	bool is_valid() const override;
};

}	// end of namespace serenegiant::app

#endif //AANDUSB_WINDOW_H
