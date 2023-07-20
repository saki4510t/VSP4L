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

#include "eglbase.h"

#include "internal.h"

// wayland
#include "wayland/wayland_display.h"
#include "wayland/wayland_window.h"

// app
#include "const.h"
#include "key_event.h"
#include "window.h"

namespace serenegiant::app {

/**
 * wayland+eglによるwindow関係の処理用ヘルパークラス
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
	wayland::WaylandDisplayUp display;

	void init_egl();
	void release_egl();
protected:
	void init_gl() override;
	void terminate_gl() override;
	bool poll_events() override;
	void swap_buffers() override;
	void init_gui() override;
	void terminate_gui() override;
public:
	static int initialize();

	EglWindow(const int &width = 640, const int &height = 480, const char *title = "aAndUsb");
	virtual ~EglWindow();

	bool is_valid() const override;

	/**
	 * アプリの終了要求をする
	 * エラー時などにEyeAppから呼び出す
	*/
	void terminate() override;
};

}	// end of namespace serenegiant::app

#endif //AANDUSB_WINDOW_H
