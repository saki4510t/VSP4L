/*
 * aAndUsb
 * Copyright (c) 2014-2023 saki t_saki@serenegiant.com
 * Distributed under the terms of the GNU Lesser General Public License (LGPL v3.0) License.
 * License details are in the file license.txt, distributed as part of this software.
 */

#ifndef KEY_EVENT_H_
#define KEY_EVENT_H_

#include "internal.h"

#include "const.h"
#include "key_state.h"

namespace serenegiant::app {

class KeyEvent {
private:
public:
	const ImGuiKey key;
    const int scancode;
	const key_action_t action;
    const int mods;
    const nsecs_t event_time_ns;
    key_state_t state;

    explicit KeyEvent(const ImGuiKey &key, const int &scancode, const key_action_t &action, const int &mods)
    :	key(key), scancode(scancode),
        action(action), mods(mods),
        event_time_ns(systemTime()),
        state(KEY_STATE_UP)
    {
		switch (action) {
		case KEY_ACTION_RELEASE:	// 0
            state = KEY_STATE_UP;
			break;
		case KEY_ACTION_PRESSED:	// 1
		case KEY_ACTION_REPEAT:	// 2
            state = KEY_STATE_DOWN;
			break;
		default:
			break;
		}
    }
    KeyEvent(const KeyEvent &src)
    :	key(src.key), scancode(src.scancode),
        action(src.action), mods(src.mods),
        event_time_ns(src.event_time_ns),
        state(src.state)
    {
    }
    KeyEvent(const KeyEvent &&src)
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
