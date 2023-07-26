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
#endif

#define LOG_TAG "EyeApp"

#define MEAS_TIME (0)				// 1フレーム当たりの描画時間を測定する時1

#include <stdio.h>
#include <string>
#include <cstring>
#include <sstream>
#include <iterator>	// istream_iterator

#include "utilbase.h"
// common
#include "charutils.h"
#include "glutils.h"
#include "image_helper.h"
#include "times.h"
#include "eglbase.h"
// gl
#include "gl/texture_vsh.h"
#include "gl/rgba_fsh.h"
// app
#include "internal.h"
#include "effect_fsh.h"
#include "eye_app.h"

namespace serenegiant::app {

// handle_drawの呼び出し回数をカウントするかどうか 0:カウントしない、1:カウントする
#define COUNT_FRAMES (0)
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
	debug(options.find(OPT_DEBUG) != options.end()),
	width(to_int(options[OPT_WIDTH], to_int(OPT_WIDTH_DEFAULT, 1920))),
	height(to_int(options[OPT_HEIGHT], to_int(OPT_HEIGHT_DEFAULT, 1080))),
	app_settings(), camera_settings(),
	window(width, height, "BOV EyeApp"),
	source(nullptr),
	m_egl_display(EGL_NO_DISPLAY),
	m_shared_context(EGL_NO_CONTEXT), m_egl_surface(EGL_NO_SURFACE),
	m_sync(EGL_NO_SYNC_KHR),
	dynamicEglCreateSyncKHR(nullptr), dynamicEglDestroySyncKHR(nullptr),
	dynamicEglSignalSyncKHR(nullptr), dynamicEglWaitSyncKHR(nullptr),
	video_renderer(nullptr),
	offscreen(nullptr), gl_renderer(nullptr),
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

