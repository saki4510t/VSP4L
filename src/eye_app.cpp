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
// app
#include "effect_fsh.h"
#include "eye_app.h"

namespace serenegiant::app {

#define COUNT_FRAMES (0)
// カメラ映像サイズ
#define VIDEO_WIDTH (1920)
#define VIDEO_HEIGHT (1080)
// 画面サイズ
#define WINDOW_WIDTH (VIDEO_WIDTH/2)
#define WINDOW_HEIGHT (VIDEO_HEIGHT/2)
// 輝度・拡大縮小率等の表示サイズ
#define MODE_WIDTH (VIDEO_WIDTH/2)
#define MODE_HEIGHT (VIDEO_WIDTH/2)
// OSD表示サイズ
#define OSD_WIDTH (WINDOW_WIDTH/4*3)
#define OSD_HEIGHT (VIDEO_HEIGHT/4*3)

//--------------------------------------------------------------------------------
/**
 * @brief コンストラクタ
 *
 */
/*public*/
EyeApp::EyeApp(const int &gl_version)
:   gl_version(gl_version),
	initialized(!Window::initialize()),
	window(WINDOW_WIDTH, WINDOW_HEIGHT, "BOV EyeApp"),
	source(nullptr), renderer_pipeline(nullptr),
	offscreen(nullptr), gl_renderer(nullptr),
    req_change_effect(false), req_freeze(false),
	effect_type(EFFECT_NON), current_effect(EFFECT_NON),
	key_dispatcher(handler),
	mvp_matrix(), zoom_ix(DEFAULT_ZOOM_IX),
	reset_mode_task(nullptr)
{
    ENTER();

	mvp_matrix.scale(ZOOM_FACTOR[zoom_ix]);
	key_dispatcher
		.set_on_key_mode_changed([this](const key_mode_t &key_mode) {
			LOGD("key_mode=%d", key_mode);
			reset_mode_delayed();
		})
		.set_on_brightness_changed([this](const bool &inc_dec) {
			reset_mode_delayed();
			request_change_brightness(inc_dec);
		})
		.set_on_scale_changed([this](const bool &inc_dec) {
			reset_mode_delayed();
			request_change_scale(inc_dec);
		})
		.set_on_effect_changed([this](const effect_t &effect) {
			request_change_effect(effect);
		})
		.set_on_freeze_changed([this](const bool &onoff){
			request_change_freeze(onoff);
		});
	// キーイベントハンドラを登録
	window
		.on_key_event([this](const int &key, const int &scancode, const int &action, const int &mods) {
			return key_dispatcher.handle_on_key_event(KeyEvent(key, scancode, action, mods));
		})
		.set_on_start([this](GLFWwindow *win) { on_start(); })
		.set_on_stop([this](GLFWwindow *win) { on_stop(); });

	// 遅延実行タスクの準備
	reset_mode_task = [this]() {
		key_dispatcher.reset_key_mode();
	};

    EXIT();
}

/**
 * @brief デストラクタ
 *
 */
/*public*/
EyeApp::~EyeApp() {
    ENTER();

	window.stop();
	handler.terminate();

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

	window.start([this](GLFWwindow *win) { on_render(); });

#if 0
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

    for ( ; window.is_running() && window ; ) {
		// GLFWのイベント処理ループ
        usleep(30000);
    }

	window.stop();

	LOGD("Finished.");

    EXIT();
}

//--------------------------------------------------------------------------------
/*private,@WorkerThread*/
void EyeApp::on_start() {
    ENTER();

    source = std::make_unique<v4l2_pipeline::V4L2SourcePipeline>("/dev/video0");
	if (!source || source->open() || source->find_stream(VIDEO_WIDTH, VIDEO_HEIGHT)) {
		LOGE("カメラをオープンできなかった");
		window.stop();
		EXIT();
	}
	LOGV("supported=%s", source->get_supported_size().c_str());
	source->resize(VIDEO_WIDTH, VIDEO_HEIGHT);
	if (source->start()) {
		LOGE("カメラを開始できなかった");
		window.stop();
		EXIT();
	}

	// カメラ映像描画用のGLRendererPipelineを生成
	const char* versionStr = (const char*)glGetString(GL_VERSION);
	LOGD("GL_VERSION=%s", versionStr);
	renderer_pipeline = std::make_unique<pipeline::GLRendererPipeline>(gl_version);
	source->set_pipeline(renderer_pipeline.get());
	renderer_pipeline->start();
	// オフスクリーンを生成
	offscreen = std::make_unique<gl::GLOffScreen>(GL_TEXTURE0, WINDOW_WIDTH, WINDOW_HEIGHT, false);

	req_change_matrix = true;

	EXIT();
}

/*private,@WorkerThread*/
void EyeApp::on_stop() {
	ENTER();

	if (source) {
		source->stop();
		source.reset();
	}
	if (renderer_pipeline) {
		renderer_pipeline->on_release();
		renderer_pipeline->stop();
		renderer_pipeline.reset();
	}
	offscreen.reset();

	EXIT();
}

/**
 * @brief 描画スレッドの実行関数
 *
 */
/*private,@WorkerThread*/
void EyeApp::on_render() {
    ENTER();

	// 描画用の設定更新を適用
	prepare_draw(offscreen, gl_renderer);
	// 描画処理
	if (!req_freeze) {
		// オフスクリーンへ描画
		offscreen->bind();
		renderer_pipeline->on_draw();
		offscreen->unbind();
	} else {
		// フレームキューが溢れないようにフリーズモード時は直接画面へ転送しておく(glClearで消される)
		renderer_pipeline->on_draw();
	}
	// 縮小時に古い画面が見えてしまうのを防ぐために塗りつぶす
	glClear(GL_COLOR_BUFFER_BIT);
	// 画面へ転送
	handle_draw(offscreen, gl_renderer);
	// GUI(2D)描画処理を実行
	handle_draw_gui();

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
		LOGD("create GLRenderer GL3");
		return std::make_unique<gl::GLRenderer>(texture_gl3_vsh, fsh, true);
	} else {
		LOGD("create GLRenderer GL2");
		return std::make_unique<gl::GLRenderer>(texture_gl2_vsh, fsh, true);
	}
}

