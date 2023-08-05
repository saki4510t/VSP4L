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
#include "glfw_window.h"

namespace serenegiant::app {

//--------------------------------------------------------------------------------
// ここのコードはimgui_impl_glfw.cppからコピー
// ImGui_ImplGlfw_KeyCallbackでの処理のうち
// ・キーイベントをチェーンする処理
// ・修飾キーの処理
// 以外をOSD::on_keyで行うことができるようにしている

static int ImGui_ImplGlfw_TranslateUntranslatedKey(int key, int scancode)
{
#if GLFW_HAS_GETKEYNAME && !defined(__EMSCRIPTEN__)
    // GLFW 3.1+ attempts to "untranslate" keys, which goes the opposite of what every other framework does, making using lettered shortcuts difficult.
    // (It had reasons to do so: namely GLFW is/was more likely to be used for WASD-type game controls rather than lettered shortcuts, but IHMO the 3.1 change could have been done differently)
    // See https://github.com/glfw/glfw/issues/1502 for details.
    // Adding a workaround to undo this (so our keys are translated->untranslated->translated, likely a lossy process).
    // This won't cover edge cases but this is at least going to cover common cases.
    if (key >= GLFW_KEY_KP_0 && key <= GLFW_KEY_KP_EQUAL)
        return key;
    GLFWerrorfun prev_error_callback = glfwSetErrorCallback(nullptr);
    const char* key_name = glfwGetKeyName(key, scancode);
    glfwSetErrorCallback(prev_error_callback);
#if GLFW_HAS_GETERROR && !defined(__EMSCRIPTEN__) // Eat errors (see #5908)
    (void)glfwGetError(nullptr);
#endif
    if (key_name && key_name[0] != 0 && key_name[1] == 0)
    {
        const char char_names[] = "`-=[]\\,;\'./";
        const int char_keys[] = { GLFW_KEY_GRAVE_ACCENT, GLFW_KEY_MINUS, GLFW_KEY_EQUAL, GLFW_KEY_LEFT_BRACKET, GLFW_KEY_RIGHT_BRACKET, GLFW_KEY_BACKSLASH, GLFW_KEY_COMMA, GLFW_KEY_SEMICOLON, GLFW_KEY_APOSTROPHE, GLFW_KEY_PERIOD, GLFW_KEY_SLASH, 0 };
        IM_ASSERT(IM_ARRAYSIZE(char_names) == IM_ARRAYSIZE(char_keys));
        if (key_name[0] >= '0' && key_name[0] <= '9')               { key = GLFW_KEY_0 + (key_name[0] - '0'); }
        else if (key_name[0] >= 'A' && key_name[0] <= 'Z')          { key = GLFW_KEY_A + (key_name[0] - 'A'); }
        else if (key_name[0] >= 'a' && key_name[0] <= 'z')          { key = GLFW_KEY_A + (key_name[0] - 'a'); }
        else if (const char* p = strchr(char_names, key_name[0]))   { key = char_keys[p - char_names]; }
    }
    // if (action == GLFW_PRESS) printf("key %d scancode %d name '%s'\n", key, scancode, key_name);
#else
    IM_UNUSED(scancode);
#endif
    return key;
}

static ImGuiKey ImGui_ImplGlfw_KeyToImGuiKey(const int &key) {
    switch (key)
    {
        case GLFW_KEY_TAB: return ImGuiKey_Tab;
        case GLFW_KEY_LEFT: return ImGuiKey_LeftArrow;
        case GLFW_KEY_RIGHT: return ImGuiKey_RightArrow;
        case GLFW_KEY_UP: return ImGuiKey_UpArrow;
        case GLFW_KEY_DOWN: return ImGuiKey_DownArrow;
        case GLFW_KEY_PAGE_UP: return ImGuiKey_PageUp;
        case GLFW_KEY_PAGE_DOWN: return ImGuiKey_PageDown;
        case GLFW_KEY_HOME: return ImGuiKey_Home;
        case GLFW_KEY_END: return ImGuiKey_End;
        case GLFW_KEY_INSERT: return ImGuiKey_Insert;
        case GLFW_KEY_DELETE: return ImGuiKey_Delete;
        case GLFW_KEY_BACKSPACE: return ImGuiKey_Backspace;
        case GLFW_KEY_SPACE: return ImGuiKey_Space;
        case GLFW_KEY_ENTER: return ImGuiKey_Enter;
        case GLFW_KEY_ESCAPE: return ImGuiKey_Escape;
        case GLFW_KEY_APOSTROPHE: return ImGuiKey_Apostrophe;
        case GLFW_KEY_COMMA: return ImGuiKey_Comma;
        case GLFW_KEY_MINUS: return ImGuiKey_Minus;
        case GLFW_KEY_PERIOD: return ImGuiKey_Period;
        case GLFW_KEY_SLASH: return ImGuiKey_Slash;
        case GLFW_KEY_SEMICOLON: return ImGuiKey_Semicolon;
        case GLFW_KEY_EQUAL: return ImGuiKey_Equal;
        case GLFW_KEY_LEFT_BRACKET: return ImGuiKey_LeftBracket;
        case GLFW_KEY_BACKSLASH: return ImGuiKey_Backslash;
        case GLFW_KEY_RIGHT_BRACKET: return ImGuiKey_RightBracket;
        case GLFW_KEY_GRAVE_ACCENT: return ImGuiKey_GraveAccent;
        case GLFW_KEY_CAPS_LOCK: return ImGuiKey_CapsLock;
        case GLFW_KEY_SCROLL_LOCK: return ImGuiKey_ScrollLock;
        case GLFW_KEY_NUM_LOCK: return ImGuiKey_NumLock;
        case GLFW_KEY_PRINT_SCREEN: return ImGuiKey_PrintScreen;
        case GLFW_KEY_PAUSE: return ImGuiKey_Pause;
        case GLFW_KEY_KP_0: return ImGuiKey_Keypad0;
        case GLFW_KEY_KP_1: return ImGuiKey_Keypad1;
        case GLFW_KEY_KP_2: return ImGuiKey_Keypad2;
        case GLFW_KEY_KP_3: return ImGuiKey_Keypad3;
        case GLFW_KEY_KP_4: return ImGuiKey_Keypad4;
        case GLFW_KEY_KP_5: return ImGuiKey_Keypad5;
        case GLFW_KEY_KP_6: return ImGuiKey_Keypad6;
        case GLFW_KEY_KP_7: return ImGuiKey_Keypad7;
        case GLFW_KEY_KP_8: return ImGuiKey_Keypad8;
        case GLFW_KEY_KP_9: return ImGuiKey_Keypad9;
        case GLFW_KEY_KP_DECIMAL: return ImGuiKey_KeypadDecimal;
        case GLFW_KEY_KP_DIVIDE: return ImGuiKey_KeypadDivide;
        case GLFW_KEY_KP_MULTIPLY: return ImGuiKey_KeypadMultiply;
        case GLFW_KEY_KP_SUBTRACT: return ImGuiKey_KeypadSubtract;
        case GLFW_KEY_KP_ADD: return ImGuiKey_KeypadAdd;
        case GLFW_KEY_KP_ENTER: return ImGuiKey_KeypadEnter;
        case GLFW_KEY_KP_EQUAL: return ImGuiKey_KeypadEqual;
        case GLFW_KEY_LEFT_SHIFT: return ImGuiKey_LeftShift;
        case GLFW_KEY_LEFT_CONTROL: return ImGuiKey_LeftCtrl;
        case GLFW_KEY_LEFT_ALT: return ImGuiKey_LeftAlt;
        case GLFW_KEY_LEFT_SUPER: return ImGuiKey_LeftSuper;
        case GLFW_KEY_RIGHT_SHIFT: return ImGuiKey_RightShift;
        case GLFW_KEY_RIGHT_CONTROL: return ImGuiKey_RightCtrl;
        case GLFW_KEY_RIGHT_ALT: return ImGuiKey_RightAlt;
        case GLFW_KEY_RIGHT_SUPER: return ImGuiKey_RightSuper;
        case GLFW_KEY_MENU: return ImGuiKey_Menu;
        case GLFW_KEY_0: return ImGuiKey_0;
        case GLFW_KEY_1: return ImGuiKey_1;
        case GLFW_KEY_2: return ImGuiKey_2;
        case GLFW_KEY_3: return ImGuiKey_3;
        case GLFW_KEY_4: return ImGuiKey_4;
        case GLFW_KEY_5: return ImGuiKey_5;
        case GLFW_KEY_6: return ImGuiKey_6;
        case GLFW_KEY_7: return ImGuiKey_7;
        case GLFW_KEY_8: return ImGuiKey_8;
        case GLFW_KEY_9: return ImGuiKey_9;
        case GLFW_KEY_A: return ImGuiKey_A;
        case GLFW_KEY_B: return ImGuiKey_B;
        case GLFW_KEY_C: return ImGuiKey_C;
        case GLFW_KEY_D: return ImGuiKey_D;
        case GLFW_KEY_E: return ImGuiKey_E;
        case GLFW_KEY_F: return ImGuiKey_F;
        case GLFW_KEY_G: return ImGuiKey_G;
        case GLFW_KEY_H: return ImGuiKey_H;
        case GLFW_KEY_I: return ImGuiKey_I;
        case GLFW_KEY_J: return ImGuiKey_J;
        case GLFW_KEY_K: return ImGuiKey_K;
        case GLFW_KEY_L: return ImGuiKey_L;
        case GLFW_KEY_M: return ImGuiKey_M;
        case GLFW_KEY_N: return ImGuiKey_N;
        case GLFW_KEY_O: return ImGuiKey_O;
        case GLFW_KEY_P: return ImGuiKey_P;
        case GLFW_KEY_Q: return ImGuiKey_Q;
        case GLFW_KEY_R: return ImGuiKey_R;
        case GLFW_KEY_S: return ImGuiKey_S;
        case GLFW_KEY_T: return ImGuiKey_T;
        case GLFW_KEY_U: return ImGuiKey_U;
        case GLFW_KEY_V: return ImGuiKey_V;
        case GLFW_KEY_W: return ImGuiKey_W;
        case GLFW_KEY_X: return ImGuiKey_X;
        case GLFW_KEY_Y: return ImGuiKey_Y;
        case GLFW_KEY_Z: return ImGuiKey_Z;
        case GLFW_KEY_F1: return ImGuiKey_F1;
        case GLFW_KEY_F2: return ImGuiKey_F2;
        case GLFW_KEY_F3: return ImGuiKey_F3;
        case GLFW_KEY_F4: return ImGuiKey_F4;
        case GLFW_KEY_F5: return ImGuiKey_F5;
        case GLFW_KEY_F6: return ImGuiKey_F6;
        case GLFW_KEY_F7: return ImGuiKey_F7;
        case GLFW_KEY_F8: return ImGuiKey_F8;
        case GLFW_KEY_F9: return ImGuiKey_F9;
        case GLFW_KEY_F10: return ImGuiKey_F10;
        case GLFW_KEY_F11: return ImGuiKey_F11;
        case GLFW_KEY_F12: return ImGuiKey_F12;
        default: return ImGuiKey_None;
    }
}

static void glfw_error_callback(int error, const char* description) {
    LOGE("GLFW Error %d: %s", error, description);
}

int GlfwWindow::initialize() {
	ENTER();

	glfwSetErrorCallback(glfw_error_callback);
    if (glfwPlatformSupported(GLFW_PLATFORM_WAYLAND)) {
        LOGD("GLFW_PLATFORM_WAYLAND");
        glfwInitHint(GLFW_PLATFORM, GLFW_PLATFORM_WAYLAND);
    }
	// GLFW の初期化
	if (glfwInit() == GL_FALSE) {
		// 初期化に失敗した処理
		LOGE("Can't initialize GLFW");
		RETURN(1, int);
	}
	// OpenGL Version 3.2 Core Profile を選択する XXX ここで指定するとエラーはなさそうなのに映像が表示できなくなる
    // Decide GL+GLSL versions
#if defined(GLFW_INCLUDE_ES2)
    // GL ES 2.0 + GLSL 100
    LOGD("GLES2");
    glfwWindowHint(GLFW_CONTEXT_CREATION_API, GLFW_EGL_CONTEXT_API);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
#elif defined(GLFW_INCLUDE_ES3)
    // GL ES 3.0 + GLSL 100
    LOGD("GLES3");
    glfwWindowHint(GLFW_CONTEXT_CREATION_API, GLFW_EGL_CONTEXT_API);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
#else
    // GL 3.0 + GLSL 130
    LOGD("GL3");
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
//	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);	// OpenGL 3.2
//	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
//	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);	// OpenGL3以降で前方互換プロファイルを使う
#endif
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);	// マウス等でリサイズ不可
    glfwWindowHint(GLFW_MAXIMIZED, GL_TRUE);    // ウインドウを最大化する
//	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	// 終了時の処理登録
	atexit(glfwTerminate);

