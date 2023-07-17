/*
 * aAndUsb
 * Copyright (c) 2014-2023 saki t_saki@serenegiant.com
 * Distributed under the terms of the GNU Lesser General Public License (LGPL v3.0) License.
 * License details are in the file license.txt, distributed as part of this software.
 */

#define LOG_TAG "GLRenderer"
#if 1	// デバッグ情報を出さない時は1
	#ifndef LOG_NDEBUG
		#define	LOG_NDEBUG		// LOGV/LOGD/MARKを出力しない時
	#endif
	#undef USE_LOGALL			// 指定したLOGxだけを出力
#else
//	#define USE_LOGALL
	#undef LOG_NDEBUG
	#undef NDEBUG
	#define DEBUG_GL_CHECK			// GL関数のデバッグメッセージを表示する時
#endif

#include <cstring>

#include "utilbase.h"
#include "glutils.h"
#include "glrenderer.h"

namespace serenegiant::gl {

/**
 * JavaのlibcommonのGLDrawer2Dと同じ頂点座標・テクスチャ座標を使うかどうか
 * 1: GLDrawer2Dと同じ, 0: テクスチャ座標を上下反転
 * XXX libcommonのGLDrawer2Dは8.0.0以降は上下反転させない様に変更
 */
#define GL_DRAWER_2D 1

/**
 * テクスチャ座標・頂点座標1点あたりの要素の数(==2)
 */
#define COORD_NUM 2
/**
 * GLfloat(float)１つのバイト数
 */
static const GLsizeiptr SIZEOF_FLOAT_BYTES = sizeof(GLfloat);

/**
 * 矩形描画用頂点座標
 */
static const GLfloat FULL_RECTANGLE_COORDS[] = {
#if GL_DRAWER_2D
	// GLDrawer2Dと同じ
	1.0f, 1.0f,		// 右上
	-1.0f, 1.0f,	// 左上
	1.0f, -1.0f,	// 右下
	-1.0f, -1.0f	// 左下
#else
	-1.0f,  1.0f,	// 2 top left 左上
	1.0f,  1.0f,	// 3 top right 右上
	-1.0f, -1.0f,	// 0 bottom left 左下
	1.0f, -1.0f,	// 1 bottom right 右下
#endif
};

/**
 * 矩形描画用テクスチャ座標
 */
static const GLfloat FULL_RECTANGLE_TEX_COORDS[] = {
#if GL_DRAWER_2D
	// GLDrawer2Dと同じ
#if 0
  	// こっちは旧GLDrawer2Dと同じ
	1.0f, 1.0f,		// 右上
	0.0f, 1.0f,		// 左上
	1.0f, 0.0f,		// 右下
	0.0f, 0.0f		// 左下
#else
	// 新しいGLDrawer2Dと同じ
	1.0f, 0.0f,		// 右上
	0.0f, 0.0f,		// 左上
	1.0f, 1.0f,		// 右下
	0.0f, 1.0f,		// 左下
#endif
#else
	// 上下反転
	0.0f, 0.0f,		// 0 bottom left 左下
	1.0f, 0.0f,		// 1 bottom right 右下
	0.0f, 1.0f,		// 2 top left 左上
	1.0f, 1.0f		// 3 top right 右上
#endif
};

/**
 * 頂点座標バッファ・テクスチャ座標バッファのサイズ
 */
const static int VERTEX_BUF_SZ = NUM_ARRAY_ELEMENTS(FULL_RECTANGLE_COORDS) * SIZEOF_FLOAT_BYTES;

/**
 * 頂点座標の数
 * 2個で1頂点なので1/2
 */
const static int  VERTEX_NUM = NUM_ARRAY_ELEMENTS(FULL_RECTANGLE_COORDS) / COORD_NUM;

/**
 * コンストラクタ
 * 頂点シェーダーとフラグメントシェーダーは文字列で引き渡す
 * @param pVertexSource
 * @param pFragmentSource
 * @param use_vbo 矩形描画時の頂点座標・テクスチャ座標にバッファオブジェクトを使うかどうか, デフォルトはfalse
 */
/*public*/
GLRenderer::GLRenderer(const char *pVertexSource, const char *pFragmentSource, const bool &use_vbo)
:	mVertexShader(0), mFragmentShader(0),
	muTexture(-1), muTexture2(-1), muTexture3(-1),
	muMVPMatrixLoc(-1),
	muTexMatrixLoc(-1),
	muTextureSzLoc(-1),
	muFrameSzLoc(-1),
	muBrightnessLoc(-1),
	maPositionLoc(-1),
	maTextureCoordLoc(-1),
	mShaderProgram(createShaderProgram(
		pVertexSource, pFragmentSource, &mVertexShader, &mFragmentShader)),
	mBrightness(0.0f) {

	ENTER();

	init(use_vbo);

	EXIT();
}

/**
 * デストラクタ
 */
/*public*/
GLRenderer::~GLRenderer() {
	ENTER();

	if (vbo[0]) {
		glDeleteBuffers(2, vbo);
		vbo[0] = vbo[1] = 0;
	}
	disposeProgram(mShaderProgram, mVertexShader, mFragmentShader);

	EXIT();
}

/**
 * バッファオブジェクトを初期化
 * @param target
 * @param bytes
 * @param data
 */
static inline void setup_buffer(GLuint &target,
	const GLsizeiptr &bytes, const void *data,
	const GLenum &usage = GL_STATIC_DRAW) {

	glBindBuffer(GL_ARRAY_BUFFER, target);
	GLCHECK("glBindBuffer");
	glBufferData(GL_ARRAY_BUFFER, bytes, data, usage);
	GLCHECK("glBufferData");
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	GLCHECK("glBindBuffer");
}

/**
 * 初期化処理
 * @param use_vbo 矩形描画時の頂点座標・テクスチャ座標にバッファオブジェクトを使うかどうか
 */
/*private*/
void GLRenderer::init(const bool &use_vbo) {
	ENTER();

    // attribute変数のロケーションを取得
    maPositionLoc = glGetAttribLocation(mShaderProgram, "aPosition");
    GLCHECK("glGetAttribLocation:maPositionLoc");
    maTextureCoordLoc = glGetAttribLocation(mShaderProgram, "aTextureCoord");
    GLCHECK("glGetAttribLocation:maTextureCoordLoc");
    // uniform変数のロケーションを取得
	muTexture = glGetUniformLocation(mShaderProgram, "sTexture");
	GLCHECK("glGetUniformLocation:sTexture");
	if (muTexture < 0) {
		muTexture = glGetUniformLocation(mShaderProgram, "sTexture1");
	}
	if (muTexture < 0) {
		LOGW("muTexture undefined");
	}
	muTexture2 = glGetUniformLocation(mShaderProgram, "sTexture2");
	GLCHECK("glGetUniformLocation:sTexture");
	muTexture3 = glGetUniformLocation(mShaderProgram, "sTexture3");
	GLCHECK("glGetUniformLocation:sTexture");
    muMVPMatrixLoc = glGetUniformLocation(mShaderProgram, "uMVPMatrix");
    GLCHECK("glGetUniformLocation:muMVPMatrixLoc");
    if (muMVPMatrixLoc < 0) {
		LOGD("muMVPMatrixLoc undefined");
    }
    muTexMatrixLoc = glGetUniformLocation(mShaderProgram, "uTexMatrix");
    GLCHECK("glGetUniformLocation:muTexMatrixLoc");
    if (muTexMatrixLoc < 0) {
		LOGD("muTexMatrixLoc undefined");
    }
    muTextureSzLoc = glGetUniformLocation(mShaderProgram, "uTextureSz");
    GLCHECK("glGetUniformLocation:muTextureSzLoc");
    if (muTextureSzLoc < 0) {
		LOGD("muTextureSzLoc undefined");	// フラグメントシェーダー内で使っていなければ最適化でなくなってしまうみたい
    }
	muFrameSzLoc =  glGetUniformLocation(mShaderProgram, "uFrameSz");
	GLCHECK("glGetUniformLocation:muFrameSzLoc");
	if (muFrameSzLoc < 0) {
		LOGD("uFrameSz undefined");	// フラグメントシェーダー内で使っていなければ最適化でなくなってしまうみたい
	}
    muBrightnessLoc = glGetUniformLocation(mShaderProgram, "uBrightness");
    GLCHECK("glGetUniformLocation:muBrightnessLoc");
    if (muBrightnessLoc < 0) {
		LOGD("muBrightnessLoc undefined");	// フラグメントシェーダー内で使っていなければ最適化でなくなってしまうみたい
    }

	if (use_vbo) {
		glGenBuffers(2, vbo);
		GLCHECK("glGenBuffers");
		// 頂点座標用バッファのセットアップ
		setup_buffer(vbo[0], VERTEX_BUF_SZ, FULL_RECTANGLE_COORDS);
		// テクスチャ座標バッファのセットアップ
		setup_buffer(vbo[1], VERTEX_BUF_SZ, FULL_RECTANGLE_TEX_COORDS);
	}

    EXIT();
}

void GLRenderer::update_vertices() {
	if (vbo[0]) {
		// 頂点座標をセット
		glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
		glVertexAttribPointer(maPositionLoc,
			COORD_NUM, GL_FLOAT, false, 0, 0);
		glEnableVertexAttribArray(maPositionLoc);
		// テクスチャ座標をセット
		glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);
		glVertexAttribPointer(maTextureCoordLoc,
			COORD_NUM, GL_FLOAT, false, 0, 0);
		glEnableVertexAttribArray(maTextureCoordLoc);
	} else {
		// 頂点座標配列"aPosition"を有効にする
		glEnableVertexAttribArray(maPositionLoc);
		GLCHECK("glEnableVertexAttribArray:maPositionLoc");
		// 頂点座標配列を"aPosition"へセット
		glVertexAttribPointer(maPositionLoc,
			COORD_NUM, GL_FLOAT, GL_FALSE,
			sizeof(GLfloat) * COORD_NUM, FULL_RECTANGLE_COORDS);
		GLCHECK("glVertexAttribPointer:maPositionLoc");
	    // テクスチャ座標配列"aTextureCoord"を有効にする
	    glEnableVertexAttribArray(maTextureCoordLoc);
	    GLCHECK("glEnableVertexAttribArray(maTextureCoordLoc)");
	    // テクスチャ座標配列を"aTextureCoord"へセット
	    glVertexAttribPointer(maTextureCoordLoc,
		    COORD_NUM, GL_FLOAT, GL_FALSE,
		    sizeof(GLfloat) * COORD_NUM, FULL_RECTANGLE_TEX_COORDS);
		GLCHECK("glVertexAttribPointer");
	}
}

