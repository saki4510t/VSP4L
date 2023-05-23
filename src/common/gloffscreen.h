/*
 * aAndUsb
 * Copyright (c) 2014-2023 saki t_saki@serenegiant.com
 * Distributed under the terms of the GNU Lesser General Public License (LGPL v3.0) License.
 * License details are in the file license.txt, distributed as part of this software.
 */

#ifndef GLOFFSCREEN_H_
#define GLOFFSCREEN_H_

#pragma interface

#include <memory>

#include "glutils.h"
#include "gltexture.h"
#include "glrenderer.h"

namespace serenegiant::gl {

/**
 * フレームバッファオブジェクト(FBO)を使ってオフスクリーン描画するためのクラス
 * バックバッファとしてテスクチャを使用する
 */
class GLOffScreen {
private:
	const GLint mWidth;
	const GLint mHeight;
	const bool m_own_texture;
	GLuint mFrameBufferObj;
	GLuint mDepthBufferObj;
	/**
	 * バックバッファとして使用するテクスチャオブジェクト
	 * FIXME スマートポインターに変える
	 */
	GLTexture *mFBOTexture;
	/**
	 * モデルビュー変換行列
	 */
	float mMvpMatrix[16]{};

	/**
	 * コンストラクタ
	 * @param texture GLTextureインスタンス FIXME スマートポインターに変える
	 * @param use_depth_buffer
	 * @param own_texture GLOffScreen内で生成したGLTextureかどうか
	 */
	GLOffScreen(GLTexture *texture,
		const bool &use_depth_buffer,
		const bool &own_texture);
public:
	/**
	 * コンストラクタ
	 * @param texture 既存のGLTextureインスタンス　FIXME スマートポインターに変える
	 * 			このオブジェクトはデストラクタで破棄されないので注意
	 * @param use_depth_buffer
	 */
	GLOffScreen(GLTexture *texture,
		const bool &use_depth_buffer = false);
	/**
	 * コンストラクタ
	 * @param tex_unit テクスチャユニット GL_TEXTURE_0..GL_TEXTURE_31
	 * @param width
	 * @param height
	 * @param use_depth_buffer
	 */
	GLOffScreen(const GLenum &tex_unit,
		const GLint &width, const GLint &height,
		const bool &use_depth_buffer = false);
	/**
	 * デストラクタ
	 */
	~GLOffScreen();
	/**
	 * オフスクリーンへの描画に切り替える
	 * Viewportも変更になるので必要であればunbind後にViewportの設定をすること
	 * @return
	 */
	int bind();
	/**
	 * デフォルトのレンダリングバッファへ戻す
	 * @return
	 */
	int unbind();
	/**
	 * オフスクリーンテクスチャを指定したGLRendererで描画する(ヘルパーメソッド)
	 * @param renderer
	 * @return
	 */
	int draw(GLRenderer *renderer);
	/**
	 * バックバッファとして使用しているテクスチャオブジェクトを取得
	 * @return
	 */
	inline GLTexture *getOffscreen() { return mFBOTexture; }
	/**
	 * モデルビュー変換行列を取得
	 * @return
	 */
	inline const GLfloat *getMatrix() const { return mMvpMatrix; }
	
	inline GLint width() const { return mWidth; };
	inline GLint height() const { return mHeight; };
};

typedef std::shared_ptr<GLOffScreen> GLOffScreenSp;
typedef std::unique_ptr<GLOffScreen> GLOffScreenUp;

}	// namespace serenegiant::gl

#endif /* GLOFFSCREEN_H_ */
