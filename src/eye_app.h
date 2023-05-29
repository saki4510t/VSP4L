#ifndef EYE_APP_H_
#define EYE_APP_H_

#include <thread>
#include <mutex>
#include <unordered_map>

// common
#include "handler.h"
#include "gloffscreen.h"
#include "glrenderer.h"
#include "gltexture.h"
#include "matrix.h"
// aandusb/pipeline
#include "pipeline/pipeline_gl_renderer.h"
#include "pipeline/pipeline_v4l2_source.h"

#include "const.h"
#include "key_event.h"
#include "key_dispatcher.h"
#include "window.h"

namespace pipeline = serenegiant::pipeline;
namespace v4l2_pipeline = serenegiant::v4l2::pipeline;

namespace serenegiant::app {

class EyeApp {
private:
	const int gl_version;
	const bool initialized;
	// 非同期実行用Handler
	thread::Handler handler;
	// 一定時間後にキーモードをリセットするタスク
	std::function<void()> reset_mode_task;
	// GLFWでの画面表示用
	Window window;
	// V4L2からの映像取得用
	v4l2_pipeline::V4L2SourcePipelineUp source;
	// 映像表示用
	pipeline::GLRendererPipelineUp renderer_pipeline;
	// 拡大縮小・映像効果付与・フリーズ用オフスクリーン
	gl::GLOffScreenUp offscreen;
	// オフスクリーン描画用
	gl::GLRendererUp gl_renderer;
	// 排他制御用
	mutable std::mutex state_lock;
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
	effect_t effect_type;
	effect_t current_effect;
	// モデルビュー変換行列
	math::Matrix mvp_matrix;
	// 拡大縮小インデックス[0,NUM_ZOOM_FACTOR)
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
	 * @brief OSD表示のON/OFF切り替え要求
	 * 
	 * @param onoff 
	 */
	void request_change_osd(const bool &onoff);
	/**
	 * @brief OSD表示中のキーイベント
	 * 
	 * @param event 
	 */
	void on_osd_key(const KeyEvent &event);

	/**
	 * @brief Create a renderer object
	 * 
	 * @param effect 
	 * @return gl::GLRendererUp 
	 */
	gl::GLRendererUp create_renderer(const effect_t &effect);
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
	 *
	 */
#if USE_GL3
	EyeApp(const int &gl_version = 300);
#else
	EyeApp(const int &gl_version = 200);
#endif
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
