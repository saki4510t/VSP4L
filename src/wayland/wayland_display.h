#ifndef WAYLAND_DISPLAY_H
#define WAYLAND_DISPLAY_H

#include "internal.h"

namespace serenegiant::wayland {

typedef struct _egl {
	EGLDisplay display;
	EGLContext context;
	EGLConfig config;
} egl_t;

class WaylandWindow;

class WaylandDisplay {
friend WaylandWindow;
private:
	bool running;
	struct wl_display *m_display;
	struct wl_registry *m_registry;
	struct wl_compositor *m_compositor;
    struct wl_subcompositor *m_subcompositor;
	struct wl_shell *m_shell;
	struct wl_seat *m_seat;
	struct wl_pointer *m_pointer;
	struct wl_keyboard *m_keyboard;
	struct wl_shm *m_shm;
	struct wl_cursor_theme *m_cursor_theme;
	struct wl_cursor *m_default_cursor;
	struct wl_surface *m_cursor_surface;
	std::unique_ptr<WaylandWindow> m_window;
	egl_t m_egl;

	static void registry_handle_global(
		void *data,
		struct wl_registry *registry,
		uint32_t name,
		const char *interface,
		uint32_t version);
	static void registry_handle_global_remove(
		void *data,
		struct wl_registry *registry,
		uint32_t name);
	//--------------------------------------------------------------------------------
	static void pointer_handle_leave(
		void *data,
		struct wl_pointer *pointer,
		uint32_t serial,
		struct wl_surface *surface);

	static void pointer_handle_motion(
		void *data,
		struct wl_pointer *pointer,
		uint32_t time,
		wl_fixed_t sx,
		wl_fixed_t sy);

	static void pointer_handle_button(
		void *data,
		struct wl_pointer *wl_pointer,
		uint32_t serial,
		uint32_t time,
		uint32_t button,
		uint32_t state);

	static void pointer_handle_axis(
		void *data,
		struct wl_pointer *wl_pointer,
		uint32_t time,
		uint32_t axis,
		wl_fixed_t value);

	static void pointer_handle_enter(
		void *data,
		struct wl_pointer *pointer,
		uint32_t serial,
		struct wl_surface *surface,
		wl_fixed_t sx,
		wl_fixed_t sy);

	//--------------------------------------------------------------------------------
	static void keyboard_handle_keymap(
		void *data,
		struct wl_keyboard *keyboard,
		uint32_t format,
		int fd,
		uint32_t size);

	static void keyboard_handle_enter(
		void *data,
		struct wl_keyboard *keyboard,
		uint32_t serial,
		struct wl_surface *surface,
		struct wl_array *keys);
	static void keyboard_handle_leave(
		void *data,
		struct wl_keyboard *keyboard,
		uint32_t serial,
		struct wl_surface *surface);
	static void keyboard_handle_key(
		void *data,
		struct wl_keyboard *keyboard,
		uint32_t serial,
		uint32_t time,
		uint32_t key,
		uint32_t state);
	static void keyboard_handle_modifiers(
		void *data,
		struct wl_keyboard *keyboard,
		uint32_t serial,
		uint32_t mods_depressed,
		uint32_t mods_latched,
		uint32_t mods_locked,
		uint32_t group);
	static void seat_handle_capabilities(
		void *data,
		struct wl_seat *seat,
		uint32_t caps);
protected:
public:
	WaylandDisplay();
	~WaylandDisplay();

	int create_window(const int &width, const int &height);
	int dispatch();

	inline WaylandWindow *get_window() { return m_window.get(); };
	inline struct wl_display *display() { return m_display; };

	inline egl_t &egl() { return m_egl; };
};

typedef std::unique_ptr<WaylandDisplay> WaylandDisplayUp;
typedef std::shared_ptr<WaylandDisplay> WaylandDisplaySp;

}   // namespace serenegiant::wayland

#endif  // WAYLAND_DISPLAY_H
