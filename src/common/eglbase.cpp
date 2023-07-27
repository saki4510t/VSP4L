/*
 * aAndUsb
 * Copyright (c) 2014-2023 saki t_saki@serenegiant.com
 * Distributed under the terms of the GNU Lesser General Public License (LGPL v3.0) License.
 * License details are in the file license.txt, distributed as part of this software.
 */

#define LOG_TAG "EGLBase"

#if 1	// デバッグ情報を出さない時は1
	#ifndef LOG_NDEBUG
		#define	LOG_NDEBUG		// LOGV/LOGD/MARKを出力しない時
	#endif
	#undef USE_LOGALL			// 指定したLOGxだけを出力
	#ifndef LOG_NDEBUG
		#define	LOG_NDEBUG		// LOGV/LOGD/MARKを出力しない時
	#endif
#else
//	#define USE_LOGALL
	#undef LOG_NDEBUG
	#undef NDEBUG
	#define DEBUG_EGL_CHECK
#endif

#include <sstream>
#include <vector>
#include <iterator>	// istream_iterator

#include "utilbase.h"
#include "eglbase.h"
#include "glutils.h"

namespace serenegiant::egl {

// 毎回Windowサイズを取得するようにする時は1
#define UPDATE_WINDOW_SIZE_EVERYTIME 0

/**
 * eglGetErrorで取得したEGLのエラーをログに出力する
 * @param filename
 * @param line
 * @param func
 * @param op 実行した関数名
 */
/*public,global*/
void checkEglError(
	const char *filename, const int &line, const char *func,
	const char *op) {
#if 0
    for (EGLint error = eglGetError(); error != EGL_SUCCESS; error = eglGetError()) {
        LOG(LOG_ERROR, LOG_TAG, "%s:%d:%s:eglError (0x%x) after %s()",
        	filename, line, func, error, op);
    }
#else
	EGLint error = eglGetError();
	if (error != EGL_SUCCESS) {
		LOGE("%s:%d:%s:eglError (0x%x) after %s()",
		  	filename, line, func, error, op);
	}
#endif
}

/**
 * EGLConfigを選択する
 * @param display
 * @param config
 * @param client_version
 * @param with_depth_buffer
 * @param with_stencil_buffer
 * @param isRecordable
 * @return 正常に選択できれば0, それ以外ならエラー
 */
/*public,global*/
EGLint getConfig(
	EGLDisplay &display, EGLConfig &config,
	const int &client_version,
	const bool &with_depth_buffer,
	const bool &with_stencil_buffer,
	const bool &isRecordable) {

	ENTER();

	EGLint opengl_bit;
	if (client_version >= 3) {
		opengl_bit = EGL_OPENGL_ES3_BIT;
	} else {
		opengl_bit = EGL_OPENGL_ES2_BIT;
	}
	// 有効にするEGLパラメータ:RGB565
	std::vector<EGLint> attribs_rgb565 = {
		// レンダリングタイプ(OpenGL|ES2 or ES3)
		EGL_RENDERABLE_TYPE, opengl_bit,
		// サーフェースタイプ, ダブルバッファを使用するのでEGL_WINDOW_BIT
		EGL_SURFACE_TYPE,	EGL_WINDOW_BIT,
		// 赤色で使用する最小フレームバッファサイズ, 8ビット
		EGL_RED_SIZE,		5,
		// 緑色で使用する最小フレームバッファサイズ, 8ビット
		EGL_GREEN_SIZE,		6,
		// 青色で使用する最小フレームバッファサイズ, 8ビット
		EGL_BLUE_SIZE,		5,
		// アルファで使用する最小フレームバッファサイズ, 8ビット
//		EGL_ALPHA_SIZE, 8,
		// ステンシルバッファとして使用する最小バッファサイズ, 8ビット
//		EGL_STENCIL_SIZE,	8,
		// 今のところ2D表示だけなのでデプスバッファは気にしない
		// デプスバッファとして使用する最小バッファサイズ, 16ビット
//		EGL_DEPTH_SIZE,		16,
//		EGL_RECORDABLE_ANDROID, 1,
	};

	// 有効にするEGLパラメータ:RGBA8888(RGBA)
	std::vector<EGLint> attribs_rgba8888 = {
		// レンダリングタイプ(OpenGL|ES2 or ES3)
		EGL_RENDERABLE_TYPE, opengl_bit,
		// サーフェースタイプ, ダブルバッファを使用するのでEGL_WINDOW_BIT
		EGL_SURFACE_TYPE,	EGL_WINDOW_BIT,
		// 赤色で使用する最小フレームバッファサイズ, 8ビット
		EGL_RED_SIZE,		8,
		// 緑色で使用する最小フレームバッファサイズ, 8ビット
		EGL_GREEN_SIZE,		8,
		// 青色で使用する最小フレームバッファサイズ, 8ビット
		EGL_BLUE_SIZE,		8,
		// アルファで使用する最小フレームバッファサイズ, 8ビット
        EGL_ALPHA_SIZE, 	8,
        // 今のところ2D表示だけなのでデプスバッファは気にしない
		// デプスバッファとして使用する最小バッファサイズ, 16ビット
//		EGL_DEPTH_SIZE,		16,
		// ステンシルバッファとして使用する最小バッファサイズ, 8ビット
//		EGL_STENCIL_SIZE,	8,
//		EGL_RECORDABLE_ANDROID, 1,
	};

	// 条件に合うEGLフレームバッファ設定のリストを取得
	EGLint numConfigs;
	EGLBoolean ret;
	EGLint err;

	if (with_depth_buffer) {
		attribs_rgba8888.push_back(EGL_DEPTH_SIZE);
		attribs_rgba8888.push_back(16);
	}
	if (with_stencil_buffer) {
		attribs_rgba8888.push_back(EGL_STENCIL_SIZE);
		attribs_rgba8888.push_back(8);
	}
	if (isRecordable) {
		attribs_rgba8888.push_back(EGL_RECORDABLE_ANDROID);
		attribs_rgba8888.push_back(1);
	}
	// 終端マーカ
	attribs_rgba8888.push_back(EGL_NONE);
	attribs_rgba8888.push_back(EGL_NONE);
	// EGLConfigを選択する
	// RGB8888を試みる
	ret = eglChooseConfig(display, &attribs_rgba8888[0], &config, 1, &numConfigs);
	if (UNLIKELY(!ret || !numConfigs)) {
		// RGB8888がだめなときはRGB565を試みる
		if (with_depth_buffer) {
			attribs_rgb565.push_back(EGL_DEPTH_SIZE);
			attribs_rgb565.push_back(16);
		}
		if (with_stencil_buffer) {
			attribs_rgb565.push_back(EGL_STENCIL_SIZE);
			attribs_rgb565.push_back(8);
		}
		if (isRecordable) {
			attribs_rgb565.push_back(EGL_RECORDABLE_ANDROID);
			attribs_rgb565.push_back(1);
		}
		// 終端マーカ
		attribs_rgb565.push_back(EGL_NONE);
		attribs_rgb565.push_back(EGL_NONE);
		ret = eglChooseConfig(display, &attribs_rgb565[0], &config, 1, &numConfigs);
		if (UNLIKELY(!ret || !numConfigs)) {
			// RGB565もだめだった時
			err = eglGetError();
			LOGE("failed to eglChooseConfig,err=%d", err);
			RETURN(-err, EGLint);
		}
	}

	RETURN(0, EGLint);

}

/**
 * @brief EGLコンテキストを生成する
 * 
 * @param display 
 * @param config 
 * @param client_version 
 * @param shared_context
 * @param with_depth_buffer デフォルトはfalse
 * @param with_stencil_buffer デフォルトはfalse
 * @param isRecordable デフォルトはfalse
 * @return EGLContext 
 */
/*public,global*/
EGLContext createEGLContext(
	EGLDisplay &display, EGLConfig &config,
	int &client_version,
	EGLContext shared_context,
	const bool &with_depth_buffer,
	const bool &with_stencil_buffer,
	const bool &isRecordable) {

	ENTER();

	EGLContext context = EGL_NO_CONTEXT;
	EGLint attrib_list[] = {
		EGL_CONTEXT_CLIENT_VERSION, client_version,		// OpenGL|ES2/ES3を指定
		EGL_NONE,
	};
	EGLint ret;
	EGLint err;

	// コンフィグレーションを選択
	if (client_version >= 3) {
		// OpenGL|ES3を試みる
		ret = getConfig(display, config, 3, with_depth_buffer, with_stencil_buffer, isRecordable);
		if (!ret) {
			client_version = 3;
			// EGLレンダリングコンテキストを取得(OpenGL|ES3)
			context = eglCreateContext(display, config, shared_context, attrib_list);
		}
	}
	if ((context == EGL_NO_CONTEXT) && (client_version >= 2)) {
		// OpenGL|ES2を試みる
		ret = getConfig(display, config, 2, with_depth_buffer, with_stencil_buffer, isRecordable);
		if (!ret) {
			client_version = 2;
			// EGLレンダリングコンテキストを取得(OpenGL|ES3)
			context = eglCreateContext(display, config, shared_context, attrib_list);
		}
	}

	if (context == EGL_NO_CONTEXT) {
		client_version = 0;
	}

	RET(context);
}

//--------------------------------------------------------------------------------
/**
 * コンストラクタ
 * @param client_version 2: OpenGL|ES2, 3:OpenGLES|3
 * @param display EGLディスプレー,
 *        nullptrなら内部でeglGetDisplay(EGL_DEFAULT_DISPLAY)を使って
 *        デフォルトのディスプレーを使う
 * @param shared_context 共有コンテキスト, Nullable
 * @param with_depth_buffer デプスバッファを使用するかどうか
 * @param with_stencil_buffer ステンシルバッファを使用するかどうか
 * @param isRecordable RECORDABLEフラグを漬けて初期化するかどうか
 */
EGLBase::EGLBase(int & client_version,
	EGLDisplay display,
	EGLContext shared_context,
	const bool &with_depth_buffer,
	const bool &with_stencil_buffer,
	const bool &isRecordable)
:	mEglDisplay(display),
	mEglContext(EGL_NO_CONTEXT),
	mEglSurface(EGL_NO_SURFACE),
 	mEglConfig(nullptr),
	mMajor(0), mMinor(0),
	client_version(0),
	mWithDepthBuffer(with_depth_buffer),
	mWithStencilBuffer(with_stencil_buffer),
	mIsRecordable(isRecordable),
	dynamicEglPresentationTimeANDROID(nullptr),
	dynamicEglDupNativeFenceFDANDROID(nullptr),
	dynamicEglCreateSyncKHR(nullptr),
	dynamicEglDestroySyncKHR(nullptr),
	dynamicEglSignalSyncKHR(nullptr),
	dynamicEglWaitSyncKHR(nullptr) {

	ENTER();

	initEGLContext(client_version,
		shared_context ? shared_context : EGL_NO_CONTEXT,
		with_depth_buffer, with_stencil_buffer, isRecordable);
	client_version = this->client_version;

#if __ANDROID__
	LOGD("try to get eglPresentationTimeANDROID");
	dynamicEglPresentationTimeANDROID
    	= (PFNEGLPRESENTATIONTIMEANDROIDPROC) eglGetProcAddress("eglPresentationTimeANDROID");
	if (!dynamicEglPresentationTimeANDROID) {
		LOGW("eglPresentationTimeANDROID is not available!");
	} else {
		LOGD("successfully could get eglPresentationTimeANDROID");
	}

	dynamicEglDupNativeFenceFDANDROID = (PFNEGLDUPNATIVEFENCEFDANDROIDPROC)
		eglGetProcAddress("eglDupNativeFenceFDANDROID");
	if (!dynamicEglDupNativeFenceFDANDROID) {
		LOGW("eglDupNativeFenceFDANDROID is not available!");
	} else {
		LOGD("successfully could get eglDupNativeFenceFDANDROID");
	}
#endif

	dynamicEglCreateSyncKHR = (PFNEGLCREATESYNCKHRPROC)
		eglGetProcAddress("eglCreateSyncKHR");
	if (!dynamicEglCreateSyncKHR) {
		LOGW("eglCreateSyncKHR is not available!");
	} else {
		LOGD("successfully could get eglCreateSyncKHR");
	}

	dynamicEglDestroySyncKHR = (PFNEGLDESTROYSYNCKHRPROC)
		eglGetProcAddress("eglDestroySyncKHR");
	if (!dynamicEglDestroySyncKHR) {
		LOGW("eglDestroySyncKHR is not available!");
	} else {
		LOGD("successfully could get eglDestroySyncKHR");
	}

	dynamicEglSignalSyncKHR = (PFNEGLSIGNALSYNCKHRPROC)
		eglGetProcAddress("eglSignalSyncKHR");
	if (!dynamicEglSignalSyncKHR) {
		LOGW("eglSignalSyncKHR is not available!");
	} else {
		LOGD("successfully could get eglSignalSyncKHR");
	}

	dynamicEglWaitSyncKHR = (PFNEGLWAITSYNCKHRPROC)
		eglGetProcAddress("eglWaitSyncKHR");
	if (!dynamicEglWaitSyncKHR) {
		LOGW("eglWaitSyncKHR is not available!");
	} else {
		LOGD("successfully could get eglWaitSyncKHR");
	}

	EXIT();
}

/**
 * コンストラクタ
 * @param client_version 2: OpenGL|ES2, 3:OpenGLES|3
 * @param shared_context 共有コンテキスト, Nullable
 * @param with_depth_buffer デプスバッファを使用するかどうか
 * @param with_stencil_buffer ステンシルバッファを使用するかどうか
 * @param isRecordable RECORDABLEフラグを付けて初期化するかどうか
 */
/*public*/
EGLBase::EGLBase(int &client_version,
	EGLBase *shared_context,
	const bool &with_depth_buffer,
	const bool &with_stencil_buffer,
	const bool &isRecordable)
:	EGLBase(client_version,
		shared_context ? shared_context->display() : EGL_NO_DISPLAY,
		shared_context ? shared_context->context() : EGL_NO_CONTEXT,
		shared_context && shared_context->mWithDepthBuffer,
		shared_context && shared_context->mWithStencilBuffer,
		shared_context && shared_context->mIsRecordable) {
}

/**
 * デストラクタ
 */
/*public*/
EGLBase::~EGLBase() {
	ENTER();

	releaseEGLContext();

	EXIT();
}

/**
 * EGLコンテキストを初期化する
 * @param client_version
 * @param shared_context
 * @param with_depth_buffer
 * @param with_stencil_buffer
 * @param isRecordable
 * @return 0:正常に初期家できた, それ以外:エラー
 */
/*private(friend)*/
int EGLBase::initEGLContext(const int &version,
	EGLContext shared_context,
	const bool &with_depth_buffer,
	const bool &with_stencil_buffer,
	const bool &isRecordable) {

	ENTER();

	if (!shared_context) shared_context = EGL_NO_CONTEXT;

	EGLint ret;
	EGLint err;

	if (!mEglDisplay || (mEglDisplay == EGL_NO_DISPLAY)) {
		LOGD("get default egl display");
		// EGLディスプレイコネクションを取得
		mEglDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);
		if (UNLIKELY(mEglDisplay == EGL_NO_DISPLAY)) {
			err = eglGetError();
			LOGW("failed to eglGetDisplay:err=%d", err);
			RETURN(-err, int);
		}
		// EGLディスプレイコネクションを初期化
		ret = eglInitialize(mEglDisplay, &mMajor, &mMinor);
		if (UNLIKELY(!ret)) {
			err = eglGetError();
			LOGW("failed to eglInitialize,err=%d", err);
			RETURN(-err, int);
		}
	}