static inline GLint texUnit2Index(const GLuint &tex_unit) {
	return (tex_unit >= GL_TEXTURE0) && (tex_unit <= GL_TEXTURE31)
		? GLint(tex_unit - GL_TEXTURE0) : 0;
}

/**
 * 描画実行
 * yuyvをrgbaに対応させる(2ピクセルの元データをテクスチャ1テクセルに代入する)時はview_widthを1/2にして呼び出すこと
 * @param texture 描画するテクスチャ
 * @param tex_matrix テクスチャ変換行列
 * @param mv_matrix モデルビュー変換行列
 * @return
 */
/*public*/
int GLRenderer::draw(GLTexture *texture, const GLfloat *tex_matrix, const GLfloat *mvp_matrix) {
	ENTER();

	// プログラムシェーダーを選択する
	glUseProgram(mShaderProgram);
    GLCHECK("draw:glUseProgram");
    texture->bind();
    // テクスチャユニット番号をセット
    glUniform1i(muTexture, texUnit2Index(texture->getTexUnit()));
	GLCHECK("draw:glUniform1i");
    // テクスチャサイズをセット
    if (LIKELY(muTextureSzLoc >= 0)) {
        glUniform2f(muTextureSzLoc, (float)texture->getTexWidth(), (float)texture->getTexHeight());
        GLCHECK("glUniform2f:muTextureSzLoc");
    }
    // イメージサイズをセット
	if (LIKELY(muFrameSzLoc >= 0)) {
		glUniform2f(muFrameSzLoc, (float)texture->getImageWidth(), (float)texture->getImageHeight());
		GLCHECK("glUniform2f:muFrameSzLoc");
	}
    // 明るさのオフセットをセット
    if (muBrightnessLoc) {
        glUniform1f(muBrightnessLoc, mBrightness);
        GLCHECK("glUniform1f:muBrightnessLoc");
    }

    // モデルビュー・射影行列をセット
    glUniformMatrix4fv(muMVPMatrixLoc, 1, GL_FALSE, mvp_matrix);
    GLCHECK("glUniformMatrix4fv:muMVPMatrixLoc");

    // テクスチャ変換行列をセット
    if (!tex_matrix) {
    	tex_matrix = texture->getTexMatrix();
	}
    glUniformMatrix4fv(muTexMatrixLoc, 1, GL_FALSE, tex_matrix);
    GLCHECK("glUniformMatrix4fv:muTexMatrixLoc");

	// 頂点座標・テクスチャ座標をセット
	update_vertices();
    // 矩形を描画
    glDrawArrays(GL_TRIANGLE_STRIP, 0, VERTEX_NUM);
    GLCHECK("glDrawArrays");

    // 終了・・・頂点配列・テクスチャ・シェーダーを無効にする
    glDisableVertexAttribArray(maPositionLoc);
    GLCHECK("glDisableVertexAttribArray:maPositionLoc");
    glDisableVertexAttribArray(maTextureCoordLoc);
    GLCHECK("glDisableVertexAttribArray:maTextureCoordLoc");
    texture->unbind();
    glUseProgram(0);
    GLCHECK("glUseProgram");

    RETURN(0, int);
}

