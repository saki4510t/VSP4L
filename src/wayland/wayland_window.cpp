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
	#define USE_LOGALL
	#define USE_LOGD
	#undef LOG_NDEBUG
	#undef NDEBUG
#endif

#include "utilbase.h"

// wayland
#include "wayland/wayland_window.h"

namespace serenegiant::wayland {

WaylandWindow::WaylandWindow(WaylandDisplay &display, const int &width, const int &height)
:   display(display),
	width(width), height(height),
	window_width(width), window_height(height),
	configured(false), fullscreen(false), opaque(false),
	m_egl_window(nullptr),
	m_surface(nullptr), m_shell_surface(nullptr),
	m_callback(nullptr),
	m_egl_surface(nullptr),
	m_on_draw(nullptr)
{
	ENTER();


	m_surface = wl_compositor_create_surface(display.m_compositor);
	if (display.m_shell) {
		static const struct wl_shell_surface_listener shell_surface_listener = {
			handle_ping,
			handle_configure,
			handle_popup_done
		};
		m_shell_surface = wl_shell_get_shell_surface(display.m_shell, m_surface);

		wl_shell_surface_add_listener(
			m_shell_surface, &shell_surface_listener, this);
	}

	m_egl_window = wl_egl_window_create(
		m_surface, window_width, window_height);
	m_egl_surface = eglCreateWindowSurface(
		display.egl().display, display.egl().config, (EGLNativeWindowType)m_egl_window, nullptr);

	if (m_shell_surface) {
		wl_shell_surface_set_title(m_shell_surface, "egl");
	}

	auto ret = eglMakeCurrent(
		display.egl().display, m_egl_surface, m_egl_surface, display.egl().context);
	if (ret != EGL_TRUE) {
		LOGE("eglMakeCurrent failed");
	}

	EXIT();
}

WaylandWindow::~WaylandWindow() {
	ENTER();

	eglMakeCurrent(
		display.egl().display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);

	if (m_egl_surface) {
		eglDestroySurface(display.egl().display, m_egl_surface);
		wl_egl_window_destroy(m_egl_window);
		m_egl_surface = nullptr;
	}

	if (m_shell_surface) {
		wl_shell_surface_destroy(m_shell_surface);
		m_shell_surface = nullptr;
	}
	if (m_surface) {
		wl_surface_destroy(m_surface);
		m_surface = nullptr;
	}

	if (m_callback) {
		wl_callback_destroy(m_callback);
		m_callback = nullptr;
	}

	EXIT();
}

void WaylandWindow::toggle_fullscreen() {
	ENTER();

	static struct wl_callback_listener configure_callback_listener = {
		configure_callback,
	};

	struct wl_callback *callback;

	fullscreen = !fullscreen;
	configured = false;

	if (m_shell_surface) {
		if (fullscreen) {
			wl_shell_surface_set_fullscreen(
				m_shell_surface,
				WL_SHELL_SURFACE_FULLSCREEN_METHOD_DEFAULT,
				0, nullptr);
		} else {
			wl_shell_surface_set_toplevel(m_shell_surface);
			handle_configure(this,
				m_shell_surface, 0,
				window_width, window_height);
		}
	}

	callback = wl_display_sync(display.m_display);
	wl_callback_add_listener(callback, &configure_callback_listener, this);

	EXIT();
}

//--------------------------------------------------------------------------------
/*private,static*/
void WaylandWindow::handle_ping(
	void *data,
	struct wl_shell_surface *shell_surface,
	uint32_t serial) {

	ENTER();

	wl_shell_surface_pong(shell_surface, serial);

	EXIT();
}

/*private,static*/
void WaylandWindow::handle_configure(
	void *data,
	struct wl_shell_surface *shell_surface,
	uint32_t edges,
	int32_t width,
	int32_t height) {

	ENTER();

	auto window = static_cast<WaylandWindow *>(data);

	if (window->m_egl_window) {
		wl_egl_window_resize(window->m_egl_window, width, height, 0, 0);
	}

	window->width = width;
	window->height = height;

	if (!window->fullscreen) {
		window->window_width = window->width;
		window->window_height = window->height;
	}

	EXIT();
}

/*private,static*/
void WaylandWindow::handle_popup_done(
	void *data,
	struct wl_shell_surface *shell_surface) {

	ENTER();
	EXIT();
}

//--------------------------------------------------------------------------------
/*private,static*/
void WaylandWindow::configure_callback(
	void *data,
	struct wl_callback *callback,
	uint32_t time) {

	ENTER();

	auto window = static_cast<WaylandWindow *>(data);

	wl_callback_destroy(callback);

	window->configured = 1;

	if (window->m_callback == nullptr) {
		redraw(data, nullptr, time);
	}

	EXIT();
}

//--------------------------------------------------------------------------------
/*private,static*/
void WaylandWindow::redraw(void *data, struct wl_callback *callback, unsigned int time)
{
	ENTER();

	const struct wl_callback_listener frame_listener = {
		WaylandWindow::redraw,
	};

	auto window = static_cast<WaylandWindow *>(data);
	struct wl_region *region;

	if (window->m_callback != callback) {
		LOGW("unexpected callback");
		EXIT();
	}

	window->m_callback = nullptr;

	if (callback) {
		wl_callback_destroy(callback);
	}

	if (!window->configured) {
		EXIT();
	}

	if (LIKELY(window->m_on_draw)) {
		window->m_on_draw();
	}

	if (window->opaque || window->fullscreen) {
		region = wl_compositor_create_region(window->display.m_compositor);
		wl_region_add(region,
			0, 0,
			window->width, window->height);
		wl_surface_set_opaque_region(window->m_surface, region);
		wl_region_destroy(region);
	} else {
		wl_surface_set_opaque_region(window->m_surface, NULL);
	}

	window->m_callback = wl_surface_frame(window->m_surface);
	wl_callback_add_listener(window->m_callback, &frame_listener, window);

	eglSwapBuffers(window->display.egl().display, window->m_egl_surface);

	EXIT();
}

}   // namespace serenegiant::wayland
