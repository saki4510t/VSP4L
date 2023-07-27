/*
 * aAndUsb
 * Copyright (c) 2014-2023 saki t_saki@serenegiant.com
 * Distributed under the terms of the GNU Lesser General Public License (LGPL v3.0) License.
 * License details are in the file license.txt, distributed as part of this software.
 */

#ifndef EGLBASE_H_
#define EGLBASE_H_

#pragma interface

#include <set>
#include <string>
#include <memory>

#if __ANDROID__
#include <android/native_window.h>
#include "hardware_buffer_stub.h"
#endif

#include "glutils.h"
#include "times.h"

#undef EGLCHECK
#ifdef DEBUG_EGL_CHECK
	#define	EGLCHECK(OP) checkEglError(basename(__FILE__), __LINE__, __FUNCTION__, OP)
#else
	#define	EGLCHECK(OP)
#endif

namespace serenegiant::egl {

/**
 * eglGetErrorで取得したEGLのエラーをログに出力する
 * @param filename
 * @param line
 * @param func
 * @param op 実行した関数名
 */
void checkEglError(
	const char *filename, const int &line, const char *func,
	const char *op);

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
EGLint getConfig(
	EGLDisplay &display, EGLConfig &config,
	const int &client_version,
	const bool &with_depth_buffer,
	const bool &with_stencil_buffer,
	const bool &isRecordable);

/**
 * @brief EGLコンテキストを生成する
 * 
 * @param display 
 * @param config 
 * @param client_version 
 * @param shared_context デフォルトはEGL_NO_CONTEXT
 * @param with_depth_buffer デフォルトはfalse
 * @param with_stencil_buffer デフォルトはfalse
 * @param isRecordable デフォルトはfalse
 * @return EGLContext 
 */
EGLContext createEGLContext(
	EGLDisplay &display, EGLConfig &config,
	int &client_version,
	EGLContext shared_context = EGL_NO_CONTEXT,
	const bool &with_depth_buffer = false,
	const bool &with_stencil_buffer = false,
	const bool &isRecordable = false);

//--------------------------------------------------------------------------------
/**
 * EGLを使ってカレントスレッドにEGL/GLコンテキストを生成するクラス
 * EGL関係のヘルパー関数を含む
 */
class EGLBase {
friend class EglSync;
friend class GLSurface;
public:
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
	EGLBase(int &client_version,
		EGLDisplay display,
		EGLContext shared_context = nullptr,
		const bool &with_depth_buffer = false,
		const bool &with_stencil_buffer = false,
		const bool &isRecordable = false);

	/**
	 * コンストラクタ
	 * @param client_version 2: OpenGL|ES2, 3:OpenGLES|3
	 * @param shared_context 共有コンテキスト, Nullable
	 * @param with_depth_buffer デプスバッファを使用するかどうか
	 * @param with_stencil_buffer ステンシルバッファを使用するかどうか
	 * @param isRecordable RECORDABLEフラグを漬けて初期化するかどうか
	 */
	EGLBase(int &client_version,
		EGLBase *shared_context = nullptr,
		const bool &with_depth_buffer = false,
		const bool &with_stencil_buffer = false,
		const bool &isRecordable = false);

	/**
	 * デストラクタ
	 */
	~EGLBase();
	/**
	 * EGLレンダリングコンテキストを取得する
	 * @return
	 */
	inline EGLContext context() const { return mEglContext; }
	inline EGLDisplay display() const { return mEglDisplay; };
	/**
	 * GLES3を使用可能かどうか
	 * @return
	 */
	inline const bool isGLES3() const { return client_version >= 3; };
	/**
	 * OpenGL|ESのバージョンを取得
	 * @return
	 */
	inline const int glVersion() const { return client_version; };
	/**
	 * アタッチされているEGLSurfaceから切り離してデフォルトの描画先へ戻す
	 * @return
	 */
	int makeDefault();
	/**
	 * 文字列で指定したEGL拡張に対応しているかどうかを取得
	 * @param s
	 * @return
	 */
	bool hasEglExtension(const std::string &s);
	/**
	 * 文字列で指定したGL拡張に対応しているかどうかを取得
	 * @param s
	 * @return
	 */
	bool hasGLExtension(const std::string &s);
private:
	EGLDisplay mEglDisplay;
	EGLContext mEglContext;
	// GLコンテキスト保持にサーフェースが必要な場合の1x1のオフスクリーンサーフェース
	EGLSurface mEglSurface;
	EGLConfig mEglConfig;
	EGLint mMajor, mMinor;
	int client_version;
	bool mWithDepthBuffer;
	bool mWithStencilBuffer;
	bool mIsRecordable;

