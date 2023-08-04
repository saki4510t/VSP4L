#if 1   // set 0 if you need debug log, otherwise set 1
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

#include <algorithm>

#include "utilbase.h"
#include "key_dispatcher.h"

namespace serenegiant::app {

// ショートタップと判定する最小押し下げ時間[ミリ秒]
#define SHORT_PRESS_MIN_MS (20)
// ショートタップと判定する最大押し下げ時間[ミリ秒]
#define SHORT_PRESS_MAX_MS (150)
// ロングタップと判定する押し下げ時間[ミリ秒]
#define LONG_PRESS_TIMEOUT_MS (1500)
// ロングロングタップと判定する押し下げ時間[ミリ秒]
#define LONG_LONG_PRESS_TIMEOUT_MS (6000)
// ダブルタップ・トリプルタップと判定するタップ間隔[ミリ秒]
#define MULTI_PRESS_MAX_INTERVALMS (300)

static const char *get_key_name(const ImGuiKey &key) {
	switch (key) {
	case ImGuiKey_LeftArrow:	return "LEFT";
	case ImGuiKey_RightArrow:	return "RIGHT";
	case ImGuiKey_UpArrow:		return "UP";
	case ImGuiKey_DownArrow:	return "DOWN";
    case ImGuiKey_Enter:		return "ENTER";
    case ImGuiKey_Escape:		return "ESC";
	case ImGuiKey_Space:		return "SPC";
	case ImGuiKey_0:			return "0";
	case ImGuiKey_1:			return "1";
	case ImGuiKey_2:			return "2";
	case ImGuiKey_3:			return "3";
	case ImGuiKey_4:			return "4";
	case ImGuiKey_5:			return "5";
	case ImGuiKey_6:			return "6";
	case ImGuiKey_7:			return "7";
	case ImGuiKey_8:			return "8";
	case ImGuiKey_9:			return "9";
	default:
		return "OTHER";
	}
}

//--------------------------------------------------------------------------------
class LongPressCheckTask : public thread::Runnable {
private:
	KeyDispatcher &dispatcher;	// こっちは参照を保持する
	const KeyEvent event;		// こっちはコピーを保持する
public:
	LongPressCheckTask(
		KeyDispatcher &dispatcher,
		const KeyEvent event)
	:	dispatcher(dispatcher), event(event)
	{
		ENTER();
		EXIT();
	}

	virtual ~LongPressCheckTask() {
		ENTER();
		EXIT();
	}

	void run() {
		ENTER();

		dispatcher.handle_on_long_key_pressed(event);

		EXIT();
	}
};

class KeyDownTask : public thread::Runnable {
private:
	KeyDispatcher &dispatcher;	// こっちは参照を保持する
	const KeyEvent event;		// こっちはコピーを保持する
public:
	KeyDownTask(
		KeyDispatcher &dispatcher,
		const KeyEvent &event)
	:	dispatcher(dispatcher), event(event)
	{
		ENTER();
		EXIT();
	}

	virtual ~KeyDownTask() {
		ENTER();
		EXIT();
	}

	void run() {
		ENTER();

		dispatcher.handle_on_key_down_confirmed(event);

		EXIT();
	}
};

class KeyUpTask : public thread::Runnable {
private:
	KeyDispatcher &dispatcher;	// こっちは参照を保持する
	const KeyEvent event;		// こっちはコピーを保持する
	const nsecs_t duration_ms;
public:
	KeyUpTask(
		KeyDispatcher &dispatcher,
		const KeyEvent &event,
		const nsecs_t &duration_ms)
	:	dispatcher(dispatcher), event(event), duration_ms(duration_ms)
	{
		ENTER();
		EXIT();
	}

	virtual ~KeyUpTask() {
		ENTER();
		EXIT();
	}

