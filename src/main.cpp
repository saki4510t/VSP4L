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

#include <string>
#include <unordered_map>

#include <getopt.h>

#include "utilbase.h"
// app
#include "eye_app.h"

namespace app = serenegiant::app;

/**
 * @brief メイン関数
 *        EyeAppを生成して呼び出すだけ
 * 
 * @param argc 
 * @param argv 
 * @return int 
 */
int main(int argc, char *const *argv) {

	ENTER();

	int opt;
	int longindex;
	std::unordered_map<std::string, std::string> options = init_options();

	while ((opt = getopt_long(argc, argv, "Dd:f:u:n:", LONG_OPTS, &longindex)) != -1) {
		LOGD("%d %s", longindex, LONG_OPTS[longindex].name);
		// FIXME 値のチェックをしたほうがいいかも
		options[LONG_OPTS[longindex].name] = optarg;
	}

	app::EyeApp app(options);
	if (app.is_initialized()) {
		app.run();
	} else {
		LOGE("GLFWを初期化できなかった");
	}

	MARK("Finished.");
	
	RETURN(0, int);
}
