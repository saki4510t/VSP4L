/*
 * aAndUsb
 * Copyright (c) 2014-2023 saki t_saki@serenegiant.com
 * Distributed under the terms of the GNU Lesser General Public License (LGPL v3.0) License.
 * License details are in the file license.txt, distributed as part of this software.
 */

#ifndef KEY_EVENT_H_
#define KEY_EVENT_H_

#include <memory>

#include <GLFW/glfw3.h>

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

class KeyEvent {
private:
public:
	const int key;
    const int scancode;
	const int action;
    const int mods;
    const nsecs_t event_time_ns;
    key_state_t state;

    explicit KeyEvent(const int &key, const int &scancode, const int &action, const int &mods)
    :	key(key), scancode(scancode),
        action(action), mods(mods),
        event_time_ns(systemTime()),
        state(KEY_STATE_UP)
    {
		switch (action) {
		case GLFW_RELEASE:	// 0
            state = KEY_STATE_UP;
			break;
		case GLFW_PRESS:	// 1
		case GLFW_REPEAT:	// 2
            state = KEY_STATE_DOWN;
			break;
		default:
			break;
		}
    }
    explicit KeyEvent(const KeyEvent &src)
    :	key(src.key), scancode(src.scancode),
        action(src.action), mods(src.mods),
        event_time_ns(src.event_time_ns),
        state(src.state)
    {
    }
    explicit KeyEvent(const KeyEvent &&src)
    :	key(src.key), scancode(src.scancode),
        action(src.action), mods(src.mods),
        event_time_ns(src.event_time_ns),
        state(src.state)
    {
    }
};

typedef std::shared_ptr<KeyEvent> KeyEventSp;
typedef std::unique_ptr<KeyEvent> KeyEventUp;

}	// namespace serenegiant::app

#endif /* KEY_EVENT_H_ */
