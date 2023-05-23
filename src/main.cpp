/*
 * aAndUsb
 * Copyright (c) 2014-2023 saki t_saki@serenegiant.com
 * Distributed under the terms of the GNU Lesser General Public License (LGPL v3.0) License.
 * License details are in the file license.txt, distributed as part of this software.
 */

#if 1    // set 0 if you need debug log, otherwise set 1
	#ifndef LOG_NDEBUG
	#define LOG_NDEBUG
	#endif
	#undef USE_LOGALL
#else
//	#define USE_LOGALL
	#define USE_LOGD
	#undef LOG_NDEBUG
	#undef NDEBUG
#endif

#include "utilbase.h"
// app
#include "eye_app.h"

namespace app = serenegiant::app;

int main(int argc, const char *argv[]) {

	ENTER();

	app::EyeApp app;
	if (app.is_initialized()) {
		app.run();
	} else {
		LOGE("GLFWを初期化できなかった");
	}

	MARK("Finished.");
	
	RETURN(0, int);
}
