#define COUNT_FRAMES (0)
// カメラ映像の幅
#define VIDEO_WIDTH (1920)
// カメラ映像の高さ
#define VIDEO_HEIGHT (1080)
// 画面の幅
#define WINDOW_WIDTH (VIDEO_WIDTH/2)
// 画面の高さ
#define WINDOW_HEIGHT (VIDEO_HEIGHT/2)

#if 0    // set 0 if you need debug log, otherwise set 1
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

#include <stdio.h>
#include <string>
#include <cstring>

#include "utilbase.h"
// common
#include "glutils.h"
#include "gloffscreen.h"
#include "times.h"
// gl
#include "gl/texture_vsh.h"
#include "gl/rgba_fsh.h"
// aandusb
#include "window.h"
// aandusb/pipeline
#include "pipeline/pipeline_v4l2_source.h"
// app
#include "effect_fsh.h"
#include "eye_app.h"

namespace pipeline = serenegiant::pipeline;
namespace v4l2_pipeline = serenegiant::v4l2::pipeline;

namespace serenegiant::app {

// 短押しと判定する最小押し下げ時間[ミリ秒]
#define SHORT_PRESS_MIN_MS (20)
// 長押し時間[ミリ秒]
#define LONG_PRESS_TIMEOUT_MS (2500)
// 長長押し時間[ミリ秒]
#define LONG_LONG_PRESS_TIMEOUT_MS (5000)

//--------------------------------------------------------------------------------
class LongPressCheckTask : public thread::Runnable {
private:
	EyeApp &app;			// こっちは参照を保持する
	const KeyEvent event;	// こっちはコピーを保持する
public:
	LongPressCheckTask(
		EyeApp &app,
		const KeyEvent &event)
	:	app(app), event(event)
	{
		ENTER();
		EXIT();
	}

	virtual ~LongPressCheckTask() {
		ENTER();
		LOGD("破棄された");
		EXIT();
	}