	RETURN(0, int);
}

GlfwWindow::GlfwWindow(const int width, const int height, const char *title)
:	Window(width, height, title),
	window(glfwCreateWindow(width, height, title, glfwGetPrimaryMonitor(), nullptr/*share*/)),
	prev_key_callback(nullptr)
{
	ENTER();

	if (window) {
		// コールバック内からWindowインスタンスを参照できるようにポインタを渡しておく
		glfwSetWindowUserPointer(window, this);
		// ウインドウサイズが変更されたときのコールバックをセット
		glfwSetWindowSizeCallback(window, resize);
        // マウスカーソルが表示されないように設定
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
        glfwShowWindow(window);
	}

	EXIT();
}

/*public*/
GlfwWindow::~GlfwWindow() {
	ENTER();

	if (window) {
		stop();
		glfwDestroyWindow(window);
		window = nullptr;
	}

	EXIT();
}

int GlfwWindow::resume() {
    ENTER();
    auto result = Window::resume();

    if (LIKELY(window)) {
        // このウインドウにフォーカスを与える
        glfwFocusWindow(window);
    }

    RETURN(result, int);
}

bool GlfwWindow::is_valid() const {
	return window != nullptr;
};

/**
 * アプリの終了要求をする
 * エラー時などにEyeAppから呼び出す
*/
/*public*/
void GlfwWindow::terminate() {
    ENTER();

    if (window) {
        glfwSetWindowShouldClose(window, 1);
    } 
    set_running(false);

    EXIT();
}

