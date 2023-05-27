#ifndef KEY_DISPATCHER_H_
#define KEY_DISPATCHER_H_

#include <mutex>
#include <unordered_map>
// common
#include "handler.h"
//
#include "const.h"
#include "key_event.h"

namespace serenegiant::app {

typedef std::function<void(const key_mode_t &/*key_mode*/)> OnKeyModeChangedFunc;
typedef std::function<void(const bool &/*inc_dec*/)> OnBrightnessChangedFunc;
typedef std::function<void(const bool &/*inc_dec*/)> OnScaleChangedFunc;
typedef std::function<void(const effect_t &/*effect*/)> OnEffectChangedFunc;
typedef std::function<void(const bool &/*onoff*/)> OnFreezeChangedFunc;

// privateクラスの前方参照宣言
class LongPressCheckTask;

/**
 * @brief キー入力を処理するためのヘルパークラス
 *
 */
class KeyDispatcher {
friend LongPressCheckTask;
private:
	// 非同期実行用Handler
	thread::Handler &handler;
	mutable std::mutex state_lock;
	key_mode_t key_mode;
	effect_t effect;
	// キーの押し下げ状態を保持するハッシュマップ
	std::unordered_map<int, KeyEventSp> key_state;
	// キーの長押し確認用ラムダ式を保持するハッシュマップ
	std::unordered_map<int, std::shared_ptr<thread::Runnable>> long_key_press_tasks;
	OnKeyModeChangedFunc on_key_mode_changed;
	OnBrightnessChangedFunc on_brightness_changed;
	OnScaleChangedFunc on_scale_changed;
	OnEffectChangedFunc on_effect_changed;
	OnFreezeChangedFunc on_freeze_changed;

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
protected:
public:
	/**
	 * @brief コンストラクタ
	 *
	 */
	KeyDispatcher(thread::Handler &handler);
	/**
	 * @brief デストラクタ
	 *
	 */
	~KeyDispatcher();

	/**
	 * @brief キーモードが変更されるときのコールバックをセット
	 *
	 * @param callback
	 * @return KeyDispatcher&
	 */
	inline KeyDispatcher &set_on_key_mode_changed(OnKeyModeChangedFunc callback) {
		on_key_mode_changed = callback;
		return *this;
	}
	/**
	 * @brief 輝度が変更されるときのコールバックをセット
	 *
	 * @param callback
	 * @return KeyDispatcher&
	 */
	inline KeyDispatcher &set_on_brightness_changed(OnBrightnessChangedFunc callback) {
		on_brightness_changed = callback;
		return *this;
	}
	/**
	 * @brief 拡大縮小率が変更されるときのコールバックをセット
	 *
	 * @param callback
	 * @return KeyDispatcher&
	 */
	inline KeyDispatcher &set_on_scale_changed(OnScaleChangedFunc callback) {
		on_scale_changed = callback;
		return *this;
	}
	/**
	 * @brief 映像効果が変更されるときのコールバックをセット
	 *
	 * @param callback
	 * @return KeyDispatcher&
	 */
	inline KeyDispatcher &set_on_effect_changed(OnEffectChangedFunc callback) {
		on_effect_changed = callback;
		return *this;
	}
	/**
	 * @brief 映像フリーズが切り替わるときのコールバックをセット
	 * 
	 * @param callback 
	 * @return KeyDispatcher& 
	 */
	inline KeyDispatcher &set_on_freeze_changed(OnFreezeChangedFunc callback) {
		on_freeze_changed = callback;
		return *this;
	}

	/**
	 * @brief GLFWからのキー入力イベントの処理
	 * とりあえずは、GLFW_KEY_RIGHT(262), GLFW_KEY_LEFT(263), GLFW_KEY_DOWN(264), GLFW_KEY_UP(265)の
	 * 4種類だけキー処理を行う
	 *
	 * @param event
	 * @return int32_t
	 */
	int32_t handle_on_key_event(const KeyEvent &event);
};

}   // namespace serenegiant::app

#endif // KEY_DISPATCHER_H_