	// API16だとeglPresentationTimeANDROIDは動的にリンクしないとビルドが通らない
	PFNEGLPRESENTATIONTIMEANDROIDPROC dynamicEglPresentationTimeANDROID;
	PFNEGLDUPNATIVEFENCEFDANDROIDPROC dynamicEglDupNativeFenceFDANDROID;
	PFNEGLCREATESYNCKHRPROC dynamicEglCreateSyncKHR;
	PFNEGLDESTROYSYNCKHRPROC dynamicEglDestroySyncKHR;
	PFNEGLSIGNALSYNCKHRPROC dynamicEglSignalSyncKHR;
	PFNEGLWAITSYNCKHRPROC dynamicEglWaitSyncKHR;

	std::set<std::string> mEGLExtensions;
	std::set<std::string> mGLExtensions;

	/**
	 * EGLConfigを選択する
	 * @param client_version
	 * @param with_depth_buffer
	 * @param with_stencil_buffer
	 * @param isRecordable
	 * @return 正常に選択できれば0, それ以外ならエラー
	 */
	inline EGLint getConfig(const int &client_version,
		const bool &with_depth_buffer,
		const bool &with_stencil_buffer,
		const bool &isRecordable) {

		return egl::getConfig(mEglDisplay, mEglConfig,
			client_version, with_depth_buffer, with_stencil_buffer, isRecordable);
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
	int initEGLContext(const int &client_version,
		EGLContext shared_context,
		const bool &with_depth_buffer,
		const bool &with_stencil_buffer,
		const bool &isRecordable);
	/**
	 * EGLコンテキストを破棄する
	 */
	void releaseEGLContext();
#if __ANDROID__
	/**
	 * 与えられたSurfaceへOpenGL|ESで描画するためのEGLSurfaceを生成する
	 * @param window
	 * @param request_width
	 * @param request_height
	 * @return
	 */
	EGLSurface createWindowSurface(ANativeWindow *window,
		const int32_t &request_width, const int32_t &request_height);
#endif
	/**
	 * オフスクリーンへOpenGL|ESで描画するためのEGLSurfaceを生成する
	 * @param request_width
	 * @param request_height
	 * @return
	 */
	EGLSurface createOffscreenSurface(
	  	const int32_t &request_width, const int32_t &request_height);
	/**
	 * 指定したEGLSurfaceへ描画できるように切り替える
	 * @param surface
	 * @return
	 */
	int makeCurrent(EGLSurface surface);
	/**
	 * 指定したEGLSurfaceへ描画できるように切り替える
	 * 書き込みと読み込みを異なるEGLSurfaceで行う場合
	 * @param draw_surface
	 * @param read_surface
	 * @return
	 */
	int makeCurrent(EGLSurface draw_surface, EGLSurface read_surface);
	/**
	 * ダブルバッファリングのバッファを入れ替える(Surfaceへの転送される)
	 * @param surface
	 * @return
	 */
	int swap(EGLSurface surface);
	/**
	 * 可能であればptsをセットする
	 * @param pts_ns
	 * @return
	 */
	int setPresentationTime(EGLSurface surface,
		const khronos_stime_nanoseconds_t &pts_ns = 0);
	/**
	 * 指定したEGLSurfaceを破棄する
	 * @param surface
	 */
	void releaseSurface(EGLSurface surface);
	/**
	 * 指定したEGLSurfaceへ描画可能化かどうかを取得する
	 * @param surface
	 * @return
	 */
	bool isCurrent(EGLSurface surface);
	
	/**
	 * eglWaitGLのシノニム
	 * eglWaitGL: コマンドキュー内のコマンドをすべて転送する, glFinish()と同様の効果
	 */
	inline void waitGL() { eglWaitGL(); };
	/**
	 * eglWaitNativeのシノニム
	 * eglWaitNative: GPU側の描画処理が終了するまで実行をブロックする
	 */
	inline void waitNative() { eglWaitNative(EGL_CORE_NATIVE_ENGINE); };
};

//================================================================================

class GLSurface {
private:
	EGLBase *mEgl;
#if __ANDROID__
	ANativeWindow *mWindow;
#endif
	EGLSurface mEglSurface;
	int32_t window_width, window_height;
public:
#if __ANDROID__
	/**
	 * 指定したSurfaceへOpenGL|ESで描画するためのEGLSurfaceを生成するコンストラクタ
	 * @param egl
	 * @param window
	 * @param width
	 * @param height
	 * @param low_reso
	 */
	GLSurface(EGLBase *egl, ANativeWindow *window,
		const int32_t &width, const int32_t &height);
#endif
	/**
	 * オフスクリーンへOpenGL|ESで描画するためのEGLSurfaceを生成するコンストラクタ
	 * @param egl
	 * @param width
	 * @param height
	 */
	GLSurface(EGLBase *egl, const int32_t &width, const int32_t &height);
	/**
	 * デストラクタ
	 */
	~GLSurface();
	/**
	 * このオブジェクトが保持しているEGLSurfaceへアタッチして描画できるようにする
	 * @return
	 */
	int bind();
	/**
	 * このオブジェクトが保持しているEGLSurfaceへアタッチして描画できるようにする
	 * #bindの別名
	 * @return
	 */
	inline int make_current() { return bind(); };
	/**
	 * このオブジェクトが保持しているEGLSurfaceからデタッチする
	 * @return
	 */
	int unbind();
	/**
	 * このオブジェクトが保持しているEGLSurfaceからデタッチする
	 * #unbindの別名
	 * @return
	 */
	inline int make_default() { return unbind(); };
	/**
	 * ダブルバッファリングのバッファを入れ替える
	 * @return
	 */
	int swap();
	/**
	 * 可能であればptsをセットする
	 * @param pts_ns
	 * @return
	 */
	int setPresentationTime(const khronos_stime_nanoseconds_t &pts_ns);
	/**
	 * このオブジェクトが保持しているEGLSurfaceへ描画中かどうかを取得する
	 * @return
	 */
	bool isCurrent();
	/**
	 * glClear呼び出しのためのヘルパー関数
	 * @param color
	 * @param need_swap
	 * @return
	 */
	int clear(const int &color, const bool &need_swap = false);

