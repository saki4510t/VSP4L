/*
 * aAndUsb
 * Copyright (c) 2014-2023 saki t_saki@serenegiant.com
 * Distributed under the terms of the GNU Lesser General Public License (LGPL v3.0) License.
 * License details are in the file license.txt, distributed as part of this software.
 */

#ifndef HANDLER_H_
#define HANDLER_H_

#include <cstdlib>
#include <functional>
#include <map>
#include <memory>
#include <thread>

#include "condition.h"
#include "mutex.h"
#include "times.h"

namespace serenegiant::thread {

/**
 * @brief 実行オブジェクトのインターフェース(純粋仮想クラス)
 *
 */
class Runnable {
public:
    /**
     * @brief Destroy the Runnable object
     *
     */
    virtual ~Runnable() = default;
    virtual void run() = 0;
};

/**
 * @brief 実行オブジェクトのラムダ関数の型
 *
 */
typedef std::function<void()> RunnableLambdaType;

/**
 * @brief ラムダ式をラップするためのRunnableクラス
 *
 */
class RunnableLambda final : public Runnable {
private:
    /**
     * @brief ::equalsで比較するときに使うラムダ関数の型情報
     *
     */
    const std::type_info &id;
protected:
public:
    /**
     * @brief 実行するラムダ関数
     *
     */
    RunnableLambdaType lambda;
    /**
     * @brief コンストラクタ
     *
     * @param lambda
     */
    explicit RunnableLambda(RunnableLambdaType lambda);
    /**
     * @brief 内包するラムダ関数を実行
     *        Runnableの純粋仮想関数を実装
     *
     */
    void run() override;
    /**
     * @brief このオブジェクトが保持しているラムダ関数と指定したタスクが一致するかどうかを確認
     *
     * @param task
     * @return true 指定したタスクと一致する場合
     * @return false 指定したタスクと一致しない場合
     */
    bool equals(RunnableLambdaType task);
};

// 前方参照宣言
class Handler;

/**
 * @brief Handlerのタスクのキューと遅延実行処理を分離
 *
 */
class Looper {
friend Handler;
private:
    volatile bool running;
    /**
     * @brief 実行待ちタスクを保持するためのキューとして使うmultimap
     * キーとして実行予定時刻(モノトニックシステム時刻)、値としてタスクオブジェクトを保持する
     */
    std::multimap<nsecs_t, std::shared_ptr<Runnable>> queue;
    /**
     * @brief キューの排他制御のためのミューテックス
     *
     */
    mutable android::Mutex queue_lock;
    /**
     * @brief キュー変更時にワーカースレッドを起床させるための同期オブジェクト
     *
     */
    android::Condition queue_sync;
protected:
    /**
     * @brief 実行待ちキューへタスクを追加する
     *
     * @param task
     * @param run_at_ns
     */
    void insert(std::shared_ptr<Runnable> task, const nsecs_t &run_at_ns);
    /**
     * @brief 指定したタスクが未実行でキューに存在する場合に取り除く
     *        すでに実行中または実行済の場合は何もしない
     *
     * @param task
     */
    void remove(std::unique_ptr<Runnable> task);
    /**
     * @brief 指定したタスクが未実行でキューに存在する場合に取り除く
     *        すでに実行中または実行済の場合は何もしない
     *
     * @param task
     */
    void remove(std::shared_ptr<Runnable> task);
    /**
     * @brief 指定したタスクが未実行でキューに存在する場合に取り除く
     *        すでに実行中または実行済の場合は何もしない
     *
     * @param task
     */
    void remove(RunnableLambdaType task);
    /**
     * @brief すべての未実行タスクをキューから取り除く
     *
     */
    void remove_all();
public:
    /**
     * @brief コンストラクタ
     *
     */
    Looper();
    /**
     * @brief デストラクタ
     *
     */
    virtual ~Looper();
    /**
     * @brief 遅延実行処理を終了させる
     *
     */
    void quit();
    /**
     * @brief 遅延実行処理を実行
     *        quitが呼び出されるまで返らないので注意！
     *
     */
    void loop();

    /**
     * @brief 遅延実行中かどうかを取得
     *
     * @return true
     * @return false
     */
    inline bool is_running() const { return running; };
};

/**
 * @brief AndroidのHandlerと同じようにワーカースレッド上で
 *        指定したタイミングでタスクを実行するためのヘルパークラス
 *
 */
class Handler {
private:
    const bool own_looper;
    std::unique_ptr<Looper> my_looper;
    /**
     * @brief ワーカースレッド
     *
     */
    std::thread worker_thread;
    /**
     * @brief ワーカースレッドの実行部
     *
     */
    void worker_thread_func();
protected:
public:
    /**
     * @brief コンストラクタ
     *        Looperを指定しないときは自前でLooperとワーカースレッドを生成して遅延実行する・
     *        Looperが指定されているときはワーカースレッドは実行しないので自前でワーカースレッド上でLooper::loopを呼び出すこと
     * @param looper
     */
    Handler(std::unique_ptr<Looper> looper = nullptr);
    /**
     * @brief デストラクタ
     *
     */
    ~Handler();
    /**
     * @brief ワーカースレッドを終了要求する
     *        一度呼び出すと再利用はできない
     *
     */
    void terminate();

    /**
     * @brief 指定したタスクをすぐに実行するようにキューに追加する
     *
     * @param task
     */
    inline void post(std::shared_ptr<Runnable> task) {
        post_delayed(std::move(task), 0);
    }
    /**
     * @brief 指定したタスクをすぐに実行するようにキューに追加する
     *
     * @param task
     */
    inline void post(RunnableLambdaType task) {
        post_delayed(task, 0);
    }
    /**
     * @brief 指定したタスクを指定した時間遅延実行するようにキューに追加する
     *
     * @param task
     * @param delay_ms
     */
    inline void post_delayed(RunnableLambdaType task, const nsecs_t &delay_ms) {
        post_delayed(std::make_unique<RunnableLambda>(task), delay_ms);
    }
    /**
     * @brief 指定したタスクを指定した時間遅延実行するようにキューに追加する
     *
     * @param task
     * @param delay_ms
     */
    void post_delayed(std::shared_ptr<Runnable> task, const nsecs_t &delay_ms);
    /**
     * @brief 指定したタスクが未実行でキューに存在する場合に取り除く
     *        すでに実行中または実行済の場合は何もしない
     *
     * @param task
     */
    void remove(std::unique_ptr<Runnable> task);
    /**
     * @brief 指定したタスクが未実行でキューに存在する場合に取り除く
     *        すでに実行中または実行済の場合は何もしない
     *
     * @param task
     */
    void remove(std::shared_ptr<Runnable> task);
    /**
     * @brief 指定したタスクが未実行でキューに存在する場合に取り除く
     *        すでに実行中または実行済の場合は何もしない
     *
     * @param task
     */
    void remove(RunnableLambdaType task);
    /**
     * @brief すべての未実行タスクをキューから取り除く
     *
     */
    void remove_all();
};

}   // namespace serenegiant::thread

#endif // HANDLER_H_
