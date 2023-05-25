#ifndef EYE_APP_H_
#define EYE_APP_H_

#include <thread>
#include <unordered_map>

// common
#include "handler.h"

// aandusb/pipeline
#include "pipeline/pipeline_gl_renderer.h"

#include "key_event.h"

namespace pipeline = serenegiant::pipeline;

namespace serenegiant::app {
class Window;

enum {
	KEY_MODE_ZOOM = 0,      // 拡大縮小モード
	KEY_MODE_BRIGHTNESS,    // 輝度調整モード
	KEY_MODE_OSD,           // OSD操作モード
};

class EyeApp {
private:
	const bool initialized;
	volatile bool is_running;
	thread::Handler handler;
	// Handlerの動作テスト用
	std::function<void()> test_task;
	// キーの押し下げ状態を保持するハッシュマップ
	std::unordered_map<int, int> key_state;
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
	 * @brief 描画処理を実行
	 *
	 * @param window
	 * @param renderer
	 */
	void handle_draw(Window &window, pipeline::GLRendererPipelineSp &renderer);
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
	EyeApp();
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
