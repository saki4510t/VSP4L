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

#include <linux/input.h>

#include "utilbase.h"

// wayland
#include "wayland/wayland_display.h"
#include "wayland/wayland_window.h"

namespace serenegiant::wayland {

WaylandDisplay::WaylandDisplay()
:   running(false),
	m_display(wl_display_connect(nullptr)), m_registry(nullptr),
	m_compositor(nullptr), m_subcompositor(nullptr),
	m_shell(nullptr), m_seat(nullptr), m_pointer(nullptr),
	m_keyboard(nullptr), m_shm(nullptr),
	m_cursor_theme(nullptr), m_default_cursor(nullptr), m_cursor_surface(nullptr),
	m_window(nullptr)
{
	ENTER();

	if (m_display) {
		static const struct wl_registry_listener registry_listener = {
			WaylandDisplay::registry_handle_global,
			WaylandDisplay::registry_handle_global_remove
		};

		LOGV("add registry listener");
		m_registry = wl_display_get_registry(m_display);
		wl_registry_add_listener(m_registry, &registry_listener, this);
		wl_display_dispatch(m_display);
	} else {
		LOGE("failed to connect to display!");
	}

	EXIT();
}

WaylandDisplay::~WaylandDisplay() {
	ENTER();

	m_window.reset();
	if (m_cursor_surface) {
		wl_surface_destroy(m_cursor_surface);
		m_cursor_surface = nullptr;
	}
	if (m_cursor_theme) {
		wl_cursor_theme_destroy(m_cursor_theme);
		m_cursor_theme = nullptr;
	}
	if (m_shell) {
		wl_shell_destroy(m_shell);
		m_shell = nullptr;
	}
	if (m_compositor) {
		wl_compositor_destroy(m_compositor);
		m_compositor = nullptr;
	}
	if (m_registry) {
		wl_registry_destroy(m_registry);
		m_registry = nullptr;
	}
	if (m_display) {
		wl_display_flush(m_display);
		wl_display_disconnect(m_display);
		m_display = nullptr;
	}

	EXIT();
}

int WaylandDisplay::create_window(const int &width, const int &height) {
	ENTER();

	int result = -1;

	if (m_display && m_compositor) {
		m_cursor_surface = wl_compositor_create_surface(m_compositor);
		// windowを生成
		m_window = std::make_unique<WaylandWindow>(*this, width, height);
		result = 0;
	}

	RETURN(result, int);
}

int WaylandDisplay::dispatch() {
	// ENTER();

	int result = -1;
	if (LIKELY(m_display)) {
		// result = wl_display_dispatch(m_display);    // これはキューが空の場合カレントスレッドをブロックする
		result = wl_display_dispatch_pending(m_display);
	}

	return result; // RETURN(result, int);
}
//--------------------------------------------------------------------------------
/*private, static*/
void WaylandDisplay::registry_handle_global_remove(
	void* data,
	struct wl_registry* registry,
	uint32_t name) {

	ENTER();
	EXIT();
}

/*private, static*/
void WaylandDisplay::registry_handle_global(
	void *data,
	struct wl_registry *registry,
	uint32_t name,
	const char *interface,
	uint32_t version) {

	// ENTER();

	static const struct wl_seat_listener seat_listener = {
		WaylandDisplay::seat_handle_capabilities,
	};

	auto d = static_cast<WaylandDisplay *>(data);

	if (strcmp(interface, "wl_compositor") == 0) {
		d->m_compositor = static_cast<wl_compositor *>(wl_registry_bind(registry, name, &wl_compositor_interface, 1));
    } else if (strcmp(interface, "wl_subcompositor") == 0) {
        d->m_subcompositor = static_cast<wl_subcompositor *>wl_registry_bind(registry, name, &wl_subcompositor_interface, 1);
	} else if (strcmp(interface, "wl_shell") == 0) {
		d->m_shell = static_cast<wl_shell *>(wl_registry_bind(registry, name, &wl_shell_interface, 1));
	} else if (strcmp(interface, "wl_seat") == 0) {
		d->m_seat = static_cast<wl_seat *>(wl_registry_bind(registry, name, &wl_seat_interface, 1));
		wl_seat_add_listener(d->m_seat, &seat_listener, d);
	} else if (strcmp(interface, "wl_shm") == 0) {
		d->m_shm = static_cast<wl_shm *>(wl_registry_bind(registry, name, &wl_shm_interface, 1));
		d->m_cursor_theme = wl_cursor_theme_load(nullptr, 32, d->m_shm);
		d->m_default_cursor = wl_cursor_theme_get_cursor(d->m_cursor_theme, "left_ptr");
	} else {
		LOGD("interface=%s", interface);
	}

	// EXIT();
}

//--------------------------------------------------------------------------------
/*private, static*/
void WaylandDisplay::pointer_handle_leave(
	void *data,
	struct wl_pointer *pointer,
	uint32_t serial,
	struct wl_surface *surface) {

