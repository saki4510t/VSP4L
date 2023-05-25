#ifndef EYE_APP_H_
#define EYE_APP_H_

#include <thread>

// common
#include "handler.h"

// aandusb/pipeline
#include "pipeline/pipeline_gl_renderer.h"

namespace pipeline = serenegiant::pipeline;


namespace serenegiant {
class Window;

namespace app {

class EyeApp {
private:
    const bool initialized;
    volatile bool is_running;
    thread::Handler handler;
    // Handlerの動作テスト用
    std::function<void()> test_task;
    /**
     * @brief GLFWからのキー入力イベントの処理
     * とりあえずは、GLFW_KEY_RIGHT(262), GLFW_KEY_LEFT(263), GLFW_KEY_DOWN(264), GLFW_KEY_UP(265)の
     * 4種類だけキー処理を行う
     * 
     * @param key 
     * @param scancode 
     * @param action 
     * @param mods 
     * @return int32_t 
     */
    int32_t handle_on_key_event(const int &key, const int &scancode, const int &action, const int &mods);
    /**
     * @brief handle_on_key_eventの下請け、キーが押されたとき/押し続けているとき
     * とりあえずは、GLFW_KEY_RIGHT(262), GLFW_KEY_LEFT(263), GLFW_KEY_DOWN(264), GLFW_KEY_UP(265)の
     * 4種類だけキー処理を行う
     * 
     * @param key 
     * @param scancode 
     * @param action 
     * @param mods 
     * @return int32_t 
     */
    int32_t handle_on_key_down(const int &key, const int &scancode, const int &action, const int &mods);
    /**
     * @brief handle_on_key_eventの下請け、キーが離されたとき
     * とりあえずは、GLFW_KEY_RIGHT(262), GLFW_KEY_LEFT(263), GLFW_KEY_DOWN(264), GLFW_KEY_UP(265)の
     * 4種類だけキー処理を行う
     * 
     * @param key 
     * @param scancode 
     * @param action 
     * @param mods 
     * @return int32_t 
     */
    int32_t handle_on_key_up(const int &key, const int &scancode, const int &action, const int &mods);
    /**
     * @brief 描画処理を実行
     * 
     * @param window 
     * @param renderer 
     */
    void handle_draw(serenegiant::Window &window, pipeline::GLRendererPipelineSp &renderer);
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

}   // namespace app
}   // namespace serenegiant

#endif /* EYE_APP_H_ */