	/**
	 * 描画サイズを更新する
	 */
	void updateWindowSize();
	inline int32_t width() const { return window_width; };
	inline int32_t height() const { return window_height; };
};

//================================================================================
/**
 * 同期オブジェクトのラッパー
 */
class EglSync {
private:
	const EGLBase *m_egl;
	EGLSyncKHR m_sync;
protected:
public:
	EglSync(const EGLBase *egl, int fence_fd = -1);
	~EglSync();

	/**
	 * 有効なEGLSyncKHRを保持しているかどうかを取得
	*/
	inline bool is_valid() const { return m_sync != EGL_NO_SYNC_KHR; };
	/**
	 * signal関数が有効かどうかを取得
	*/
	inline bool can_signal() const { return is_valid() && m_egl->dynamicEglSignalSyncKHR != nullptr; }
	inline EGLSyncKHR sync() const { return m_sync; };

	/**
	 * 同期オブジェクトのファイルディスクリプタを複製する
	 * @return
	 */
	int dup();
	/**
	 * 同期オブジェクトがシグナル状態になるのを待機する
	 * @return
	 */
	int wait_sync();
	/**
	 * 同期オブジェクトをシグナル状態をセットする
	 * @param reset デフォルトfalse, false: シグナル状態にする, true: シグナル状態を解除する
	 * @return
	 */
	int signal(const bool &reset = false);
};

//================================================================================
#if __ANDROID__
/**
 * AHardwareBufferとメモリーを共有するEGLImageKHRを生成してGL|ESのテクスチャとして
 * アクセスできるようにするためのヘルパークラス
 */
class EglImageWrapper {
private:
	/**
	 * 使用するテクスチャの種類GL_TEXTURE_2D/GL_TEXTURE_EXTERNAL_OES
	 */
	const GLuint m_tex_target;
	/**
	 * テクスチャユニット
	 */
	const GLenum m_tex_unit;
	/**
	 * テクスチャIDを自前で生成したかどうか
	 * コンストラクタへ引き渡したテクスチャIDが0の時には内部で
	 * テクスチャIDを生成するのでtrue
	 */
	const bool own_tex_id;
	/**
	 * EglImageWrapperが利用可能かどうか
	 * AHardwareBuffer_xxxとeglGetNativeClientBufferANDROIDが使用可能な場合にtrueになる
	 */
	bool m_supported;
	/**
	 * テクスチャID
	 */
	GLuint m_tex_id;
	/**
	 * ラップしているAHardwareBufferオブジェクト
	 */
	AHardwareBuffer *m_buffer;
	/**
	 * AHardwareBufferとメモリーを共有してテクスチャとしてアクセスできるようにするための
	 * EGLImageKHRオブジェクト
	 */
	EGLImageKHR m_egl_image;
	/**
	 * AHardwareBufferをラップしている間のみ有効
	 * #wrap実行時にAAHardwareBuffer_describeで更新する
	 */
	AHardwareBuffer_Desc m_desc;
	/**
	 * テクスチャ変換行列
	 */
	GLfloat m_tex_matrix[16]{};
	/**
	 * API16だとeglGetNativeClientBufferANDROIDは動的にリンクしないとビルドが通らないので動的にリンク
	 */
	PFNEGLGETNATIVECLIENTBUFFERANDROIDPROC dynamicEglGetNativeClientBufferANDROID;
protected:
public:
	/**
	 * コンストラクタ
	 * @param tex_target
	 * param tex_unit
	 * @param tex_id
	 */
	EglImageWrapper(
		const GLenum &tex_target,
		const GLenum &tex_unit,
		const GLuint &tex_id);
	/**
	 * デストラクタ
	 */
	~EglImageWrapper();

