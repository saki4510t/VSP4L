/*
 * aAndUsb
 * Copyright (c) 2014-2023 saki t_saki@serenegiant.com
 * Distributed under the terms of the GNU Lesser General Public License (LGPL v3.0) License.
 * License details are in the file license.txt, distributed as part of this software.
 */

#ifndef KEY_EVENT_H_
#define KEY_EVENT_H_

#include "times.h"

namespace serenegiant::app {

class KeyEvent {
private:
public:
	const int key;
    const int scancode;
	const int action;
    const int mods;
    const nsecs_t event_time_ns;

    explicit KeyEvent(const int &key, const int &scancode, const int &action, const int &mods)
    :	key(key), scancode(scancode),
        action(action), mods(mods),
        event_time_ns(systemTime())
    {
    }
};

}	// namespace serenegiant::app

#endif /* KEY_EVENT_H_ */
