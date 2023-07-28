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

#include "egl_image_wrapper.h"

namespace serenegiant::egl {

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
:   m_tex_target(tex_target), m_tex_unit(tex_unit),
	own_tex_id(tex_id == 0), m_tex_id(tex_id),
	m_egl_image(nullptr),
#if __ANDROID__
    m_buffer(nullptr),  m_desc(),
	dynamicEglGetNativeClientBufferANDROID(nullptr),
    m_supported(init_hardware_buffer())
#else
    m_supported(true)
#endif
{
	ENTER();

#if __ANDROID__
	dynamicEglGetNativeClientBufferANDROID
 		= (PFNEGLGETNATIVECLIENTBUFFERANDROIDPROC)
 			eglGetProcAddress("eglGetNativeClientBufferANDROID");
 	if (!dynamicEglGetNativeClientBufferANDROID) {
 		m_supported = false;
		LOGW("eglGetNativeClientBufferANDROID not found!");
 	}
    m_supported &= (dynamicEglGetNativeClientBufferANDROID != nullptr);
#endif
	dynamicEglCreateImageKHR
 		= (PFNEGLCREATEIMAGEKHRPROC) eglGetProcAddress("eglCreateImageKHR");
 	if (!dynamicEglCreateImageKHR) {
		LOGW("eglCreateImageKHR not found!");
 	}
	dynamicEglDestroyImageKHR
 		= (PFNEGLDESTROYIMAGEKHRPROC) eglGetProcAddress("eglDestroyImageKHR");
 	if (!dynamicEglDestroyImageKHR) {
		LOGW("eglDestroyImageKHR not found!");
	}
	dynamicGlEGLImageTargetTexture2DOES
		= (PFNGLEGLIMAGETARGETTEXTURE2DOESPROC) eglGetProcAddress("glEGLImageTargetTexture2DOES");
 	if (!dynamicGlEGLImageTargetTexture2DOES) {
		LOGW("glEGLImageTargetTexture2DOES not found!");
	}
    m_supported &= (dynamicEglCreateImageKHR != nullptr)
        && (dynamicEglDestroyImageKHR != nullptr)
        && (dynamicGlEGLImageTargetTexture2DOES != nullptr);

	// テクスチャ変換行列を単位行列に初期化
	gl::setIdentityMatrix(m_tex_matrix, 0);
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

#if __ANDROID__
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
            m_tex_width = m_desc.width;
            m_tex_height = m_desc.height;
			if (LIKELY(m_desc.width && m_desc.stride)) {
				m_tex_matrix[0] = (float)m_desc.width / (float)m_desc.stride;
                m_tex_stride = m_desc.stride;
			} else {
				m_tex_matrix[0] = 1.0f;
                m_tex_stride = m_tex_width;
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
				dynamicGlEGLImageTargetTexture2DOES(m_tex_target, m_egl_image);
				result = 0;
			} else {
				LOGW("eglCreateImageKHR failed");
				AAHardwareBuffer_release(buffer);
				m_buffer = nullptr;
                m_tex_width = m_tex_height = m_tex_stride = 0;
			}
		} else {	// if (display != EGL_NO_DISPLAY)
			LOGW("eglGetCurrentDisplay failed");
			m_buffer = nullptr;
            m_tex_width = m_tex_height = m_tex_stride = 0;
		}	// if (display != EGL_NO_DISPLAY)
	}

	RETURN(result, int);
}
#else
    /**
     * EGLImageKHRが保持するメモリーをテクスチャとでして利用できるようにする
	 * FIXME width, height, strideを指定しないとだめ
     * @param image
	 * @param width
	 * @param height
	 * @param stride
	 * @return 0: 正常終了, それ以外: エラー
    */
	int EglImageWrapper::wrap(EGLImageKHR image, const uint32_t &width, const uint32_t &height, const uint32_t &stride) {
        ENTER();

    	int result = -1;
        m_tex_width = width;
		m_tex_height = height;
		if (!stride) {
			m_tex_stride = width;
		} else {
			m_tex_stride = stride;
		}
        m_egl_image = image;
		if (LIKELY(image && m_tex_width && m_tex_stride)) {
			m_tex_matrix[0] = (float)m_tex_width / (float)m_tex_stride;
			bind();
			// 同じテクスチャに対してEGL Imageをバインドする → テクスチャとEGLImageのメモリーが共有される
			dynamicGlEGLImageTargetTexture2DOES(m_tex_target, m_egl_image);
			result = 0;
		} else {
			m_tex_matrix[0] = 1.0f;
		}

	    RETURN(result, int);
    }
#endif

/**
 * wrapで生成したEGLImageKHRを解放、AHardwareBufferの参照も解放する
 * @return
 */
/*public*/
int EglImageWrapper::unwrap() {
	ENTER();

	if (m_egl_image) {
		EGLDisplay display = eglGetCurrentDisplay();
		EGLCHECK("eglGetCurrentDisplay");
		dynamicEglDestroyImageKHR(display, m_egl_image);
		EGLCHECK("eglDestroyImageKHR");
		m_egl_image = nullptr;
	}
#if __ANDROID__
	if (m_buffer) {
		AAHardwareBuffer_release(m_buffer);
		m_buffer = nullptr;
	}
#endif

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

}   // namespace serenegiant::egl
