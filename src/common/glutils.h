/*
 * aAndUsb
 * Copyright (c) 2014-2023 saki t_saki@serenegiant.com
 * Distributed under the terms of the GNU Lesser General Public License (LGPL v3.0) License.
 * License details are in the file license.txt, distributed as part of this software.
 */

#ifndef GLUTILS_H_
#define GLUTILS_H_

#include <string>
#define EGL_EGLEXT_PROTOTYPES
#define GL_GLEXT_PROTOTYPES
#if defined(__ANDROID__)
	#include <EGL/egl.h>
	#include <EGL/eglext.h>
	#if DYNAMIC_ES3
		#include "gl3stub.h"
	#else
		// Include the latest possible header file( GL version header )
		#if __ANDROID_API__ >= 24
			#include <GLES3/gl32.h>
		#elif __ANDROID_API__ >= 21
			#include <GLES3/gl31.h>
		#else
			#include <GLES3/gl3.h>
		#endif
		#include <GLES2/gl2ext.h>
	#endif
#else
	#if defined(__APPLE__)
		#include <OpenGL/gl.h>
		#include <OpenGL/glu.h>
		#include <GLUT/glut.h>
	#else
		#if defined(_WIN32)
			#include <windows.h>
		#endif
        #include <GL/glew.h>
		#include <GL/gl.h>
#endif
#endif

#undef GLCHECK
#ifdef DEBUG_GL_CHECK
//	#define	GLCHECK(OP) checkGlError(OP)
	#define	GLCHECK(OP) checkGlError(basename(__FILE__), __LINE__, __FUNCTION__, OP)
#else
	#define	GLCHECK(OP)
#endif

#include <string>

namespace serenegiant::gl {

/**
 * 単位行列
 */
extern const GLfloat IDENTITY_MATRIX[];

/**
 * GLES3を使用可能かどうかを取得
 * @return
 */
bool isGLES3();

/**
 * OpenGL3を使用可能かどうかを取得
 * @return
 */
bool isGL3();

/**
 * 単位行列にする
 * 境界チェックしていないのでOffsetから16個必ず確保しておくこと
 * @param m
 * @param offset
 */
void setIdentityMatrix(GLfloat *m, const int &offset = 0);

/**
 * OpenGL|ESの4x4マトリックスを文字列へ変換する
 * @param mat 要素数16+offset以上確保しておくこと
 * @param offset
 * @return
 */
std::string mat2String(const float *mat, const int &offset);

/**
 * デバッグ用に行列をログに出力する
 * 境界チェックしていないので16個必ず確保しておくこと
 * @param m
 */
void printMatrix(const GLfloat *m);

/**
 * 指定したGLenumに対応する文字列をglGetStringで取得してログへ出力する
 * @param name
 * @param s
 */
void printGLString(const char *name, GLenum s);

/**
 * glGetErrorで取得したOpenGLのエラーをログに出力する
 * @param op 実行した関数名
 */
void checkGlError(const char *op);

/**
 * glGetErrorで取得したOpenGLのエラーをログに出力する
 * @param filename
 * @param line
 * @param func
 * @param op 実行した関数名
 */
void checkGlError(
	const char *filename, const int &line, const char *func,
	const char *op);

/**
 * 文字列で指定したGL拡張に対応しているかどうかを取得
 * キャッシュが効くので可能ならEGLBase::hasGLExtensionを使った方がよい
 * @param s
 * @return
 */
bool hasGLExtension(const std::string &s);

/**
 * テクスチャを生成する
 * @param target GL_TEXTURE_2D等
 * @param tex_unit
 * @param alignment alignmentは1, 2, 4, 8のいずれか
 * @return
 */
GLuint createTexture(const GLenum &target, const GLenum  &tex_unit, const int &alignment);

//--------------------------------------------------------------------------------
// シェーダー関係の処理
//--------------------------------------------------------------------------------
/**
 * シェーダプログラムをロード・コンパイルする
 * createShaderProgramの下請け
 * @param shaderType
 * @param pSource
 * @return
 */
GLuint loadShader(GLenum shaderType, const char* pSource);

/**
 * シェーダプログラムをビルド・設定する
 * @param pVertexSource
 * @param pFragmentSource
 * @param vertex_shader
 * @param fragment_shader
 * @return
 */
GLuint createShaderProgram(
	const char* pVertexSource,
	const char* pFragmentSource,
	GLuint *vertex_shader = nullptr, GLuint *fragment_shader = nullptr);

/**
 * シェーダープログラムを破棄する
 * @param shader_program
 * @param vertex_shader
 * @param fragment_shader
 */
void disposeProgram(GLuint &shader_program, GLuint &vertex_shader, GLuint &fragment_shader);

/**
 * ウインドウを消去するためのヘルパー関数
 * @param cl ARGB
 */
void clear_window(const uint32_t &cl = 0);

}	// namespace serenegiant::gl

#endif /* GLUTILS_H_ */