/**
 * @brief 描画用の設定更新を適用
 *
 * @param offscreen
 * @param gl_renderer
 */
void EyeApp::prepare_draw(gl::GLOffScreenUp &offscreen, gl::GLRendererUp &renderer) {
	ENTER();

	if (UNLIKELY(req_change_matrix)) {
		float mat[16];
		{	// モデルビュー変換行列が変更されたとき
			std::lock_guard<std::mutex> lock(state_lock);
			req_change_matrix = false;
			mvp_matrix.getOpenGLSubMatrix(mat);
		}
		mat[5] *= -1.f;			// 上下反転
		offscreen->set_mvp_matrix(mat, 0);
	}
	if (UNLIKELY(req_change_effect)) {
		std::lock_guard<std::mutex> lock(state_lock);
		req_change_effect = false;
		if (current_effect != effect_type) {
			current_effect = effect_type;
			renderer.reset();
		}
	}
	if (UNLIKELY(!renderer)) {
		// オフスクリーンの描画用GLRendererを生成
		renderer = create_renderer(current_effect);
	}

	EXIT();
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
	offscreen->draw(renderer.get());

	EXIT();
}

/**
 * @brief GUI(2D)描画処理を実行
 *
 */
void EyeApp::handle_draw_gui() {
	ENTER();

	// Start the Dear ImGui frame
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	// ウインドウ位置を指定するにはImGui::Beginの前にImGui::SetNextWindowPosを呼び出す
	// ウインドウサイズを指定するにはImGui::Beginの前にImGui::SetNextWindowSizeを呼び出す
#if !defined(NDEBUG)
	{
		ImGui::Begin("DEBUG");
		const auto fps = ImGui::GetIO().Framerate;
		ImGui::Text("%.3f ms/frame(%.1f FPS)", 1000.0f / fps, fps);
		ImGui::End();
	}
#endif

	// Rendering
	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

	EXIT();
}

//--------------------------------------------------------------------------------
/**
 * @brief 一定時間後にキーモードをリセットする
 *
 */
void EyeApp::reset_mode_delayed() {
	handler.remove(reset_mode_task);
	handler.post_delayed(reset_mode_task, 5000);
}

//--------------------------------------------------------------------------------
/**
 * @brief 輝度変更要求
 *
 * @param inc_dec trueなら輝度増加、falseなら輝度減少
 */
void EyeApp::request_change_brightness(const bool &inc_dec) {
	ENTER();

	LOGD("inc_dec=%d", inc_dec);
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

	std::lock_guard<std::mutex> lock(state_lock);
	int ix = zoom_ix + (inc_dec ? 1 : -1);
	if (ix < 0) {
		ix = 0;
	} else if (ix >= NUM_ZOOM_FACTOR) {
		ix = NUM_ZOOM_FACTOR - 1;
	}
	if (ix != zoom_ix) {
		LOGD("ix=%d", ix);
		zoom_ix = ix;
		auto factor = ZOOM_FACTOR[ix];
		mvp_matrix.setScale(factor, factor, 1.0f);
		req_change_matrix = true;
	}

	EXIT();
}

/**
 * @brief 映像効果変更要求
 *
 * @param effect
 */
void EyeApp::request_change_effect(const effect_t &effect) {
	ENTER();

	LOGD("effect=%d", effect);
	std::lock_guard<std::mutex> lock(state_lock);
	const bool changed = effect_type != effect;
	effect_type = effect;
	req_change_effect = changed;

	EXIT();
}

/**
 * @brief 映像フリーズのON/OFF切り替え要求
 *
 * @param effect
 */
void EyeApp::request_change_freeze(const bool &onoff) {
	ENTER();

	LOGD("onoff=%d", onoff);
	req_freeze = onoff;

	EXIT();
}

}   // namespace serenegiant::app