	int client_version = version;
	mEglContext = createEGLContext(
		mEglDisplay, mEglConfig, client_version,
		shared_context,
		with_depth_buffer, with_stencil_buffer, isRecordable);

	if (mEglContext == EGL_NO_CONTEXT) {
		// EGLレンダリングコンテキストを取得できなかった
		err = eglGetError();
		LOGW("failed to getConfig,err=%d", err);
		RETURN(-err, int);
	}

	this->client_version = client_version;
	EGLint value;
	eglQueryContext(mEglDisplay, mEglContext, EGL_CONTEXT_CLIENT_VERSION, &value);
	MARK("EGLContext created, client version %d", value);
	eglQueryContext(mEglDisplay, mEglContext, EGL_CONTEXT_MAJOR_VERSION, &mMajor);
	eglQueryContext(mEglDisplay, mEglContext, EGL_CONTEXT_MINOR_VERSION, &mMinor);
	MARK("EGL ver.%d.%d", mMajor, mMinor);

	// 対応しているEGL拡張を解析
	std::istringstream eglext_stream(eglQueryString(mEglDisplay, EGL_EXTENSIONS));
	mEGLExtensions = std::set<std::string> {
		std::istream_iterator<std::string> {eglext_stream},
		std::istream_iterator<std::string>{}
	};