	/**
	 * 対応しているかどうか
	 * @return
	 */
	inline bool is_supported() const { return m_supported; };
	/**
	 * AHardwareBufferをラップしていて使用可能かどうか
	 * AHardwareBuffer_xxxとeglGetNativeClientBufferANDROIDが使用可能な場合にtrueを返す
	 * @return
	 */
	inline bool is_wrapped() const { return m_buffer && m_egl_image; };
	/**
	 * テクスチャターゲットを取得
	 * @return
	 */
	inline GLenum tex_target() const { return m_tex_target; };
	/**
	 * テクスチャユニットを取得
	 * @return
	 */
	inline GLenum tex_unit() const { return m_tex_unit; };
	/**
	 * テクスチャIDを取得
	 * @return
	 */
	inline GLuint tex_id() const { return m_tex_id; };
	/**
	 * #wrap実行時にAAHardwareBuffer_describeで取得した
	 * AHardwareBuffer_Desc構造体を返す
	 * @return
	 */
	inline const AHardwareBuffer_Desc &description() const { return m_desc; };
	/**
	 * テクスチャサイズ(幅)を取得
	 * AHardwareBufferをラップしている間のみ有効
	 * #wrap実行時にAAHardwareBuffer_describeで取得した
	 * AHardwareBuffer_Desc構造体のwidthフィールドを返す
	 * @return
	 */
	inline uint32_t width() const { return m_desc.width; };
	/**
	 * テクスチャサイズ(高さ)を取得
	 * AHardwareBufferをラップしている間のみ有効
	 * #wrap実行時にAAHardwareBuffer_describeで取得した
	 * AHardwareBuffer_Desc構造体のheightフィールドを返す
	 * @return
	 */
	inline uint32_t height() const { return m_desc.height; };
	/**
	 * ストライドを取得
	 * #wrap実行時にAAHardwareBuffer_describeで取得した
	 * AHardwareBuffer_Desc構造体のstrideフィールドを返す
	 * @return
	 */
	inline uint32_t stride() const { return m_desc.stride; };
	/**
	 * テクスチャ変換行列を取得
	 * widthとstrideに応じて幅方向のスケーリングをした変換行列を返す
	 * @return
	 */
	inline const GLfloat *tex_matrix() const { return m_tex_matrix; }
	
	/**
	 * AHardwareBufferとメモリーを共有するEGLImageKHRを生成して
	 * テクスチャとして利用できるようにする
	 * @param buffer
	 * @return 0: 正常終了, それ以外: エラー
	 */
	int wrap(AHardwareBuffer *buffer);
	/**
	 * wrapで生成したEGLImageKHRを解放、AHardwareBufferの参照も解放する
	 * @return
	 */
	int unwrap();

	/**
	 * テクスチャをバインド
	 * @return
	 */
	int bind();
	/**
	 * テクスチャのバインドを解除
	 * @return
	 */
	int unbind();
};
#endif

typedef std::shared_ptr<EGLBase> EGLBaseSp;
typedef std::unique_ptr<EGLBase> EGLBaseUp;
typedef std::shared_ptr<GLSurface> GLSurfaceSp;
typedef std::unique_ptr<GLSurface> GLSurfaceUp;
typedef std::shared_ptr<EglSync> EglSyncSp;
typedef std::unique_ptr<EglSync> EglSyncUp;

#if __ANDROID__
typedef std::shared_ptr<EglImageWrapper> EglImageWrapperSp;
typedef std::unique_ptr<EglImageWrapper> EglImageWrapperUp;
#endif

}	// namespace serenegiant::egl

#endif /* EGLBASE_H_ */
