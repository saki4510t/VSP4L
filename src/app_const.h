/*
 * aAndUsb
 * Copyright (c) 2014-2023 saki t_saki@serenegiant.com
 * Distributed under the terms of the GNU Lesser General Public License (LGPL v3.0) License.
 * License details are in the file license.txt, distributed as part of this software.
 */

#ifndef APP_CONST_H_
#define APP_CONST_H_

#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <inttypes.h>

#include <map>
#include <string>
#include <typeinfo>
#include <iostream>
#include <cinttypes>

#include <rapidjson/rapidjson.h>
#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/error/en.h>

#include "endian_unaligned.h"
#include "mutex.h"
#include "condition.h"
#include "times.h"
#include "semaphore.h"

using namespace android;
using namespace rapidjson;

#endif /* APP_CONST_H_ */
