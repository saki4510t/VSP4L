/*
 * aAndUsb
 * Copyright (c) 2014-2023 saki t_saki@serenegiant.com
 * Distributed under the terms of the GNU Lesser General Public License (LGPL v3.0) License.
 * License details are in the file license.txt, distributed as part of this software.
 */

#ifndef AANDUSB_WINDOW_H
#define AANDUSB_WINDOW_H

#include <cstdlib>
#include <functional>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl2.h"

namespace serenegiant::app {

typedef std::function<int32_t(const int&/*key*/, const int&/*scancode*/, const int&/*action*/, const int&/*mods*/)> OnKeyEventFunc;

/**
 * glfwによるwindow関係の処理用ヘルパークラス
 * あらかじめglfwInitを呼び出してglfwを初期化しておくこと
 */
class Window {
private:
	GLFWwindow *window;
	GLfloat aspect;
	OnKeyEventFunc on_key_event_func;
	void init_gui();
	void terminate_gui();
protected:
	static void resize(GLFWwindow *win, int width, int height);
	static void key_callback(GLFWwindow *win, int key, int scancode, int action, int mods);
public:
	static int initialize();

	Window(const int width = 640, const int height = 480, const char *title = "aAndUsb");
	virtual ~Window();

	explicit operator bool();
	void swap_buffers();
	inline bool is_valid() const { return window != nullptr; };
	inline GLfloat get_aspect() const { return aspect; };

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
};

}	// end of namespace serenegiant::app

#endif //AANDUSB_WINDOW_H
