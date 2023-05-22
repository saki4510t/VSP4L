/*
 * aAndUsb
 * Copyright (c) 2014-2023 saki t_saki@serenegiant.com
 * Distributed under the terms of the GNU Lesser General Public License (LGPL v3.0) License.
 * License details are in the file license.txt, distributed as part of this software.
 */

#define LOG_TAG "glutils"

#if 1	// デバッグ情報を出さない時は1
	#ifndef LOG_NDEBUG
		#define	LOG_NDEBUG		// LOGV/LOGD/MARKを出力しない時
	#endif
	#undef USE_LOGALL			// 指定したLOGxだけを出力
#else
//	#define USE_LOGALL
	#define USE_LOGD
	#undef LOG_NDEBUG
	#undef NDEBUG
	#define DEBUG_GL_CHECK			// GL関数のデバッグメッセージを表示する時
#endif


#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <iterator>	// istream_iterator
#include <sstream>
#include <cstring>	// memcpy
#include <set>

#include "utilbase.h"
#include "glutils.h"

#if defined(__ANDROID__)
#include "hardware_buffer_stub.h"
#endif

namespace serenegiant::gl {

/**
 * 単位行列
 */
const GLfloat IDENTITY_MATRIX[] = {
	1.0f,	0.0f,	0.0f,	0.0f,
	0.0f,	1.0f,	0.0f,	0.0f,
	0.0f,	0.0f,	1.0f,	0.0f,
	0.0f,	0.0f,	0.0f,	1.0f,
};

#if !defined(DYNAMIC_ES3)
// GLES3のライブラリーをダイナミックリンクしないときのダミー用
GLboolean gl3stubInit() {
    return GL_TRUE;
}
#endif

#if !defined(DYNAMIC_HARDWARE_BUFFER_NDK)
#if __ANDROID__
bool init_hardware_buffer() {
	LOGI("minAPI=%d", __ANDROID_API__);
	return true;
}
bool is_hardware_buffer_supported_v29() {
	return false;	// FIXME とりあえず未サポートにしておく
}
#else
// Android以外は未対応なのでfalseを返す
bool init_hardware_buffer() {
	return false;
}
bool is_hardware_buffer_supported_v29() {
	return false;
}
#endif
#endif

/**
 * GLES3を使用可能かどうかを取得
 * @return
 */
bool isGLES3() {
	if (gl3stubInit()) {
		const char *versionStr = (const char*)glGetString(GL_VERSION);
	    return strstr(versionStr, "OpenGL ES 3.");
	} else {
		return false;
	}
}

/**
 * OpenGL3を使用可能かどうかを取得
 * FIXME この判定方法だとうまく行かなかった
 * #glGetString(GL_VERSION)で"4.5.0 NVIDIA 384.130"とかが返ってくる
 * @return
 */
bool isGL3() {
	const char* versionStr = (const char*)glGetString(GL_VERSION);
	return strstr(versionStr, "OpenGL ver. 3")
		|| strstr(versionStr, "OpenGL ver. 4");
}

/**
 * 単位行列にする
 * 境界チェックしていないのでOffsetから16個必ず確保しておくこと
 * @param m
 * @param offset
 */
void setIdentityMatrix(GLfloat *m, const int &offset) {
	memcpy(&m[offset], IDENTITY_MATRIX, sizeof(IDENTITY_MATRIX));
}

/**
 * OpenGL|ESの4x4マトリックスを文字列へ変換する
 * @param mat 要素数16+offset以上確保しておくこと
 * @param offset
 * @return
 */
std::string mat2String(const float *mat, const int &offset) {
	std::stringstream ss;
	if (mat) {
		ss << std::endl;
		for (int i = 0; i < 4; i++) {
			for (int j= 0; j < 4; j++) {
				ss << mat[i * 4 + j] << ',';
			}
			ss << std::endl;
		}
	}
	return ss.str();
}

/**
 * デバッグ用に行列をログに出力する
 * 境界チェックしていないので16個必ず確保しておくこと
 * @param m
 */
void printMatrix(const GLfloat *m) {
	const GLfloat *p;
	for (int i = 0; i < 4; i++) {
		p = &m[i * 4];
		LOGI("%2d)%8.4f,%8.4f,%8.4f,%8.4f", i * 4, p[0], p[1], p[2], p[3]);
	}
}

/**
 * 指定したGLenumに対応する文字列をglGetStringで取得してログへ出力する
 * @param name
 * @param s
 */
void printGLString(const char *name, GLenum s) {
    const char *v = (const char *) glGetString(s);
    MARK("GL %s = %s", name, v);
}

/**
 * glGetErrorで取得したOpenGLのエラーをログに出力する
 * @param op
 */
void checkGlError(const char* op) {
    for (auto error = glGetError(); error; error = glGetError()) {
        LOGE("glError (0x%x) after %s()", error, op);
    }
}

/**
 * glGetErrorで取得したOpenGLのエラーをログに出力する
 * @param filename
 * @param line
 * @param func
 * @param op 実行した関数名
 */
void checkGlError(
	const char *filename, const int &line, const char *func,
	const char *op) {
    for (auto error = glGetError(); error; error = glGetError()) {
        LOGE("%s:%d:%s:glError (0x%x) after %s()",
        	filename, line, func, error, op);
    }
}

/**
 * 文字列で指定したGL拡張に対応しているかどうかを取得
 * キャッシュが効くので可能ならEGLBase::hasGLExtensionを使った方がよい
 * @param s
 * @return
 */
bool hasGLExtension(const std::string &s) {
	// 対応しているGL拡張を解析
	std::istringstream glext_stream(reinterpret_cast<const char*>(glGetString(GL_EXTENSIONS)));
	auto gLExtensions = std::set<std::string> {
		std::istream_iterator<std::string>{glext_stream},
		std::istream_iterator<std::string>{}
	};
	return gLExtensions.find(s) != gLExtensions.end();
}

/**
 * テクスチャを生成する
 * @param target GL_TEXTURE_2D等
 * @param tex_unit
 * @param alignment alignmentは1, 2, 4, 8のいずれか
 * @return
 */
GLuint createTexture(const GLenum &target, const GLenum  &tex_unit, const int &alignment) {
	ENTER();

	// テクスチャはバイト単位でつめ込まれている
	/* glPixelStorei() は，コンピュータ側のメモリと OpenGL 側のメモリとの間でデータをやり取りする際に，
	 * 画像（この場合はテクスチャ）がメモリにどのように格納されているかを OpenGL に伝えるために用います．
	 * 引数 pname に GL_UNPACK_ALIGNMENT を指定した場合，param にはメモリを参照するときの
	 * 「アドレス境界」を指定します．param には 1, 2, 4, 8 のいずれかが指定できます．
	 * 画素の色が各色 1 バイトで表されているとき，1 画素が RGBA で表現されていれば，
	 * メモリ上の 4 バイトごとに 1 画素が配置されていることになります（4 バイト境界＝ワード境界）．
	 * このときは param に 4 が指定できます．一方，1 画素が RGB で表現されていれば，メモリ上の
	 * 3 バイトごとに 1 画素が配置されていることになります．"3" というアドレス境界は許されませんから
	 * （param に 3 は指定できない），この場合は param に 1 を指定する必要があります．
	 * 一般に，この数が多いほど効率的なメモリアクセスが可能になります．pname に指定できる
	 * その他の値については，マニュアル等を参照してください．
	 * なお，同じことをする関数に glPixelStoref(GLenum pname, GLfloat param) があります
	 * (param のデータ型が違うだけ）．*/
	GLuint tex;
	glActiveTexture(tex_unit);	// テクスチャユニットを選択
	GLCHECK("glActiveTexture");
	glPixelStorei(GL_UNPACK_ALIGNMENT, alignment);
	GLCHECK("glPixelStorei");
	glGenTextures(1, &tex);
	GLCHECK("glGenTextures");
	glBindTexture(target, tex);
	GLCHECK("glBindTexture");
	// テクスチャの繰り返し方法を指定
	glTexParameteri(target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	GLCHECK("glTexParameteri");
	glTexParameteri(target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	GLCHECK("glTexParameteri");
	// テクスチャの拡大・縮小方法を指定GL_NEARESTにすると補間無し
	glTexParameteri(target, GL_TEXTURE_MIN_FILTER, /*GL_NEAREST*/ GL_LINEAR);
//	glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	GLCHECK("glTexParameteri");
	glTexParameteri(target, GL_TEXTURE_MAG_FILTER, /*GL_NEAREST*/ GL_LINEAR);
//	glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	GLCHECK("glTexParameteri");

	RETURN(tex, GLuint);
}

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
GLuint loadShader(GLenum shaderType, const char *pSource) {
	ENTER();

	LOGV("shaderType=%d,shader=%s", shaderType, pSource);

    GLuint shader = glCreateShader(shaderType);
    if (shader) {
        glShaderSource(shader, 1, &pSource, nullptr);
    	GLCHECK("glShaderSource");
        glCompileShader(shader);
    	GLCHECK("glCompileShader");
        GLint compiled = 0;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    	GLCHECK("glGetShaderiv");
        if (!compiled) {	// コンパイル失敗?
            GLint infoLen = 0;
            glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen);
        	GLCHECK("glGetShaderiv");
            if (infoLen) {
                char *buf = (char*) malloc(infoLen);
                if (buf) {
                    glGetShaderInfoLog(shader, infoLen, nullptr, buf);
                	GLCHECK("glGetShaderInfoLog");
                    LOGE("loadShader:Could not compile shader %d:\n%s\nsrc=%s\n", shaderType, buf, pSource);
                    free(buf);
                }
                glDeleteShader(shader);
            	GLCHECK("glDeleteShader");
                shader = 0;
            }
        }
    }

	RETURN(shader, GLuint);
}

/**
 * シェーダプログラムをビルド・設定する
 * @param pVertexSource
 * @param pFragmentSource
 * @param vertex_shader
 * @param fragment_shader
 * @return
 */
GLuint createShaderProgram(
	const char *pVertexSource, const char *pFragmentSource,
	GLuint *vertex_shader, GLuint *fragment_shader) {

	ENTER();

	// 頂点シェーダプログラムをロード・コンパイルする
    GLuint vertexShader = loadShader(GL_VERTEX_SHADER, pVertexSource);
    if (!vertexShader) {
    	LOGE("createShaderProgram:fail to compile vertexShader\n%s", pVertexSource);
        return 0;
    }
    // フラグメントシェーダプログラムをロード・コンパイルする
    GLuint fragmentShader = loadShader(GL_FRAGMENT_SHADER, pFragmentSource);
    if (!fragmentShader) {
    	LOGE("createShaderProgram:fail to compile fragmentShader\n%s", pFragmentSource);
        return 0;
    }

    GLuint program = glCreateProgram();
	GLCHECK("glCreateProgram");
    if (program) {
        glAttachShader(program, vertexShader);
        GLCHECK("glAttachShader");
        glAttachShader(program, fragmentShader);
        GLCHECK("glAttachShader");

        glLinkProgram(program);
    	GLCHECK("glLinkProgram");
        GLint linkStatus = GL_FALSE;
        glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);
    	GLCHECK("glGetProgramiv");
        if (linkStatus != GL_TRUE) {
            GLint bufLength = 0;
            glGetProgramiv(program, GL_INFO_LOG_LENGTH, &bufLength);
        	GLCHECK("glGetProgramiv");
            if (bufLength) {
                char* buf = (char*) malloc(bufLength);
                if (buf) {
                    glGetProgramInfoLog(program, bufLength, nullptr, buf);
                	GLCHECK("glGetProgramInfoLog");
                    LOGE("createShaderProgram:Could not link program:\n%s\n", buf);
                    free(buf);
                }
            }
            disposeProgram(program, vertexShader, fragmentShader);
        }
    }
    if (LIKELY(program)) {
    	if (vertex_shader) {
			*vertex_shader = vertexShader;
    	}
    	if (fragment_shader) {
			*fragment_shader = fragmentShader;
    	}
    	LOGV("createShaderProgram:success to create shader program");
    }

    RETURN(program, GLuint);
}

/**
 * シェーダープログラムを破棄する
 * @param shader_program
 * @param vertex_shader
 * @param fragment_shader
 */
void disposeProgram(GLuint &shader_program, GLuint &vertex_shader, GLuint &fragment_shader) {
	ENTER();

	if (LIKELY(shader_program)) {
		if (LIKELY(vertex_shader)) {
			glDetachShader(shader_program, vertex_shader);
        	GLCHECK("glDetachShader");
			glDeleteShader(vertex_shader);
        	GLCHECK("glDeleteShader");
			vertex_shader = 0;
		}
		if (LIKELY(fragment_shader)) {
			glDetachShader(shader_program, fragment_shader);
        	GLCHECK("glDetachShader");
			glDeleteShader(fragment_shader);
        	GLCHECK("glDeleteShader");
			fragment_shader = 0;
		}
		glDeleteProgram(shader_program);
    	GLCHECK("glDeleteProgram");
		shader_program = 0;
	}

	EXIT();
}

/**
 * ウインドウを消去するためのヘルパー関数
 * @param cl ARGB
 */
void clear_window(const uint32_t &cl) {
	glClearColor(
		(float)((cl >> 16u) & 0xffu) / 255.0f,		// R
		(float)((cl >> 8u) & 0xffu) / 255.0f,		// G
		(float)(cl & 0xffu) / 255.0f,				// B
		(float)((cl >> 24u) & 0xffu) / 255.0f);	// A
	glClear(GL_COLOR_BUFFER_BIT/*mask*/);
}

}	// namespace serenegiant::gl
