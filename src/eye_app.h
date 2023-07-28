#ifndef EYE_APP_H_
#define EYE_APP_H_

// 映像のバッファリングを行うかどうか
// 0: バッファリングを行わない(V4L2スレッドの共有コンテキスト上でテクスチャへ転送)
// 1: バッファリングを行う(V4L2スレッドでバッファへコピー、描画スレッドでレンダリング)
#define BUFFURING (0)

#include <thread>
#include <mutex>
#include <unordered_map>

// common
#include "eglbase.h"
#include "handler.h"
#include "gloffscreen.h"
#include "glrenderer.h"
#include "gltexture.h"
#include "matrix.h"
// core
#include "core/video_frame_wrapped.h"

#include "const.h"

#include "v4l2/v4l2_source.h"
#include "core/video_gl_renderer.h"

#if BUFFURING
	#include "core/video_frame_base.h"
#endif

#include "key_event.h"
#include "key_dispatcher.h"
#include "osd.h"
#include "settings.h"
#include "window.h"

#include "glfw_window.h"

namespace serenegiant::app {

#if USE_GL3
#define DEFAULT_GL_VERSION (300)
#else
#define DEFAULT_GL_VERSION (200)
#endif

class EyeApp {
private:
	std::unordered_map<std::string, std::string> options;
	const int gl_version;
	const bool initialized;
	const bool show_debug;
	const uint32_t width;
	const uint32_t height;
	std::string resources;
	AppSettings app_settings;
	CameraSettings camera_settings;

	// 非同期実行用Handler
	thread::Handler handler;
	// 一定時間後にキーモードをリセットするタスク
	std::function<void()> reset_mode_task;
	// 一定時間毎にステータス(LED点滅等)を更新するタスク
	std::function<void()> update_state_task;
	// GLFWでの画面表示用
	GlfwWindow window;
	// V4L2からの映像取得用
	v4l2::V4l2SourceUp source;
	egl::EGLBaseUp m_egl;

	core::WrappedVideoFrameUp frame_wrapper;
	core::VideoGLRendererUp video_renderer;
	gl::GLRendererUp image_renderer;
	egl::EglImageWrapperUp image_wrapper;
#if BUFFURING
	core::BaseVideoFrame buffer;
#endif
	// オフスクリーン描画用
	gl::GLRendererUp screen_renderer;
	// 拡大縮小・映像効果付与・フリーズ用オフスクリーン
	gl::GLOffScreenUp offscreen;
	// 排他制御用
	mutable std::mutex state_lock;
	mutable std::mutex image_lock;
	/**
	 * @brief キー操作用
	 * 
	 */
	KeyDispatcher key_dispatcher;
	// 映像効果変更要求
	volatile bool req_change_effect;
	// モデルビュー変換行列変更要求
	volatile bool req_change_matrix;
	// 映像フリーズ要求
	volatile bool req_freeze;
	effect_t req_effect_type;
	effect_t current_effect;
	// モデルビュー変換行列
	math::Matrix mvp_matrix;
	// 拡大縮小インデックス[0,NUM_ZOOM_FACTORS)
	int zoom_ix;
	// 輝度インデックス[1,10]
	int brightness_ix;
	// デフォルトのフォント(このポインターはImGuiIO側で管理しているので自前で破棄しちゃだめ)
	ImFont *default_font;
	// 大きな文字用のフォント(このポインターはImGuiIO側で管理しているので自前で破棄しちゃだめ)
	ImFont *large_font;
	// 輝度変更モード用
	bool show_brightness;
	gl::GLTextureSp icon_brightness;
	// 拡大縮小モード用
	bool show_zoom;
	gl::GLTextureSp icon_zoom;
	// OSDモード用
	bool show_osd;
	OSD osd;

	/**
	 * @brief 一定時間後にキーモードをリセットする
	 * 
	 */
	void reset_mode_delayed();
	/**
	 * @brief ウオッチドッグをリセット要求
	 * 
	 */
	void reset_watchdog();
	/**
	 * @brief ステータス(LED点滅等)を更新
	 * 
	 */
	void update_state();
	/**
	 * @brief カメラ設定を適用する
	 * 
	 * @param settings 
	 */
	void apply_settings(const CameraSettings &settings);

	/**
	 * @brief キーモード変更時の処理
	 * 
	 * @param mode 
	 */
	void on_key_mode_changed(const key_mode_t &key_mode);
	/**
	 * @brief 輝度変更要求
	 * 
	 * @param inc_dec 
	 */
	void request_change_brightness(const bool &inc_dec);
	/**
	 * @brief 拡大縮小率変更要求
	 * 
	 * @param inc_dec trueなら拡大、falseなら縮小
	 */
	void request_change_scale(const bool &inc_dec);
	/**
	 * @brief 映像効果変更要求
	 * 
	 * @param effect 
	 */
	void request_change_effect(const effect_t &effect);
	/**
	 * @brief 映像フリーズのON/OFF切り替え要求
	 * 
	 * @param onoff 
	 */
	void request_change_freeze(const bool &onoff);
	/**
	 * @brief 測光モード切替要求
	 * 
	 * @param exp_mode 
	 */
	void request_change_exp_mode(const exp_mode_t &exp_mode);

	/**
	 * @brief Create a renderer object
	 * 
	 * @param effect 
	 * @return gl::GLRendererUp 
	 */
	gl::GLRendererUp create_renderer(const effect_t &effect);
	/**
	 * 描画用のオブジェクトやオフスクリーンン等を破棄する
	*/
	void reset_renderers();
	/**
	 * @brief 描画用の設定更新を適用
	 * 
	 * @param offscreen 
	 * @param gl_renderer 
	 */
	void prepare_draw(gl::GLOffScreenUp &offscreen, gl::GLRendererUp &renderer);
	/**
	 * @brief 描画処理を実行
	 *
	 * @param renderer
	 */
	void handle_draw(gl::GLOffScreenUp &offscreen, gl::GLRendererUp &renderer);
	/**
	 * @brief GUI(2D)描画処理を実行
	 * 
	 */
	void handle_draw_gui();
	
	/**
	 * @brief 描画開始時の追加処理, Windowのレンダリングスレッド上で実行される
	 *
	 */
	void on_start();
	/**
	 * @brief 画面が表示されたときの追加処理, Windowのレンダリングスレッド上で実行される
	 * 
	 */
	void on_resume();
	/**
	 * @brief 画面が消灯するときの追加処理, Windowのレンダリングスレッド上で実行される
	 * 
	 */
	void on_pause();
	/**
	 * @brief 描画終了時の追加処理, Windowのレンダリングスレッド上で実行される
	 * 
	 */
	void on_stop();
	/**
	 * @brief 描画処理, Windowのレンダリングスレッド上で実行される
	 *
	 */
	void on_render();
protected:
public:
	/**
	 * @brief コンストラクタ
	 * @param options
	 * @param gl_version
	 *
	 */
	EyeApp(std::unordered_map<std::string, std::string> options,
		const int &gl_version = DEFAULT_GL_VERSION);

	/**
	 * @brief デストラクタ
	 *
	 */
	~EyeApp();
	/**
	 * @brief アプリを実行する
	 *        実行終了までこの関数を抜けないので注意
	 *
	 */
	void run();

	inline bool is_initialized() const { return initialized; };
};

}   // namespace serenegiant::app

#endif /* EYE_APP_H_ */
