#define COUNT_FRAMES (0)
#define VIDEO_WIDTH (1280)
#define VIDEO_HEIGHT (720)

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
#include <unordered_map>

#include "utilbase.h"
// common
#include "glutils.h"
// aandusb
#include "window.h"
// aandusb/pipeline
#include "pipeline/pipeline_gl_renderer.h"
#include "pipeline/pipeline_v4l2_source.h"
// app
#include "eye_app.h"

namespace pipeline = serenegiant::pipeline;
namespace v4l2_pipeline = serenegiant::v4l2::pipeline;

namespace serenegiant::app {

/**
 * @brief コンストラクタ
 * 
 */
/*public*/
EyeApp::EyeApp()
:   initialized(!serenegiant::Window::initialize()),
    is_running(false)
{
    ENTER();
    EXIT();
}

/**
 * @brief デストラクタ
 * 
 */
/*public*/
EyeApp::~EyeApp() {
    ENTER();

    is_running = false;

    EXIT();
}

/**
 * @brief アプリを実行
 * 
 */
/*public*/
void EyeApp::run() {
    ENTER();

    is_running = true;
    auto handler_thread = std::thread([this] { handler_thread_func(); });

    for ( ; is_running ; ) {
        usleep(300000);
    }

    is_running = false;
    if (handler_thread.joinable()) {
        handler_thread.join();
    }

	LOGI("Finished.");

    EXIT();
}

//--------------------------------------------------------------------------------
/**
 * @brief ワーカースレッドの実行関数
 * 
 */
/*private*/
void EyeApp::handler_thread_func() {
    ENTER();

    pipeline::GLRendererPipelineSp renderer = nullptr;
    auto source = std::make_shared<v4l2_pipeline::V4L2SourcePipeline>("/dev/video0");
	if (source && !source->open() && !source->find_stream(VIDEO_WIDTH, VIDEO_HEIGHT)) {
		LOGI("supported=%s", source->get_supported_size().c_str());
		source->resize(VIDEO_WIDTH, VIDEO_HEIGHT);
		if (!source->start()) {
			LOGD("windowを初期化");
			sere::Window window(VIDEO_WIDTH / 2, VIDEO_HEIGHT / 2, "BOV EyeApp");
			if (is_running && window.is_valid()) {
				// キーイベントハンドラを登録
				window.on_key_event([this](const int &key, const int &scancode, const int &action, const int &mods) {
                    return handle_on_key_event(key, scancode, action, mods);
                });
                if (UNLIKELY(!renderer)) {
                    const char* versionStr = (const char*)glGetString(GL_VERSION);
                    LOGD("create and start GLRendererPipeline,%s", versionStr);
                    renderer = std::make_shared<pipeline::GLRendererPipeline>(300);
                    source->set_pipeline(renderer.get());
                    renderer->start();
                }
				LOGD("GLFWのイベントループ開始");
				// ウィンドウが開いている間繰り返す
				while (is_running && window) {
					// 描画処理
					handle_draw(window, renderer);
					// ダブルバッファーをスワップ
					window.swap_buffers();
				}
				LOGD("GLFWのイベントループ終了");
				source.reset();
				renderer.reset();
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
	renderer.reset();
    is_running = false;

    EXIT();
}

int32_t EyeApp::handle_on_key_event(const int &key, const int &scancode, const int &action, const int &mods) {
	ENTER();

	int32_t result = 0;

    LOGD("key=%d,scancode=%d/%s,action=%d,mods=%d", key, scancode, glfwGetKeyName(key, scancode), action, mods);
	switch (action) {
	case GLFW_RELEASE:	// 0
		result = handle_on_key_up(key, scancode, action, mods);
		break;
	case GLFW_PRESS:	// 1
	case GLFW_REPEAT:	// 2
		result = handle_on_key_down(key, scancode, action, mods);
		break;
	default:
		break;
	}

	RETURN(result, int32_t);
}

/**
 * @brief handle_on_key_eventの下請け、キーが押されたとき
 * 
 * @param key 
 * @param scancode 
 * @param action 
 * @param mods 
 * @return int32_t 
 */
int32_t EyeApp::handle_on_key_down(const int &key, const int &scancode, const int &action, const int &mods) {
	ENTER();

	int32_t result = 0;

	RETURN(result, int32_t);
}

/**
 * @brief handle_on_key_eventの下請け、キーが離されたとき
 * 
 * @param key 
 * @param scancode 
 * @param action 
 * @param mods 
 * @return int32_t 
 */
int32_t EyeApp::handle_on_key_up(const int &key, const int &scancode, const int &action, const int &mods) {
	ENTER();

	int32_t result = 0;

#if 1
    // デバッグ用にESC/ENTERでアプリを終了させる
	if ((key == GLFW_KEY_ENTER)
		||(key == GLFW_KEY_ESCAPE)) {
		is_running = false;
	}
#endif

	RETURN(result, int32_t);
}

/**
 * @brief 描画処理を実行
 * 
 * @param window 
 */
void EyeApp::handle_draw(sere::Window &window, pipeline::GLRendererPipelineSp &renderer) {
	ENTER();

#if COUNT_FRAMES && !defined(LOG_NDEBUG) && !defined(NDEBUG)
    static int cnt = 0;
    if ((++cnt % 100) == 0) {
        MARK("cnt=%d", cnt);
    }
#endif
    if (LIKELY(renderer)) {
		// FIXME ここで拡大縮小のモデルビュー変換行列を適用する
        renderer->on_draw();
	}

	EXIT();
}

}   // namespace serenegiant::app