	void run() {
		ENTER();

		dispatcher.handle_on_key_up_confirmed(event, duration_ms);

		EXIT();
	}
};

//--------------------------------------------------------------------------------
/**
 * @brief コンストラクタ
 *
 */
/*public*/
KeyDispatcher::KeyDispatcher(thread::Handler &handler)
:   handler(handler),
	key_mode(KEY_MODE_NORMAL),
	effect(EFFECT_NON), freeze(false),
	on_key_mode_changed(nullptr),
	on_brightness_changed(nullptr),
	on_effect_changed(nullptr),
	on_scale_changed(nullptr),
	on_freeze_changed(nullptr),
	osd_key_event(nullptr)
{
	ENTER();
	EXIT();
}

/**
 * @brief デストラクタ
 *
 */
/*public*/
KeyDispatcher::~KeyDispatcher() {
	ENTER();
	EXIT();
}

/**
 * @brief キーモードをリセット(KEY_MODE_NORMALへ戻す)
 * 
 */
/*public*/
void KeyDispatcher::reset_key_mode() {
	ENTER();

	LOGD("reset key mode %d->%d", key_mode, KEY_MODE_NORMAL);
	change_key_mode(KEY_MODE_NORMAL);

	EXIT();
}

/**
 * すべてのキーの状態をクリアしてキーモードをリセットする(KEY_MODE_NORMALへ戻す)
*/
/*public*/
void KeyDispatcher::clear() {
	ENTER();

	{
 		std::lock_guard<std::mutex> lock(state_lock);
		for (auto itr: long_key_press_tasks) {
			handler.remove(itr.second);
		}
		long_key_press_tasks.clear();
		for (auto itr: key_down_tasks) {
			handler.remove(itr.second);
		}
		key_down_tasks.clear();
		for (auto itr: key_up_tasks) {
			handler.remove(itr.second);
		}
		key_up_tasks.clear();
		key_states.clear();
	}

	LOGD("reset key mode %d->%d", key_mode, KEY_MODE_NORMAL);
	change_key_mode(KEY_MODE_NORMAL, true);

	EXIT();
}

/**
 * @brief キー入力イベントの処理
 * とりあえずは、KEY_RIGHT, KEY_LEFT, KEY_DOWN, KEY_UPの4種類だけキー処理を行う
 *
 * @param event
 * @return KeyEvent
 */
/*public*/
KeyEvent KeyDispatcher::handle_on_key_event(const KeyEvent &event) {
	ENTER();

	int result = 0;

	// XXX 複数キー同時押しのときに最後に押したキーに対してのみKEY_ACTION_REPEATがくるみたいなのでKEY_ACTION_REPEATは使わない
	const auto key = event.key;
	key_mode_t current_key_mode;
	{
 		std::lock_guard<std::mutex> lock(state_lock);
		current_key_mode = key_mode;
	}
	switch (key) {
	case ImGuiKey_LeftArrow:
	case ImGuiKey_RightArrow:
	case ImGuiKey_UpArrow:
	case ImGuiKey_DownArrow:
    case ImGuiKey_Enter:
    case ImGuiKey_Escape:
	case ImGuiKey_Space:
	case ImGuiKey_0:
	case ImGuiKey_1:
	case ImGuiKey_2:
	case ImGuiKey_3:
	case ImGuiKey_4:
	case ImGuiKey_5:
	case ImGuiKey_6:
	case ImGuiKey_7:
	case ImGuiKey_8:
	case ImGuiKey_9:
		switch (event.action) {
		case KEY_ACTION_RELEASE:	// 0
			result = handle_on_key_up(event);
			break;
		case KEY_ACTION_PRESSED:	// 1
			result = handle_on_key_down(event);
			break;
		case KEY_ACTION_REPEAT:	// 2
		default:
			break;
		}
		break;
	default:
		LOGD("key=%d,scancode=%d,action=%d,mods=%d",
			key, event.scancode, event.action, event.mods);
		break;
	}
	if (result/*handled*/) {
		// 処理済みならkeyをImGuiKey_Noneに変更して返す
		RET(KeyEvent(ImGuiKey_None, event.scancode, event.action, event.mods));
	} else {
		RET(event);
	}

}

//--------------------------------------------------------------------------------
/*private*/
void KeyDispatcher::change_key_mode(const key_mode_t &mode, const bool &force_callback) {
	ENTER();

	LOGD("change key mode %d->%d,force=%d", key_mode, mode, force_callback);
	if ((key_mode != mode) || force_callback) {
		key_mode = mode;
		if (on_key_mode_changed) {
			// FIXME Handlerで呼ぶ？
			on_key_mode_changed(mode);
		}
	}

	EXIT();
}

/**
 * @brief キーの状態を更新, 排他制御してないので上位でロックすること
 * 
 * @param event 
 * @param handled
 * @return KeyStateUp 更新前のキーステートのコピー
 */
/*private*/
KeyStateUp KeyDispatcher::update(const KeyEvent &event, const bool &handled) {
	ENTER();

	const auto key = event.key;
	if (UNLIKELY(key_states.find(key) == key_states.end())) {
		// KeyStateがセットされていないとき(初めて押されたとき)は新規追加
		LOGD("create KeyState,key=%d/%s", key, get_key_name(key));
		key_states[key] = std::make_shared<KeyState>(key);
	}

	auto &state = key_states[key];
	// 更新前のKeyStateのコピーを返す
	auto prev = std::make_unique<KeyState>(*state);
	const auto sts = state->state;

	switch (event.action) {
	case KEY_ACTION_RELEASE:	// 0
	{
		state->state = handled ? KEY_STATE_HANDLED : KEY_STATE_UP;
		// 前回同じキーを押したときからの経過時間を計算
		const auto tap_interval_ms = (event.event_time_ns - state->last_tap_time_ns) / 1000000L;
		state->last_tap_time_ns = event.event_time_ns;
		if (!handled && (sts == KEY_STATE_DOWN)) {
			if (tap_interval_ms <= MULTI_PRESS_MAX_INTERVALMS) {
				// マルチタップ
				state->tap_count = state->tap_count + 1;
			} else {
				LOGD("key=%d/%s,clear tap_count 1", key, get_key_name(key));
				state->tap_count = 1;
			}
		} else {
			LOGD("key=%d/%s,clear tap_count 0", key, get_key_name(key));
			state->tap_count = 0;
		}
		LOGD("key=%d/%s,sts=%d,interval=%ld,tap_count=%d",
			key, get_key_name(key), sts, tap_interval_ms, state->tap_count);
		break;
	}
	case KEY_ACTION_PRESSED:	// 1
		state->press_time_ns = event.event_time_ns;
		// pass through
	case KEY_ACTION_REPEAT:	// 2
		state->state = KEY_STATE_DOWN;
		break;
	default:
		break;
	}

	RET(prev);
}

/*private*/
void KeyDispatcher::clear_key_state(const ImGuiKey &key) {
	ENTER();

	if (LIKELY(key_states.find(key) != key_states.end())) {
		// XXX 直接インデックスアクセスすると空エントリーができてしまうのでcontainsでチェック必須
		auto &state = key_states[key];
		if (state) {
			state->state = KEY_STATE_HANDLED;
			state->tap_count = 0;
		}
	}

	EXIT();
}

/**
 * @brief キーの長押し・マルチタップ確認用Runnableが生成されていることを確認、未生成なら新たに生成する
 *
 * @param event
 */
/*private*/
void KeyDispatcher::confirm_key_task(const KeyEvent &event) {
	ENTER();

	const auto key = event.key;
	if (long_key_press_tasks.find(key) == long_key_press_tasks.end()) {
		long_key_press_tasks[key] = std::make_shared<LongPressCheckTask>(*this, event);
	}

	EXIT();
}

/**
 * @brief キーアップの遅延処理用タスクがあればキャンセルする
 * 
 * @param key 
 */
/*private*/
void KeyDispatcher::cancel_key_task(const ImGuiKey &key) {
	ENTER();

	// キーダウンの遅延処理用Runnableがあれば削除する
	auto key_down_task = key_down_tasks[key];
	if (key_down_task) {
		handler.remove(key_down_task);
	}	
	// キーアップの遅延処理用Runnableがあれば削除する
	auto key_up_task = key_up_tasks[key];
	if (key_up_task) {
		handler.remove(key_up_task);
	}	

	EXIT();
}

/**
 * @brief 押されているキーの個数を取得, 排他制御してないので上位でロックすること
 * 
 * @return int 
 */
/*private*/
int KeyDispatcher::num_pressed() {
	return std::count_if(key_states.begin(), key_states.end(), [](const auto itr) {
		const auto state = itr.second->state;
		return (state == KEY_STATE_DOWN) || (state == KEY_STATE_DOWN_LONG);
	});
}

/**
 * @brief キーが押されているかどうか, 排他制御してないので上位でロックすること
 *
 * @param key
 * @return true
 * @return false
 */
/*private*/
bool KeyDispatcher::is_pressed(const ImGuiKey &key) {
	return (key_states.find(key) != key_states.end())
		&& ((key_states[key]->state == KEY_STATE_DOWN)
			|| ((key_states[key]->state == KEY_STATE_DOWN_LONG)));
}

/**
 * @brief 短押しかどうか, 排他制御してないので上位でロックすること
 *
 * @param key
 * @return true
 * @return false
 */
/*private*/
bool KeyDispatcher::is_short_pressed(const ImGuiKey &key) {
	return (key_states.find(key) != key_states.end())
		&& (key_states[key]->state == KEY_STATE_DOWN);
}

/**
 * @brief 長押しされているかどうか, 排他制御してないので上位でロックすること
 *
 * @param key
 * @return true
 * @return false
 */
/*private*/
bool KeyDispatcher::is_long_pressed(const ImGuiKey &key) {
	return (key_states.find(key) != key_states.end())
		&& (key_states[key]->state == KEY_STATE_DOWN_LONG);
}

/**
 * @brief 指定したキーのタップカウントを取得, 排他制御してないので上位でロックすること
 * 
 * @param key 
 * @return int
 */
/*private*/
int KeyDispatcher::tap_counts(const ImGuiKey &key) {
	return (key_states.find(key) != key_states.end())
		? key_states[key]->tap_count : 0;
}

//--------------------------------------------------------------------------------
/**
 * マルチタップの処理が必要かどうかを取得する
 * 通常モード、OSDモードで下矢印のときにマルチタップを有効にする
 * @param event
 * @param key_mode
 * @return false: マルチタップの処理は不要, true: マルチタップの処理が必要
*/
static inline bool need_multi_tap(const KeyEvent &event, const key_mode_t &key_mode) {
	return (key_mode == KEY_MODE_NORMAL)
		|| ((key_mode == KEY_MODE_OSD) && (event.key == ImGuiKey_DownArrow));
}

/**
 * @brief handle_on_key_eventの下請け、キーが押されたとき
 * とりあえずは、KEY_RIGHT, KEY_LEFT, KEY_DOWN, KEY_UPの4種類だけキー処理を行う
 *
 * @param event
 * @return int
 */
/*private*/
int KeyDispatcher::handle_on_key_down(const KeyEvent &event) {
	ENTER();

	int result = 0;
	const auto key = event.key;

	LOGD("key=%d/%s", key, get_key_name(key));

	key_mode_t current_key_mode;
	{
 		std::lock_guard<std::mutex> lock(state_lock);
		current_key_mode = key_mode;
		update(event);
	}
	// キーダウン/キーアップの遅延処理用Runnableがあればキャンセルする
	cancel_key_task(key);
	if (current_key_mode != KEY_MODE_OSD) {
		// 長押し確認用Runnableを遅延実行する
		confirm_key_task(event);
		handler.remove(long_key_press_tasks[key]);
		handler.post_delayed(long_key_press_tasks[key], LONG_PRESS_TIMEOUT_MS);
	}
	if (need_multi_tap(event, current_key_mode)) {
		// マルチタップの処理
		LOGD("key=%d/%s,post key_down_task", key, get_key_name(key));
		auto key_down_task = std::make_shared<KeyDownTask>(*this, event);
		key_down_tasks[key] = key_down_task;
		handler.post_delayed(key_down_task, MULTI_PRESS_MAX_INTERVALMS);
		result = 1;
	} else {
		// それ以外のキーモードは直接タップ処理をする
		result = handle_on_key_down_confirmed(event);
	}

	RETURN(result, int);
}

/**
 * @brief handle_on_key_eventの下請け、キーが離されたとき
 * とりあえずは、KEY_RIGHT, KEY_LEFT, KEY_DOWN, KEY_UPの4種類だけキー処理を行う
 *
 * @param event
 * @return int 処理済みなら1、未処理なら0, エラーなら負
 */
/*private*/
int KeyDispatcher::handle_on_key_up(const KeyEvent &event) {
	ENTER();

	int result = 0;
	const auto key = event.key;

	// キーダウン/キーアップの遅延処理用Runnableがあればキャンセルする
	cancel_key_task(key);
	// 長押し確認用Runnableをキャンセルする
	confirm_key_task(event);
	handler.remove(long_key_press_tasks[key]);

	KeyStateUp prev;
	key_mode_t current_key_mode;
	{
 		std::lock_guard<std::mutex> lock(state_lock);
		current_key_mode = key_mode;
		prev = update(event);
	}

	// キーの押し下げ時間を計算
	const auto duration_ms = (event.event_time_ns - prev->press_time_ns) / 1000000L;
	if (!result && (prev->state == KEY_STATE_DOWN)
		&& (duration_ms >= SHORT_PRESS_MIN_MS) && (duration_ms < SHORT_PRESS_MAX_MS)) {
		// ショートタップ時間内に収まっているときのみ処理する
		if (need_multi_tap(event, current_key_mode)) {
			// マルチタップの処理
			LOGD("key=%d/%s,post key_up_task", key, get_key_name(key));
			auto key_up_task = std::make_shared<KeyUpTask>(*this, event, duration_ms);
			key_up_tasks[key] = key_up_task;
			handler.post_delayed(key_up_task, MULTI_PRESS_MAX_INTERVALMS);
		} else {
			// それ以外のキーモードは直接タップ処理をする
			result = handle_on_key_up_confirmed(event, duration_ms);
		}
	}

	RETURN(result, int);
}

/**
 * 最後にキーを押してから一定時間(MULTI_PRESS_MAX_INTERVALMS)経過してマルチタップが途切れたときの処理
 * @param event
*/
/*private*/
int KeyDispatcher::handle_on_key_down_confirmed(const KeyEvent &event) {
	ENTER();

	int result = 0;
	const auto key = event.key;
	KeyStateSp state;
	key_mode_t current_key_mode;
	int tap_counts;
	{
 		std::lock_guard<std::mutex> lock(state_lock);
		current_key_mode = key_mode;
		tap_counts = this->tap_counts(key);
		state = key_states[key];
	}
	if (state) {
		// handle_on_key_downを通った後なのでstateはnullではないはずだけど念の為に塗るチェックしておく
		// キーの押し下げ時間を計算
		const auto duration_ms = (event.event_time_ns - state->press_time_ns) / 1000000L;
		LOGD("key=%d/%s,duration=%ld,tap_counts=%d",
			key, get_key_name(key), duration_ms, tap_counts);
	}

	RETURN(result, int);
}

/**
 * 最後にキーを押してから一定時間(MULTI_PRESS_MAX_INTERVALMS)経過してマルチタップが途切れたときの処理
 * @param event
 * @param duration_ms
*/
/*private*/
int KeyDispatcher::handle_on_key_up_confirmed(const KeyEvent &event, const nsecs_t &duration_ms) {
	ENTER();

	int result = 0;
	const auto key = event.key;
	key_mode_t current_key_mode;
	int tap_counts;
	{
 		std::lock_guard<std::mutex> lock(state_lock);
		current_key_mode = key_mode;
		tap_counts = this->tap_counts(key);
	}

	LOGD("key=%d/%s,duration_ms=%ld,tap_counts=%d",
		key, get_key_name(key), duration_ms, tap_counts);
	if (!result && (tap_counts == 3)) {
		// トリプルタップ
		result = on_tap_triple(current_key_mode, event);
	}
	if (!result && (tap_counts == 2)) {
		// ダブルタップ
		result = on_tap_double(current_key_mode, event);
	}
	if (!result) {
		// ショートタップ, 押し下げ時間的にはロングタップ等も含むが
		// その場合はstateがKEY_STATE_DOWN_LONGなのでここには来ない
		result = on_tap_short(current_key_mode, event);
	}
	if (!result && (current_key_mode == KEY_MODE_OSD) && osd_key_event) {
		// OSDモードのときは未処理のすべてのキーイベントをOSDクラスへ送る
		osd_key_event(KeyEvent(key, event.scancode, KEY_ACTION_PRESSED, event.mods));
		handler.post_delayed([this, event] {
			osd_key_event(event);
		}, 10);
		result = 1;
	}
	if (result == 1/*handled*/) {
		std::lock_guard<std::mutex> lock(state_lock);
		update(event, true/*handled*/);
	}

	RETURN(result, int);
}

//--------------------------------------------------------------------------------
/**
 * @brief 長押し時間経過したときの処理
 *
 * @param event
 * @return int
 */
/*private*/
int KeyDispatcher::handle_on_long_key_pressed(const KeyEvent &event) {
	ENTER();

	int result = 0;
	const auto key = event.key;
	bool long_pressed = false;
	key_mode_t current_key_mode;
	{
 		std::lock_guard<std::mutex> lock(state_lock);
		current_key_mode = key_mode;
		if (key_states[key]->state == KEY_STATE_DOWN) {
			key_states[key]->state = KEY_STATE_DOWN_LONG;
			long_pressed = true;
		}
	}
	if (long_pressed) {
		LOGD("on_long_key_pressed,key=%d", key);
		result = on_tap_long(current_key_mode, event);
		if (result == 1/*handled*/) {
			std::lock_guard<std::mutex> lock(state_lock);
			// update(event, true/*handled*/);
			clear_key_state(key);
		}
	}

	RETURN(result, int);
}

//--------------------------------------------------------------------------------
// ショートタップ
/**
 * @brief ショートタップしたときの処理, handle_on_key_upの下請け
 * 
 * @param current_key_mode
 * @param event 
 * @return int 
 */
/*private*/
int KeyDispatcher::on_tap_short(const key_mode_t &current_key_mode, const KeyEvent &event) {
	ENTER();

	LOGD("key=%d/%s,mode=%d", event.key, get_key_name(event.key), current_key_mode);

	int result = 0;
	switch (current_key_mode) {
	case KEY_MODE_NORMAL:
		result = on_tap_short_normal(event);
		break;
	case KEY_MODE_BRIGHTNESS:
		result = on_tap_short_brightness(event);
		break;
	case KEY_MODE_ZOOM:
		result = on_tap_short_zoom(event);
		break;
	case KEY_MODE_OSD:
		result = on_tap_short_osd(event);
		break;
	default:
		LOGW("unknown key mode,%d", current_key_mode);
		break;
	}

	RETURN(result, int);
}

/**
 * @brief 通常モードでショートタップしたときの処理, on_tap_shortの下請け
 *
 * @param event
 * @return int 処理済みなら1、未処理なら0, エラーなら負
 */
/*private*/
int KeyDispatcher::on_tap_short_normal(const KeyEvent &event) {
	ENTER();

	int result = 0;
	const auto key = event.key;

	RETURN(result, int);
}

/**
 * @brief 輝度調整モードでショートタップしたときの処理, on_tap_shortの下請け
 *
 * @param event
 * @return int 処理済みなら1、未処理なら0, エラーなら負
 */
/*private*/
int KeyDispatcher::on_tap_short_brightness(const KeyEvent &event) {
	ENTER();

	int result = 0;
	const auto key = event.key;
	switch (key) {
	case ImGuiKey_RightArrow:
	case ImGuiKey_LeftArrow:
		// 輝度変更
		if (on_brightness_changed) {
			on_brightness_changed(key == ImGuiKey_RightArrow);
		}
		result = 1;
		break;
	case ImGuiKey_DownArrow:
	case ImGuiKey_UpArrow:
	default:
		LOGW("unexpected key code,%d", key);
		break;
	}

	RETURN(result, int);
}

/**
 * @brief 拡大縮小モードでショートタップしたときの処理, on_tap_shortの下請け
 *
 * @param event
 * @return int 処理済みなら1、未処理なら0, エラーなら負
 */
/*private*/
int KeyDispatcher::on_tap_short_zoom(const KeyEvent &event) {
	ENTER();

	int result = 0;
	const auto key = event.key;
	switch (key) {
	case ImGuiKey_DownArrow:
	case ImGuiKey_UpArrow:
		// 拡大縮小
		if (on_scale_changed) {
			on_scale_changed(key == ImGuiKey_UpArrow);
		}
		result = 1;
		break;
	case ImGuiKey_RightArrow:
	case ImGuiKey_LeftArrow:
	default:
		LOGW("unexpected key code,%d", key);
		break;
	}

	RETURN(result, int);
}

/**
 * @brief OSD操作モードでショートタップしたときの処理, on_tap_shortの下請け
 *
 * @param event
 * @return int 処理済みなら1、未処理なら0, エラーなら負
 */
/*private*/
int KeyDispatcher::on_tap_short_osd(const KeyEvent &event) {
	ENTER();

	int result = 0;
	const auto key = event.key;
	// FIXME 未実装

	RETURN(result, int);
}

//--------------------------------------------------------------------------------
// ロングタップ
/**
 * @brief ロングタップの処理, handle_on_long_key_pressedの下請け
 * 
 * @param current_key_mode
 * @param event 
 * @return int 
 */
/*private*/
int KeyDispatcher::on_tap_long(const key_mode_t &current_key_mode, const KeyEvent &event) {
	ENTER();

	LOGD("key=%d/%s,mode=%d", event.key, get_key_name(event.key), current_key_mode);

	int result = 0;
	switch (current_key_mode) {
	case KEY_MODE_NORMAL:
		result = on_tap_long_normal(event);
		break;
	case KEY_MODE_BRIGHTNESS:
		result = on_tap_long_brightness(event);
		break;
	case KEY_MODE_ZOOM:
		result = on_tap_long_zoom(event);
		break;
	case KEY_MODE_OSD:
		result = on_tap_long_osd(event);
		break;
	default:
		LOGW("unknown key mode,%d", current_key_mode);
		break;
	}

	RETURN(result, int);
}

/**
 * @brief 通常モードでロングタップしたときの処理, on_tap_longの下請け
 *
 * @param event
 * @return int 処理済みなら1、未処理なら0, エラーなら負
 */
/*private*/
int KeyDispatcher::on_tap_long_normal(const KeyEvent &event) {
	ENTER();

	int result = 0;
	int n;
	{
 		std::lock_guard<std::mutex> lock(state_lock);
		n = num_pressed();
	}
	LOGD("num_pressed=%d", n);
	if (n == 1) {
		// 単独でキー操作したときのみ受け付ける
		const auto key = event.key;
		switch (key) {
		case ImGuiKey_DownArrow:
		case ImGuiKey_UpArrow:
			// 拡大縮小モードへ
			change_key_mode(KEY_MODE_ZOOM);
			result = 1;
			break;
		case ImGuiKey_RightArrow:
		case ImGuiKey_LeftArrow:
			// 輝度調整モードへ
			change_key_mode(KEY_MODE_BRIGHTNESS);
			result = 1;
			break;
		default:
			LOGW("unexpected key code,%d", key);
			break;
		}
	} else if (n == 2) {
		bool is_osd_toggle;
		{
	 		std::lock_guard<std::mutex> lock(state_lock);
			is_osd_toggle
				= (is_long_pressed(ImGuiKey_RightArrow) || is_long_pressed(ImGuiKey_LeftArrow))
				&& (is_long_pressed(ImGuiKey_UpArrow) || is_long_pressed(ImGuiKey_DownArrow));
			if (is_osd_toggle) {
				clear_key_state(ImGuiKey_RightArrow);
				clear_key_state(ImGuiKey_LeftArrow);
				clear_key_state(ImGuiKey_UpArrow);
				clear_key_state(ImGuiKey_DownArrow);
			}
		}
		if (is_osd_toggle) {
			// ソフトウエアOSDの表示ON/OFFを変更する
			change_key_mode(KEY_MODE_OSD);
			result = 1;	// handled
		}
	}

	RETURN(result, int);
}

/**
 * @brief 輝度調整モードでロングタップしたときの処理, on_tap_longの下請け
 *
 * @param event
 * @return int 処理済みなら1、未処理なら0, エラーなら負
 */
/*private*/
int KeyDispatcher::on_tap_long_brightness(const KeyEvent &event) {
	ENTER();

	int result = 0;
	const auto n = num_pressed();
	LOGD("num_pressed=%d", n);
	if (n == 1) {
		// 単独でキー操作したときのみ受け付ける
		const auto key = event.key;
		switch (key) {
		case ImGuiKey_DownArrow:
		case ImGuiKey_UpArrow:
			// 拡大縮小モードへ
			change_key_mode(KEY_MODE_ZOOM);
			result = 1;
			break;
		case ImGuiKey_RightArrow:
		case ImGuiKey_LeftArrow:
			break;
		default:
			LOGW("unexpected key code,%d", key);
			break;
		}
	}

	RETURN(result, int);
}

/**
 * @brief 拡大縮小モードでロングタップしたときの処理, on_tap_longの下請け
 *
 * @param event
 * @return int 処理済みなら1、未処理なら0, エラーなら負
 */
/*private*/
int KeyDispatcher::on_tap_long_zoom(const KeyEvent &event) {
	ENTER();

	int result = 0;
	const auto n = num_pressed();
	LOGD("num_pressed=%d", n);
	if (n == 1) {
		// 単独でキー操作したときのみ受け付ける
		const auto key = event.key;
		switch (key) {
		case ImGuiKey_DownArrow:
		case ImGuiKey_UpArrow:
			break;
		case ImGuiKey_RightArrow:
		case ImGuiKey_LeftArrow:
			// 輝度調整モードへ
			change_key_mode(KEY_MODE_BRIGHTNESS);
			result = 1;
			break;
		default:
			LOGW("unexpected key code,%d", key);
			break;
		}
	}

	RETURN(result, int);
}

/**
 * @brief OSD操作モードでロングタップしたときの処理, on_tap_longの下請け
 *
 * @param event
 * @return int 処理済みなら1、未処理なら0, エラーなら負
 */
/*private*/
int KeyDispatcher::on_tap_long_osd(const KeyEvent &event) {
	ENTER();

	int result = 0;
	const auto key = event.key;
	// FIXME 未実装

	RETURN(result, int);
}

//--------------------------------------------------------------------------------
// ダブルタップ
/**
 * @brief ダブルタップの処理, handle_on_key_upの下請け
 * 
 * @param current_key_mode
 * @param event 
 * @return int 
 */
/*private*/
int KeyDispatcher::on_tap_double(const key_mode_t &current_key_mode, const KeyEvent &event) {
	ENTER();

	LOGD("key=%d/%s,mode=%d", event.key, get_key_name(event.key), current_key_mode);

	int result = 0;
	switch (current_key_mode) {
	case KEY_MODE_NORMAL:
		result = on_tap_double_normal(event);
		break;
	case KEY_MODE_BRIGHTNESS:
		result = on_tap_double_brightness(event);
		break;
	case KEY_MODE_ZOOM:
		result = on_tap_double_zoom(event);
		break;
	case KEY_MODE_OSD:
		result = on_tap_double_osd(event);
		break;
	default:
		LOGW("unknown key mode,%d", current_key_mode);
		break;
	}

	RETURN(result, int);
}
/**
 * @brief 通常モードでダブルタップしたときの処理, handle_on_key_upの下請け
 *
 * @param event
 * @return int 処理済みなら1、未処理なら0, エラーなら負
 */
/*private*/
int KeyDispatcher::on_tap_double_normal(const KeyEvent &event) {
	ENTER();

	int result = 0;
	const auto n = num_pressed();
	LOGD("num_pressed=%d", n);
	if (!n) {
		// 単独でキー操作したときのみ受け付ける
		// キーアップイベントからくるので他にキーが押されていなければ0
		const auto key = event.key;
		switch (key) {
		case ImGuiKey_RightArrow:
		case ImGuiKey_LeftArrow:
			// 映像フリーズON/OFF
			freeze = !freeze;
			if (on_freeze_changed) {
				on_freeze_changed(freeze);
			}
			result = 1;
			break;
		case ImGuiKey_DownArrow:
		case ImGuiKey_UpArrow:
			// FIXME 現在の映像効果切り替えはデバッグ用に映像効果をローテーション
			effect = (effect + 1) % EFFECT_NUM;
			if (on_effect_changed) {
				on_effect_changed((effect_t)effect);
			}
			result = 1;
			break;
		default:
			LOGW("unexpected key code,%d", key);
			break;
		}
	}

	RETURN(result, int);
}

/**
 * @brief 輝度調整モードでダブルタップしたときの処理, handle_on_key_upの下請け
 *
 * @param event
 * @return int 処理済みなら1、未処理なら0, エラーなら負
 */
/*private*/
int KeyDispatcher::on_tap_double_brightness(const KeyEvent &event) {
	ENTER();

	int result = 0;
	const auto key = event.key;
	// FIXME 未実装

	RETURN(result, int);
}

/**
 * @brief 拡大縮小モードでダブルタップしたときの処理, handle_on_key_upの下請け
 *
 * @param event
 * @return int 処理済みなら1、未処理なら0, エラーなら負
 */
/*private*/
int KeyDispatcher::on_tap_double_zoom(const KeyEvent &event) {
	ENTER();

	int result = 0;
	const auto key = event.key;
	// FIXME 未実装

	RETURN(result, int);
}

/**
 * @brief OSD操作モードでダブルタップしたときの処理, handle_on_key_upの下請け
 *
 * @param event
 * @return int 処理済みなら1、未処理なら0, エラーなら負
 */
/*private*/
int KeyDispatcher::on_tap_double_osd(const KeyEvent &event) {
	ENTER();

	int result = 0;
	const auto key = event.key;
	// FIXME 未実装
	if ((key == ImGuiKey_DownArrow) && osd_key_event) {
		// osd_key_event(event);
		osd_key_event(KeyEvent(ImGuiKey_Space, 57, KEY_ACTION_PRESSED, 0));
		handler.post_delayed([this] {
			osd_key_event(KeyEvent(ImGuiKey_Space, 57, KEY_ACTION_RELEASE, 0));
		}, 10);
		result = 1;
	}

	RETURN(result, int);
}

//--------------------------------------------------------------------------------
/**
 * @brief トリプルタップの処理, handle_on_key_upの下請け
 * 
 * @param current_key_mode
 * @param event 
 * @return int 
 */
/*private*/
int KeyDispatcher::on_tap_triple(const key_mode_t &current_key_mode, const KeyEvent &event) {
	ENTER();

	LOGD("key=%d/%s,mode=%d", event.key, get_key_name(event.key), current_key_mode);

	int result = 0;
	switch (current_key_mode) {
	case KEY_MODE_NORMAL:
		result = on_tap_triple_normal(event);
		break;
	case KEY_MODE_BRIGHTNESS:
		result = on_tap_triple_brightness(event);
		break;
	case KEY_MODE_ZOOM:
		result = on_tap_triple_zoom(event);
		break;
	case KEY_MODE_OSD:
		result = on_tap_triple_osd(event);
		break;
	default:
		LOGW("unknown key mode,%d", current_key_mode);
		break;
	}

	RETURN(result, int);
}

/**
 * @brief 通常モードでトリプルタップしたときの処理, on_tap_tripleの下請け
 *
 * @param event
 * @return int 処理済みなら1、未処理なら0, エラーなら負
 */
/*private*/
int KeyDispatcher::on_tap_triple_normal(const KeyEvent &event) {
	ENTER();

	int result = 0;
	const auto n = num_pressed();
	LOGD("num_pressed=%d", n);
	if (!n) {
		// 単独でキー操作したときのみ受け付ける
		// キーアップイベントからくるので他にキーが押されていなければ0
		const auto key = event.key;
		switch (key) {
		case ImGuiKey_RightArrow:
		case ImGuiKey_LeftArrow:
			// 測光モードを平均測光へ
			if (on_exp_mode_changed) {
				on_exp_mode_changed(EXP_MODE_AVERAGE);
			}
			result = 1;
			break;
		case ImGuiKey_DownArrow:
		case ImGuiKey_UpArrow:
			// 測光モードを中央測光へ
			if (on_exp_mode_changed) {
				on_exp_mode_changed(EXP_MODE_CENTER);
			}
			result = 1;
			break;
		default:
			LOGW("unexpected key code,%d", key);
			break;
		}
	}

	RETURN(result, int);
}

/**
 * @brief 輝度調整モードでトリプルタップしたときの処理, on_tap_tripleの下請け
 *
 * @param event
 * @return int 処理済みなら1、未処理なら0, エラーなら負
 */
/*private*/
int KeyDispatcher::on_tap_triple_brightness(const KeyEvent &event) {
	ENTER();

	int result = 0;
	const auto key = event.key;
	// FIXME 未実装

	RETURN(result, int);
}

/**
 * @brief 拡大縮小モードでトリプルタップしたときの処理, on_tap_tripleの下請け
 *
 * @param event
 * @return int 処理済みなら1、未処理なら0, エラーなら負
 */
/*private*/
int KeyDispatcher::on_tap_triple_zoom(const KeyEvent &event) {
	ENTER();

	int result = 0;
	const auto key = event.key;
	// FIXME 未実装

	RETURN(result, int);
}

/**
 * @brief OSD操作モードでトリプルタップしたときの処理, on_tap_tripleの下請け
 *
 * @param event
 * @return int 処理済みなら1、未処理なら0, エラーなら負
 */
/*private*/
int KeyDispatcher::on_tap_triple_osd(const KeyEvent &event) {
	ENTER();

	int result = 0;
	const auto key = event.key;
	// FIXME 未実装

	RETURN(result, int);
}

}   // namespace serenegiant::app
