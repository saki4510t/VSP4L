/*
 * aAndUsb
 * Copyright (c) 2014-2023 saki t_saki@serenegiant.com
 * Distributed under the terms of the GNU Lesser General Public License (LGPL v3.0) License.
 * License details are in the file license.txt, distributed as part of this software.
 */

#define FOUNT_FRAMES 0
#define VIDEO_WIDTH 1280
#define VIDEO_HEIGHT 720

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

#include <stdio.h>
#include <string>
#include <cstring>
#include <unordered_map>
#include <thread>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "utilbase.h"

// common
#include "glutils.h"
// aandusb
#include "window.h"
// aandusb/pipeline
#include "pipeline/pipeline_gl_renderer.h"
#include "pipeline/pipeline_v4l2_source.h"
// app
#include "app_const.h"

namespace sere = serenegiant;
namespace pipeline = serenegiant::pipeline;
namespace v4l2 = serenegiant::v4l2::pipeline;

//================================================================================
static volatile bool is_running;
static v4l2::V4L2SourcePipelineSp source;
static pipeline::GLRendererPipelineSp renderer;

static void handle_draw(sere::Window &window) {
	ENTER();

#if FOUNT_FRAMES && !defined(LOG_NDEBUG) && !defined(NDEBUG)
    static int cnt = 0;
    if ((++cnt % 100) == 0) {
        MARK("cnt=%d", cnt);
    }
#endif
	if (LIKELY(source)) {
		if (UNLIKELY(!renderer)) {
			const char* versionStr = (const char*)glGetString(GL_VERSION);
			LOGD("create and start GLRendererPipeline,%s", versionStr);
			renderer = std::make_shared<pipeline::GLRendererPipeline>(300);
			source->set_pipeline(renderer.get());
			renderer->start();
		}
		renderer->on_draw();
	}

	EXIT();
}

/**
 * 描画処理等を実行するスレッドのメイン関数
 */
void handler_thread_func() {
    ENTER();

    LOGD("GLFWの初期化");
    if (sere::Window::initialize()) {
        is_running = false;
        return;
    }

	source = std::make_shared<v4l2::V4L2SourcePipeline>("/dev/video0");
	if (source && !source->open() && !source->find_stream(VIDEO_WIDTH, VIDEO_HEIGHT)) {
		LOGI("supported=%s", source->get_supported_size().c_str());
		source->resize(VIDEO_WIDTH, VIDEO_HEIGHT);
		if (!source->start()) {
			LOGD("windowを初期化");
			sere::Window window(VIDEO_WIDTH / 2, VIDEO_HEIGHT / 2, "aAndUsb-Linux sample");
			if (is_running && window.is_valid()) {
				LOGD("GLFWのイベントループ開始");
				// ウィンドウが開いている間繰り返す
				while (is_running && window) {
					// 描画処理
					handle_draw(window);
					// ダブルバッファーをスワップ
					window.swap_buffers();
				}
				LOGD("GLFWのイベントループ終了");
				source.reset();
				renderer.reset();
			} else {
				// ウィンドウ作成に失敗した処理
				LOGE("Can't create GLFW window.");
			}
		}
	} else {
		LOGE("Failed to init v4l2");
	}

	LOGD("GLFWスレッド終了");

	source.reset();
    is_running = false;

    EXIT();
}

/**
 * 指定したファイルディスクリプタから読み込み可能かどうかをチェック
 * @param fd 読み込み可能かどうかを確認するファイルディスクリプタ
 * @param 0: 読み込みできない, 1: 読み込みできる
 */
static int can_read(int fd) {
    fd_set fdset;
    struct timeval tv { .tv_sec = 0, .tv_usec = 0};
    FD_ZERO(&fdset);
    FD_SET(fd, &fdset);
    return select(fd+1 , &fdset, nullptr, nullptr, &tv);
}

int main(int argc, const char *argv[]) {

	ENTER();

	LOGI("aAndUsb test app for linux.\n");
	LOGI("Pressing [CTRL+C] or [ENTER] will finish this app\n");

    // 入力した文字をすぐに引き渡す
    system("stty -echo -icanon min 1 time 0");

    is_running = true;
    std::thread handler_thread(handler_thread_func);

	bool first_time = true;
    for ( ; is_running ; ) {
        // ブロッキングしないように標準入力stdin(ファイルディスクリプタ0)から
        // 読み込み可能な場合のみgetcharを呼び出す
        if (can_read(0)) {
            // 何か入力するまで実行する
            const char c = getchar();
			if (first_time) {
				// なぜか最初に[ENTER]が来るので1つ目を読み飛ばす
				first_time = false;
				continue;
			}
            switch (c) {
            }
            if (c != EOF) break;
        }
        usleep(300000);
    }
    is_running = false;
    if (handler_thread.joinable()) {
        handler_thread.join();
    }
    // コンソールの設定をデフォルトに戻す
    system("stty echo -icanon min 1 time 0");

	LOGI("Finished.");
	
	RETURN(0, int);
}
