#ifndef KEY_DISPATCHER_H_
#define KEY_DISPATCHER_H_

#include <mutex>
#include <unordered_map>
// common
#include "handler.h"
//
#include "const.h"
#include "key_state.h"
#include "key_event.h"

namespace serenegiant::app {

typedef std::function<void(const key_mode_t &/*key_mode*/)> OnKeyModeChangedFunc;
typedef std::function<void(const bool &/*inc_dec*/)> OnBrightnessChangedFunc;
typedef std::function<void(const bool &/*inc_dec*/)> OnScaleChangedFunc;
typedef std::function<void(const effect_t &/*effect*/)> OnEffectChangedFunc;
typedef std::function<void(const bool &/*onoff*/)> OnFreezeChangedFunc;
typedef std::function<void(const exp_mode_t &/*exp_mode*/)> OnExposureModeChangedFunc;
typedef std::function<void(const KeyEvent &event)> OSDKeyEventFunc;

/**
 * @brief キー入力を処理するためのヘルパークラス
 *
 */
class KeyDispatcher {
friend class LongPressCheckTask;
friend class KeyDownTask;
friend class KeyUpTask;

private:
	// 非同期実行用Handler
	thread::Handler &handler;
	mutable std::mutex state_lock;
	key_mode_t key_mode;
	int effect;
	bool freeze;
	// キーの押し下げ状態を保持するハッシュマップ
	std::unordered_map<ImGuiKey, KeyStateSp> key_states;
	// キーの長押し確認用Runnableを保持するハッシュマップ
	std::unordered_map<ImGuiKey, std::shared_ptr<thread::Runnable>> long_key_press_tasks;
	// マルチタップ確認用Runnableを保持するハッシュマップ
	std::unordered_map<ImGuiKey, std::shared_ptr<thread::Runnable>> key_down_tasks;
	std::unordered_map<ImGuiKey, std::shared_ptr<thread::Runnable>> key_up_tasks;
	OnKeyModeChangedFunc on_key_mode_changed;
	OnBrightnessChangedFunc on_brightness_changed;
	OnScaleChangedFunc on_scale_changed;
	OnEffectChangedFunc on_effect_changed;
	OnFreezeChangedFunc on_freeze_changed;
	OnExposureModeChangedFunc on_exp_mode_changed;
	OSDKeyEventFunc osd_key_event;

	void change_key_mode(const key_mode_t &mode, const bool &force_callback = false);
	/**
	 * @brief キーの状態を更新, 排他制御してないので上位でロックすること
	 * 
	 * @param event 
	 * @param handled
	 * @return KeyStateSp 更新前のキーステートのコピー
	 */
	KeyStateUp update(const KeyEvent &event, const bool &handled = false);
	/**
	 * @brief 指定したキーの状態をKEY_STATE_HANDLEDにする
	 * 
	 * @param key 
	 */
	void clear_key_state(const ImGuiKey &key);
	/**
	 * @brief キーの長押し・マルチタップ確認用Runnableが生成されていることを確認、未生成なら新たに生成する
	 *
	 * @param event
	 */
	void confirm_key_task(const KeyEvent &event);
	/**
	 * @brief キーアップ/キーダウンの遅延処理用タスクがあればキャンセルする
	 * 
	 * @param key 
	 */
	void cancel_key_task(const ImGuiKey &key);
	/**
	 * @brief 押されているキーの個数を取得, 排他制御してないので上位でロックすること
	 * 
	 * @return int 
	 */
	int num_pressed();
	/**
	 * @brief キーが押されているかどうか, 排他制御してないので上位でロックすること
	 *
	 * @param key
	 * @return true
	 * @return false
	 */
	bool is_pressed(const ImGuiKey &key);
	/**
	 * @brief 短押しかどうか, 排他制御してないので上位でロックすること
	 *
	 * @param key
	 * @return true
	 * @return false
	 */
	bool is_short_pressed(const ImGuiKey &key);
	/**
	 * @brief 長押しされているかどうか, 排他制御してないので上位でロックすること
	 *
	 * @param key
	 * @return true
	 * @return false
	 */
	bool is_long_pressed(const ImGuiKey &key);
	/**
	 * @brief 指定したキーのタップカウントを取得, 排他制御してないので上位でロックすること
	 * 
	 * @param key 
	 * @return int
	 */
	int tap_counts(const ImGuiKey &key);
	/**
	 * @brief handle_on_key_eventの下請け、キーが押されたとき/押し続けているとき
	 * とりあえずは、KEY_RIGHT, KEY_LEFT, KEY_DOWN, KEY_UPの4種類だけキー処理を行う
	 *
	 * @param event
	 * @return int
	 */
	int handle_on_key_down(const KeyEvent &event);
	/**
	 * @brief handle_on_key_eventの下請け、キーが離されたとき
	 * とりあえずは、KEY_RIGHT, KEY_LEFT, KEY_DOWN, KEY_UPの4種類だけキー処理を行う
	 *
	 * @param event
	 * @return int 処理済みなら1、未処理なら0, エラーなら負
	 */
	int handle_on_key_up(const KeyEvent &event);

