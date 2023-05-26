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
#endif

#include "utilbase.h"
#include "key_dispatcher.h"

namespace serenegiant::app {

// ショートタップと判定する最小押し下げ時間[ミリ秒]
#define SHORT_PRESS_MIN_MS (20)
// ショートタップと判定する最大押し下げ時間[ミリ秒]
#define SHORT_PRESS_MAX_MS (300)
// ミドルタップと判定する最小押し下げ時間[ミリ秒]
#define MIDDLE_PRESS_MIN_MS (500)
// ミドルタップと判定する最大押し下げ時間[ミリ秒]
#define MIDDLE_PRESS_MAX_MS (1500)
// ロングタップと判定する押し下げ時間[ミリ秒]
#define LONG_PRESS_TIMEOUT_MS (2500)
// ロングロングタップと判定する押し下げ時間[ミリ秒]
#define LONG_LONG_PRESS_TIMEOUT_MS (6000)
// ダブルタップ・トリプルタップと判定するタップ間隔[ミリ秒]
#define MULTI_PRESS_MIN_INTERVALMS (200)

//--------------------------------------------------------------------------------
class LongPressCheckTask : public thread::Runnable {
private:
	KeyDispatcher &dispatcher;	// こっちは参照を保持する
	const KeyEvent event;		// こっちはコピーを保持する
public:
	LongPressCheckTask(
		KeyDispatcher &dispatcher,
		const KeyEvent &event)
	:	dispatcher(dispatcher), event(event)
	{
		ENTER();
		EXIT();
	}

	virtual ~LongPressCheckTask() {
		ENTER();
		LOGD("破棄された");
		EXIT();
	}

