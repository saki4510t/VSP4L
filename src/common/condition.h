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

#ifndef _LIBS_UTILS_CONDITION_H
#define _LIBS_UTILS_CONDITION_H

#include <memory>
#include <stdint.h>
#include <sys/types.h>
#include <time.h>

#if defined(HAVE_PTHREADS)
# include <pthread.h>
#endif

#include "utilbase.h"
#include "mutex.h"
#include "times.h"

// ---------------------------------------------------------------------------
namespace android {
// ---------------------------------------------------------------------------

/*
 * Condition variable class.  The implementation is system-dependent.
 *
 * Condition variables are paired up with mutexes.  Lock the mutex,
 * call wait(), then either re-wait() if things aren't quite what you want,
 * or unlock the mutex and continue.  All threads calling wait() must
 * use the same mutex for a given Condition.
 */
class Condition {
public:
    enum {
        PRIVATE = 0,
        SHARED = 1
    };

    enum WakeUpType {
        WAKE_UP_ONE = 0,
        WAKE_UP_ALL = 1
    };

	/**
	 * コンストラクタ
	 */
    Condition();
    /**
     * コンストラクタ
     * @param type
     */
    Condition(int type);
	/**
	 * ムーブコンストラクタ
	 * @param src
	 */
	Condition(Condition &&src) noexcept;
    /**
     * デストラクタ
     */
    ~Condition();
    /**
     * ムーブ代入演算子
     * @param src
     * @return
     */
	Condition &operator=(Condition &&src) noexcept;
    // Wait on the condition variable.  Lock the mutex before calling.
    status_t wait(Mutex& mutex);
    // same with relative timeout[nano seconds]
    status_t waitRelative(Mutex& mutex, nsecs_t reltime);
    // Signal the condition variable, allowing one thread to continue.
    void signal();
    // Signal the condition variable, allowing one or all threads to continue.
    void signal(WakeUpType type) {
        if (type == WAKE_UP_ONE) {
            signal();
        } else {
            broadcast();
        }
    }
    // Signal the condition variable, allowing all threads to continue.
    void broadcast();

private:
#if defined(HAVE_PTHREADS)
	std::unique_ptr<pthread_cond_t> cond;
#else
    void*   mState;
#endif
	// コピーコンストラクタを無効に
	Condition(const Condition&) = delete;
	// 代入演算子を無効に
	Condition& operator = (const Condition&) = delete;
};

// ---------------------------------------------------------------------------

#if defined(HAVE_PTHREADS)

/**
 * コンストラクタ
 */
inline Condition::Condition()
:	cond(new pthread_cond_t)
{
    pthread_cond_init(cond.get(), nullptr);
}

/**
 * コンストラクタ
 * @param type
 */
inline Condition::Condition(int type)
:	cond(new pthread_cond_t)
{
    if (type == SHARED) {
        pthread_condattr_t attr;
        pthread_condattr_init(&attr);
        pthread_condattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
        pthread_cond_init(cond.get(), &attr);
        pthread_condattr_destroy(&attr);
    } else {
        pthread_cond_init(cond.get(), nullptr);
    }
}

/**
 * ムーブコンストラクタ
 * @param src
 */
inline Condition::Condition(Condition &&src) noexcept
:	cond(std::move(src.cond))
{
}

/**
 * デストラクタ
 */
inline Condition::~Condition() {
	if (cond) {
		pthread_cond_destroy(cond.get());
		cond.reset();
	}
}

/**
 * ムーブ代入演算子
 * @param src
 * @return
 */
inline Condition &Condition::operator=(Condition &&src) noexcept
{
	cond = std::move(src.cond);
	return *this;
}

/**
 * シグナル状態になるまで待機する
 * @param mutex
 * @return
 */
inline status_t Condition::wait(Mutex& mutex) {
	if (cond && mutex.mutex) {
		return -pthread_cond_wait(cond.get(), mutex.mutex.get());
	} else {
		LOGE("Illegal state, already released!");
		return -1;
	}
}

/**
 * シグナル状態になるまで待機する、最大待ち時間を指定
 * @param mutex
 * @param reltime
 * @return
 */
inline status_t Condition::waitRelative(Mutex& mutex, nsecs_t reltime) {
#if defined(HAVE_PTHREAD_COND_TIMEDWAIT_RELATIVE)
    struct timespec ts;
    ts.tv_sec  = reltime/1000000000;
    ts.tv_nsec = reltime%1000000000;
    return -pthread_cond_timedwait_relative_np(&mCond, &mutex.mMutex, &ts);
#else // HAVE_PTHREAD_COND_TIMEDWAIT_RELATIVE
    struct timespec ts;
#if defined(HAVE_POSIX_CLOCKS)
    clock_gettime(CLOCK_REALTIME, &ts);
#else // HAVE_POSIX_CLOCKS
    // we don't support the clocks here.
    struct timeval t;
    gettimeofday(&t, nullptr);
    ts.tv_sec = t.tv_sec;
    ts.tv_nsec= t.tv_usec*1000;
#endif // HAVE_POSIX_CLOCKS
    ts.tv_sec += reltime/1000000000;
    ts.tv_nsec+= reltime%1000000000;
    if (ts.tv_nsec >= 1000000000) {
        ts.tv_nsec -= 1000000000;
        ts.tv_sec  += 1;
    }
    if (cond) {
		return -pthread_cond_timedwait(cond.get(), mutex.mutex.get(), &ts);
    } else {
		LOGE("Illegal state, already released!");
		return -1;
    }
#endif // HAVE_PTHREAD_COND_TIMEDWAIT_RELATIVE
}

/**
 * シグナル状態にして待ちがあれば１つ解除する
 */
inline void Condition::signal() {
	if (cond) {
		pthread_cond_signal(cond.get());
	} else {
		LOGE("Illegal state, already released!");
	}
}

/**
 * シグナル状態にして全ての待ちを解除する
 */
inline void Condition::broadcast() {
	if (cond) {
		pthread_cond_broadcast(cond.get());
	} else {
		LOGE("Illegal state, already released!");
	}
}

#endif // HAVE_PTHREADS

typedef std::shared_ptr<Condition> ConditionSp;
typedef std::unique_ptr<Condition> ConditionUp;

// ---------------------------------------------------------------------------
}; // namespace android
// ---------------------------------------------------------------------------

#endif // _LIBS_UTILS_CONDITON_H