 #if !defined(LOG_NDEBUG)
	for (auto itr = mEGLExtensions.begin(); itr != mEGLExtensions.end(); itr++) {
		LOGD("extension=%s", (*itr).c_str());
	}
#endif

	// GLコンテキストを保持するためにサーフェースが必要な場合は1x1のオフスクリーンサーフェースを生成
	if (!hasEglExtension("EGL_KHR_surfaceless_context")) {
		EGLint const surface_attrib_list[] = {
			EGL_WIDTH, 1,
			EGL_HEIGHT, 1,
			EGL_NONE
		};
		mEglSurface = eglCreatePbufferSurface(mEglDisplay, mEglConfig, surface_attrib_list);
		EGLCHECK("eglCreatePbufferSurface");
	}
	makeDefault();

	// 対応しているGL拡張を解析
	std::istringstream glext_stream(reinterpret_cast<const char*>(glGetString(GL_EXTENSIONS)));
	mGLExtensions = std::set<std::string> {
		std::istream_iterator<std::string>{glext_stream},
		std::istream_iterator<std::string>{}
	};
	
    RETURN(0, int);
}

/**
 * EGLコンテキストを破棄する
 */
/*private(friend)*/
void EGLBase::releaseEGLContext() {
	ENTER();

	// GLコンテキスト保持用のマスターサーフェースを破棄
	if (mEglSurface != EGL_NO_SURFACE) {
		eglDestroySurface(mEglDisplay, mEglSurface);
		mEglSurface = EGL_NO_SURFACE;
	}

	// EGLディスプレイを破棄
	if (mEglDisplay != EGL_NO_DISPLAY) {
		eglMakeCurrent(mEglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
		EGLCHECK("eglMakeCurrent");
		// EGLレンダリングコンテキストを破棄
		MARK("eglDestroyContext");
		eglDestroyContext(mEglDisplay, mEglContext);
		MARK("eglTerminate");
		eglTerminate(mEglDisplay);
		// eglReleaseThreadを入れるとSC-06D(4.1.2)がハングアップする
		// ・・・Android4.2以降でサポートされたと書いてるWebが有った
		// EGL1.2以上でサポート
//		if ((mMajar > 1) || ((mMajar == 1) && (mMinor >= 2)))
//			eglReleaseThread();
	}
	mEglDisplay = EGL_NO_DISPLAY;
	mEglContext = EGL_NO_CONTEXT;
	mEglConfig = nullptr;
	mEGLExtensions.clear();
	mGLExtensions.clear();
	MARK("release:finished");

	EXIT();
}

#if __ANDROID__
/**
 * 与えられたSurfaceへOpenGL|ESで描画するためのEGLSurfaceを生成する
 * @param window
 * @param request_width
 * @param request_height
 * @return
 */
/*private(friend)*/
EGLSurface EGLBase::createWindowSurface(ANativeWindow *window,
	const int32_t &request_width, const int32_t &request_height) {

	ENTER();

	EGLint err;
	int32_t format;

	// 参照をキープ
	ANativeWindow_acquire(window);

	// EGLウインドウサーフェースを取得
	// XXX 一度でもANativeWindow_lockを呼び出してしまっているとeglCreateWindowSurfaceが失敗する
	EGLSurface surface = eglCreateWindowSurface(mEglDisplay, mEglConfig, window, nullptr);
	if (UNLIKELY(surface == EGL_NO_SURFACE)) {
		err = eglGetError();
		LOGW("failed to eglCreateWindowSurface");
		// 参照を解放
		ANativeWindow_release(window);
		RET(EGL_NO_SURFACE);
	}
	// EGLフレームバッファ設定情報を取得
	EGLBoolean ret = eglGetConfigAttrib(mEglDisplay, mEglConfig, EGL_NATIVE_VISUAL_ID, &format);
	if (UNLIKELY(!ret)) {
		err = eglGetError();
		LOGW("failed to eglGetConfigAttrib,err=%d", err);
		releaseSurface(surface);
		// 参照を解放
		ANativeWindow_release(window);
		RET(EGL_NO_SURFACE);
	}
	MARK("format=%d", format);
	// NativeWindowへバッファを設定
	// XXX サイズのチェックを入れるとNexus7(2012)でFullHDの際に画面が出ない。bad packetばかりになる
	ANativeWindow_setBuffersGeometry(window, request_width, request_height, format);
	// EGLレンダンリングコンテキストをEGLウインドウサーフェースにアタッチ
	if (eglMakeCurrent(mEglDisplay, surface, surface, mEglContext) == EGL_FALSE) {
		EGLCHECK("eglMakeCurrent");
		err = eglGetError();
		LOGW("Failed to eglMakeCurrent,err=%d", err);
		releaseSurface(surface);
		// 参照を解放
		ANativeWindow_release(window);
		RET(EGL_NO_SURFACE);
	}

	RET(surface);
}
#endif

/**
 * オフスクリーンへOpenGL|ESで描画するためのEGLSurfaceを生成する
 * @param request_width
 * @param request_height
 * @return
 */
/*private(friend)*/
EGLSurface EGLBase::createOffscreenSurface(const int32_t &request_width, const int32_t &request_height) {
	ENTER();

	EGLint surfaceAttribs[] = {
            EGL_WIDTH, request_width,
            EGL_HEIGHT, request_height,
            EGL_NONE
    };
    eglWaitGL();
	EGLSurface result = eglCreatePbufferSurface(mEglDisplay, mEglConfig, surfaceAttribs);
	EGLCHECK("eglCreatePbufferSurface");
	if (!result) {
		LOGE("surface was null");
		RET(EGL_NO_SURFACE);
	}
	makeCurrent(result);

	RET(result);
}

/**
 * 指定したEGLSurfaceへ描画できるように切り替える
 * @param surface
 * @return
 */
/*private(friend)*/
int EGLBase::makeCurrent(EGLSurface surface) {
	ENTER();

	// EGLレンダンリングコンテキストをEGLウインドウサーフェースにアタッチ
	if (LIKELY(mEglDisplay != EGL_NO_DISPLAY)) {
		EGLBoolean ret = eglMakeCurrent(mEglDisplay, surface, surface, mEglContext);
		EGLCHECK("eglMakeCurrent");
		if (UNLIKELY(ret == EGL_FALSE)) {
			LOGW("bind:Failed to eglMakeCurrent");
			RETURN(-1, int);
		}
		RETURN(0, int);
	} else {
		RETURN(-1, int);
	}
}

/**
 * 指定したEGLSurfaceへ描画できるように切り替える
 * 書き込みと読み込みを異なるEGLSurfaceで行う場合
 * @param draw_surface
 * @param read_surface
 * @return
 */
/*private(friend)*/
int EGLBase::makeCurrent(EGLSurface draw_surface, EGLSurface read_surface) {
	ENTER();

	// EGLレンダンリングコンテキストをEGLウインドウサーフェースにアタッチ
	if (LIKELY(mEglDisplay != EGL_NO_DISPLAY)) {
		EGLBoolean ret = eglMakeCurrent(mEglDisplay, draw_surface, read_surface, mEglContext);
		EGLCHECK("eglMakeCurrent");
		if (UNLIKELY(ret == EGL_FALSE)) {
			LOGW("bind:Failed to eglMakeCurrent");
			RETURN(-1, int);
		}
		RETURN(0, int);
	} else {
		RETURN(-1, int);
	}
}

/**
 * アタッチされているEGLSurfaceから切り離してデフォルトの描画先へ戻す
 * @return
 */
/*public*/
int EGLBase::makeDefault() {
	ENTER();

	// EGLレンダリングコンテキストとEGLサーフェースをデタッチ
	if (LIKELY(mEglDisplay != EGL_NO_DISPLAY)) {
		MARK("eglMakeCurrent");
		EGLBoolean ret = eglMakeCurrent(mEglDisplay, mEglSurface, mEglSurface, mEglContext);
		EGLCHECK("eglMakeCurrent");
		if (UNLIKELY(ret == EGL_FALSE)) {
			LOGW("makeDefault:Failed to eglMakeCurrent");
			RETURN(-1, int);
		}
	}

	RETURN(0, int);
}

/**
 * 文字列で指定したEGL拡張に対応しているかどうかを取得
 * @param s
 * @return
 */
bool EGLBase::hasEglExtension(const std::string &s) {
	return mEGLExtensions.find(s) != mEGLExtensions.end();
};

/**
 * 文字列で指定したGL拡張に対応しているかどうかを取得
 * @param s
 * @return
 */
bool EGLBase::hasGLExtension(const std::string &s) {
	return mGLExtensions.find(s) != mGLExtensions.end();
};

/**
 * ダブルバッファリングのバッファを入れ替える(Surfaceへの転送される)
 * @param surface
 * @return
 */
/*private(friend)*/
int EGLBase::swap(EGLSurface surface) {
//	ENTER();

	eglWaitGL();
	EGLCHECK("eglWaitGL");
	EGLint err = 0;
    EGLBoolean ret = eglSwapBuffers(mEglDisplay, surface);
    if (UNLIKELY(!ret)) {
        err = eglGetError();
        LOGW("eglSwapBuffers:err=%d", err);
    	return -err;	// RETURN(-err, int);
    }
	return -err;	// RETURN(-err, int);
}

/**
 * 可能であればptsをセットする
 * @param pts_ns
 * @return
 */
int EGLBase::setPresentationTime(EGLSurface surface,
	const khronos_stime_nanoseconds_t &pts_ns) {

//	ENTER();

	EGLint err = 0;
	if (pts_ns && dynamicEglPresentationTimeANDROID) {
		EGLBoolean ret = dynamicEglPresentationTimeANDROID(mEglDisplay, surface, pts_ns);
		if (UNLIKELY(!ret)) {
      		err = -eglGetError();
      		LOGW("dynamicEglPresentationTimeANDROID:err=%d", err);
		}
	}

	return err;	// RETURN(err, int);
}

/**
 * 指定したEGLSurfaceを破棄する
 * @param surface
 */
/*private(friend)*/
void EGLBase::releaseSurface(EGLSurface surface) {
	ENTER();

    if (surface != EGL_NO_SURFACE) {
    	eglMakeCurrent(mEglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
		EGLCHECK("eglMakeCurrent");
    	eglDestroySurface(mEglDisplay, surface);
    }

	EXIT();
}

/**
 * 指定したEGLSurfaceへ描画可能化かどうかを取得する
 * @param surface
 * @return
 */
/*private(friend)*/
bool EGLBase::isCurrent(EGLSurface surface) {
	return eglGetCurrentSurface(EGL_DRAW) == surface;
}

//================================================================================
#if __ANDROID__
/**
 * 指定したSurfaceへOpenGL|ESで描画するためのEGLSurfaceを生成するコンストラクタ
 * @param egl
 * @param window
 * @param width
 * @param height
 * @param low_reso
 */
/*public*/
GLSurface::GLSurface(EGLBase *egl, ANativeWindow *window,
	const int32_t &request_width, const int32_t &request_height)
:	mEgl(egl),
	mWindow(window),
 	mEglSurface(EGL_NO_SURFACE),
	window_width(0), window_height(0) {

	ENTER();

	if (window) {
		mEglSurface = egl->createWindowSurface(window, request_width, request_height);
		if (mEglSurface == EGL_NO_SURFACE) {
			// mWindowがNonNullなのにEGLSurfaceの生成に失敗した
			mWindow = nullptr;
		}
	}

	updateWindowSize();

	EXIT();
}
#endif

/**
 * オフスクリーンへOpenGL|ESで描画するためのEGLSurfaceを生成するコンストラクタ
 * @param egl
 * @param width
 * @param height
 */
/*public*/
GLSurface::GLSurface(EGLBase *egl, const int32_t &request_width, const int32_t &request_height)
:	mEgl(egl),
#if __ANDROID__
	mWindow(nullptr),
#endif
 	mEglSurface(egl->createOffscreenSurface(request_width, request_height)),
	window_width(request_width), window_height(request_height) {

	ENTER();

	updateWindowSize();

	EXIT();
}

/**
 * デストラクタ
 */
/*public*/
GLSurface::~GLSurface() {
	ENTER();

	if (mEglSurface != EGL_NO_SURFACE) {
		if (!bind()) {
			clear(0/*0xffff0000*/, true);
		}
		mEgl->makeDefault();
		mEgl->releaseSurface(mEglSurface);
		mEglSurface = EGL_NO_SURFACE;
	}
#if __ANDROID__
	if (mWindow) {
		ANativeWindow_release(mWindow);
		mWindow = nullptr;
	}
#endif
	EXIT();
}

/**
 * このオブジェクトが保持しているEGLSurfaceへアタッチして描画できるようにする
 * @return
 */
/*public*/
int GLSurface::bind() {
//	ENTER();

	int result = mEgl->makeCurrent(mEglSurface);
	if (LIKELY(!result)) {
#if UPDATE_WINDOW_SIZE_EVERYTIME
		updateWindowSize();
#else
		glViewport(0, 0, window_width, window_height);
		GLCHECK("glViewport");
#endif
	}

	return result; // RETURN(result, int);
}

/**
 * このオブジェクトが保持しているEGLSurfaceからデタッチする
 * @return
 */
/*public*/
int GLSurface::unbind() {
	return mEgl->makeDefault();
};

/**
 * ダブルバッファリングのバッファを入れ替える
 * @return
 */
/*public*/
int GLSurface::swap() {
	return mEgl->swap(mEglSurface);
}

/**
 * 可能であればptsをセットする
 * @param pts_ns
 * @return
 */
int GLSurface::setPresentationTime(const khronos_stime_nanoseconds_t &pts_ns) {
	return mEgl->setPresentationTime(mEglSurface, pts_ns);
}

/**
 * このオブジェクトが保持しているEGLSurfaceへ描画中かどうかを取得する
 * @return
 */
/*public*/
bool GLSurface::isCurrent() {
	return mEgl->isCurrent(mEglSurface);
}

/**
 * glClear呼び出しのためのヘルパー関数
 * @param color　color = ARGB
 * @param need_swap
 * @return
 */
/*public*/
int GLSurface::clear(const int &color, const bool &need_swap) {
	ENTER();

	const GLclampf a = (GLfloat)((color & 0xff000000) >> 24) / 255.0f;
	const GLclampf r = (GLfloat)((color & 0x00ff0000) >> 16) / 255.0f;
	const GLclampf g = (GLfloat)((color & 0x0000ff00) >> 8) / 255.0f;
	const GLclampf b = (GLfloat)(color & 0x000000ff) / 255.0f;

    glClearColor(r, g, b, a);
	GLCHECK("glClearColor");
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	GLCHECK("glClear");
    if (need_swap)
    	swap();

    RETURN(0, int);
}

/**
 * 描画サイズを更新する
 */
/*public*/
void GLSurface::updateWindowSize() {
	ENTER();

	EGLint err;
	EGLint width = window_width, height = window_height;
	// 画面サイズ・フォーマットの取得
	EGLBoolean ret = eglQuerySurface(mEgl->mEglDisplay, mEglSurface, EGL_WIDTH, &width);
	if (ret) {
		ret = eglQuerySurface(mEgl->mEglDisplay, mEglSurface, EGL_HEIGHT, &height);
		EGLCHECK("eglQuerySurface:height");
	}
	if (!ret) {
		err = eglGetError();
		LOGW("Failed to eglQuerySurface:err=%d", err);
		return;
	}
	window_width = width;
	window_height = height;
	MARK("window(%d,%d)", window_width, window_height);
	glViewport(0, 0, window_width, window_height);
	GLCHECK("glViewport");
	
	EXIT();
}

//================================================================================
/**
 * コンストラクタ
 * @param egl
 * @param fence_fd
 */
EglSync::EglSync(const EGLBase *egl, int fence_fd)
:	m_egl(egl), m_sync(EGL_NO_SYNC_KHR)
{
	ENTER();

	if (egl && egl->dynamicEglCreateSyncKHR && egl->dynamicEglDestroySyncKHR) {
		if (fence_fd != -1) {
			EGLint attribs[] = {
				EGL_SYNC_NATIVE_FENCE_FD_ANDROID, fence_fd,
				EGL_NONE
			};
			m_sync = egl->dynamicEglCreateSyncKHR(egl->display(),
				EGL_SYNC_NATIVE_FENCE_ANDROID, attribs);
			if (m_sync == EGL_NO_SYNC_KHR) {
				close(fence_fd);
			}
		} else {
			m_sync = egl->dynamicEglCreateSyncKHR(egl->display(), EGL_SYNC_TYPE_KHR, nullptr);
			if (m_sync == EGL_NO_SYNC_KHR) {
				const auto err = eglGetError();
				LOGW("Failed to eglCreateSyncKHR:err=0x%08x", err);
			}
		}
	} else {
		LOGE("EGLSyncKHR not supported!!");
	}
	LOGD("msync=%p,is_valid=%d, can_signal=%d", m_sync, is_valid(), can_signal());

	EXIT();
}

/**
 * デストラクタ
 */
EglSync::~EglSync() {
	ENTER();

	if (m_egl && m_sync && m_egl->dynamicEglDestroySyncKHR) {
		m_egl->dynamicEglDestroySyncKHR(m_egl->display(), m_sync);
		m_sync = EGL_NO_SYNC_KHR;
	}
	m_sync = EGL_NO_SYNC_KHR;
	m_egl = nullptr;

	EXIT();
}

/**
 * 同期オブジェクトのファイルディスクリプタを複製する
 * XXX 要確認 これってEGL/GLコンテキスト上でなくてもいいのかな？
 * @return
 */
int EglSync::dup() {
	ENTER();

	int result = -1;

	if (m_egl && m_sync && m_egl->dynamicEglDupNativeFenceFDANDROID) {
		result = m_egl->dynamicEglDupNativeFenceFDANDROID(m_egl->display(), m_sync);
	}

	RETURN(result, int);
}

/**
 * 同期オブジェクトがシグナル状態になるのを待機する
 * @return
 */
int EglSync::wait_sync() {
	ENTER();

	int result = -1;
	if (m_egl && m_sync && m_egl->dynamicEglWaitSyncKHR) {
		const auto r = m_egl->dynamicEglWaitSyncKHR(m_egl->display(), m_sync, 0);
		result = (r == EGL_TRUE) ? 0 : r;
	}
	
	
	RETURN(result, int);
}

/**
 * 同期オブジェクトをシグナル状態をセットする
 * @param reset デフォルトfalse, false: シグナル状態にする, true: シグナル状態を解除する
 * @return
 */
int EglSync::signal(const bool &reset) {
	ENTER();

	int result = -1;
	if (m_egl && m_sync && m_egl->dynamicEglSignalSyncKHR) {
		const auto r = m_egl->dynamicEglSignalSyncKHR(m_egl->display(), m_sync, reset ? EGL_UNSIGNALED_KHR : EGL_SIGNALED_KHR);
		if (r == EGL_TRUE) {
			// no error occurred
			result = 0;
		} else {
			result = -errno;
		}
	}
	
	RETURN(result, int);
}

//================================================================================
#if __ANDROID__
/**
 * コンストラクタ
 * @param tex_target
 * @param tex_unit
 * @param tex_id
 */
/*public*/
EglImageWrapper::EglImageWrapper(
	const GLenum &tex_target,
	const GLenum &tex_unit,
	const GLuint &tex_id)
:	m_supported(init_hardware_buffer()),
	m_tex_target(tex_target), m_tex_unit(tex_unit),
	own_tex_id(tex_id == 0), m_tex_id(tex_id),
	m_buffer(nullptr), m_egl_image(nullptr),
	m_desc(),
	dynamicEglGetNativeClientBufferANDROID(nullptr)
{
	ENTER();

	// テクスチャ変換行列を単位行列に初期化
	gl::setIdentityMatrix(m_tex_matrix, 0);
	dynamicEglGetNativeClientBufferANDROID
 		= (PFNEGLGETNATIVECLIENTBUFFERANDROIDPROC)
 			eglGetProcAddress("eglGetNativeClientBufferANDROID");
 	if (!dynamicEglGetNativeClientBufferANDROID) {
 		m_supported = false;
		LOGW("eglGetNativeClientBufferANDROID not found!");
 	}
	if (!m_tex_id) {
		m_tex_id = gl::createTexture(m_tex_target, m_tex_unit, 1);
	}

	EXIT();
}

/**
 * デストラクタ
 */
/*public*/
EglImageWrapper::~EglImageWrapper() {
	ENTER();

	unwrap();
	if (own_tex_id && m_tex_id) {
		glDeleteTextures(1, &m_tex_id);
		GLCHECK("glDeleteTextures");
	}
	m_tex_id = 0;

	EXIT();
}

/**
 * AHardwareBufferとメモリーを共有するEGLImageKHRを生成して
 * テクスチャとして利用できるようにする
 * @param buffer
 * @return 0: 正常終了, それ以外: エラー
 */
/*public*/
int EglImageWrapper::wrap(AHardwareBuffer *buffer) {

	ENTER();

	int result = -1;
	if (is_supported() && buffer) {
		// テクスチャ変換行列を単位行列に初期化
		gl::setIdentityMatrix(m_tex_matrix, 0);
		m_buffer = buffer;
		MARK("外部からのハードウエアバッファーをラップするとき");
		EGLDisplay display = eglGetCurrentDisplay();
		if (display != EGL_NO_DISPLAY) {
			AAHardwareBuffer_acquire(m_buffer);
			AAHardwareBuffer_describe(buffer, &m_desc);
			MARK("desc=(%dx%d),stride=%d",
				desc.width, desc.height, desc.stride);
			if (LIKELY(m_desc.width && m_desc.stride)) {
				m_tex_matrix[0] = (float)m_desc.width / (float)m_desc.stride;
			} else {
				m_tex_matrix[0] = 1.0f;
			}
			
			static const EGLint eglImageAttributes[] = {
				EGL_IMAGE_PRESERVED_KHR, EGL_TRUE,
				EGL_NONE
			};
			EGLClientBuffer clientBuf = dynamicEglGetNativeClientBufferANDROID(buffer);
			if (!clientBuf) {
				LOGW("failed to get EGLClientBuffer");
			}
			m_egl_image = eglCreateImageKHR(display,
				EGL_NO_CONTEXT, EGL_NATIVE_BUFFER_ANDROID, clientBuf, eglImageAttributes);
			if (m_egl_image) {
				bind();
				// 同じテクスチャに対してEGL Imageをバインドする → テクスチャとEGLImageのメモリーが共有される
				glEGLImageTargetTexture2DOES(m_tex_target, m_egl_image);
				result = 0;
			} else {
				LOGW("eglCreateImageKHR failed");
				AAHardwareBuffer_release(buffer);
				m_buffer = nullptr;
			}
		} else {	// if (display != EGL_NO_DISPLAY)
			LOGW("eglGetCurrentDisplay failed");
			m_buffer = nullptr;
		}	// if (display != EGL_NO_DISPLAY)
	}

	RETURN(result, int);
}

/**
 * wrapで生成したEGLImageKHRを解放、AHardwareBufferの参照も解放する
 * @return
 */
/*public*/
int EglImageWrapper::unwrap() {
	ENTER();

	if (m_egl_image) {
		EGLDisplay display = eglGetCurrentDisplay();
		eglDestroyImageKHR(display, m_egl_image);
		m_egl_image = nullptr;
	}
	if (m_buffer) {
		AAHardwareBuffer_release(m_buffer);
		m_buffer = nullptr;
	}
	
	RETURN(0, int);
}

/**
 * テクスチャをバインド
 * @return
 */
/*public*/
int EglImageWrapper::bind() {
	ENTER();

	int result = -1;

	if (m_egl_image) {
		glActiveTexture(m_tex_unit);	// テクスチャユニットを選択
		GLCHECK("glActiveTexture");
		glBindTexture(m_tex_target, m_tex_id);
		GLCHECK("glBindTexture");
	}

	RETURN(result, int);
}

/**
 * テクスチャのバインドを解除
 * @return
 */
/*public*/
int EglImageWrapper::unbind() {
	ENTER();

	if (m_egl_image) {
		glActiveTexture(m_tex_unit);	// テクスチャユニットを選択
		GLCHECK("glActiveTexture");
	    glBindTexture(m_tex_target, 0);
		GLCHECK("glBindTexture");
	}

	RETURN(0, int);
}
#endif

}	// namespace serenegiant::egl
