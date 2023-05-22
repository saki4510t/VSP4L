/*
 * Copyright (C) 2007 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _LIBS_UTILS_MUTEX_H
#define _LIBS_UTILS_MUTEX_H

#include <memory>
#include <stdint.h>
#include <sys/types.h>
#include <time.h>

#if defined(HAVE_PTHREADS)
# include <pthread.h>
#endif

#include "utilbase.h"
#include "Errors.h"
#include "atomics.h"

#define LOCAL_DEBUG 0

namespace android {

class Condition;

/*
 * Simple mutex class.  The implementation is system-dependent.
 *
 * The mutex must be unlocked by the thread that locked it.  They are not
 * recursive, i.e. the same thread can't lock it multiple times.
 */
class Mutex {
public:
    enum {
        PRIVATE = 0,
        SHARED = 1
    };
    
	/**
	 * コンストラクタ
	 */
    Mutex();
    /**
     * コンストラクタ
     * @param name
     */
    Mutex(const char* name);
    /**
     * コンストラクタ
     * @param type
     * @param name
     */
    Mutex(int type, const char *name = nullptr);
	/**
	 * ムーブコンストラクタ
	 * @param src
	 */
	Mutex(Mutex &&src) noexcept;
    /**
     * デストラクタ
     */
    ~Mutex();
	/**
	 * ムーブ代入演算子
	 * @param src
	 * @return
	 */
	Mutex &operator=(Mutex &&src) noexcept;

    /**
     * ミューテックスをロック
     * @return
     */
    status_t lock();
    /**
     * ミューテックスのロックを解除
     */
    void unlock();

    /**
     * ミューテックスのロックを試みる
     * @return 0: ミューテックスをロックできた, その他: エラー
     */
    status_t tryLock();

    // Manages the mutex automatically. It'll be locked when Autolock is
    // constructed and released when Autolock goes out of scope.
	/**
	 * ミューテックスのロック・アンロックをスコープに合わせて実行するためのヘルパークラス
	 * スコープに入った時(Autolockインスタンスが生成された時)にロック
	 * スコープを抜けてAutolockインスタンスが破棄されるときにアンロックされる
	 */
    class Autolock {
    public:
        inline Autolock(Mutex &mutex) : mLock(mutex)  { mLock.lock(); }
        inline Autolock(Mutex *mutex) : mLock(*mutex) { mLock.lock(); }
        inline ~Autolock() { mLock.unlock(); }
    private:
        Mutex& mLock;
    };

private:
    friend class Condition;
#if LOCAL_DEBUG
    volatile int lock_count;
#endif
    // コピーコンストラクタを無効に
    Mutex(const Mutex&) = delete;
    // 代入演算子を無効に
    Mutex& operator = (const Mutex&) = delete;
    
#if defined(HAVE_PTHREADS)
    std::unique_ptr<pthread_mutex_t> mutex;
#else
    void    _init();
    void*   mState;
#endif
};

typedef Mutex::Autolock AutoMutex;

#if defined(HAVE_PTHREADS)

/**
 * コンストラクタ
 */
inline Mutex::Mutex()
:	mutex(new pthread_mutex_t)
#if LOCAL_DEBUG
	, lock_count(0)
#endif
{
    pthread_mutex_init(mutex.get(), nullptr);
}

/**
 * コンストラクタ
 * @param name
 */
inline Mutex::Mutex(__attribute__((unused)) const char *name)
:	mutex(new pthread_mutex_t)
#if LOCAL_DEBUG
	, lock_count(0)
#endif
{
	pthread_mutex_init(mutex.get(), nullptr);
}

/**
 * コンストラクタ
 * @param type
 * @param name
 */
inline Mutex::Mutex(int type, __attribute__((unused)) const char *name)
:	mutex(new pthread_mutex_t)
#if LOCAL_DEBUG
	, lock_count(0)
#endif
{
    if (type == SHARED) {
        pthread_mutexattr_t attr;
        pthread_mutexattr_init(&attr);
        pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
        pthread_mutex_init(mutex.get(), &attr);
        pthread_mutexattr_destroy(&attr);
    } else {
        pthread_mutex_init(mutex.get(), nullptr);
    }
}

/**
 * ムーブコンストラクタ
 * @param src
 */
inline Mutex::Mutex(Mutex &&src) noexcept
:	mutex(std::move(src.mutex))
{
}

/**
 * デストラクタ
 */
inline Mutex::~Mutex() {
	if (mutex) {
		pthread_mutex_destroy(mutex.get());
		mutex.reset();
	}
}

/**
 * ムーブ代入演算子
 * @param src
 * @return
 */
inline Mutex &Mutex::operator=(Mutex &&src) noexcept {
	mutex = std::move(src.mutex);
	return *this;
}

/**
 * ミューテックスをロック
 * @return
 */
inline status_t Mutex::lock() {
#if LOCAL_DEBUG
	status_t result = -pthread_mutex_lock(&mMutex);
	if (LIKELY(!result)) {
		int cnt = __atomic_swap(lock_count, &lock_count);
		if (UNLIKELY(cnt)) LOGE("lock_count=%d", cnt);
		__atomic_inc(&lock_count);
	} else {
		LOGE("pthread_mutex_lock:r=%d", result);
	}
	return result;
#else
	if (mutex) {
		return -pthread_mutex_lock(mutex.get());
	} else {
		LOGE("Illegal state, already released!");
		return -1;
	}
#endif
}

/**
 * ミューテックスのロックを解除
 */
inline void Mutex::unlock() {
#if LOCAL_DEBUG
	int cnt = __atomic_dec(&lock_count) - 1;
	if (UNLIKELY(cnt)) LOGE("lock_count=%d", cnt);
#endif
	if (mutex) {
	    pthread_mutex_unlock(mutex.get());
	} else {
		LOGE("Illegal state, already released!");
	}
}

/**
 * ミューテックスのロックを試みる
 * @return 0: ミューテックスをロックできた, その他: エラー
 */
inline status_t Mutex::tryLock() {
#if LOCAL_DEBUG
	status_t result = -pthread_mutex_trylock(&mMutex);
	if (LIKELY(!result)) __atomic_inc(&lock_count);
    return result;
#else
    if (mutex) {
		return -pthread_mutex_trylock(mutex.get());
    } else {
		LOGE("Illegal state, already released!");
    	return -1;
    }
#endif
}

#endif // #if defined(HAVE_PTHREADS)

typedef std::shared_ptr<Mutex> MutexSp;
typedef std::unique_ptr<Mutex> MutexUp;

} // namespace android

#endif // _LIBS_UTILS_MUTEX_H