int GLRenderer::draw(
	GLTexture *texture1,
	GLTexture *texture2,
	GLTexture *texture3,
	const GLfloat *mvp_matrix) {

	ENTER();

	// プログラムシェーダーを選択する
	glUseProgram(mShaderProgram);
    GLCHECK("draw:glUseProgram");
	// テクスチャユニット番号をセット
    texture1->bind();
    glUniform1i(muTexture, texUnit2Index(texture1->getTexUnit()));
	GLCHECK("draw:glUniform1i");
    if (texture2 && (muTexture2 >= 0)) {
		texture2->bind();
		glUniform1i(muTexture2, texUnit2Index(texture2->getTexUnit()));
		GLCHECK("draw:glUniform1i");
	}
	if (texture3 && (muTexture3 >= 0)) {
		texture3->bind();
		glUniform1i(muTexture2, texUnit2Index(texture3->getTexUnit()));
		GLCHECK("draw:glUniform1i");
	}
    // テクスチャサイズをセット
    if (LIKELY(muTextureSzLoc >= 0)) {
        glUniform2f(muTextureSzLoc, (float)texture1->getTexWidth(), (float)texture1->getTexHeight());
        GLCHECK("glUniform2f:muTextureSzLoc");
    }
    // イメージサイズをセット
	if (LIKELY(muFrameSzLoc >= 0)) {
		glUniform2f(muFrameSzLoc, (float)texture1->getImageWidth(), (float)texture1->getImageHeight());
		GLCHECK("glUniform2f:muFrameSzLoc");
	}
    // 明るさのオフセットをセット
    if (muBrightnessLoc) {
        glUniform1f(muBrightnessLoc, mBrightness);
        GLCHECK("glUniform1f:muBrightnessLoc");
    }

    // モデルビュー・射影行列をセット
    glUniformMatrix4fv(muMVPMatrixLoc, 1, GL_FALSE, mvp_matrix);
    GLCHECK("glUniformMatrix4fv:muMVPMatrixLoc");

    // テクスチャ変換行列をセット
	const GLfloat *tex_matrix = texture1->getTexMatrix();
    glUniformMatrix4fv(muTexMatrixLoc, 1, GL_FALSE, tex_matrix);
    GLCHECK("glUniformMatrix4fv:muTexMatrixLoc");

	// 頂点座標・テクスチャ座標をセット
	update_vertices();
    // 矩形を描画
    glDrawArrays(GL_TRIANGLE_STRIP, 0, VERTEX_NUM);
    GLCHECK("glDrawArrays");

    // 終了・・・頂点配列・テクスチャ・シェーダーを無効にする
    glDisableVertexAttribArray(maPositionLoc);
    GLCHECK("glDisableVertexAttribArray:maPositionLoc");
    glDisableVertexAttribArray(maTextureCoordLoc);
    GLCHECK("glDisableVertexAttribArray:maTextureCoordLoc");
    texture1->unbind();
	if (texture2) {
		texture2->unbind();
	}
	if (texture3) {
		texture3->unbind();
	}
    glUseProgram(0);
    GLCHECK("glUseProgram");

    RETURN(0, int);
}

