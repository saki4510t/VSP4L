#ifndef WAYLAND_WINDOW_H
#define WAYLAND_WINDOW_H

#include <functional>

#include "internal.h"

// wayland
#include "wayland/wayland_display.h"

namespace serenegiant::wayland {

typedef std::function<void()> OnDrawFunc;

class WaylandWindow {
private:
	WaylandDisplay &display;
	int width;
	int height;
	int window_width;
	int window_height;
	bool configured;
	bool fullscreen;
	bool opaque;
	struct wl_egl_window *m_egl_window;
	struct wl_surface *m_surface;
	struct wl_shell_surface *m_shell_surface;
	struct wl_callback *m_callback;
	EGLSurface m_egl_surface;
	OnDrawFunc m_on_draw;

	static void handle_ping(
		void *data,
		struct wl_shell_surface *shell_surface,
		uint32_t serial);
	static void handle_configure(
		void *data,
		struct wl_shell_surface *shell_surface,
		uint32_t edges,
		int32_t width,
		int32_t height);
	static void handle_popup_done(
		void *data,
		struct wl_shell_surface *shell_surface);
	//--------------------------------------------------------------------------------
	static void configure_callback(
		void *data,
		struct wl_callback *callback,
		uint32_t time);
	//--------------------------------------------------------------------------------
	static void redraw(void *data, struct wl_callback *callback, unsigned int time);
protected:
public:
	WaylandWindow(WaylandDisplay &display, const int &width, const int &height);
	~WaylandWindow();

	bool is_fullscreen() const { return fullscreen; };
	struct wl_shell_surface *shell_surface() { return m_shell_surface; };

	void toggle_fullscreen();
	inline WaylandWindow &set_on_draw(OnDrawFunc callback) {
		m_on_draw = callback;
		return *this;
	}
};

typedef std::unique_ptr<WaylandWindow> WaylandWindowUp;
typedef std::shared_ptr<WaylandWindow> WaylandWindowSp;

}   // namespace serenegiant::wayland

#endif  // WAYLAND_WINDOW_H
