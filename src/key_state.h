/*
 * aAndUsb
 * Copyright (c) 2014-2023 saki t_saki@serenegiant.com
 * Distributed under the terms of the GNU Lesser General Public License (LGPL v3.0) License.
 * License details are in the file license.txt, distributed as part of this software.
 */

#ifndef KEY_STATE_H_
#define KEY_STATE_H_

#include <memory>

#include "times.h"

namespace serenegiant::app {

/**
 * @brief キーの押し下げ状態定数
 * 
 */
typedef enum {
	KEY_STATE_HANDLED = -1,	// 処理済み
	KEY_STATE_UP = 0,		// キーが押されていない
	KEY_STATE_DOWN,			// キーが押された
	KEY_STATE_DOWN_LONG,	// キーが長押しされた
} key_state_t;

class KeyState {
private:
public:
	const int key;
    nsecs_t press_time_ns;
    nsecs_t last_tap_time_ns;
    int tap_caount;
    key_state_t state;

    explicit KeyState(const int &key)
    :	key(key),
        press_time_ns(systemTime()),
        last_tap_time_ns(systemTime()),
        tap_caount(0),
        state(KEY_STATE_UP)
    {
    }

    KeyState(const KeyState &src)
    :	key(src.key),
        press_time_ns(src.press_time_ns),
        last_tap_time_ns(src.last_tap_time_ns),
        tap_caount(src.tap_caount),
        state(src.state)
    {
    }

    KeyState(const KeyState &&src)
    :	key(src.key),
        press_time_ns(src.press_time_ns),
        last_tap_time_ns(src.last_tap_time_ns),
        tap_caount(src.tap_caount),
        state(src.state)
    {
    }
};

typedef std::shared_ptr<KeyState> KeyStateSp;
typedef std::unique_ptr<KeyState> KeyStateUp;

}	// namespace serenegiant::app

#endif /* KEY_STATE_H_ */
