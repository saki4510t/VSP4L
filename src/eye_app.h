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

namespace pipeline = serenegiant::pipeline;

namespace serenegiant::app {
class Window;
class LongPressCheckTask;

/**
 * @brief キー操作モード定数
 * 
 */
typedef enum {
	KEY_MODE_NORMAL = 0,	// 通常モード
	KEY_MODE_OSD,			// OSD操作モード
} key_mode_t;

/**
 * @brief 映像効果定数
 * 
 */
typedef enum {
	EFFECT_NON = 0,			// 映像効果なし
	EFFECT_GRAY,			// グレースケール
	EFFECT_GRAY_REVERSE,	// グレースケール反転
	EFFECT_BIN,				// 白黒二値
	EFFECT_BIN_REVERSE,		// 白黒二値反転
} effect_t;

class EyeApp {
friend LongPressCheckTask;
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
	key_mode_t key_mode;
	// キーの押し下げ状態を保持するハッシュマップ
	std::unordered_map<int, KeyEventSp> key_state;
	// キーの長押し確認用ラムダ式を保持するハッシュマップ
	std::unordered_map<int, std::shared_ptr<thread::Runnable>> long_key_press_tasks;
	// モデルビュー変換行列
	math::Matrix mvp_matrix;
	// 拡大縮小インデックス[0,NUM_ZOOM_FACTOR)
	int zoom_ix;

	/**
	 * @brief キーの長押し確認用ラムダ式が生成されていることを確認、未生成なら新たに生成する
	 * 
	 * @param event 
	 */
	void confirm_long_press_task(const KeyEvent &event);
	/**
	 * @brief 短押しかどうか
	 * 
	 * @param key 
	 * @return true 
	 * @return false 
	 */
	bool is_short_pressed(const int &key);
	/**
	 * @brief 長押しされているかどうか
	 * 
	 * @param key 
	 * @return true 
	 * @return false 
	 */
	bool is_long_pressed(const int &key);

	/**
	 * @brief GLFWからのキー入力イベントの処理
	 * とりあえずは、GLFW_KEY_RIGHT(262), GLFW_KEY_LEFT(263), GLFW_KEY_DOWN(264), GLFW_KEY_UP(265)の
	 * 4種類だけキー処理を行う
	 *
	 * @param event
	 * @return int32_t
	 */
	int32_t handle_on_key_event(const KeyEvent &event);
	/**
	 * @brief handle_on_key_eventの下請け、キーが押されたとき/押し続けているとき
	 * とりあえずは、GLFW_KEY_RIGHT(262), GLFW_KEY_LEFT(263), GLFW_KEY_DOWN(264), GLFW_KEY_UP(265)の
	 * 4種類だけキー処理を行う
	 *
	 * @param event
	 * @return int32_t
	 */
	int32_t handle_on_key_down(const KeyEvent &event);
	/**
	 * @brief handle_on_key_eventの下請け、キーが離されたとき
	 * とりあえずは、GLFW_KEY_RIGHT(262), GLFW_KEY_LEFT(263), GLFW_KEY_DOWN(264), GLFW_KEY_UP(265)の
	 * 4種類だけキー処理を行う
	 *
	 * @param event
	 * @return int32_t
	 */
	int32_t handle_on_key_up(const KeyEvent &event);
	/**
	 * @brief 通常モードで短押ししたときの処理, handle_on_key_upの下請け
	 * 
	 * @param event 
	 * @return int32_t 
	 */
	int32_t on_key_up_normal(const KeyEvent &event);
	/**
	 * @brief OSD操作モードで短押ししたときの処理, handle_on_key_upの下請け
	 * 
	 * @param event 
	 * @return int32_t 
	 */
	int32_t on_key_up_osd(const KeyEvent &event);

	/**
	 * @brief 長押し時間経過したときの処理
	 * 
	 * @param event 
	 * @return int32_t 
	 */
	int32_t handle_on_long_key_pressed(const KeyEvent &event);
	/**
	 * @brief 通常モードで長押し時間経過したときの処理, handle_on_long_key_pressedの下請け
	 * 
	 * @param event 
	 * @return int32_t 
	 */
	int32_t on_long_key_pressed_normal(const KeyEvent &event);
	/**
	 * @brief OSD操作モードで長押し時間経過したときの処理, handle_on_long_key_pressedの下請け
	 * 
	 * @param event 
	 * @return int32_t 
	 */
	int32_t on_long_key_pressed_osd(const KeyEvent &event);

	/**
	 * @brief 輝度変更要求
	 * 
	 * @param inc_dec trueなら輝度増加、falseなら輝度減少
	 */
	void request_change_brightness(const bool &inc_dec);
	/**
	 * @brief 拡大縮小率変更要求
	 * 
	 * @param inc_dec trueなら拡大、falseなら縮小
	 */
	void request_change_scale(const bool &inc_dec);

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
	EyeApp(const int &gl_version = 300);
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
