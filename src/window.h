/*
 * aAndUsb
 * Copyright (c) 2014-2023 saki t_saki@serenegiant.com
 * Distributed under the terms of the GNU Lesser General Public License (LGPL v3.0) License.
 * License details are in the file license.txt, distributed as part of this software.
 */

#ifndef AANDUSB_WINDOW_H
#define AANDUSB_WINDOW_H

#include <cstdlib>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

namespace serenegiant {

/**
 * glfwによるwindow関係の処理用ヘルパークラス
 * あらかじめglfwInitを呼び出してglfwを初期化しておくこと
 */
class Window {
private:
	GLFWwindow *window;
	GLfloat aspect;
protected:
	static void resize(GLFWwindow *window, int width, int height);
public:
	static int initialize();

	Window(const int width = 640, const int height = 480, const char *title = "aAndUsb");
	virtual ~Window();

	explicit operator bool();
	void swap_buffers();
	inline bool is_valid() const { return window != nullptr; };
	inline GLfloat get_aspect() const { return aspect; };

};

}	// end of namespace serenegiant

#endif //AANDUSB_WINDOW_H