	ENTER();
	EXIT();
}

/*private, static*/
void WaylandDisplay::pointer_handle_motion(
	void *data,
	struct wl_pointer *pointer,
	uint32_t time,
	wl_fixed_t sx,
	wl_fixed_t sy) {

	ENTER();
	EXIT();
}

/*private, static*/
void WaylandDisplay::pointer_handle_button(
	void *data,
	struct wl_pointer *wl_pointer,
	uint32_t serial,
	uint32_t time,
	uint32_t button,
	uint32_t state)
{
	ENTER();

	auto display = static_cast<WaylandDisplay *>(data);
	auto window = display->get_window();

	if ((button == BTN_LEFT) && (state == WL_POINTER_BUTTON_STATE_PRESSED)) {
		wl_shell_surface_move(
			window->shell_surface(), display->m_seat, serial);
	}

	EXIT();
}

/*private, static*/
void WaylandDisplay::pointer_handle_axis(
	void *data,
	struct wl_pointer *wl_pointer,
	uint32_t time,
	uint32_t axis,
	wl_fixed_t value) {

	ENTER();
	EXIT();
}

/*private, static*/
void WaylandDisplay::pointer_handle_enter(
	void *data,
	struct wl_pointer *pointer,
	uint32_t serial,
	struct wl_surface *surface,
	wl_fixed_t sx,
	wl_fixed_t sy) {

	ENTER();

	auto display = static_cast<WaylandDisplay *>(data);
	struct wl_buffer *buffer;
	struct wl_cursor *cursor = display->m_default_cursor;
	struct wl_cursor_image *image;

	auto window = display->get_window();

	if (window->is_fullscreen()) {
		wl_pointer_set_cursor(pointer, serial, nullptr, 0, 0);
	} else if (cursor) {
		image = display->m_default_cursor->images[0];
		buffer = wl_cursor_image_get_buffer(image);
		wl_pointer_set_cursor(pointer, serial, display->m_cursor_surface,
							  image->hotspot_x, image->hotspot_y);
		wl_surface_attach(display->m_cursor_surface, buffer, 0, 0);
		wl_surface_damage(display->m_cursor_surface, 0, 0, image->width,
						  image->height);
		wl_surface_commit(display->m_cursor_surface);
	}
}

//--------------------------------------------------------------------------------
/*private, static*/
void WaylandDisplay::keyboard_handle_keymap(
	void *data,
	struct wl_keyboard *keyboard,
	uint32_t format,
	int fd,
	uint32_t size) {

	ENTER();
	EXIT();
}

/*private, static*/
void WaylandDisplay::keyboard_handle_enter(
	void *data,
	struct wl_keyboard *keyboard,
	uint32_t serial,
	struct wl_surface *surface,
	struct wl_array *keys) {

	ENTER();
	EXIT();
}

/*private, static*/
void WaylandDisplay::keyboard_handle_leave(
	void *data,
	struct wl_keyboard *keyboard,
	uint32_t serial,
	struct wl_surface *surface) {

}

/*private, static*/
void WaylandDisplay::keyboard_handle_key(
	void *data,
	struct wl_keyboard *keyboard,
	uint32_t serial,
	uint32_t time,
	uint32_t key,
	uint32_t state)
{
	auto display = static_cast<WaylandDisplay *>(data);
	auto window = display->get_window();

	if (key == KEY_F11 && state) {
		window->toggle_fullscreen();
	} else if (key == KEY_ESC && state) {
		display->running = false;
	}
}

/*private, static*/
void WaylandDisplay::keyboard_handle_modifiers(
	void *data,
	struct wl_keyboard *keyboard,
	uint32_t serial,
	uint32_t mods_depressed,
	uint32_t mods_latched,
	uint32_t mods_locked,
	uint32_t group) {

	ENTER();
	EXIT();
}

/*private, static*/
void WaylandDisplay::seat_handle_capabilities(
	void *data,
	struct wl_seat *seat,
	uint32_t caps) {

	ENTER();

	static const struct wl_pointer_listener pointer_listener = {
		WaylandDisplay::pointer_handle_enter,
		WaylandDisplay::pointer_handle_leave,
		WaylandDisplay::pointer_handle_motion,
		WaylandDisplay::pointer_handle_button,
		WaylandDisplay::pointer_handle_axis,
	};
	static const struct wl_keyboard_listener keyboard_listener = {
		WaylandDisplay::keyboard_handle_keymap,
		WaylandDisplay::keyboard_handle_enter,
		WaylandDisplay::keyboard_handle_leave,
		WaylandDisplay::keyboard_handle_key,
		WaylandDisplay::keyboard_handle_modifiers,
	};

	auto d = static_cast<WaylandDisplay *>(data);

	if ((caps & WL_SEAT_CAPABILITY_POINTER) && !d->m_pointer) {
		LOGD("WL_SEAT_CAPABILITY_POINTER");
		d->m_pointer = wl_seat_get_pointer(seat);
		wl_pointer_add_listener(d->m_pointer, &pointer_listener, d);
	} else if (!(caps & WL_SEAT_CAPABILITY_POINTER) && d->m_pointer) {
		wl_pointer_destroy(d->m_pointer);
		d->m_pointer = nullptr;
	}

	if ((caps & WL_SEAT_CAPABILITY_KEYBOARD) && !d->m_keyboard) {
		LOGD("WL_SEAT_CAPABILITY_KEYBOARD");
		d->m_keyboard = wl_seat_get_keyboard(seat);
		wl_keyboard_add_listener(d->m_keyboard, &keyboard_listener, d);
	} else if (!(caps & WL_SEAT_CAPABILITY_KEYBOARD) && d->m_keyboard) {
		wl_keyboard_destroy(d->m_keyboard);
		d->m_keyboard = nullptr;
	}

	EXIT();
}

}   // namespace serenegiant::wayland