	//--------------------------------------------------------------------------------
	/**
	 * 最後にキーを押してから一定時間(MULTI_PRESS_MAX_INTERVALMS)経過してマルチタップが途切れたときの処理
	 * @param event
	*/
	int handle_on_key_down_confirmed(const KeyEvent &event);
	/**
	 * 最後にキーを押してから一定時間(MULTI_PRESS_MAX_INTERVALMS)経過してマルチタップが途切れたときの処理
	 * @param event
	 * @param duration_ms
	*/
	int handle_on_key_up_confirmed(const KeyEvent &event, const nsecs_t &duration_ms);
	/**
	 * @brief 長押し時間経過したときの処理
	 *
	 * @param event
	 * @return int
	 */
	int handle_on_long_key_pressed(const KeyEvent &event);
	//--------------------------------------------------------------------------------
	/**
	 * @brief ショートタップしたときの処理, handle_on_key_upの下請け
	 * 
	 * @param current_key_mode
	 * @param event 
	 * @return int 
	 */
	int on_tap_short(const key_mode_t &current_key_mode, const KeyEvent &event);
	/**
	 * @brief 通常モードでショートタップしたときの処理, on_tap_shortの下請け
	 *
	 * @param event
	 * @return int 処理済みなら1、未処理なら0, エラーなら負
	 */
	int on_tap_short_normal(const KeyEvent &event);
	/**
	 * @brief 輝度調整モードでショートタップしたときの処理, on_tap_shortの下請け
	 *
	 * @param event
	 * @return int 処理済みなら1、未処理なら0, エラーなら負
	 */
	int on_tap_short_brightness(const KeyEvent &event);
	/**
	 * @brief 拡大縮小モードでショートタップしたときの処理, on_tap_shortの下請け
	 *
	 * @param event
	 * @return int 処理済みなら1、未処理なら0, エラーなら負
	 */
	int on_tap_short_zoom(const KeyEvent &event);
	/**
	 * @brief OSD操作モードでショートタップしたときの処理, handle_on_key_upの下請け
	 *
	 * @param event
	 * @return int 処理済みなら1、未処理なら0, エラーなら負
	 */
	int on_tap_short_osd(const KeyEvent &event);
	//--------------------------------------------------------------------------------
	/**
	 * @brief ロングタップの処理, handle_on_long_key_pressedの下請け
	 * 
	 * @param current_key_mode
	 * @param event 
	 * @return int 
	 */
	int on_tap_long(const key_mode_t &current_key_mode, const KeyEvent &event);
	/**
	 * @brief 通常モードでロングタップしたときの処理, on_tap_longの下請け
	 *
	 * @param event
	 * @return int 処理済みなら1、未処理なら0, エラーなら負
	 */
	int on_tap_long_normal(const KeyEvent &event);
	/**
	 * @brief 輝度調整モードで長押し時間経過したときの処理, on_tap_longの下請け
	 *
	 * @param event
	 * @return int 処理済みなら1、未処理なら0, エラーなら負
	 */
	int on_tap_long_brightness(const KeyEvent &event);
	/**
	 * @brief 拡大縮小モードで長押し時間経過したときの処理, on_tap_longの下請け
	 *
	 * @param event
	 * @return int 処理済みなら1、未処理なら0, エラーなら負
	 */
	int on_tap_long_zoom(const KeyEvent &event);
	/**
	 * @brief OSD操作モードで長押し時間経過したときの処理, on_tap_longの下請け
	 *
	 * @param event
	 * @return int 処理済みなら1、未処理なら0, エラーなら負
	 */
	int on_tap_long_osd(const KeyEvent &event);
	//--------------------------------------------------------------------------------
	/**
	 * @brief ダブルタップの処理, handle_on_key_upの下請け
	 * 
	 * @param current_key_mode
	 * @param event 
	 * @return int 
	 */
	int on_tap_double(const key_mode_t &current_key_mode, const KeyEvent &event);
	/**
	 * @brief 通常モードでダブルタップしたときの処理, on_tap_doubleの下請け
	 *
	 * @param event
	 * @return int 処理済みなら1、未処理なら0, エラーなら負
	 */
	int on_tap_double_normal(const KeyEvent &event);
	/**
	 * @brief 輝度調整モードでダブルタップしたときの処理, on_tap_doubleの下請け
	 *
	 * @param event
	 * @return int 処理済みなら1、未処理なら0, エラーなら負
	 */
	int on_tap_double_brightness(const KeyEvent &event);
	/**
	 * @brief 拡大縮小モードでダブルタップしたときの処理, on_tap_doubleの下請け
	 *
	 * @param event
	 * @return int 処理済みなら1、未処理なら0, エラーなら負
	 */
	int on_tap_double_zoom(const KeyEvent &event);
	/**
	 * @brief OSD操作モードでダブルタップしたときの処理, on_tap_doubleの下請け
	 *
	 * @param event
	 * @return int 処理済みなら1、未処理なら0, エラーなら負
	 */
	int on_tap_double_osd(const KeyEvent &event);
	//--------------------------------------------------------------------------------
	/**
	 * @brief トリプルタップの処理, handle_on_key_upの下請け
	 * 
	 * @param current_key_mode
	 * @param event 
	 * @return int 
	 */
	int on_tap_triple(const key_mode_t &current_key_mode, const KeyEvent &event);
	/**
	 * @brief 通常モードでトリプルタップしたときの処理, on_tap_tripleの下請け
	 *
	 * @param event
	 * @return int 処理済みなら1、未処理なら0, エラーなら負
	 */
	int on_tap_triple_normal(const KeyEvent &event);
	/**
	 * @brief 輝度調整モードでトリプルタップしたときの処理, on_tap_tripleの下請け
	 *
	 * @param event
	 * @return int 処理済みなら1、未処理なら0, エラーなら負
	 */
	int on_tap_triple_brightness(const KeyEvent &event);
	/**
	 * @brief 拡大縮小モードでトリプルタップしたときの処理, on_tap_tripleの下請け
	 *
	 * @param event
	 * @return int 処理済みなら1、未処理なら0, エラーなら負
	 */
	int on_tap_triple_zoom(const KeyEvent &event);
	/**
	 * @brief OSD操作モードでトリプルタップしたときの処理, on_tap_tripleの下請け
	 *
	 * @param event
	 * @return int 処理済みなら1、未処理なら0, エラーなら負
	 */
	int on_tap_triple_osd(const KeyEvent &event);
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

	inline key_mode_t get_key_mode() const { return key_mode; };

	/**
	 * @brief キーモードをリセット(KEY_MODE_NORMALへ戻す)
	 * 
	 */
	void reset_key_mode();

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
	 * @brief 測光モードが切り替わるときのコールバックをセット
	 * 
	 * @param callback 
	 * @return KeyDispatcher& 
	 */
	inline KeyDispatcher &set_exp_mode_changed(OnExposureModeChangedFunc callback) {
		on_exp_mode_changed = callback;
		return *this;
	}
	/**
	 * @brief OSD表示中のキーイベント
	 * 
	 * @param callback 
	 * @return KeyDispatcher& 
	 */
	inline KeyDispatcher &set_osd_key_event(OSDKeyEventFunc callback) {
		osd_key_event = callback;
		return *this;
	}

	/**
	 * @brief キー入力イベントの処理
	 * とりあえずは、KEY_RIGHT, KEY_LEFT, KEY_DOWN, KEY_UPの4種類だけキー処理を行う
	 *
	 * @param event
	 * @return KeyEvent
	 */
	KeyEvent handle_on_key_event(const KeyEvent &event);
};

}   // namespace serenegiant::app

#endif // KEY_DISPATCHER_H_