//--------------------------------------------------------------------------------
/*protected*/
void GlfwWindow::swap_buffers() {
	if (window) {
		glfwMakeContextCurrent(window);
		glfwSwapBuffers(window);
	}
}

/*static, protected*/
void GlfwWindow::resize(GLFWwindow *win, int width, int height) {
    ENTER();

	// フレームバッファのサイズを調べる
	int fbWidth, fbHeight;
	glfwGetFramebufferSize(win, &fbWidth, &fbHeight);
    LOGD("sz(%dx%d),fb(%dx%d)", width, height, fbWidth, fbHeight);

	// フレームバッファ全体をビューポートに設定する
	glViewport(0, 0, fbWidth, fbHeight);

	// このインスタンスの this ポインタを得る
	auto self = reinterpret_cast<GlfwWindow *>(glfwGetWindowUserPointer(win));
	if (self) {
		// このインスタンスが保持する縦横比を更新する
		self->internal_resize(fbWidth, fbHeight);
	}

	EXIT();
}

/*static, protected*/
void GlfwWindow::key_callback(GLFWwindow *win, int key, int scancode, int action, int mods) {
	ENTER();

	// このインスタンスの this ポインタを得る
	auto self = reinterpret_cast<GlfwWindow *>(glfwGetWindowUserPointer(win));
	if (self && self->on_key_event_func) {
		const auto keycode = ImGui_ImplGlfw_TranslateUntranslatedKey(key, scancode);
		LOGD("key=%d(%d),scancode=%d/%s,action=%d,mods=%d",
			key, keycode, scancode, glfwGetKeyName(key, scancode),
			action, mods);
		const auto imgui_key = ImGui_ImplGlfw_KeyToImGuiKey(keycode);
		// コールバックが設定されていればそれを呼び出す
		const auto event = self->on_key_event_func(imgui_key, scancode, (key_action_t)action, mods);
		if ((event.key != ImGuiKey_None) && self->prev_key_callback) {
			// ここでprev_key_callbackを呼び出せばimgui自体のキーコールバックが呼ばれる
			// キーが有効な場合のみprev_key_callbackを呼び出す
			self->prev_key_callback(self->window, key, scancode, (const int)action, mods);
		}
	} else if (self && self->prev_key_callback) {
        self->prev_key_callback(self->window, key, scancode, (const int)action, mods);
    }

	EXIT();
}

