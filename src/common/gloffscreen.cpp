/*
 * aAndUsb
 * Copyright (c) 2014-2023 saki t_saki@serenegiant.com
 * Distributed under the terms of the GNU Lesser General Public License (LGPL v3.0) License.
 * License details are in the file license.txt, distributed as part of this software.
 */

#define LOG_TAG "GLOffScreen"
#if 1	// デバッグ情報を出さない時は1
	#ifndef LOG_NDEBUG
		#define	LOG_NDEBUG		// LOGV/LOGD/MARKを出力しない時
		#endif
	#undef USE_LOGALL			// 指定したLOGxだけを出力
#else
	#define USE_LOGALL
	#undef LOG_NDEBUG
	#undef NDEBUG
	#define DEBUG_GL_CHECK			// GL関数のデバッグメッセージを表示する時
#endif

#include <cstring>	// memcpy

#include "utilbase.h"
#include "gloffscreen.h"

namespace serenegiant::gl {

#define USE_PBO false

/**
 * コンストラクタ
 * @param texture GLTextureインスタンス
 * @param use_depth_buffer
 */
GLOffScreen::GLOffScreen(GLTexture *texture,
	const bool &use_depth_buffer,
	const bool &own_texture)
:	mWidth(texture->getImageWidth()),
	mHeight(texture->getImageHeight()),
	m_own_texture(own_texture),
	mFBOTexture(texture),
	mFrameBufferObj(0),
	mDepthBufferObj(0)
{
	ENTER();

	MARK("size(%d,%d)", width, height);
	if (use_depth_buffer) {
		// デプスバッファが必要な場合は、レンダーバッファオブジェクトを生成・初期化する
		glGenRenderbuffers(1, &mDepthBufferObj);
		GLCHECK("glGenRenderbuffers");
		glBindRenderbuffer(GL_RENDERBUFFER, mDepthBufferObj);
		GLCHECK("glBindRenderbuffer");
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, mFBOTexture->getTexWidth(), mFBOTexture->getTexHeight());	// 16ビット
		GLCHECK("glRenderbufferStorage");
	}
	// フレームバッファーオブジェクトを生成・初期化
	glGenFramebuffers(1, &mFrameBufferObj);
	GLCHECK("glGenFramebuffers");
    glBindFramebuffer(GL_FRAMEBUFFER, mFrameBufferObj);	// READ & WRITE用にバインドする
    GLCHECK("GLOffScreen:glBindFramebuffer");

    // フレームバッファにテクスチャ(カラーバッファ)を接続する
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mFBOTexture->getTexture(), 0);
    GLCHECK("GLOffScreen:glFramebufferTexture2D");
	if (use_depth_buffer) {
		// フレームバッファにデプスバッファを接続する
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, mDepthBufferObj);
		GLCHECK("glFramebufferRenderbuffer");
	}
    // 正常に終了したかどうかを確認する
    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	GLCHECK("glCheckFramebufferStatus");
    if UNLIKELY(status != GL_FRAMEBUFFER_COMPLETE) {
        LOGE("Framebuffer not complete, status=%d", status);
    }
    // デフォルトのフレームバッファに戻す
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
	GLCHECK("glBindFramebuffer");
    // GLRendererは上限反転させて描画するので上下を入れ替える
	setIdentityMatrix(mMvpMatrix, 0);
	mMvpMatrix[5] *= -1.f;			// 上下反転

	EXIT();
}

/**
 * コンストラクタ
 * @param texture 既存のGLTextureインスタンス、このオブジェクトはデストラクタで破棄されないので注意
 * @param use_depth_buffer
 */
GLOffScreen::GLOffScreen(GLTexture *texture,
	const bool &use_depth_buffer)
:	GLOffScreen(texture, use_depth_buffer, false)
{
	ENTER();
	EXIT();
}

/**
 * コンストラクタ
 * @param tex_unit テクスチャユニット GL_TEXTURE_0..GL_TEXTURE_31
 * @param width
 * @param height
 * @param use_depth_buffer
 */
/*public*/
GLOffScreen::GLOffScreen(
	const GLenum &tex_unit,
	const GLint &width, const GLint &height,
	const bool &use_depth_buffer)
:	GLOffScreen(new GLTexture(GL_TEXTURE_2D, tex_unit, width, height, USE_PBO), use_depth_buffer, true)
{
	ENTER();
	EXIT();
}

/**
 * デストラクタ
 */
/*public*/
GLOffScreen::~GLOffScreen() {
	// フレームバッファオブジェクトを破棄
	if (mFrameBufferObj > 0) {
		glDeleteFramebuffers(1, &mFrameBufferObj);
		GLCHECK("glDeleteFramebuffers");
		mFrameBufferObj = 0;
	}
	// デプスバッファがある時はデプスバッファを破棄
	if (mDepthBufferObj > 0) {
		glDeleteRenderbuffers(1, &mDepthBufferObj);
		GLCHECK("glDeleteRenderbuffers");
	}
	// テクスチャを破棄
	if (m_own_texture) {
		SAFE_DELETE(mFBOTexture);
	}
}

/**
 * オフスクリーンへの描画に切り替える
 * Viewportも変更になるので必要であればunbind後にViewportの設定をすること
 * @return
 */
/*public*/
int GLOffScreen::bind() {
	ENTER();
    glBindFramebuffer(GL_FRAMEBUFFER, mFrameBufferObj);	// READ & WRITE用にバインドする
    GLCHECK("glBindFramebuffer");
    glViewport(0, 0, mWidth, mHeight);
	GLCHECK("glViewport");
	RETURN(0, int);
}

/**
 * デフォルトのレンダリングバッファへ戻す
 * @return
 */
/*public*/
int GLOffScreen::unbind() {
	ENTER();
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
	GLCHECK("glBindFramebuffer");
	RETURN(0, int);
}

/**
 * オフスクリーンテクスチャを指定したGLRendererで描画する(ヘルパーメソッド)
 * @param renderer
 * @return
 */
/*public*/
int GLOffScreen::draw(GLRenderer *renderer) {
	ENTER();
	if LIKELY(renderer) {
		renderer->draw(mFBOTexture, mFBOTexture->getTexMatrix(), mMvpMatrix);
		RETURN(0, int);
	}
	RETURN(-1, int);
}

/**
 * モデルビュー変換行列を設定
 * @param mvp_matrix
 * @return
 */
/*public*/
int GLOffScreen::set_mvp_matrix(const GLfloat *mat, const int &offset) {
	ENTER();
	
	if (mat) {
		memcpy(mMvpMatrix, &mat[offset], sizeof(GLfloat) * 16);
	} else {
		gl::setIdentityMatrix(mMvpMatrix, 0);
	}

	RETURN(0, int);
}

}	// namespace serenegiant::gl