	load(app_settings);
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
			// FIXME 未実装 設定を再ロードして適用する
		} else {
			// 一時的に変更されたかもしれないので元の設定に戻す
			apply_settings(camera_settings);
		}
		key_dispatcher.reset_key_mode();
	})
	.set_on_camera_settings_changed([this](const CameraSettings &settings) {
		// 一時的に設定を適用する
		apply_settings(settings);
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
	// ステータス更新開始
	handler.post(update_state_task);

	// ウインドウのイベント処理ループ
    for ( ; window.is_running() && window ; ) {
        usleep(30000);	//　30ms
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

	// カメラ設定を読み込む
	load(camera_settings);
    source = std::make_unique<v4l2::V4l2Source>(options[OPT_DEVICE].c_str(), !HANDLE_FRAME, options[OPT_UDMABUF].c_str());
#if BUFFURING || HANDLE_FRAME
	video_renderer = std::make_unique<core::VideoGLRenderer>(gl_version, 0, false);
	frame_wrapper = std::make_unique<core::WrappedVideoFrame>(nullptr, 0);
#else
	source->set_on_start([this]() {
		// 共有EGL/GLESコンテキストを生成してこのスレッドに割り当てる
		LOGD("create shared context");
		m_egl_display = glfwGetEGLDisplay();
		auto context = glfwGetEGLContext(window.get_window());
		EGLConfig config;
		int client_version = 3;
		m_shared_context = egl::createEGLContext(m_egl_display, config, client_version, context);
		LOGD("shared context gles%d", client_version);

		// 対応しているEGL拡張を解析
		std::istringstream eglext_stream(eglQueryString(m_egl_display, EGL_EXTENSIONS));
		auto extensions = std::set<std::string> {
			std::istream_iterator<std::string> {eglext_stream},
			std::istream_iterator<std::string>{}
		};

		if (extensions.find("EGL_KHR_surfaceless_context") == extensions.end()) {
			// GLコンテキストを保持するためにサーフェースが必要な場合は1x1のオフスクリーンサーフェースを生成
			const EGLint surface_attrib_list[] = {
				EGL_WIDTH, 1,
				EGL_HEIGHT, 1,
				EGL_NONE
			};
			m_egl_surface = eglCreatePbufferSurface(m_egl_display, config, surface_attrib_list);
			EGLCHECK("eglCreatePbufferSurface");
		}
		eglMakeCurrent(m_egl_display, m_egl_surface, m_egl_surface, m_shared_context);
		dynamicEglCreateSyncKHR = (PFNEGLCREATESYNCKHRPROC)eglGetProcAddress("eglCreateSyncKHR");
		if (!dynamicEglCreateSyncKHR) {
			LOGW("eglCreateSyncKHR is not available!");
		}
		dynamicEglDestroySyncKHR = (PFNEGLDESTROYSYNCKHRPROC) eglGetProcAddress("eglDestroySyncKHR");
		if (!dynamicEglDestroySyncKHR) {
			LOGW("eglDestroySyncKHR is not available!");
		}
		dynamicEglSignalSyncKHR = (PFNEGLSIGNALSYNCKHRPROC)eglGetProcAddress("eglSignalSyncKHR");
		if (!dynamicEglSignalSyncKHR) {
			LOGW("eglSignalSyncKHR is not available!");
		}
		dynamicEglWaitSyncKHR = (PFNEGLWAITSYNCKHRPROC)eglGetProcAddress("eglWaitSyncKHR");
		if (!dynamicEglWaitSyncKHR) {
			LOGW("eglWaitSyncKHR is not available!");
		}
		if (dynamicEglCreateSyncKHR && dynamicEglDestroySyncKHR
			&& dynamicEglSignalSyncKHR && dynamicEglWaitSyncKHR) {

			LOGD("create sync");
			m_sync = dynamicEglCreateSyncKHR(m_egl_display, EGL_SYNC_TYPE_KHR, nullptr);
		}

		video_renderer = std::make_unique<core::VideoGLRenderer>(300, 0, false);
		frame_wrapper = std::make_unique<core::WrappedVideoFrame>(nullptr, 0);
		const auto versionStr = (const char*)glGetString(GL_VERSION);
		LOGD("gl_version=%s", versionStr);
		// オフスクリーンを生成
		offscreen = std::make_unique<gl::GLOffScreen>(GL_TEXTURE0, width, height, false);
	})
	.set_on_stop([this]() {
		video_renderer.reset();
		frame_wrapper.reset();
#if !BUFFURING
		offscreen.reset();
#endif
		m_egl_display = EGL_NO_DISPLAY;
		if (m_shared_context != EGL_NO_CONTEXT) {
			// 共有EGL/GLESコンテキストを破棄
			auto display = glfwGetEGLDisplay();
			if (m_sync != EGL_NO_SYNC_KHR) {
				LOGD("release sync");
				dynamicEglDestroySyncKHR(display, m_sync);
			}
			m_sync = EGL_NO_SYNC_KHR;
			eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, m_shared_context);
			if (m_egl_surface) {
				LOGD("release surface");
				eglDestroySurface(display, m_egl_surface);
				m_egl_surface = EGL_NO_SURFACE;
			}
			LOGD("release shared context");
			eglDestroyContext(display, m_shared_context);
			m_shared_context = EGL_NO_CONTEXT;
		}
	})
	.set_on_error([this]() {
		window.terminate();
	});
#endif // #if BUFFURING
	source->set_on_frame_ready([this](const uint8_t *image, const size_t &bytes) {
    MEAS_TIME_INIT

	MEAS_TIME_START
#if BUFFURING
		std::lock_guard<std::mutex> lock(image_lock);
		buffer.resize(VIDEO_WIDTH, VIDEO_HEIGHT, source->get_frame_type());
		memcpy(buffer.frame(), image, bytes);
#else
#if !HANDLE_FRAME
		if (LIKELY(m_sync != EGL_NO_SYNC_KHR)) {
			dynamicEglWaitSyncKHR(m_egl_display, m_sync, 0);
			dynamicEglSignalSyncKHR(m_egl_display, m_sync, EGL_UNSIGNALED_KHR);
		} else {
			glFlush();	// XXX これを入れておかないと描画スレッドと干渉して激重になる
		}
		if (m_egl_surface) {
			eglMakeCurrent(m_egl_display, m_egl_surface, m_egl_surface, m_shared_context);
			glClearColor(0, 0, 0, 1);
		}
#endif

		if (LIKELY(frame_wrapper && offscreen && video_renderer)) {
#if COUNT_FRAMES && !defined(LOG_NDEBUG) && !defined(NDEBUG)
			static int cnt = 0;
			if (++cnt % 120 == 0) LOGD("cnt=%d", cnt);
#endif
			if (!req_freeze) {
				frame_wrapper->assign(const_cast<uint8_t *>(image), bytes, width, height, source->get_frame_type());
				offscreen->bind();
				video_renderer->draw_frame(*frame_wrapper);
				offscreen->unbind();
			}
		}

#if !HANDLE_FRAME
		if (LIKELY(m_sync != EGL_NO_SYNC_KHR)) {
			dynamicEglSignalSyncKHR(m_egl_display, m_sync, EGL_SIGNALED_KHR);
		}
		if (m_egl_surface) {
			const auto ret = eglSwapBuffers(m_egl_display, m_egl_surface);
			if (UNLIKELY(!ret)) {
				int err = eglGetError();
				LOGW("eglSwapBuffers:err=%d", err);
			}
		}
#endif // #if !HANDLE_FRAME
#endif // #if BUFFURING

		MEAS_TIME_STOP

		return bytes;
	});

	if (!source || source->open() || source->find_stream(width, height)) {
		LOGE("カメラをオープンできなかった");
		source.reset();
		window.stop();
		EXIT();
	}
	LOGV("supported=%s", source->get_supported_size().c_str());
	source->resize(width, height);
	const auto buf_nums = to_int(options[OPT_BUF_NUMS], to_int(OPT_BUF_NUMS_DEFAULT, 4));
	if (source->start(buf_nums)) {
		LOGE("カメラを開始できなかった");
		source.reset();
		window.stop();
		EXIT();
	}

	// カメラ設定を適用
	apply_settings(camera_settings);

#if BUFFURING || HANDLE_FRAME
	const auto versionStr = (const char*)glGetString(GL_VERSION);
	LOGD("GL_VERSION=%s", versionStr);
	// オフスクリーンを生成
	offscreen = std::make_unique<gl::GLOffScreen>(GL_TEXTURE0, VIDEO_WIDTH, VIDEO_HEIGHT, false);
#endif
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

#if BUFFURING || HANDLE_FRAME
	video_renderer.reset();
	frame_wrapper.reset();
	offscreen.reset();
#endif

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
	prepare_draw(offscreen, gl_renderer);
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
	if (LIKELY(m_sync != EGL_NO_SYNC_KHR)) {
		dynamicEglWaitSyncKHR(m_egl_display, m_sync, 0);
		dynamicEglSignalSyncKHR(m_egl_display, m_sync, EGL_UNSIGNALED_KHR);
	} else {
		glFlush();	// XXX これを入れておかないと描画スレッドと干渉して激重になる
	}
#endif
	// 画面へ転送
	handle_draw(offscreen, gl_renderer);
	// GUI(2D)描画処理を実行
	handle_draw_gui();
	reset_watchdog();

	if (LIKELY(m_sync != EGL_NO_SYNC_KHR)) {
		dynamicEglSignalSyncKHR(m_egl_display, m_sync, EGL_SIGNALED_KHR);
	}

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
 * @brief 描画用の設定更新を適用
 *
 * @param offscreen
 * @param gl_renderer
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
 * @brief 描画処理を実行
 *
 * @param renderer
 */
/*private,@WorkerThread*/
void EyeApp::handle_draw(gl::GLOffScreenUp &offscreen, gl::GLRendererUp &renderer) {
	ENTER();

#if COUNT_FRAMES && !defined(LOG_NDEBUG) && !defined(NDEBUG)
    static int cnt = 0;
    if ((++cnt % 100) == 0) {
        MARK("cnt=%d", cnt);
    }
#endif
	auto r = renderer.get();
	if (r && offscreen) {
		offscreen->draw(renderer.get());
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
	if (debug) {
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
	handler.post_delayed(reset_mode_task, 5000);
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

/**
 * @brief カメラ設定を適用する
 * 
 * @param settings 
 */
void EyeApp::apply_settings(const CameraSettings &settings) {
	ENTER();

	if (LIKELY(source)) {
		// FIXME 未実装
	}

	EXIT();
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
		osd.prepare();
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