/*protected*/
bool GlfwWindow::poll_events() {
	// イベントを確認
//	glfwWaitEvents(); // こっちはイベントが起こるまで実行をブロックする
	// glfwWaitEventsTimeout(0.010); // イベントが起こるかタイム・アウトするまで実行をブロック, glfw3.2以降
	glfwPollEvents(); // イベントをチェックするが実行をブロックしない
	// ウィンドウを閉じる必要がなければ true を返す
	return window && !glfwWindowShouldClose(window);
}

/*protected*/
bool GlfwWindow::should_close() const {
    return !is_running() || !window || glfwWindowShouldClose(window);
}

/*protected,@WorkerThread*/
void GlfwWindow::init_gl() {
	ENTER();

	if (window) {
		// 作成したウィンドウへOpenGLで描画できるようにする
		glfwMakeContextCurrent(window);
		// ダブルバッファの入れ替えタイミングを指定, 垂直同期のタイミングを待つ
		glfwSwapInterval(1);

		// 作成したウインドウを初期化
		resize(window, width(), height());
		// IMGUIでのGUI描画用に初期化する
		init_gui();

		// XXX init_gui(imguiの初期化後)にキーイベントコールバックをセットすることで
		//     imguiのキーコールバックを無効にする
		// キー入力イベントコールバックをセット
		prev_key_callback = glfwSetKeyCallback(window, key_callback);
		LOGD("prev_key_callback=%p", prev_key_callback);
	} else {
        LOGW("has no window");
    }

	EXIT();
}