#if __ANDROID__
/**
 * 描画実行
 * yuyvをrgbaに対応させる(2ピクセルの元データをテクスチャ1テクセルに代入する)時はview_widthを1/2にして呼び出すこと
 * @param texture 描画するテクスチャ
 * @param tex_matrix テクスチャ変換行列
 * @param mvp_matrix モデルビュー変換行列
 * @return
 */
int GLRenderer::draw(egl::EglImageWrapper *texture, const GLfloat *tex_matrix, const GLfloat *mvp_matrix) {
	ENTER();

	// プログラムシェーダーを選択する
	glUseProgram(mShaderProgram);
    GLCHECK("draw:glUseProgram");
    texture->bind();
    // テクスチャユニット番号をセット
    glUniform1i(muTexture, texUnit2Index(texture->tex_unit()));
	GLCHECK("draw:glUniform1i");
    // テクスチャサイズをセット
    if (LIKELY(muTextureSzLoc >= 0)) {
        glUniform2f(muTextureSzLoc, (float)texture->width(), (float)texture->height());
        GLCHECK("glUniform2f:muTextureSzLoc");
    }
    // イメージサイズをセット
	if (LIKELY(muFrameSzLoc >= 0)) {
		glUniform2f(muFrameSzLoc, (float)texture->width(), (float)texture->height());
		GLCHECK("glUniform2f:muFrameSzLoc");
	}
    // 明るさのオフセットをセット
    if (muBrightnessLoc) {
        glUniform1f(muBrightnessLoc, mBrightness);
        GLCHECK("glUniform1f:muBrightnessLoc");
    }

    // モデルビュー・射影行列をセット
    glUniformMatrix4fv(muMVPMatrixLoc, 1, GL_FALSE, mvp_matrix);
    GLCHECK("glUniformMatrix4fv:muMVPMatrixLoc");

    // テクスチャ変換行列をセット
    if (!tex_matrix) {
    	tex_matrix = texture->tex_matrix();
	}
    glUniformMatrix4fv(muTexMatrixLoc, 1, GL_FALSE, tex_matrix);
    GLCHECK("glUniformMatrix4fv:muTexMatrixLoc");

	// 頂点座標・テクスチャ座標をセット
	update_vertices();
    // 矩形を描画
    glDrawArrays(GL_TRIANGLE_STRIP, 0, VERTEX_NUM);
    GLCHECK("glDrawArrays");

    // 終了・・・頂点配列・テクスチャ・シェーダーを無効にする
    glDisableVertexAttribArray(maPositionLoc);
    GLCHECK("glDisableVertexAttribArray:maPositionLoc");
    glDisableVertexAttribArray(maTextureCoordLoc);
    GLCHECK("glDisableVertexAttribArray:maTextureCoordLoc");
    texture->unbind();
    glUseProgram(0);
    GLCHECK("glUseProgram");

    RETURN(0, int);
}
#endif

}	// namespace serenegiant::gl