	void run() {
		ENTER();

		app.handle_on_long_key_pressed(event);

		EXIT();
	}
};

//--------------------------------------------------------------------------------
/**
 * @brief コンストラクタ
 * 
 */
/*public*/
EyeApp::EyeApp(const int &gl_version)
:   gl_version(gl_version),
	initialized(!Window::initialize()),
    is_running(false), req_change_effect(false),
	effect_type(EFFECT_NON),
	key_mode(KEY_MODE_NORMAL),
	mvp_matrix(), zoom_ix(DEFAULT_ZOOM_IX),
	test_task(nullptr)
{
    ENTER();

	mvp_matrix.scale(ZOOM_FACTOR[zoom_ix]);
#if 1
	// XXX ラムダ式内でラムダ式自体へアクセスする場合はstd::functionで受けないといけない
	//     ラムダ式内でラムダ式自体へアクセスしないのであればautoにしたほうがオーバーヘッドが少ない
	test_task = [&]() {
		LOGD("run %ld", systemTime());
		if (test_task) {
			handler.post_delayed(test_task, 1000);
		}
	};
	LOGD("post_delayed %ld", systemTime());
	handler.post_delayed(test_task, 10000);
	handler.remove(test_task);
	handler.post_delayed(test_task, 10000);
#endif

    EXIT();
}

/**
 * @brief デストラクタ
 * 
 */
/*public*/
EyeApp::~EyeApp() {
    ENTER();

	test_task = nullptr;
	handler.terminate();
    is_running = false;

    EXIT();
}

/**
 * @brief アプリを実行
 *        実行終了までこの関数を抜けないので注意
 * 
 */
/*public*/
void EyeApp::run() {
    ENTER();

    is_running = true;
    auto handler_thread = std::thread([this] { renderer_thread_func(); });

    for ( ; is_running ; ) {
		// ここでなにかするかも
        usleep(300000);
    }

    is_running = false;
    if (handler_thread.joinable()) {
        handler_thread.join();
    }

	LOGD("Finished.");

    EXIT();
}

//--------------------------------------------------------------------------------
/**
 * @brief 描画スレッドの実行関数
 * 
 */
/*private*/
void EyeApp::renderer_thread_func() {
    ENTER();

    auto source = std::make_shared<v4l2_pipeline::V4L2SourcePipeline>("/dev/video0");
	if (source && !source->open() && !source->find_stream(VIDEO_WIDTH, VIDEO_HEIGHT)) {
		LOGV("supported=%s", source->get_supported_size().c_str());
		source->resize(VIDEO_WIDTH, VIDEO_HEIGHT);
		if (!source->start()) {
			LOGD("windowを初期化");
			Window window(WINDOW_WIDTH, WINDOW_HEIGHT, "BOV EyeApp");
			if (is_running && window.is_valid()) {
				// キーイベントハンドラを登録
				window.on_key_event([this](const int &key, const int &scancode, const int &action, const int &mods) {
                    return handle_on_key_event(KeyEvent(key, scancode, action, mods));
                });
				// カメラ映像描画用のGLRendererPipelineを生成
				const char* versionStr = (const char*)glGetString(GL_VERSION);
				LOGD("create and start GLRendererPipeline,%s", versionStr);
				auto renderer_pipeline = std::make_unique<pipeline::GLRendererPipeline>(gl_version);
                source->set_pipeline(renderer_pipeline.get());
                renderer_pipeline->start();
				// オフスクリーンを生成
				auto offscreen = std::make_unique<gl::GLOffScreen>(GL_TEXTURE0, WINDOW_WIDTH, WINDOW_HEIGHT, false);
				gl::GLRendererUp gl_renderer = nullptr;
				effect_t current_effect = effect_type;
				req_change_matrix = true;
				float mat[16]{};
				LOGD("GLFWのイベントループ開始");
				// ウィンドウが開いている間繰り返す
				while (is_running && window) {
					const auto start = systemTime();
					// 描画処理
					if (UNLIKELY(req_change_matrix)) {
						{	// モデルビュー変換行列が変更されたとき
							std::lock_guard<std::mutex> lock(state_lock);
							req_change_matrix = false;
							mvp_matrix.getOpenGLSubMatrix(mat);
						}
						// FIXME フリーズモードで拡大縮小するにはrenderer_pipelineにモデルビュー変換行列を当てるのはだめ
						renderer_pipeline->set_mvp_matrix(mat, 0);
					}
					// 縮小時に古い画面が見えてしまうのを防ぐために塗りつぶす
					glClear(GL_COLOR_BUFFER_BIT);
					if (current_effect == EFFECT_NON) {
						renderer_pipeline->on_draw();
					} else {
						// オフスクリーンへ描画
						offscreen->bind();
						renderer_pipeline->on_draw();
						offscreen->unbind();
						if (UNLIKELY(req_change_effect)) {
					 		std::lock_guard<std::mutex> lock(state_lock);		
							req_change_effect = false;
							if (current_effect != effect_type) {
								current_effect = effect_type;
								gl_renderer.reset();
							}
						}
						if (UNLIKELY(!gl_renderer)) {
							// オフスクリーンの描画用GLRendererを生成
							gl_renderer = create_renderer(current_effect);
						}
						// 画面へ転送
						handle_draw(offscreen, gl_renderer);
					}
					// GUI(2D)描画処理を実行
					handle_draw_gui();
					// ダブルバッファーをスワップ
					window.swap_buffers();
					auto t = (systemTime() - start) / 1000L;
					if (t < 12000) {
						// 60fpsだと16.6msだけど少し余裕をみて最大12ms待機する
						usleep(12000 - t);
					}
				}
				LOGD("GLFWのイベントループ終了");
				source->stop();
				renderer_pipeline->on_release();
				renderer_pipeline->stop();
				renderer_pipeline.reset();
				offscreen.reset();
				source.reset();
			} else {
				// ウィンドウ作成に失敗した処理
				LOGE("Can't create GLFW window.");
			}
		}
	} else {
		LOGE("Failed to init v4l2");
	}

	LOGD("GLFWスレッド終了");

	source.reset();
    is_running = false;

    EXIT();
}

/**
 * @brief Create a renderer object
 * 
 * @param effect 
 * @return gl::GLRendererUp 
 */
gl::GLRendererUp EyeApp::create_renderer(const effect_t &effect) {
	// FIXME 今は映像効果のない転送するだけのGLRendererを生成している
	const char *fsh = (gl_version >= 300) ? fsh = rgba_gl3_fsh : rgba_gl2_fsh;
	switch (effect) {
	case EFFECT_GRAY:
		fsh = (gl_version >= 300) ? rgba_gray_gl3_fsh : rgba_gray_gl2_fsh;
		break;
	case EFFECT_GRAY_REVERSE:
		fsh = (gl_version >= 300) ? rgba_gray_reverse_gl3_fsh : rgba_gray_reverse_gl2_fsh;
		break;
	case EFFECT_BIN:
		fsh = (gl_version >= 300) ? rgba_bin_gl3_fsh : rgba_bin_gl2_fsh;
		break;
	case EFFECT_BIN_REVERSE:
		fsh = (gl_version >= 300) ? rgba_bin_reverse_gl3_fsh : rgba_bin_reverse_gl2_fsh;
		break;
	case EFFECT_NON:
	default:
		fsh = (gl_version >= 300) ? fsh = rgba_gl3_fsh : rgba_gl2_fsh;
		break;
	}
	if (gl_version >= 300) {
		LOGD("create GLRenderer GL/GLES3");
		return std::make_unique<gl::GLRenderer>(texture_gl3_vsh, fsh, true);
	} else {
		LOGD("create GLRenderer GL/GLES2");
		return std::make_unique<gl::GLRenderer>(texture_gl2_vsh, fsh, true);
	}
}

/**
 * @brief 描画処理を実行
 * 
 * @param renderer 
 */
/*private*/
void EyeApp::handle_draw(gl::GLOffScreenUp &offscreen, gl::GLRendererUp &renderer) {
	ENTER();

#if COUNT_FRAMES && !defined(LOG_NDEBUG) && !defined(NDEBUG)
    static int cnt = 0;
    if ((++cnt % 100) == 0) {
        MARK("cnt=%d", cnt);
    }
#endif
	// FIXME ここで拡大縮小のモデルビュー変換行列を適用する
	offscreen->draw(renderer.get());

	EXIT();
}

/**
 * @brief GUI(2D)描画処理を実行
 * 
 */
void EyeApp::handle_draw_gui() {
	ENTER();

	// FIXME 未実装

	EXIT();
}

//--------------------------------------------------------------------------------
/**
 * @brief キーの長押し確認用ラムダ式が生成されていることを確認、未生成なら新たに生成する
 * 
 * @param event 
 */
void EyeApp::confirm_long_press_task(const KeyEvent &event) {
	ENTER();

	const auto key = event.key;
	if (long_key_press_tasks.find(key) == long_key_press_tasks.end()) {
		long_key_press_tasks[key] = std::make_shared<LongPressCheckTask>(*this, event);
	}

	EXIT();
}

/**
 * @brief 短押しかどうか, 排他制御してないので上位でロックすること
 * 
 * @param key 
 * @return true 
 * @return false 
 */
bool EyeApp::is_short_pressed(const int &key) {
	return (key_state.find(key) != key_state.end())
		&& (key_state[key]->state == KEY_STATE_DOWN);
}

/**
 * @brief 長押しされているかどうか, 排他制御してないので上位でロックすること
 * 
 * @param key 
 * @return true 
 * @return false 
 */
bool EyeApp::is_long_pressed(const int &key) {
	return (key_state.find(key) != key_state.end())
		&& (key_state[key]->state == KEY_STATE_DOWN_LONG);
}

/**
 * @brief GLFWからのキー入力イベントの処理
 * とりあえずは、GLFW_KEY_RIGHT(262), GLFW_KEY_LEFT(263), GLFW_KEY_DOWN(264), GLFW_KEY_UP(265)の
 * 4種類だけキー処理を行う
 * 
 * @param event 
 * @return int32_t 
 */
/*private*/
int32_t EyeApp::handle_on_key_event(const KeyEvent &event) {
	ENTER();

	int32_t result = 0;

	// XXX 複数キー同時押しのときに最後に押したキーに対してのみGLFW_REPEATがくるみたいなのでGLFW_REPEATは使わない
	const auto key = event.key;
	if ((key >= GLFW_KEY_RIGHT) && (key <= GLFW_KEY_UP)) {
		switch (event.action) {
		case GLFW_RELEASE:	// 0
			result = handle_on_key_up(event);
			break;
		case GLFW_PRESS:	// 1
			result = handle_on_key_down(event);
			break;
		case GLFW_REPEAT:	// 2
		default:
			break;
		}
	} else {
		LOGD("key=%d,scancode=%d/%s,action=%d,mods=%d",
			key, event.scancode, glfwGetKeyName(key, event.scancode),
			event.action, event.mods);
	}

	RETURN(result, int32_t);
}

/**
 * @brief handle_on_key_eventの下請け、キーが押されたとき
 * とりあえずは、GLFW_KEY_RIGHT(262), GLFW_KEY_LEFT(263), GLFW_KEY_DOWN(264), GLFW_KEY_UP(265)の
 * 4種類だけキー処理を行う
 * 
 * @param event 
 * @return int32_t 
 */
/*private*/
int32_t EyeApp::handle_on_key_down(const KeyEvent &event) {
	ENTER();

	const auto key = event.key;
	{
 		std::lock_guard<std::mutex> lock(state_lock);		
		key_state[key] = std::make_shared<KeyEvent>(event);
	}
	confirm_long_press_task(event);
	handler.remove(long_key_press_tasks[key]);
	handler.post_delayed(long_key_press_tasks[key], LONG_PRESS_TIMEOUT_MS);

	RETURN(0, int32_t);
}

/**
 * @brief handle_on_key_eventの下請け、キーが離されたとき
 * とりあえずは、GLFW_KEY_RIGHT(262), GLFW_KEY_LEFT(263), GLFW_KEY_DOWN(264), GLFW_KEY_UP(265)の
 * 4種類だけキー処理を行う
 * 
 * @param event 
 * @return int32_t 
 */
/*private*/
int32_t EyeApp::handle_on_key_up(const KeyEvent &event) {
	ENTER();

	int result = -1;
	const auto key = event.key;
	confirm_long_press_task(event);
	handler.remove(long_key_press_tasks[key]);

	KeyEventSp state;
	key_mode_t current_key_mode;
	{
 		std::lock_guard<std::mutex> lock(state_lock);		
		current_key_mode = key_mode;
		state = key_state[key];
		key_state[key] = std::make_shared<KeyEvent>(event);
	}
	const auto duration_ms = (event.event_time_ns - state->event_time_ns) / 1000000L;
	if ((state->state == KEY_STATE_DOWN) && (duration_ms >= SHORT_PRESS_MIN_MS)) {
		LOGD("on_key_up,key=%d,state=%d,duration_ms=%ld", key, state->state, duration_ms);
		// FIXME ここで同時押しの処理を行う

		if (result) {
			// 同時押しで処理済みでなければキーモード毎の処理を行う
			switch (current_key_mode) {
			case KEY_MODE_NORMAL:
				result = on_key_up_normal(event);
			case KEY_MODE_OSD:
				result = on_key_up_osd(event);
				break;
			default:
				LOGW("unknown key mode,%d", current_key_mode);
				break;
			}
		}
	} else if ((state->state == KEY_STATE_DOWN_LONG) && (duration_ms >= LONG_LONG_PRESS_TIMEOUT_MS)) {
		// FIXME 未実装 長長押しの処理
	}

	RETURN(result, int32_t);
}

/**
 * @brief 通常モードで短押ししたときの処理, handle_on_key_upの下請け
 * 
 * @param event 
 * @return int32_t 
 */
int32_t EyeApp::on_key_up_normal(const KeyEvent &event) {
	ENTER();

	const auto key = event.key;
	switch (key) {
	case GLFW_KEY_RIGHT:
	case GLFW_KEY_LEFT:
		// 輝度変更
		request_change_brightness(key == GLFW_KEY_RIGHT);
		break;
	case GLFW_KEY_DOWN:
	case GLFW_KEY_UP:
		// 拡大縮小
		request_change_scale(key == GLFW_KEY_UP);
		break;
	default:
		LOGW("unexpected key code,%d", key);
		break;
	}

	RETURN(0, int32_t);
}

/**
 * @brief OSD操作モードで短押ししたときの処理, handle_on_key_upの下請け
 * 
 * @param event 
 * @return int32_t 
 */
int32_t EyeApp::on_key_up_osd(const KeyEvent &event) {
	ENTER();

	// FIXME 未実装

	RETURN(0, int32_t);
}

//--------------------------------------------------------------------------------
/**
 * @brief 長押し時間経過したときの処理
 * 
 * @param event 
 * @return int32_t 
 */
int32_t EyeApp::handle_on_long_key_pressed(const KeyEvent &event) {
	ENTER();

	int result = -1;
	const auto key = event.key;
	bool long_pressed = false;
	key_mode_t current_key_mode;
	{
 		std::lock_guard<std::mutex> lock(state_lock);
		current_key_mode = key_mode;
		if (key_state[key]->state == KEY_STATE_DOWN) {
			key_state[key]->state = KEY_STATE_DOWN_LONG;
			long_pressed = true;
		}
	}
	if (long_pressed) {
		const auto duration_ms = (event.event_time_ns - systemTime()) / 1000000L;
		LOGD("on_long_key_pressed,key=%d,duration_ms=%ld", key, duration_ms);
		// FIXME ここで同時押しの処理を行う

		if (result) {
			// 同時押しで処理済みでなければキーモード毎の処理を行う
			switch (current_key_mode) {
			case KEY_MODE_NORMAL:
				result = on_long_key_pressed_normal(event);
				break;
			case KEY_MODE_OSD:
				result = on_long_key_pressed_osd(event);
				break;
			default:
				LOGW("unknown key mode,%d", current_key_mode);
				break;
			}
		}
	}


	RETURN(result, int32_t);
}

/**
 * @brief 通常モードで長押し時間経過したときの処理, handle_on_long_key_pressedの下請け
 * 
 * @param event 
 * @return int32_t 
 */
int32_t EyeApp::on_long_key_pressed_normal(const KeyEvent &event) {
	ENTER();

	const auto key = event.key;
	// FIXME 未実装

	RETURN(0, int32_t);
}

/**
 * @brief OSD操作モードで長押し時間経過したときの処理, handle_on_long_key_pressedの下請け
 * 
 * @param event 
 * @return int32_t 
 */
int32_t EyeApp::on_long_key_pressed_osd(const KeyEvent &event) {
	ENTER();

	// FIXME 未実装

	RETURN(0, int32_t);
}

/**
 * @brief 輝度変更要求
 * 
 * @param inc_dec trueなら輝度増加、falseなら輝度減少
 */
void EyeApp::request_change_brightness(const bool &inc_dec) {
	ENTER();

	// FIXME 未実装

	EXIT();
}

/**
 * @brief 拡大縮小率変更要求
 * 
 * @param inc_dec trueなら拡大、falseなら縮小
 */
void EyeApp::request_change_scale(const bool &inc_dec) {
	ENTER();

	int ix;
	std::lock_guard<std::mutex> lock(state_lock);		
	ix = zoom_ix + (inc_dec ? 1 : -1);
	if (ix < 0) {
		ix = 0;
	} else if (ix >= NUM_ZOOM_FACTOR) {
		ix = NUM_ZOOM_FACTOR - 1;
	}
	if (ix != zoom_ix) {
		zoom_ix = ix;
		auto factor = ZOOM_FACTOR[ix];
		mvp_matrix.setScale(factor, factor, 1.0f);
		req_change_matrix = true;
	}

	EXIT();
}

}   // namespace serenegiant::app
