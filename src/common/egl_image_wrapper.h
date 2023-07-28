/*
 * aAndUsb
 * Copyright (c) 2014-2023 saki t_saki@serenegiant.com
 * Distributed under the terms of the GNU Lesser General Public License (LGPL v3.0) License.
 * License details are in the file license.txt, distributed as part of this software.
 */

#ifndef EGL_IMAGE_WRAPPER_H_
#define EGL_IMAGE_WRAPPER_H_

#if __ANDROID__
#include <android/native_window.h>
#include "hardware_buffer_stub.h"
#endif

#include "glutils.h"
#include "eglbase.h"

namespace serenegiant::egl {

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
     * テクスチャの幅
    */
    uint32_t m_tex_width;
    /**
     * テクスチャの高さ
    */
    uint32_t m_tex_height;
    /**
     * テクスチャのストライド
    */
    uint32_t m_tex_stride;
	/**
	 * テクスチャID
	 */
	GLuint m_tex_id;
	/**
	 * AHardwareBufferとメモリーを共有してテクスチャとしてアクセスできるようにするための
	 * EGLImageKHRオブジェクト
	 */
	EGLImageKHR m_egl_image;
	/**
	 * テクスチャ変換行列
	 */
	GLfloat m_tex_matrix[16]{};
#if __ANDROID__
	/**
	 * ラップしているAHardwareBufferオブジェクト
	 */
	AHardwareBuffer *m_buffer;
	/**
	 * AHardwareBufferをラップしている間のみ有効
	 * #wrap実行時にAAHardwareBuffer_describeで更新する
	 */
	AHardwareBuffer_Desc m_desc;
	/**
	 * API16だとeglGetNativeClientBufferANDROIDは動的にリンクしないとビルドが通らないので動的にリンク
	 */
	PFNEGLGETNATIVECLIENTBUFFERANDROIDPROC dynamicEglGetNativeClientBufferANDROID;
#endif
	PFNEGLCREATEIMAGEKHRPROC dynamicEglCreateImageKHR;
	PFNEGLDESTROYIMAGEKHRPROC dynamicEglDestroyImageKHR;
	PFNGLEGLIMAGETARGETTEXTURE2DOESPROC dynamicGlEGLImageTargetTexture2DOES;
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
	 * EGLImageをラップしていて使用可能かどうか
	 * @return
	 */
	inline bool is_wrapped() const { return m_egl_image; };
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
	 * テクスチャサイズ(幅)を取得
     * is_wrapped==trueの間のみ有効
	 * @return
	 */
	inline uint32_t width() const { return m_tex_width; };
	/**
	 * テクスチャサイズ(高さ)を取得
     * is_wrapped==trueの間のみ有効
	 * @return
	 */
	inline uint32_t height() const { return m_tex_height; };
	/**
	 * ストライドを取得
     * is_wrapped==trueの間のみ有効
	 * @return
	 */
	inline uint32_t stride() const { return m_tex_stride; };
	/**
	 * テクスチャ変換行列を取得
	 * widthとstrideに応じて幅方向のスケーリングをした変換行列を返す
	 * @return
	 */
	inline const GLfloat *tex_matrix() const { return m_tex_matrix; }
	
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

#if __ANDROID__
	/**
	 * #wrap実行時にAAHardwareBuffer_describeで取得した
	 * AHardwareBuffer_Desc構造体を返す
	 * @return
	 */
	inline const AHardwareBuffer_Desc &description() const { return m_desc; };
	/**
	 * AHardwareBufferとメモリーを共有するEGLImageKHRを生成して
	 * テクスチャとして利用できるようにする
	 * @param buffer
	 * @return 0: 正常終了, それ以外: エラー
	 */
	int wrap(AHardwareBuffer *buffer);
#else
    /**
     * EGLImageKHRが保持するメモリーをテクスチャとでして利用できるようにする
     * @param image
	 * @param width
	 * @param height
	 * @param stride
	 * @return 0: 正常終了, それ以外: エラー
    */
	int wrap(EGLImageKHR image, const uint32_t &width, const uint32_t &height, const uint32_t &stride);
#endif
	/**
	 * wrapで生成したEGLImageKHRを解放、AHardwareBufferの参照も解放する
	 * @return
	 */
	int unwrap();
};

typedef std::shared_ptr<EglImageWrapper> EglImageWrapperSp;
typedef std::unique_ptr<EglImageWrapper> EglImageWrapperUp;

}   // namespace serenegiant::egl

#endif  // #ifndef EGL_IMAGE_WRAPPER_H_
