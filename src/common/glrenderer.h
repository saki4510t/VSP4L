/*
 * aAndUsb
 * Copyright (c) 2014-2023 saki t_saki@serenegiant.com
 * Distributed under the terms of the GNU Lesser General Public License (LGPL v3.0) License.
 * License details are in the file license.txt, distributed as part of this software.
 */

#ifndef GLRENDERER_H_
#define GLRENDERER_H_

#pragma interface

#include <memory>

#if __ANDROID__
#include "eglbase.h"
#endif
#include "gltexture.h"

namespace serenegiant::gl {

/**
 * 指定したイメージをview全面にOpenGL|ESで描画するクラス
 * すべての呼び出しはGLコンテキストを保持したスレッド上で行うこと
 */
class GLRenderer {
protected:
	GLuint mShaderProgram;
	GLuint mVertexShader;
	GLuint mFragmentShader;
	GLuint vbo[2]{0, 0};
    // uniform変数のロケーション
    GLint muTexture;			// テクスチャ(テクスチャユニット番号)のロケーション
	GLint muTexture2;			// テクスチャ(テクスチャユニット番号)のロケーション
	GLint muTexture3;			// テクスチャ(テクスチャユニット番号)のロケーション
	GLint muMVPMatrixLoc;		// モデルビュー行列のロケーション
	GLint muTexMatrixLoc;		// テクスチャ行列のロケーション
	GLint muTextureSzLoc;		// テクスチャサイズ変数のロケーション
	GLint muFrameSzLoc;			// フレームサイズ変数のロケーション
	GLint muBrightnessLoc;		// 明るさのオフセット変数の
    // attribute変数のロケーション
	GLint maPositionLoc;		// 頂点情報配列のロケーション
	GLint maTextureCoordLoc;	// テクスチャ座標配列のロケーション
	//
	float mBrightness;			// 明るさのオフセット値
	/**
	 * 初期化処理
	 * @param use_vbo 矩形描画時の頂点座標・テクスチャ座標にバッファオブジェクトを使うかどうか
	 */
	void init(const bool &use_vbo);
	/**
	 * 頂点座標・テクスチャ座標をセット
	 */
	void update_vertices();
public:
	/**
	 * コンストラクタ
	 * 頂点シェーダーとフラグメントシェーダーは文字列で引き渡す
	 * @param pVertexSource
	 * @param pFragmentSource
	 * @param use_vbo 矩形描画時の頂点座標・テクスチャ座標にバッファオブジェクトを使うかどうか, デフォルトはfalse
	 */
	GLRenderer(const char *pVertexSource, const char *pFragmentSource, const bool &use_vbo = false);
	/**
	 * デストラクタ
	 */
	~GLRenderer();
	/**
	 * 描画実行
	 * yuyvをrgbaに対応させる(2ピクセルの元データをテクスチャ1テクセルに代入する)時はview_widthを1/2にして呼び出すこと
	 * @param texture 描画するテクスチャ
	 * @param tex_matrix テクスチャ変換行列
	 * @param mvp_matrix モデルビュー変換行列
	 * @return
	 */
	int draw(GLTexture *texture, const GLfloat *tex_matrix = nullptr, const GLfloat *mvp_matrix = IDENTITY_MATRIX);
	/**
	 * 描画実効
	 * @param texture1
	 * @param texture2
	 * @param texture3
	 * @param mvp_matrix モデルビュー変換行列
	 * @return
	 */
	int draw(GLTexture *texture1, GLTexture *texture2, GLTexture *texture3 = nullptr, const GLfloat *mvp_matrix = IDENTITY_MATRIX);
#if __ANDROID__
	/**
	 * 描画実行
	 * yuyvをrgbaに対応させる(2ピクセルの元データをテクスチャ1テクセルに代入する)時はview_widthを1/2にして呼び出すこと
	 * @param texture 描画するテクスチャ
	 * @param tex_matrix テクスチャ変換行列
	 * @param mvp_matrix モデルビュー変換行列
	 * @return
	 */
	int draw(egl::EglImageWrapper *texture, const GLfloat *tex_matrix = nullptr, const GLfloat *mvp_matrix = IDENTITY_MATRIX);
#endif
};

typedef std::shared_ptr<GLRenderer> GLRendererSp;
typedef std::unique_ptr<GLRenderer> GLRendererUp;

}	// namespace serenegiant::gl

#endif /* GLRENDERER_H_ */