	void run() {
		ENTER();

		dispatcher.handle_on_long_key_pressed(event);

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
	on_key_mode_changed(nullptr),
	on_brightness_changed(nullptr),
	on_scale_changed(nullptr)
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
 * @brief キーモードが変更されるときのコールバックをセット
 *
 * @param callback
 * @return KeyDispatcher&
 */
KeyDispatcher &KeyDispatcher::set_on_key_mode_changed(OnKeyModeChangedFunc callback) {
	ENTER();

	on_key_mode_changed = callback;

	RET(*this);
}

/**
 * @brief 輝度が変更されるときのコールバックをセット
 *
 * @param callback
 * @return KeyDispatcher&
 */
KeyDispatcher &KeyDispatcher::set_on_brightness_changed(OnBrightnessChangedFunc callback) {
	ENTER();

	on_brightness_changed = callback;

	RET(*this);
}

/**
 * @brief 拡大縮小率が変更されるときのコールバックをセット
 *
 * @param callback
 * @return KeyDispatcher&
 */
KeyDispatcher &KeyDispatcher::set_on_scale_changed(OnScaleChangedFunc callback) {
	ENTER();

	on_scale_changed = callback;

	RET(*this);
}

/**
 * @brief 映像効果が変更されるときのコールバックをセット
 *
 * @param callback
 * @return KeyDispatcher&
 */
KeyDispatcher &KeyDispatcher::set_on_effect_changed(OnEffectChangedFunc callback) {
	ENTER();

	on_effect_changed = callback;

	RET(*this);
}

/**
 * @brief GLFWからのキー入力イベントの処理
 * とりあえずは、GLFW_KEY_RIGHT(262), GLFW_KEY_LEFT(263), GLFW_KEY_DOWN(264), GLFW_KEY_UP(265)の
 * 4種類だけキー処理を行う
 *
 * @param event
 * @return int32_t
 */
/*public*/
int32_t KeyDispatcher::handle_on_key_event(const KeyEvent &event) {
	ENTER();

	int32_t result = 0;

	// XXX 複数キー同時押しのときに最後に押したキーに対してのみGLFW_REPEATがくるみたいなのでGLFW_REPEATは使わない
	const auto key = event.key;
	if ((key >= GLFW_KEY_RIGHT) && (key <= GLFW_KEY_UP)) {
		switch (event.action) {
		case GLFW_RELEASE:	// 0
			result = handle_on_key_up(event);
			break;
		case GLFW_PRESS:	// 1
			result = handle_on_key_down(event);
			break;
		case GLFW_REPEAT:	// 2
		default:
			break;
		}
	} else {
		LOGD("key=%d,scancode=%d/%s,action=%d,mods=%d",
			key, event.scancode, glfwGetKeyName(key, event.scancode),
			event.action, event.mods);
	}

	RETURN(result, int32_t);
}

/**
 * @brief キーの長押し確認用ラムダ式が生成されていることを確認、未生成なら新たに生成する
 *
 * @param event
 */
/*private*/
void KeyDispatcher::confirm_long_press_task(const KeyEvent &event) {
	ENTER();

	const auto key = event.key;
	if (long_key_press_tasks.find(key) == long_key_press_tasks.end()) {
		long_key_press_tasks[key] = std::make_shared<LongPressCheckTask>(*this, event);
	}

	EXIT();
}

/**
 * @brief 短押しかどうか, 排他制御してないので上位でロックすること
 *
 * @param key
 * @return true
 * @return false
 */
/*private*/
bool KeyDispatcher::is_short_pressed(const int &key) {
	return (key_state.find(key) != key_state.end())
		&& (key_state[key]->state == KEY_STATE_DOWN);
}

/**
 * @brief 長押しされているかどうか, 排他制御してないので上位でロックすること
 *
 * @param key
 * @return true
 * @return false
 */
/*private*/
bool KeyDispatcher::is_long_pressed(const int &key) {
	return (key_state.find(key) != key_state.end())
		&& (key_state[key]->state == KEY_STATE_DOWN_LONG);
}

/**
 * @brief handle_on_key_eventの下請け、キーが押されたとき
 * とりあえずは、GLFW_KEY_RIGHT(262), GLFW_KEY_LEFT(263), GLFW_KEY_DOWN(264), GLFW_KEY_UP(265)の
 * 4種類だけキー処理を行う
 *
 * @param event
 * @return int32_t
 */
/*private*/
int32_t KeyDispatcher::handle_on_key_down(const KeyEvent &event) {
	ENTER();

	const auto key = event.key;
	{
 		std::lock_guard<std::mutex> lock(state_lock);
		key_state[key] = std::make_shared<KeyEvent>(event);
	}
	confirm_long_press_task(event);
	handler.remove(long_key_press_tasks[key]);
	handler.post_delayed(long_key_press_tasks[key], LONG_PRESS_TIMEOUT_MS);

	RETURN(0, int32_t);
}

/**
 * @brief handle_on_key_eventの下請け、キーが離されたとき
 * とりあえずは、GLFW_KEY_RIGHT(262), GLFW_KEY_LEFT(263), GLFW_KEY_DOWN(264), GLFW_KEY_UP(265)の
 * 4種類だけキー処理を行う
 *
 * @param event
 * @return int32_t
 */
/*private*/
int32_t KeyDispatcher::handle_on_key_up(const KeyEvent &event) {
	ENTER();

	int result = -1;
	const auto key = event.key;
	confirm_long_press_task(event);
	handler.remove(long_key_press_tasks[key]);

	KeyEventSp state;
	key_mode_t current_key_mode;
	{
 		std::lock_guard<std::mutex> lock(state_lock);
		current_key_mode = key_mode;
		state = key_state[key];
		key_state[key] = std::make_shared<KeyEvent>(event);
	}
	const auto duration_ms = (event.event_time_ns - state->event_time_ns) / 1000000L;
	if ((state->state == KEY_STATE_DOWN) && (duration_ms >= SHORT_PRESS_MIN_MS)) {
		LOGD("on_key_up,key=%d,state=%d,duration_ms=%ld", key, state->state, duration_ms);
		// FIXME ここで同時押しの処理を行う

		if (result) {
			// 同時押しで処理済みでなければキーモード毎の処理を行う
			switch (current_key_mode) {
			case KEY_MODE_NORMAL:
				result = on_key_up_normal(event);
			case KEY_MODE_OSD:
				result = on_key_up_osd(event);
				break;
			default:
				LOGW("unknown key mode,%d", current_key_mode);
				break;
			}
		}
	} else if ((state->state == KEY_STATE_DOWN_LONG) && (duration_ms >= LONG_LONG_PRESS_TIMEOUT_MS)) {
		// FIXME 未実装 長長押しの処理
	}

	RETURN(result, int32_t);
}

/**
 * @brief 通常モードで短押ししたときの処理, handle_on_key_upの下請け
 *
 * @param event
 * @return int32_t
 */
int32_t KeyDispatcher::on_key_up_normal(const KeyEvent &event) {
	ENTER();

	const auto key = event.key;
	switch (key) {
	case GLFW_KEY_RIGHT:
	case GLFW_KEY_LEFT:
		// 輝度変更
		if (on_brightness_changed) {
			on_brightness_changed(key == GLFW_KEY_RIGHT);
		}
		break;
	case GLFW_KEY_DOWN:
	case GLFW_KEY_UP:
		// 拡大縮小
		if (on_scale_changed) {
			on_scale_changed(key == GLFW_KEY_UP);
		}
		break;
	default:
		LOGW("unexpected key code,%d", key);
		break;
	}

	RETURN(0, int32_t);
}

/**
 * @brief OSD操作モードで短押ししたときの処理, handle_on_key_upの下請け
 *
 * @param event
 * @return int32_t
 */
int32_t KeyDispatcher::on_key_up_osd(const KeyEvent &event) {
	ENTER();

	// FIXME 未実装

	RETURN(0, int32_t);
}

//--------------------------------------------------------------------------------
/**
 * @brief 長押し時間経過したときの処理
 *
 * @param event
 * @return int32_t
 */
int32_t KeyDispatcher::handle_on_long_key_pressed(const KeyEvent &event) {
	ENTER();

	int result = -1;
	const auto key = event.key;
	bool long_pressed = false;
	key_mode_t current_key_mode;
	{
 		std::lock_guard<std::mutex> lock(state_lock);
		current_key_mode = key_mode;
		if (key_state[key]->state == KEY_STATE_DOWN) {
			key_state[key]->state = KEY_STATE_DOWN_LONG;
			long_pressed = true;
		}
	}
	if (long_pressed) {
		const auto duration_ms = (event.event_time_ns - systemTime()) / 1000000L;
		LOGD("on_long_key_pressed,key=%d,duration_ms=%ld", key, duration_ms);
		// FIXME ここで同時押しの処理を行う

		if (result) {
			// 同時押しで処理済みでなければキーモード毎の処理を行う
			switch (current_key_mode) {
			case KEY_MODE_NORMAL:
				result = on_long_key_pressed_normal(event);
				break;
			case KEY_MODE_OSD:
				result = on_long_key_pressed_osd(event);
				break;
			default:
				LOGW("unknown key mode,%d", current_key_mode);
				break;
			}
		}
	}


	RETURN(result, int32_t);
}

/**
 * @brief 通常モードで長押し時間経過したときの処理, handle_on_long_key_pressedの下請け
 *
 * @param event
 * @return int32_t
 */
int32_t KeyDispatcher::on_long_key_pressed_normal(const KeyEvent &event) {
	ENTER();

	const auto key = event.key;
	// FIXME 未実装

	RETURN(0, int32_t);
}

/**
 * @brief OSD操作モードで長押し時間経過したときの処理, handle_on_long_key_pressedの下請け
 *
 * @param event
 * @return int32_t
 */
int32_t KeyDispatcher::on_long_key_pressed_osd(const KeyEvent &event) {
	ENTER();

	// FIXME 未実装

	RETURN(0, int32_t);
}

}   // namespace serenegiant::app
