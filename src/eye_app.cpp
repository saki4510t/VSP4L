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
	#define DEBUG_GL_CHECK			// GL関数のデバッグメッセージを表示する時
	#define DEBUG_EGL_CHECK			// EGL関数のデバッグメッセージを表示するとき
#endif

#define LOG_TAG "EyeApp"

#define MEAS_TIME (0)				// 1フレーム当たりの描画時間を測定する時1

#include <stdio.h>
#include <string>
#include <cstring>
#include <sstream>
#include <iterator>	// istream_iterator

#include <drm/drm_fourcc.h>

#include "utilbase.h"
// common
#include "charutils.h"
#include "glutils.h"
#include "image_helper.h"
#include "times.h"
// gl
#include "gl/texture_vsh.h"
#include "gl/rgba_fsh.h"
// app
#include "internal.h"
#include "effect_fsh.h"
#include "eye_app.h"

namespace serenegiant::app {

// 自前でV4l2Source::handle_frameを呼び出すかどうか
// 0: v4l2sourceのワーカースレッドでhandle_frameを呼び出す, 1:自前でhandle_frameを呼び出す
#define HANDLE_FRAME (0)

// 輝度調整・拡大縮小率モード表示の背景アルファ
#define MODE_BK_ALPHA (0.3f)
// 輝度調整モード・拡大縮小モード表示でアイコンと一緒に表示する文字のサイズ
#define MODE_ICON_FONT_SZ (216)
// 輝度調整・拡大縮小率モード表示のアイコンサイズ
#define MODE_ICON_SZ (240)
// 輝度調整・拡大縮小率モード表示のアイコンのテクスチャサイズ
#define MODE_ICON_TEX_SZ (192)
// ウオッチドッグをリセットする頻度[フレーム数]
#define RESET_WATCHDOG_CNT (25)
// カレントパスの取得用バッファサイズ
#define PATH_SIZE (512)
// ステータスの更新間隔(LED点滅の最小間隔), とりあえず1秒にしておく
#define UPDATE_STATE_INTERVAL_MS (1000)
// キーモードをリセットするまでの待機時間[ミリ秒]
#define KEY_MODE_RESET_DELAY_MS (5000)

#if MEAS_TIME
#define MEAS_TIME_INIT static nsecs_t _meas_time_ = 0;\
   	static nsecs_t _init_time_ = systemTime();\
   	static int _meas_count_ = 0;
#define MEAS_TIME_START	const nsecs_t _meas_t_ = systemTime();
#define MEAS_TIME_STOP \
	_meas_time_ += (systemTime() - _meas_t_); \
	_meas_count_++; \
	if (UNLIKELY((_meas_count_ % 100) == 0)) { \
		const float d = _meas_time_ / (1000000.f * _meas_count_); \
		const float fps = _meas_count_ * 1000000000.f / (systemTime() - _init_time_); \
		LOGI("meas time=%5.2f[msec]/fps=%5.2f", d, fps); \
	}
#define MEAS_RESET \
	_meas_count_ = 0; \
	_meas_time_ = 0; \
	_init_time_ = systemTime();
#else
#define MEAS_TIME_INIT
#define MEAS_TIME_START
#define MEAS_TIME_STOP
#define MEAS_RESET
#endif

//--------------------------------------------------------------------------------
/**
 * @brief コンストラクタ
 *
 */
/*public*/
EyeApp::EyeApp(
	std::unordered_map<std::string, std::string> _options,
	const int &gl_version)
:   options(std::move(_options)),
	gl_version(gl_version),
	initialized(!GlfwWindow::initialize()),
	exit_esc(options.find(OPT_DEBUG_EXIT_ESC) != options.end()),
	show_fps(options.find(OPT_DEBUG_SHOW_FPS) != options.end()),
	width(to_int(options[OPT_WIDTH], to_int(OPT_WIDTH_DEFAULT, 1920))),
	height(to_int(options[OPT_HEIGHT], to_int(OPT_HEIGHT_DEFAULT, 1080))),
	app_settings(), camera_settings(),
	window(width, height, "BOV EyeApp"),
	source(nullptr),
	m_egl(nullptr),
	video_renderer(nullptr), image_renderer(nullptr),
	offscreen(nullptr), screen_renderer(nullptr),
    req_change_effect(false), req_freeze(false),
	req_effect_type(EFFECT_NON), current_effect(req_effect_type),
	key_dispatcher(handler),
	mvp_matrix(), zoom_ix(DEFAULT_ZOOM_IX), brightness_ix(5),
	reset_mode_task(nullptr),
	default_font(nullptr), large_font(nullptr),
	show_brightness(false), show_zoom(false),
	show_osd(false), osd()
{
    ENTER();

	// FIXME これだとワーキングディレクトリ(コマンドを実行したときの
	//     カレントディレクトリになってしまってEyeAppの実行ファイルが
	//     存在するディレクトリじゃない)
	//     今のところEyeAppファイルが存在するディレクトリに移動してから
	//     実行しないと正常にアイコンとフォントを読み込めない。
	//     ・argv[0]が相対パスのときはcurrent_pathを追加、
	//     ・argv[0]が絶対パス(/から始まる)ときはそのままで
	//     　最後のEyeAppを除いたパスを使うようにすればいける？
	char current_path[PATH_SIZE];
    getcwd(current_path, PATH_SIZE);
	resources = format("%s/resources", current_path);

	app_settings.load();
	mvp_matrix.scale(ZOOM_FACTORS[zoom_ix]);
	key_dispatcher
		.set_on_key_mode_changed([this](const key_mode_t &key_mode) {
			if (key_mode != KEY_MODE_OSD) {
				reset_mode_delayed();
			} else {
				handler.remove(reset_mode_task);
			}
			on_key_mode_changed(key_mode);
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
		})
		.set_exp_mode_changed([this](const exp_mode_t &exp_mode) {
			request_change_exp_mode(exp_mode);
		})
		.set_osd_key_event([this](const KeyEvent &event) {
			osd.on_key(event);
		});
	// キーイベントハンドラを登録
	window
		.on_key_event([this](const ImGuiKey &key, const int &scancode, const key_action_t &action, const int &mods) {
			if (exit_esc && (key == ImGuiKey_Escape)
				&& (key_dispatcher.get_key_mode() != KEY_MODE_OSD)) {
				// OSDモード以外でESCキーを押したときはアプリを終了させる
				window.terminate();
			}
			return key_dispatcher.handle_on_key_event(KeyEvent(key, scancode, action, mods));
		})
		.set_on_start([this]() { on_start(); })
		.set_on_resume([this]() { on_resume(); })
		.set_on_pause([this]() { on_pause(); })
		.set_on_stop([this]() { on_stop(); });
	// OSD表示が終了したときのコールバック
	osd.set_on_osd_close([this](const bool &changed) {
		LOGD("on_osd_close, changed=%d", changed);
		if (changed) {
			// カメラから設定を読みこんで保存する
			save_settings();
		} else {
			// 一時的に変更されたかもしれないので元の設定に戻す
			apply_settings(camera_settings);
		}
		key_dispatcher.reset_key_mode();
	})
	.set_on_camera_settings_changed([this](const uvc::control_value32_t &value) {
		ENTER();

		int r = -1;
		LOGD("id=0x%08x,v=%d", value.id, value.current);
		if (value.id == V4L2_CID_RESTORE_SETTINGS) {
			LOGD("restore settings");
			restore_settings();
			key_dispatcher.clear();
			r = 0;
		} else {
			// 一時的に設定を適用する
			int32_t val = value.current;
			r = set_ctrl_value(value.id, val);
		}

		RETURN(r, int);
	});
	// 遅延実行タスクの準備, 一定時間後にキーモードをリセットする
	reset_mode_task = [this]() {
		key_dispatcher.reset_key_mode();
	};
	// 遅延実行タスクの準備, 一定時間毎にステートを更新する
	update_state_task = [this]() {
		if (update_state_task) {
			handler.post_delayed(update_state_task, UPDATE_STATE_INTERVAL_MS);
		}
		update_state();
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

	window.start([this]() { on_render(); });

	// ステータス更新開始
	handler.post(update_state_task);

	// ウインドウのイベント処理ループ
    for ( ; window.is_running() /*&& window*/ ; ) {
        // usleep(30000);	//　30ms
		glfwWaitEventsTimeout(0.100);	// 100ms
		// FIXME ここで装着検知センサーの読み込みを行い未装着ならpauseさせる
		const auto attached = true;	// FIXME 未実装
		if (attached && !window.is_resumed()) {
			window.resume();
		} else if (!attached) {
			window.pause();
		}
    }

	handler.remove_all();
	window.stop();

	LOGD("Finished.");

    EXIT();
}

//--------------------------------------------------------------------------------
/*private,@WorkerThread*/
void EyeApp::on_start() {
    ENTER();

	std::string font = format("%s/fonts/DroidSans.ttf", resources.c_str());
	LOGD("resources=%s,font=%s", resources.c_str(), font.c_str());
	// imguiの追加設定
    ImGuiIO& io = ImGui::GetIO();
	// 最初に読み込んだのがデフォルトのフォントになる
	default_font = io.Fonts->AddFontDefault();
	// 大きい文字用のフォント(FIXME 実働時はフォントファイルのパスを変えないといけない)
	large_font = io.Fonts->AddFontFromFileTTF(font.c_str(), MODE_ICON_FONT_SZ);
	// 読み込めなければnulptrになる
	LOGD("default_font=%p", default_font);
	LOGD("large_font=%p", large_font);

	EXIT();
}

/*private,@WorkerThread*/
void EyeApp::on_resume() {
	ENTER();

	frame_wrapper = std::make_unique<core::WrappedVideoFrame>(nullptr, 0);
    source = std::make_unique<v4l2::V4l2Source>(options[OPT_DEVICE].c_str(), !HANDLE_FRAME, options[OPT_UDMABUF].c_str());
#if BUFFURING || HANDLE_FRAME
	const auto versionStr = (const char*)glGetString(GL_VERSION);
	LOGD("GL_VERSION=%s", versionStr);
	// オフスクリーンを生成
	offscreen = std::make_unique<gl::GLOffScreen>(GL_TEXTURE0, width, height, false);
	video_renderer = std::make_unique<core::VideoGLRenderer>(gl_version, 0, false);
#else
	source->set_on_start([this]() {
		// 共有EGL/GLESコンテキストを生成してこのスレッドに割り当てる
		LOGD("create shared context");
		auto display = glfwGetEGLDisplay();
		auto context = glfwGetEGLContext(window.get_window());
		int client_version = 3;
		m_egl = std::make_unique<egl::EGLBase>(client_version, display, context);
		const auto versionStr = (const char*)glGetString(GL_VERSION);
		LOGD("gl_version=%s", versionStr);

		// 共有EGL/GLコンテキスト上でvideo_rendererとimage_rendererを初期化
		video_renderer = std::make_unique<core::VideoGLRenderer>(gl_version, 0, false);
		image_renderer = create_renderer(EFFECT_NON);
		// オフスクリーンを生成
		offscreen = std::make_unique<gl::GLOffScreen>(GL_TEXTURE0, width, height, false);
	})
	.set_on_stop([this]() {
		reset_renderers();
		m_egl.reset();
	});
#endif // #if BUFFURING || HANDLE_FRAME

	source->set_on_error([this]() {
		window.terminate();
	})
	.set_on_frame_ready([this](const uint8_t *image, const size_t &bytes, const v4l2::buffer_t &buf) {
    	MEAS_TIME_INIT

		MEAS_TIME_START
#if BUFFURING
		std::lock_guard<std::mutex> lock(image_lock);
		buffer.resize(width, height, source->get_frame_type());
		memcpy(buffer.frame(), image, bytes);
#else
#if !HANDLE_FRAME
		glFlush();	// XXX これを入れておかないと描画スレッドと干渉して激重になる
		if (LIKELY(m_egl)) {
			m_egl->makeDefault();
		}
#endif	// #if !HANDLE_FRAME

		if (LIKELY(offscreen)) {
			if (!req_freeze) {
				// フリーズ中でなければオフスクリーンテクスチャをカメラ映像で更新する
				const auto try_egl_image = buf.fd != 0;
				auto display = eglGetCurrentDisplay();
				EGLImageKHR egl_image = EGL_NO_IMAGE_KHR;
				if (try_egl_image) {
					// EGLImageKHRを使ったゼロコピーテクスチャでの描画を試みる場合
					// FIXME 未実装 EGLIMageKHRを生成
					egl_image = egl::createEGLImage(display, buf.fd, DRM_FORMAT_UYVY,
						width, height, buf.offset, width);
				}
				if (egl_image) {
					// EGLImageKHRを生成できたとき
					// FIXME 未実装 EGLIMageWrapperでラップ、image_rendererで描画する
					image_wrapper->wrap(egl_image, width, height, width);
					{
						offscreen->bind();
						{
							image_renderer->draw(image_wrapper.get());
						}
						offscreen->unbind();
					}
					image_wrapper->unwrap();
					egl::EGL.eglDestroyImageKHR(display, egl_image);
					EGLCHECK("eglDestroyImageKHR");
				} else if (frame_wrapper && video_renderer) {
					// 4K2Kのディスプレーだとglfwのウインドウが画面全体へ広がらないのに
					// ウインドウサイズとして画面全体を返すのでビューポートの設定がおかしくなって
					// バッファリングありよりカメラ映像の画角が狭くなってしまう
					frame_wrapper->assign(const_cast<uint8_t *>(image), bytes, width, height, source->get_frame_type());
					offscreen->bind();
					{
						video_renderer->draw_frame(*frame_wrapper);
					}
					offscreen->unbind();
				}
			}
		}	// if (LIKELY(offscreen))

#endif // #if BUFFURING

		MEAS_TIME_STOP

		return bytes;
	});	// source->set_on_frame_ready

	if (!source || source->open() || source->find_stream(width, height)) {
		LOGE("カメラをオープンできなかった");
		source.reset();
		window.stop();
		EXIT();
	}

	LOGV("supported=%s", source->get_supported_size().c_str());
	source->resize(width, height);
	if (source->is_ctrl_supported(V4L2_CID_FRAMERATE)) {
		LOGD("set frame rate to 30");
		source->set_ctrl_value(V4L2_CID_FRAMERATE, 30);
	}
	const auto buf_nums = to_int(options[OPT_BUF_NUMS], to_int(OPT_BUF_NUMS_DEFAULT, 4));
	if (source->start(buf_nums)) {
		LOGE("カメラを開始できなかった");
		source.reset();
		window.stop();
		EXIT();
	}

	// カメラ設定を読み込む
	camera_settings.load();
	const bool empty = camera_settings.empty();
	fix_camera_settings(camera_settings);

	if (UNLIKELY(empty)) {
		// カメラ設定が保存されていないとき=初回起動時
		// カメラから設定を読みこんで保存する
		save_settings();
	}
	// カメラ設定を適用
	apply_settings(camera_settings);

	req_change_matrix = true;

	EXIT();
}

/*private,@WorkerThread*/
void EyeApp::on_pause() {
	ENTER();

	if (source) {
		source->stop();
		source.reset();
	}
	reset_renderers();

	EXIT();
}

/*private,@WorkerThread*/
void EyeApp::on_stop() {
	ENTER();

	icon_zoom.reset();
	icon_brightness.reset();

	EXIT();
}

/**
 * @brief 描画スレッドの実行関数
 *
 */
/*private,@WorkerThread*/
void EyeApp::on_render() {
    ENTER();

	MEAS_TIME_INIT

	if (UNLIKELY(!source || !offscreen)) return;

	MEAS_TIME_START
#if HANDLE_FRAME
	// V4L2から映像取得を試みる
	source->handle_frame(10000L);
#endif
	// 描画用の設定更新を適用
	prepare_draw(offscreen, screen_renderer);
#if BUFFURING
	if (!req_freeze) {
		offscreen->bind();
		{
			std::lock_guard<std::mutex> lock(image_lock);
			video_renderer->draw_frame(buffer);
		}
		offscreen->unbind();
	}
#endif
	// 縮小時に古い画面が見えてしまうのを防ぐために塗りつぶす
	glClearColor(0, 0 , 0 , 1.0f);	// RGBA
	glClear(GL_COLOR_BUFFER_BIT);
#if !HANDLE_FRAME
	glFlush();	// XXX これを入れておかないと描画スレッドと干渉して激重になる
#endif	// #if !HANDLE_FRAME

	// 画面へ転送
	handle_draw(offscreen, screen_renderer);

	if (show_fps || show_brightness || show_zoom || show_osd) {
		// GUI(2D)描画処理を実行
		handle_draw_gui();
	}

	reset_watchdog();

	MEAS_TIME_STOP

    EXIT();
}

/**
 * @brief Create a renderer object
 *
 * @param effect
 * @return gl::GLRendererUp
 */
/*private,@WorkerThread*/
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
 * 描画用のオブジェクトやオフスクリーンン等を破棄する
*/
void EyeApp::reset_renderers() {
	ENTER();

	video_renderer.reset();
	frame_wrapper.reset();
	image_renderer.reset();
	image_wrapper.reset();
	offscreen.reset();

	EXIT();
}

/**
 * @brief 描画用の設定更新を適用
 *
 * @param offscreen
 * @param renderer
 */
/*private,@WorkerThread*/
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
		if (current_effect != req_effect_type) {
			current_effect = req_effect_type;
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
 * @brief オフスクリーンを画面表示用に描画処理する
 *
 * @param off
 * @param renderer
 */
/*private,@WorkerThread*/
void EyeApp::handle_draw(gl::GLOffScreenUp &off, gl::GLRendererUp &renderer) {
	ENTER();

	auto r = renderer.get();
	auto o = off.get();
	if (r && o) {
		o->draw(r);
	}

	EXIT();
}

/**
 * @brief GUI(2D)描画処理を実行
 *
 */
/*private,@WorkerThread*/
void EyeApp::handle_draw_gui() {
	ENTER();

	// Start the Dear ImGui frame
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	// ウインドウ位置を指定するにはImGui::Beginの前にImGui::SetNextWindowPosを呼び出す
	// ウインドウサイズを指定するにはImGui::Beginの前にImGui::SetNextWindowSizeを呼び出す
	if (show_fps) {
		ImGui::Begin("DEBUG");
		const auto fps = ImGui::GetIO().Framerate;
		ImGui::Text("%.3f ms/frame(%.1f FPS)", 1000.0f / fps, fps);
		ImGui::End();
	}
	// static bool show_demo = true;
	// ImGui::ShowDemoWindow(&show_demo);

	const static ImVec2 icon_size(MODE_ICON_SZ, MODE_ICON_SZ);
	const static ImVec2 pivot(0.5f, 0.5f);
	const static ImVec2 uv_min = ImVec2(0.0f, 0.0f);                 // Top-left
    const static ImGuiWindowFlags window_flags
		= ImGuiWindowFlags_NoDecoration
		| ImGuiWindowFlags_NoMove
		| ImGuiWindowFlags_NoTitleBar
		| ImGuiWindowFlags_AlwaysAutoResize
		| ImGuiWindowFlags_NoSavedSettings
		| ImGuiWindowFlags_NoFocusOnAppearing
		| ImGuiWindowFlags_NoNav;

	if (show_brightness) {
		if (UNLIKELY(!icon_brightness)) {
			std::string icon = format("%s/ic_brightness.png", resources.c_str());
			LOGD("resources=%s,icon=%s", resources.c_str(), icon.c_str());
			media::Image bitmap;
			if (!media::read_png_from_file(bitmap, icon.c_str())) {
				LOGD("success load png, assign to texture");
				//  輝度アイコンをテクスチャへ読み込む
				icon_brightness = std::make_unique<gl::GLTexture>(GL_TEXTURE_2D, GL_TEXTURE0, MODE_ICON_TEX_SZ, MODE_ICON_TEX_SZ);
				icon_brightness->assignTexture(bitmap.data());
			}
		}
        ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Always, pivot);
		ImGui::SetNextWindowBgAlpha(MODE_BK_ALPHA);
		ImGui::Begin("Brightness", &show_brightness, window_flags);
		if (LIKELY(icon_brightness)) {
			// デフォルトだとテクスチャサイズを2のべき乗に繰り上げるのでイメージサイズよりテクスチャのほうが大きい可能性があるため左下UV座標を調整
			const ImVec2 uv_max = ImVec2(icon_brightness->getTexScaleX(), icon_brightness->getTexScaleY());	// Lower-right
			icon_brightness->bind();
			ImGui::Image(reinterpret_cast <void*>(icon_brightness->getTexture()), icon_size, uv_min, uv_max);
			icon_brightness->unbind();
		}
		if (LIKELY(large_font)) {
			ImGui::SameLine();
			ImGui::PushFont(large_font);
			ImGui::Text("%d", brightness_ix/*FIXME 暫定値*/);
			ImGui::PopFont();
		}
		ImGui::End();
	}
	if (show_zoom) {
		if (UNLIKELY(!icon_zoom)) {
			std::string icon = format("%s/ic_zoom.png", resources.c_str());
			LOGD("resources=%s,icon=%s", resources.c_str(), icon.c_str());
			media::Image bitmap;
			if (!media::read_png_from_file(bitmap, icon.c_str())) {
				LOGD("success load png, assign to texture");
				// 拡大縮小アイコンをテクスチャへ読み込む
				icon_zoom = std::make_unique<gl::GLTexture>(GL_TEXTURE_2D, GL_TEXTURE1, MODE_ICON_TEX_SZ, MODE_ICON_TEX_SZ);
				icon_zoom->assignTexture(bitmap.data());
			}
		}
        ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Always, pivot);
		ImGui::SetNextWindowBgAlpha(MODE_BK_ALPHA);
		ImGui::Begin("Zoom", &show_zoom, window_flags);
		if (LIKELY(icon_zoom)) {
			// デフォルトだとテクスチャサイズを2のべき乗に繰り上げるのでイメージサイズよりテクスチャのほうが大きい可能性があるため左下UV座標を調整
			const ImVec2 uv_max = ImVec2(icon_zoom->getTexScaleX(), icon_zoom->getTexScaleY());	// Lower-right
			icon_zoom->bind();
			ImGui::Image(reinterpret_cast <void*>(icon_zoom->getTexture()), icon_size, uv_min, uv_max);
			icon_zoom->unbind();
		}
		if (LIKELY(large_font)) {
			ImGui::SameLine();
			const auto factor = ZOOM_FACTORS[zoom_ix];
			ImGui::PushFont(large_font);
			ImGui::Text("%3.1f", factor);
			ImGui::PopFont();
		}
		ImGui::End();
	}
	if (show_osd) {
		osd.draw(large_font);
	}

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
	handler.post_delayed(reset_mode_task, KEY_MODE_RESET_DELAY_MS);
}

/**
 * @brief ウオッチドッグをリセット要求
 *
 */
void EyeApp::reset_watchdog() {
	static uint32_t cnt = 0;
	if ((++cnt % RESET_WATCHDOG_CNT) == 0) {
		// FIXME 未実装 実際のウオッチドッグリセット処理
	}
}

/**
 * @brief ステータス(LED点滅等)を更新
 *
 */
void EyeApp::update_state() {
	ENTER();

	LOGV("%ld", systemTime());
	// FIXME 未実装

	EXIT();
}

//--------------------------------------------------------------------------------
/**
 * 設定を復元させる
 * SUPPORTED_CTRLSに含まれるV4L2コントロールidのうち機器側で対応しているものについて
 * デフォルト値へ戻す
*/
void EyeApp::restore_settings() {
	ENTER();

	if (LIKELY(source)) {
		// camera_settingsで保持している値をデフォルト値に変更する
		camera_settings.clear();
		for (const auto id: SUPPORTED_CTRLS) {
			if (source->is_ctrl_supported(id)) {
				uvc::control_value32_t val;
				auto r = source->get_ctrl(id, val);
				if (LIKELY(!r)) {
					LOGD("set id=0x%08x,def=%d", id, val.def);
					camera_settings.set_value(id, val.def);
				}
			}
		}
		// デフォルトの設定だとよろしくないのを修正する
		fix_camera_settings(camera_settings, true);
		// カメラ設定を適用
		apply_settings(camera_settings);
	}

	EXIT();
}

/**
 * カメラから設定を読み込んで保存する
*/
void EyeApp::save_settings() {
	ENTER();

	if (LIKELY(source)) {
		// camera_settingsをカメラの現在設定値に変更する
		camera_settings.clear();
		for (const auto id: SUPPORTED_CTRLS) {
			if (source->is_ctrl_supported(id)) {
				uvc::control_value32_t val;
				auto r = source->get_ctrl(id, val);
				if (LIKELY(!r)) {
					LOGD("set id=0x%08x,cur=%d", id, val.current);
					camera_settings.set_value(id, val.current);
				}
			}
		}
		// カメラ設定を保存
		camera_settings.save();
	}

	EXIT();
}

/**
 * カメラ設定のデフォルト値を修正する
 * @param settings
 * @param force true: 強制的に上書きする
*/
void EyeApp::fix_camera_settings(CameraSettings &settings, const bool &force) {
	ENTER();

	if (source) {
		if (source->is_ctrl_supported(V4L2_CID_POWER_LINE_FREQUENCY)) {
			// 電源周波数設定(フリッカー抑制設定)に対応していている場合
			if (force || !settings.contains(V4L2_CID_POWER_LINE_FREQUENCY)) {
				// カメラ設定に値がないかforce指定なら3:AUTOを試みる
				int32_t val = 3;
				if (!set_ctrl_value(V4L2_CID_POWER_LINE_FREQUENCY, val)) {
					settings.set_value(V4L2_CID_POWER_LINE_FREQUENCY, val);
				}
			}
		}

		if (source->is_ctrl_supported(V4L2_CID_AUTO_N_PRESET_WHITE_BALANCE)) {
			// オートホワイトバランス設定に対応していている場合
			if (force || !settings.contains(V4L2_CID_AUTO_N_PRESET_WHITE_BALANCE)) {
				// カメラ設定に値がないかforce指定なら1:AUTOを試みる
				int32_t val = 1;
				if (!set_ctrl_value(V4L2_CID_AUTO_N_PRESET_WHITE_BALANCE, val)) {
					settings.set_value(V4L2_CID_AUTO_N_PRESET_WHITE_BALANCE, val);
				}
			}
		}

		if (source->is_ctrl_supported(V4L2_CID_DENOISE)) {
			// ノイズ除去に対応している場合
			if (force || !settings.contains(V4L2_CID_DENOISE)) {
				// カメラ設定に値がないかforce指定なら1:OFFを試みる
				int32_t val = 1;
				if (!set_ctrl_value(V4L2_CID_DENOISE, val)) {
					settings.set_value(V4L2_CID_DENOISE, val);
				}
			}
		}

		if (source->is_ctrl_supported(V4L2_CID_EXPOSURE_AUTO)) {
			// 自動露出に対応している場合
			if (force || !settings.contains(V4L2_CID_EXPOSURE_AUTO)) {
				// カメラ設定に値がないかforce指定なら0:Autoを試みる
				int32_t val = 0;
				if (!set_ctrl_value(V4L2_CID_EXPOSURE_AUTO, val)) {
					settings.set_value(V4L2_CID_EXPOSURE_AUTO, val);
				} else {
					// 0:Autoがだめなときはデフォルト値にする
					uvc::control_value32_t ctrl;
					auto r = source->get_ctrl(V4L2_CID_EXPOSURE_AUTO, ctrl);
					if (!r && !set_ctrl_value(V4L2_CID_EXPOSURE_AUTO, ctrl.def)) {
						settings.set_value(V4L2_CID_EXPOSURE_AUTO, ctrl.def);
					}
				}
			}
		}

		if (settings.is_modified()) {
			// 変更されていれば保存する
			settings.save();
		}
	}	// if (source)

	EXIT();
}

/**
 * @brief カメラ設定を適用する
 * 
 * @param settings 
 */
void EyeApp::apply_settings(CameraSettings &settings) {
	ENTER();

	if (LIKELY(source)) {
		// フレームレートは30fps固定にする
		settings.remove(V4L2_CID_FRAMERATE);
		if (source->is_ctrl_supported(V4L2_CID_FRAMERATE)) {
			LOGD("set frame rate to 30");
			source->set_ctrl_value(V4L2_CID_FRAMERATE, 30);
		}
		for (const auto id: SUPPORTED_CTRLS) {
			int32_t val;
			auto r = settings.get_value(id, val);
			if (!r) {
				r = !source->is_ctrl_supported(id);
				if (!r) {
					bool apply = true;
					switch (id) {
					case V4L2_CID_BRIGHTNESS:
						apply = !settings.is_auto_brightness();
						break;
					case V4L2_CID_HUE:
						apply = !settings.is_auto_hue();
						break;
					case V4L2_CID_GAIN:
						apply = !settings.is_auto_gain();
						break;
					case V4L2_CID_EXPOSURE_ABSOLUTE:
						apply = !settings.is_auto_exposure();
						break;
					case V4L2_CID_WHITE_BALANCE_TEMPERATURE:
						apply = !settings.is_auto_white_blance();
						break;
					default:
						break;
					}
					if (apply) {
						LOGD("try set value,id=0x%08x,val=%d", id, val);
						r = set_ctrl_value(id, val);
						if (!r) {
							LOGD("current=%d", val);
							settings.set_value(id, val);
						}
					}
				} else {
					settings.remove(id);
				}
			}
		}
		if (settings.is_modified()) {
			// 変更されていれば保存する
			settings.save();
		}
	}

	EXIT();
}

/**
 * 設定をカメラへ適用する
 * 正常に適用できた場合はvalに実際に設定された値が入って返る
 * @param id
 * @param val
 * @return int 0: 適用できた
*/
int EyeApp::set_ctrl_value(const uint32_t &id, int32_t &val) {
	ENTER();

	int result = -1;
	if (source) {
		result = source->set_ctrl_value(id, val);
		if (!result) {
			result = source->get_ctrl_value(id, val);
		} else {
			LOGW("failed to apply id=0x%08x,value=%d", id, val);
		}
	}

	RETURN(result, int);
}

//--------------------------------------------------------------------------------
/**
 * @brief キーモード変更時の処理
 *
 * @param key_mode
 */
void EyeApp::on_key_mode_changed(const key_mode_t &key_mode) {
	ENTER();

	LOGD("key_mode=%d", key_mode);
	switch (key_mode) {
	case KEY_MODE_BRIGHTNESS:
		show_brightness = true;
		show_zoom = show_osd = false;
		break;
	case KEY_MODE_ZOOM:
		show_brightness = show_osd = false;
		show_zoom = true;
		break;
	case KEY_MODE_OSD:
		show_brightness = show_zoom = false;
		osd.prepare(source);
		show_osd = true;
		break;
	case KEY_MODE_NORMAL:
	default:
		show_brightness = show_zoom = show_osd = false;
		break;
	}

	EXIT();
}

/**
 * @brief 輝度変更要求
 *
 * @param inc_dec trueなら輝度増加、falseなら輝度減少
 */
void EyeApp::request_change_brightness(const bool &inc_dec) {
	ENTER();

	std::lock_guard<std::mutex> lock(state_lock);
	int ix = brightness_ix + (inc_dec ? 1 : -1);
	if (ix <= 1) {
		ix = 1;
	} else if (ix > 10) {
		ix = 10;
	}
	if (ix != brightness_ix) {
		LOGD("ix=%d", ix);
		brightness_ix = ix;
		// FIXME 実際の輝度変更処理は未実装
	}


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
	} else if (ix >= NUM_ZOOM_FACTORS) {
		ix = NUM_ZOOM_FACTORS - 1;
	}
	if (ix != zoom_ix) {
		LOGD("ix=%d", ix);
		zoom_ix = ix;
		auto factor = ZOOM_FACTORS[ix];
		mvp_matrix.setScale(factor, LENSE_FACTOR * factor, 1.0f);
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
	const bool changed = req_effect_type != effect;
	req_effect_type = effect;
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

/**
 * @brief 測光モード切替要求
 *
 * @param exp_mode
 */
void EyeApp::request_change_exp_mode(const exp_mode_t &exp_mode) {
	ENTER();

	LOGD("exp_mode=%d", exp_mode);
	// FIXME 実際の測光モード切り替え処理は未実装

	EXIT();
}

}   // namespace serenegiant::app

