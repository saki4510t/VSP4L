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

    int32_t handle_on_key_event(const int &key, const int &scancode, const int &action, const int &mods);
    int32_t handle_on_key_down(const int &key, const int &scancode, const int &action, const int &mods);
    int32_t handle_on_key_up(const int &key, const int &scancode, const int &action, const int &mods);
    void handle_draw(serenegiant::Window &window, pipeline::GLRendererPipelineSp &renderer);
    /**
     * @brief ワーカースレッドの実行関数
     * 
     */
    void handler_thread_func();
protected:
public:
    /**
     * @brief Construct a new App object
     * 
     */
    EyeApp();
    /**
     * @brief Destroy the App object
     * 
     */
    ~EyeApp();
    /**
     * @brief イベントループを実行
     * 
     */
    void run();

    inline bool is_initialized() const { return initialized; };
};

}   // namespace app
}   // namespace serenegiant

#endif /* EYE_APP_H_ */
