/*
 * aAndUsb
 * Copyright (c) 2014-2023 saki t_saki@serenegiant.com
 * Distributed under the terms of the GNU Lesser General Public License (LGPL v3.0) License.
 * License details are in the file license.txt, distributed as part of this software.
 */

#ifndef AANDUSB_TEXTURE_VSH_H
#define AANDUSB_TEXTURE_VSH_H

// 矩形にテクスチャを貼り付けるための頂点シェーダー

#if __ANDROID__
constexpr const char *texture_gl2_vsh =
R"SHADER(#version 100
attribute vec4 aPosition;
attribute vec4 aTextureCoord;
varying vec2 vTextureCoord;
uniform mat4 uMVPMatrix;
uniform mat4 uTexMatrix;

void main() {
	vTextureCoord = (uTexMatrix * aTextureCoord).xy;
	gl_Position = uMVPMatrix * aPosition;
}
)SHADER";

constexpr const char *texture_gl3_vsh =
R"SHADER(#version 300 es
in vec4 aPosition;
in vec4 aTextureCoord;
out vec2 vTextureCoord;
uniform mat4 uMVPMatrix;
uniform mat4 uTexMatrix;

void main() {
	vTextureCoord = (uTexMatrix * aTextureCoord).xy;
	gl_Position = uMVPMatrix * aPosition;
}
)SHADER";
#else	// #if __ANDROID__
constexpr const char *texture_gl2_vsh =
R"SHADER(#version 100
attribute vec4 aPosition;
attribute vec4 aTextureCoord;
varying vec2 vTextureCoord;
uniform mat4 uMVPMatrix;
uniform mat4 uTexMatrix;

void main() {
	vTextureCoord = (uTexMatrix * aTextureCoord).xy;
	gl_Position = uMVPMatrix * aPosition;
}
)SHADER";

constexpr const char *texture_gl3_vsh =
R"SHADER(#version 330
in vec4 aPosition;
in vec4 aTextureCoord;
out vec2 vTextureCoord;
uniform mat4 uMVPMatrix;
uniform mat4 uTexMatrix;

void main() {
	vTextureCoord = (uTexMatrix * aTextureCoord).xy;
	gl_Position = uMVPMatrix * aPosition;
}
)SHADER";
#endif

#endif //AANDUSB_TEXTURE_VSH_H