/*protected,@WorkerThread*/
void GlfwWindow::terminate_gl() {
	ENTER();

	if (prev_key_callback) {
		// 念のためにもとに戻しておく
		glfwSetKeyCallback(window, prev_key_callback);
	}

	terminate_gui();

	EXIT();
}

/*protected,@WorkerThread*/
void GlfwWindow::init_gui() {
	ENTER();

    // Decide GL+GLSL versions
#if defined(GLFW_INCLUDE_ES2)
    // GL ES 2.0 + GLSL 100
    static const char *GLSL_VERSION = "#version 100";
#elif defined(GLFW_INCLUDE_ES3)
    // GL ES 3.0 + GLSL 100
    static const char *GLSL_VERSION = "#version 300 es";
#else
    // GL 3.0 + GLSL 130
    static const char *GLSL_VERSION = "#version 130";
#endif

// Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
    io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;
    io.IniFilename = nullptr;	// 設定ファイルへの読み書きを行わない

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsLight();

    LOGV("ImGui_ImplGlfw_InitForOpenGL");
    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);

    LOGV("ImGui_ImplOpenGL3_Init:%s", GLSL_VERSION);
    ImGui_ImplOpenGL3_Init(GLSL_VERSION);

	EXIT();
}

/*protected,@WorkerThread*/
void GlfwWindow::terminate_gui() {
	ENTER();

	// Cleanup
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	EXIT();
}

}	// end of namespace serenegiant::app
