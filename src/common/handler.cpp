/*
 * aAndUsb
 * Copyright (c) 2014-2023 saki t_saki@serenegiant.com
 * Distributed under the terms of the GNU Lesser General Public License (LGPL v3.0) License.
 * License details are in the file license.txt, distributed as part of this software.
 */

#define LOG_TAG "Handler"

#if 1	// デバッグ情報を出さない時は1
	#ifndef LOG_NDEBUG
		#define	LOG_NDEBUG		// LOGV/LOGD/MARKを出力しない時
	#endif
	#undef USE_LOGALL			// 指定したLOGxだけを出力
#else
//	#define USE_LOGALL
	#define USE_LOGD
	#undef LOG_NDEBUG
	#undef NDEBUG
	#define DEBUG_GL_CHECK			// GL関数のデバッグメッセージを表示する時
#endif

#include "utilbase.h"

#include "handler.h"

namespace serenegiant::thread {

/**
 * @brief c++のerase_ifはc++20以降なので同様の関数を自前で実装
 * 
 * @param q 
 * @param compare 
 * @return int 
 */
static int erase_if(
    std::multimap<nsecs_t, std::shared_ptr<Runnable>> &q,
    std::function<bool(std::pair<nsecs_t, std::shared_ptr<Runnable>>)> compare) {

    auto original_size = q.size();
    for (auto itr = q.begin(), last = q.end(); itr != last; ) {
        if (compare(*itr)) {
            LOGD("erase");
            itr = q.erase(itr);
        } else {
            ++itr;
        }
    }
    return original_size - q.size();
}

//--------------------------------------------------------------------------------
/**
 * @brief コンストラクタ
 * 
 * @param lambda 
 */
Handler::RunnableLambda::RunnableLambda(RunnableLambdaType lambda)
:   lambda(lambda),
    id(typeid(decltype(lambda)))
{
    ENTER();

    LOGD("lambda_name=%s", id.name());

    EXIT();
}

/**
 * @brief 内包するラムダ関数を実行
 *        Runnableの純粋仮想関数を実装
 * 
 */
void Handler::RunnableLambda::run() {
    ENTER();

    lambda();

    EXIT();
}

/**
 * @brief このオブジェクトが保持しているラムダ関数と指定したタスクが一致するかどうかを確認
 * 
 * @param task 
 * @return true 指定したタスクと一致する場合
 * @return false 指定したタスクと一致しない場合
 */
bool Handler::RunnableLambda::equals(RunnableLambdaType lambda) {
    LOGD("lambda_name=%s", typeid(decltype(lambda)).name());
    return typeid(decltype(lambda)) == id;
}

//--------------------------------------------------------------------------------
/**
 * @brief コンストラクタ
 *        自動的にワーカースレッドが起動する
 */
Handler::Handler()
:   running(false),
    worker_thread()
{
    ENTER();

    running = true;
    worker_thread = std::thread([this] { worker_thread_func(); });

    EXIT();
}

/**
 * @brief デストラクタ
 * 
 */
Handler::~Handler() {
    ENTER();

    terminate();

    EXIT();
}

/**
 * @brief ワーカースレッドを終了要求する
 *        一度呼び出すと再利用はできない
 * 
 */
void Handler::terminate() {
    ENTER();

    running = false;
    queue_sync.signal();
    if (worker_thread.joinable()) {
        worker_thread.join();
    }

    EXIT();
}

/**
 * @brief 指定したタスクを指定した時間遅延実行するようにキューに追加する
 * 
 * @param task 
 * @param delay_ms 
 */
void Handler::post_delayed(std::unique_ptr<Runnable> task, const nsecs_t &delay_ms) {
    ENTER();

    android::Mutex::Autolock lock(queue_lock);
    auto t = systemTime();
    if (delay_ms > 0) {
        t = t + delay_ms * 1000000L;
        LOGD("will run at %ld", t);
    }
    queue.insert(std::make_pair(t, std::move(task)));
    queue_sync.signal();

    EXIT();
}

/**
 * @brief 指定したタスクが未実行でキューに存在する場合に取り除く
 *        すでに実行中または実行済の場合は何もしない
 * 
 * @param task 
 */
void Handler::remove(std::unique_ptr<Runnable> task) {
    ENTER();

    android::Mutex::Autolock lock(queue_lock);
    // 実行待ちキュー内に同じRunnableが存在すれば削除する
    const auto value = task.get();
    erase_if(queue, [&](std::pair<nsecs_t, std::shared_ptr<Runnable>> it) {
        const auto v = it.second.get();
        return v == value;
    });
    queue_sync.signal();

    EXIT(); 
}

/**
 * @brief 指定したタスクが未実行でキューに存在する場合に取り除く
 *        すでに実行中または実行済の場合は何もしない
 * 
 * @param task 
 */
void Handler::remove(RunnableLambdaType task) {
    ENTER();

    android::Mutex::Autolock lock(queue_lock);
    // 実行待ちキュー内に同じRunnableLambdaTypeが存在すれば削除する
    erase_if(queue, [&](std::pair<nsecs_t, std::shared_ptr<Runnable>> it) {
        const auto v = dynamic_cast<RunnableLambda *>(it.second.get());
        return v && v->equals(task);
    });
    queue_sync.signal();

    EXIT();
}

/**
 * @brief すべての未実行タスクをキューから取り除く
 * 
 */
void Handler::remove_all() {
    ENTER();

    android::Mutex::Autolock lock(queue_lock);
    queue.clear();
    queue_sync.signal();

    EXIT();
}

/**
 * @brief 遅延実行するためのワーカースレッド
 * 
 */
void Handler::worker_thread_func() {
    ENTER();

    for ( ; running ; ) {
        std::shared_ptr<Runnable> task = nullptr;
        nsecs_t next = 0;
        {
            android::Mutex::Autolock lock(queue_lock);
            auto current = systemTime();
            if (!queue.empty()) {
                // 実行待ちが存在するとき
                auto begin = queue.begin();
                next = begin->first;
                if (next <= current) {
                    LOGD("すでに実行予定時刻を過ぎているときcurrent=%ld,next=%ld", current, next);
                    task = begin->second;
                    queue.erase(begin);
                }
            }
            if (!task) {
                LOGD("実行するタスクがない");
                if (next > current) {
                    LOGD("waitRelative:%ld", (next - current));
                    queue_sync.waitRelative(queue_lock, next - current);
                } else {
                    LOGD("wait:");
                    queue_sync.wait(queue_lock);
                }
                continue;
            }
        }
        if (task) {
            // タスクを実行
            task->run();
        }
    }

    MARK("finished");

    EXIT();
}

}   // namespace serenegiant::thread
