#ifndef EYE_APP_H_
#define EYE_APP_H_

#include <thread>
#include <mutex>
#include <unordered_map>

// common
#include "handler.h"
#include "gloffscreen.h"
#include "glrenderer.h"
#include "matrix.h"
// aandusb/pipeline
#include "pipeline/pipeline_gl_renderer.h"

#include "const.h"
#include "key_event.h"
#include "key_dispatcher.h"

namespace pipeline = serenegiant::pipeline;

namespace serenegiant::app {
class Window;

class EyeApp {
private:
	const int gl_version;
	const bool initialized;
	// 実行中フラグ
	volatile bool is_running;
	// 映像効果変更要求
	volatile bool req_change_effect;
	// モデルビュー変換行列変更要求
	volatile bool req_change_matrix;
	// 映像フリーズ要求
	volatile bool req_freeze;
	// 非同期実行用Handler
	thread::Handler handler;
	// Handlerの動作テスト用
	std::function<void()> test_task;
	mutable std::mutex state_lock;
	effect_t effect_type;
	/**
	 * @brief キー操作用
	 * 
	 */
	KeyDispatcher key_dispatcher;
	// モデルビュー変換行列
	math::Matrix mvp_matrix;
	// 拡大縮小インデックス[0,NUM_ZOOM_FACTOR)
	int zoom_ix;

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
	 * @param effect 
	 */
	void request_change_freeze(const bool &onoff);

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
	void prepare_draw(gl::GLOffScreenUp &offscreen, gl::GLRendererUp &renderer, effect_t &current_effect);
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
	 * @brief 描画スレッドの実行関数
	 *
	 */
	void renderer_thread_func();
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
