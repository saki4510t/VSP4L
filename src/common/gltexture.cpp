/*
 * aAndUsb
 * Copyright (c) 2014-2023 saki t_saki@serenegiant.com
 * Distributed under the terms of the GNU Lesser General Public License (LGPL v3.0) License.
 * License details are in the file license.txt, distributed as part of this software.
 */

#define LOG_TAG "GLTexture"

#if 1	// デバッグ情報を出さない時は1
	#ifndef LOG_NDEBUG
		#define	LOG_NDEBUG		// LOGV/LOGD/MARKを出力しない時
	#endif
	#undef USE_LOGALL			// 指定したLOGxだけを出力
#else
//	#define USE_LOGALL
	#undef LOG_NDEBUG
	#undef NDEBUG
//	#define DEBUG_GL_CHECK		// GL関数のデバッグメッセージを表示する時
#endif

#include <cstring>

#include "utilbase.h"

#include "eglbase.h"
#include "gltexture.h"

namespace serenegiant::gl {

/**
 * ピクセルフォーマットから1ピクセルあたりのバイト数を取得する
 * ピクセルフォーマットとして使用できるのはGL_ALPHA, GL_RGB, GL_RGBA, GL_LUMINANCE, and GL_LUMINANCE_ALPHAのみ
 * @param pixel_format
 * @return
 */
static GLsizeiptr get_pixel_bytes(const GLenum &pixel_format) {
	switch (pixel_format) {
	case GL_ALPHA:
	case GL_LUMINANCE:
		return 1;
	case GL_RGB:
		return 3;
	case GL_RGBA:
		return 4;
	case GL_LUMINANCE_ALPHA:
		return 2;
	default:
		return 1;
	}
}

/**
 * データタイプから1ピクセルあたりのバイト数を取得する
 * データタイプとして使用できるのはGL_UNSIGNED_BYTE, GL_UNSIGNED_SHORT,
 * GL_UNSIGNED_SHORT_5_6_5, GL_UNSIGNED_SHORT_4_4_4_4, and GL_UNSIGNED_SHORT_5_5_5_1のみ
 * @param data_type
 * @return
 */
static GLsizeiptr get_data_bytes(const GLenum &data_type) {
	switch (data_type) {
	case GL_UNSIGNED_SHORT:
	case GL_UNSIGNED_SHORT_5_6_5:
	case GL_UNSIGNED_SHORT_4_4_4_4:
	case GL_UNSIGNED_SHORT_5_5_5_1:
		return 2;
	case GL_UNSIGNED_BYTE:
	default:
		return 1;
	}
}

/**
 * ピクセルフォーマットとデータタイプからテクスチャのメモリアライメントを取得する
 * @param pixel_format
 * @param data_type
 * @return 1, 2, 4, 8のいずれか
 */
static GLint get_alignment(const GLenum &pixel_format, const GLenum &data_type) {
	switch (data_type) {
	case GL_UNSIGNED_SHORT_5_6_5:
	case GL_UNSIGNED_SHORT_4_4_4_4:
	case GL_UNSIGNED_SHORT_5_5_5_1:
		// XXX ピクセルフォーマットがGL_RGBAならアライメント4バイトでいいのかな？
		return 2;
	case GL_UNSIGNED_BYTE:
	default:
		switch (pixel_format) {
		case GL_RGBA:
			return 4;
		case GL_LUMINANCE_ALPHA:
			return 2;
		default:
			return 1;
		}
	}
}

#if __ANDROID__
/**
 * OpenGL|ESのピクセルフォーマットとデータタイプからhardware bufferのフォーマット値を取得する
 * @param pixel_format
 * @param data_type
 * @return
 */
static uint32_t get_hw_buffer_format(const GLenum &pixel_format, const GLenum &data_type) {
	switch (data_type) {
	case GL_UNSIGNED_SHORT:
	case GL_UNSIGNED_SHORT_5_6_5:
	case GL_UNSIGNED_SHORT_4_4_4_4:
	case GL_UNSIGNED_SHORT_5_5_5_1:
		return AHARDWAREBUFFER_FORMAT_R5G6B5_UNORM;
	case GL_UNSIGNED_BYTE:
	default:
		switch (pixel_format) {
		case GL_DEPTH_COMPONENT:
			return AHARDWAREBUFFER_FORMAT_D16_UNORM;
		case GL_RGB:
			return AHARDWAREBUFFER_FORMAT_R8G8B8_UNORM;
		case GL_RGBA:
			return AHARDWAREBUFFER_FORMAT_R8G8B8A8_UNORM;
		case GL_ALPHA:
		case GL_LUMINANCE:
		case GL_LUMINANCE_ALPHA:
			return AHARDWAREBUFFER_FORMAT_Y8Cb8Cr8_420;
		default:
			return AHARDWAREBUFFER_FORMAT_R8G8B8A8_UNORM;
		}
	}
}
#endif

/**
 * 外部で生成済みのテクスチャidをラップする
 * internal_formatまたはformatのデフォルトはGL_RGBA,
 * internal_pixel_formatとpixel_formatに使えるのはGL_ALPHA, GL_RGB, GL_RGBA, GL_LUMINANCE, and GL_LUMINANCE_ALPHAのみ
 * GL_RGBA以外にすると1バイトアライメントになるので効率が悪くなるので注意！
 * データタイプとして使用できるのはGL_UNSIGNED_BYTE, GL_UNSIGNED_SHORT_5_6_5, GL_UNSIGNED_SHORT_4_4_4_4, and GL_UNSIGNED_SHORT_5_5_5_1のみ
 * @param tex_target GL_TEXTURE_2D
 * @param tex_unit テクスチャユニット GL_TEXTURE_0..GL_TEXTURE_31
 * @param tex_id 外部で生成済みのテクスチャid, 0なら内部で自動生成・破棄する
 * @param width
 * @param height
 * @param use_pbo assignTextureでPBOを使うかどうか
 * @param internal_pixel_format, テクスチャの内部フォーマット, デフォルトはGL_RGBA
 * @param pixel_format, assignTextureするときのテクスチャのフォーマット, デフォルトはGL_RGBA
 * @param data_type テクスチャのデータタイプ, デフォルトはGL_UNSIGNED_BYTE
 * @param use_powered2 true: テクスチャを2の乗数サイズにする
 * @param try_hw_buf 可能であればhardware bufferを使う,use_pbo=trueなら無効
 * @return
 */
/*static, public*/
GLTexture *GLTexture::wrap(
	const GLenum &tex_target,
	const GLenum &tex_unit,
	const GLuint &tex_id,
	const GLint &width, const GLint &height,
	const bool &use_pbo,
	const GLenum &internal_pixel_format, const GLenum &pixel_format,
	const GLenum &data_type, const bool &use_powered2,
  	const bool &try_hw_buf) {

	return new GLTexture(
		tex_target, tex_unit, tex_id,
		width, height,
		use_pbo,
		internal_pixel_format, pixel_format,
		data_type, use_powered2, try_hw_buf
#if __ANDROID__
		, nullptr
#endif
		);
}

#if __ANDROID__
/**
 * 既存のAHardwareBufferをラップしてテクスチャとしてアクセスできるようにする
 * @param graphicBuffer
 * @param tex_target
 * @param tex_unit
 * @param width
 * @param height
 * @return
 */
GLTexture *GLTexture::wrap(
	AHardwareBuffer *graphicBuffer,
	const GLenum &tex_target,
	const GLenum &tex_unit,
	const GLint &width, const GLint &height) {

	return new GLTexture(
		tex_target, tex_unit, 0,
		width, height,
		false,
		GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE,
		false, false,
		graphicBuffer);
}
#endif

/**
 * コンストラクタ
 * internal_formatまたはformatのデフォルトはGL_RGBA,
 * internal_pixel_formatとpixel_formatに使えるのはGL_ALPHA, GL_RGB, GL_RGBA, GL_LUMINANCE, and GL_LUMINANCE_ALPHAのみ
 * GL_RGBA以外にすると1バイトアライメントになるので効率が悪くなるので注意！
 * データタイプとして使用できるのはGL_UNSIGNED_BYTE, GL_UNSIGNED_SHORT_5_6_5, GL_UNSIGNED_SHORT_4_4_4_4, and GL_UNSIGNED_SHORT_5_5_5_1のみ
 * @param tex_target GL_TEXTURE_2D
 * @param tex_unit テクスチャユニット GL_TEXTURE_0..GL_TEXTURE_31
 * @param tex_id 外部で生成済みのテクスチャid, 0なら内部で自動生成・破棄する
 * @param width
 * @param height
 * @param use_pbo assignTextureでPBOを使うかどうか
 * @param internal_format, テクスチャの内部フォーマット, デフォルトはGL_RGBA
 * @param format, assignTextureするときのテクスチャのフォーマット, デフォルトはGL_RGBA
 * @param data_type テクスチャのデータタイプ, デフォルトはGL_UNSIGNED_BYTE
 * @param use_powered2 true: テクスチャを2の乗数サイズにする
 * @param try_hw_buf 可能であればhardware bufferを使う,use_pbo=trueなら無効
 */
/*protected*/
GLTexture::GLTexture(
	const GLenum &tex_target,
	const GLenum &tex_unit,
	const GLuint &tex_id,
	const GLint &width, const GLint &height,
	const bool &use_pbo,
	const GLenum &internal_pixel_format, const GLenum &pixel_format,
	const GLenum &data_type, const bool &_use_powered2,
	const bool &try_hw_buf
#if __ANDROID__
	, AHardwareBuffer *a_hardware_buffer
#endif
	)
:	TEX_TARGET(tex_target),
	TEX_UNIT(tex_unit),
	PIXEL_FORMAT_INTERNAL(internal_pixel_format),
	PIXEL_FORMAT(pixel_format),
	DATA_TYPE(data_type),
	mTexId(tex_id),
	is_own_tex(!tex_id),
	mTexWidth(width), mTexHeight(height),
	mImageWidth(width), mImageHeight(height),
	image_size(width * height * get_pixel_bytes(internal_pixel_format)),
#if __ANDROID__
	m_use_powered2(_use_powered2 && (a_hardware_buffer == nullptr)),
	own_hardware_buffer(a_hardware_buffer == nullptr),
	graphicBuffer(a_hardware_buffer),
#else
	m_use_powered2(_use_powered2),
#endif
	eglImage(nullptr),
	pbo_ix(0),
	pbo_sync(nullptr), pbo_need_write(false) {

	ENTER();

	MARK("size(%d,%d)", width, height);

	init(width, height, use_pbo, m_use_powered2, try_hw_buf);

	EXIT();
}

/**
 * コンストラクタ
 * internal_formatまたはformatのデフォルトはGL_RGBA,
 * internal_pixel_formatとpixel_formatに使えるのはGL_ALPHA, GL_RGB, GL_RGBA, GL_LUMINANCE, and GL_LUMINANCE_ALPHAのみ
 * GL_RGBA以外にすると1バイトアライメントになるので効率が悪くなるので注意！
 * データタイプとして使用できるのはGL_UNSIGNED_BYTE, GL_UNSIGNED_SHORT_5_6_5, GL_UNSIGNED_SHORT_4_4_4_4, and GL_UNSIGNED_SHORT_5_5_5_1のみ
 * @param tex_target GL_TEXTURE_2D
 * @param tex_unit テクスチャユニット GL_TEXTURE_0..GL_TEXTURE_31
 * @param width
 * @param height
 * @param use_pbo assignTextureでPBOを使うかどうか
 * @param internal_format, テクスチャの内部フォーマット, デフォルトはGL_RGBA
 * @param format, assignTextureするときのテクスチャのフォーマット, デフォルトはGL_RGBA
 * @param data_type テクスチャのデータタイプ, デフォルトはGL_UNSIGNED_BYTE
 * @param use_powered2 true: テクスチャを2の乗数サイズにする
 * @param try_hw_buf 可能であればhardware bufferを使う,use_pbo=trueなら無効
 */
/*public*/
GLTexture::GLTexture(
	const GLenum &tex_target,
	const GLenum &tex_unit,
	const GLint &width, const GLint &height,
	const bool &use_pbo,
	const GLenum &internal_pixel_format, const GLenum &pixel_format,
	const GLenum &data_type, const bool &use_powered2,
	const bool &try_hw_buf)
:	GLTexture(
		tex_target, tex_unit, 0,
		width, height,
		use_pbo,
		internal_pixel_format, pixel_format,
		data_type, use_powered2, try_hw_buf
#if __ANDROID__
		, nullptr
#endif
	)
{
	ENTER();
	EXIT();
}

/**
 * デストラクタ
 */
/*public*/
GLTexture::~GLTexture() {
	ENTER();

	if (eglImage) {
		auto display = eglGetCurrentDisplay();
		egl::EGL.eglDestroyImageKHR(display, eglImage);
	}
#if __ANDROID__
	if (graphicBuffer) {
		AAHardwareBuffer_release(graphicBuffer);
		graphicBuffer = nullptr;
	}
#endif
	eglImage = nullptr;
	if (mTexId && is_own_tex) {
		glDeleteTextures(1, &mTexId);
		GLCHECK("glDeleteTextures");
	}
	mTexId = 0;
	if (mPBO[0]) {
		// PBOを破棄する
		glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
		glDeleteBuffers(2, mPBO);
		GLCHECK("glDeleteBuffers");
		mPBO[0] = mPBO[1] = 0;
	}
	EXIT();
}

/**
 * 初期化処理
 * @param width テクスチャに割り当てる映像の幅
 * @param height テクスチャに割り当てる映像の高さ
 * @param use_pbo pboを使ってテクスチャの読み書きを行うかどうか
 * @param use_powered2 true: テクスチャを2の乗数サイズにする
 * @param try_hw_buf 可能であればhardware bufferを使う,use_pbo=trueなら無効
 */
/*protected*/
void GLTexture::init(
	const GLint &width, const GLint &height,
	const bool &use_pbo,
	const bool &use_powered2,
	const bool &try_hw_buf) {

	ENTER();

    if (!mTexId) {
    	// テクスチャ名が指定されていないとき
    	MARK("テクスチャを生成");
		glActiveTexture(TEX_UNIT);	// テクスチャユニットを選択
		GLCHECK("glActiveTexture");
#if __ANDROID__
		mTexId = createTexture(TEX_TARGET, TEX_UNIT,
		  own_hardware_buffer ? get_alignment(PIXEL_FORMAT_INTERNAL, DATA_TYPE) : 1);
#else
		mTexId = createTexture(TEX_TARGET, TEX_UNIT, 1);
#endif
		is_own_tex = true;
		if (use_powered2) {
			// 2の乗数にする
			int w = 32;
			for (; w < width; w <<= 1);
			int h = 32;
			for (; h < height; h <<= 1);
			if (mTexWidth != w || mTexHeight != h) {
				mTexWidth = w;
				mTexHeight = h;
				this->m_use_powered2 = true;
			}
		}
    }	// if (!mTexId)
	EGLImageKHR image = EGL_NO_IMAGE_KHR;
	if (is_own_tex) {
		// FIXME Android以外の時もEGL Imageを使ったテクスチャ転送が可能なはず
#if __ANDROID__
		if (!own_hardware_buffer && (graphicBuffer != nullptr)) {
			MARK("外部からのハードウエアバッファーをラップするとき");
			EGLDisplay display = eglGetCurrentDisplay();
			if (display != EGL_NO_DISPLAY) {
				AAHardwareBuffer_acquire(graphicBuffer);
				AHardwareBuffer_Desc usage;
				AAHardwareBuffer_describe(graphicBuffer, &usage);
				MARK("sz=(%dx%d),tex(%dx%d),usage=(%dx%d),stride=%d",
					mImageWidth, mImageHeight,
					mTexWidth, mTexHeight,
					usage.width, usage.height, usage.stride);
				mTexWidth = (int)usage.stride;
//				mTexHeight = (int)usage.height;	// YV12を流用しているときなどは高さが違うので代入しない
				static const EGLint eglImageAttributes[] = {
					EGL_IMAGE_PRESERVED_KHR, EGL_TRUE,
					EGL_NONE
				};
				EGLClientBuffer clientBuf = egl::EGL.eglGetNativeClientBufferANDROID(graphicBuffer);
				if (!clientBuf) {
					LOGW("failed to get EGLClientBuffer");
				}
				image = eglCreateImageKHR(display, EGL_NO_CONTEXT, EGL_NATIVE_BUFFER_ANDROID, clientBuf, eglImageAttributes);
				if (!image) {
					LOGW("eglCreateImageKHR failed");
					AAHardwareBuffer_release(graphicBuffer);
					graphicBuffer = nullptr;
				}
			} else {
				LOGW("eglGetCurrentDisplay failed");
				graphicBuffer = nullptr;
			}
		} else if (try_hw_buf && !use_pbo) {
			if (init_hardware_buffer()) {
				// 自前でハードウエアバッファーを初期化してテクスチャに割り当てるとき
				EGLDisplay display = eglGetCurrentDisplay();
				if (display != EGL_NO_DISPLAY) {
					MARK("create hardware buffer");
					// filling in the usage for HardwareBuffer
					AHardwareBuffer_Desc usage {
						.width = static_cast<uint32_t>(mTexWidth),
						.height = static_cast<uint32_t>(mTexHeight),
						.layers = 1,
						.format = get_hw_buffer_format(PIXEL_FORMAT_INTERNAL, DATA_TYPE),
						.usage = AHARDWAREBUFFER_USAGE_CPU_WRITE_OFTEN	// CPUから書き込み用に頻繁にロックする
							| AHARDWAREBUFFER_USAGE_CPU_READ_NEVER		// CPUから読み込み用にはロックしない
							| AHARDWAREBUFFER_USAGE_GPU_SAMPLED_IMAGE,	// GPUからテクスチャとして読み込む
						.stride = static_cast<uint32_t>(mTexWidth * get_pixel_bytes(PIXEL_FORMAT_INTERNAL)),
						.rfu0 = 0,
						.rfu1 = 0,
					};
					AAHardwareBuffer_allocate(&usage, &graphicBuffer);
					if (graphicBuffer) {
						static const EGLint eglImageAttributes[] = {
							EGL_IMAGE_PRESERVED_KHR, EGL_TRUE,
							EGL_NONE
						};
						AAHardwareBuffer_describe(graphicBuffer, &usage);
						MARK("sz=(%dx%d),tex(%dx%d),usage=(%dx%d)",
							mImageWidth, mImageHeight,
							mTexWidth, mTexHeight,
							usage.width, usage.height);
						EGLClientBuffer clientBuf = egl::EGL.eglGetNativeClientBufferANDROID(graphicBuffer);
						if (!clientBuf) {
							LOGW("failed to get EGLClientBuffer");
						}
						image = eglCreateImageKHR(display, EGL_NO_CONTEXT, EGL_NATIVE_BUFFER_ANDROID, clientBuf, eglImageAttributes);
						if (!eglImage) {
							LOGW("eglCreateImageKHR failed");
							AAHardwareBuffer_release(graphicBuffer);
							graphicBuffer = nullptr;
						}
					}
				} else {
					LOGW("eglGetCurrentDisplay() returned 'EGL_NO_DISPLAY', error = %x", eglGetError());
				}
			}
		}
#else	// #if __ANDROID__
		// FIXME 未実装
#endif	// #if __ANDROID__
		if (!eglImage) {
			MARK("allocate by glTexImage2D");
			// テクスチャのメモリ領域を確保する
			glTexImage2D(TEX_TARGET,
				0,					// ミップマップレベル
				PIXEL_FORMAT_INTERNAL,	// 内部フォーマット
				mTexWidth, mTexHeight,	// サイズ
				0,				// 境界幅
				PIXEL_FORMAT,			// 引き渡すデータのフォーマット
				DATA_TYPE,				// データの型
				nullptr);			// ピクセルデータ
			GLCHECK("glTexImage2D");
		}
	}	// if (is_own_tex)
	init(width, height, use_pbo, image);

    EXIT();
}

void GLTexture::init(const GLint &width, const GLint &height,
	const bool &use_pbo,
	EGLImageKHR image) {

	ENTER();

	bind();
	eglImage = image;
	if (image && (image != EGL_NO_IMAGE_KHR)) {
		// 同じテクスチャに対してEGL Imageをバインドする → テクスチャとEGLImageのメモリーが共有される
		egl::EGL.glEGLImageTargetTexture2DOES(TEX_TARGET, image);
		GLCHECK("glEGLImageTargetTexture2DOES");
	}
    // テクスチャ変換行列を単位行列に初期化
	setIdentityMatrix(mTexMatrix, 0);
	// テクスチャ変換行列を設定
	mTexMatrix[0] = width / (float)mTexWidth;
	mTexMatrix[5] = height / (float)mTexHeight;
	if (use_pbo) {
		// PBO作成とバインド
		glGenBuffers(2, mPBO);
		// バッファの作成と初期化
		glBindBuffer(GL_PIXEL_UNPACK_BUFFER, mPBO[0]);
		glBufferData(GL_PIXEL_UNPACK_BUFFER, image_size, nullptr, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_PIXEL_UNPACK_BUFFER, mPBO[1]);
		glBufferData(GL_PIXEL_UNPACK_BUFFER, image_size, nullptr, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
		LOGI("pbo=%d,%d", mPBO[0], mPBO[1]);
	}
	MARK("tex(%d,%d),request(%d,%d)", mTexWidth, mTexHeight, width, height);
//	printMatrix(mTexMatrix);

	EXIT();
}

/**
 * テキスチャへイメージを書き込む
 * @param src イメージデータ, コンストラクタで引き渡したフォーマットに合わせること
 * @return
 */
int GLTexture::assignTexture(const uint8_t *src) {
	ENTER();

	// テクスチャをバインド
	bind();
#if __ANDROID__
	if (!own_hardware_buffer /*&& (graphicBuffer != nullptr)*/ && (eglImage != nullptr)) {
		// 外部からのハードウエアバッファーをEGLImageKHRでラップしているときは何もしない
		MARK("do nothing for wrapped hardware buffer");
		RETURN(0, int);
	}
#else	// #if __ANDROID__
#endif	// #if __ANDROID__
    if (mPBO[0]) {
    	// PBOを使う時
		const int write_ix = pbo_ix;
		// index of PBO to request read(will actually read into memory on next frame
		const int next_ix = pbo_ix = (pbo_ix + 1) % 2;
		if (pbo_need_write) {
			glDeleteSync(pbo_sync);
			// PBOをバインド
			glBindBuffer(GL_PIXEL_UNPACK_BUFFER, mPBO[write_ix]);
			// Pboからテクスチャへコピー
			glTexSubImage2D(TEX_TARGET,
				0,					// ミップマップレベル
				0, 0,			// オフセットx,y
				mImageWidth, mImageHeight,	// 上書きするサイズ
				PIXEL_FORMAT,				// 引き渡すデータのフォーマット
				DATA_TYPE,					// データの型
				nullptr);			// ピクセルデータとしてPBOを使う
			GLCHECK("glTexSubImage2D");
		}
		// 次のデータを書き込む
		// PBOへの書き込み完了をチェックするための同期オブジェクトを挿入
		pbo_sync = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
		pbo_need_write = true;
		glBindBuffer(GL_PIXEL_UNPACK_BUFFER, mPBO[next_ix]);
		glBufferData(GL_PIXEL_UNPACK_BUFFER, image_size, nullptr, GL_DYNAMIC_DRAW);
		auto dst = (uint8_t *)glMapBufferRange(GL_PIXEL_UNPACK_BUFFER, 0, image_size, GL_MAP_WRITE_BIT);
		if (LIKELY(dst)) {
			// write image data into PBO
			memcpy(dst, src, image_size);
			glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);
		}
		glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
#if __ANDROID__
	} else if (graphicBuffer && eglImage) {
		uint8_t *dst;
		AAHardwareBuffer_lock(graphicBuffer, AHARDWAREBUFFER_USAGE_CPU_WRITE_OFTEN, -1, nullptr, (void**) &dst);
		{
			if (m_use_powered2) {
				// イメージサイズとテクスチャサイズが異なるとき
				size_t w = mImageWidth * get_pixel_bytes(PIXEL_FORMAT_INTERNAL);
				size_t stride = mTexWidth * get_pixel_bytes(PIXEL_FORMAT_INTERNAL);
				for (int row = 0; row < mImageHeight; row++) {
					memcpy(dst, src, w);
					dst += stride;
					src += w;
				}
			} else {
				// イメージサイズとテクスチャサイズが同じ時は単純コピー
				memcpy(dst, src, image_size);
			}
		}
		AAHardwareBuffer_unlock(graphicBuffer, nullptr);
#else	// #if __ANDROID__
	// } else if (eglImage) {

#endif	// #if __ANDROID__
	} else {
		// PBOを使わない時
		glTexSubImage2D(TEX_TARGET,
			0,					// ミップマップレベル
			0, 0,			// オフセットx,y
			mImageWidth, mImageHeight,	// 上書きするサイズ
			PIXEL_FORMAT,				// 引き渡すデータのフォーマット
			DATA_TYPE,					// データの型
			src);						// ピクセルデータ
		GLCHECK("glTexSubImage2D");
    }

    RETURN(0, int);
}

/**
 * PBOへの書き込み処理が完了していればテクスチャへ反映させる
 * @param timeout 最大待ち時間［ナノ秒］
 */
void GLTexture::refresh(const GLuint64 &timeout) {
	if (mPBO[0] && pbo_need_write && pbo_sync) {
		const GLenum r = glClientWaitSync(pbo_sync, 0, timeout);
		if ((r == GL_ALREADY_SIGNALED)
			|| (r == GL_CONDITION_SATISFIED)) {

			// PBOをバインド
			glBindBuffer(GL_PIXEL_UNPACK_BUFFER, mPBO[pbo_ix]);
			// Pboからテクスチャへコピー
			glTexSubImage2D(TEX_TARGET,
				0,					// ミップマップレベル
				0, 0,			// オフセットx,y
				mImageWidth, mImageHeight,	// 上書きするサイズ
				PIXEL_FORMAT,				// 引き渡すデータのフォーマット
				DATA_TYPE,					// データの型
				nullptr);			// ピクセルデータとしてPBOを使う
			pbo_need_write = false;
			glDeleteSync(pbo_sync);
			pbo_sync = nullptr;
		}
	}
}

/**
 * テクスチャをバインド
 * @return
 */
/*public*/
int GLTexture::bind() {
	ENTER();

	glActiveTexture(TEX_UNIT);	// テクスチャユニットを選択
	GLCHECK("glActiveTexture");
	glBindTexture(TEX_TARGET, mTexId);
	GLCHECK("glBindTexture");

	RETURN(0, int);
}

/**
 * テクスチャのバインドを解除
 * @return
 */
/*public*/
int GLTexture::unbind() {
	ENTER();

	glActiveTexture(TEX_UNIT);	// テクスチャユニットを選択
	GLCHECK("glActiveTexture");
    glBindTexture(TEX_TARGET, 0);
	GLCHECK("glBindTexture");

	RETURN(0, int);
}

/**
 * テクスチャの補間方法をセット
 * min_filter/max_filterはそれぞれGL_NEARESTかGL_LINEAR
 * GL_LINEARにすると補間なし
 * @param min_filter
 * @param max_filter
 * @return
 */
/*public*/
int GLTexture::setFilter(const GLint &min_filter, const GLint &max_filter) {
	ENTER();

	bind();
	// テクスチャの拡大・縮小方法を指定GL_NEARESTにすると補間無し
	glTexParameteri(TEX_TARGET, GL_TEXTURE_MIN_FILTER, min_filter);
	GLCHECK("glTexParameteri");
	glTexParameteri(TEX_TARGET, GL_TEXTURE_MAG_FILTER, max_filter);
	GLCHECK("glTexParameteri");

	RETURN(0, int);
}

}	// namespace serenegiant::gl
