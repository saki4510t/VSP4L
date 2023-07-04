/*
 * aAndUsb
 * Copyright (c) 2014-2023 saki t_saki@serenegiant.com
 * Distributed under the terms of the GNU Lesser General Public License (LGPL v3.0) License.
 * License details are in the file license.txt, distributed as part of this software.
 */

#ifndef AANDUSB_RGBA_FSH_H
#define AANDUSB_RGBA_FSH_H

// rgbaのテクスチャを描画するフラグメントシェーダー

#if __ANDROID__ || defined(ENABLE_GLES)
constexpr const char *rgba_gl2_fsh =
R"SHADER(#version 100

precision highp float;
varying vec2 vTextureCoord;
uniform sampler2D sTexture;
uniform vec2 uTextureSz;
uniform vec2 uFrameSz;

void main() {
	gl_FragColor = texture2D(sTexture, vTextureCoord);
}
)SHADER";

constexpr const char *rgba_gl2_ext_fsh =
R"SHADER(#version 100
#extension GL_OES_EGL_image_external : require
precision highp float;
varying vec2 vTextureCoord;
uniform samplerExternalOES sTexture;
uniform vec2 uTextureSz;
uniform vec2 uFrameSz;

void main() {
	gl_FragColor = texture2D(sTexture, vTextureCoord);
}
)SHADER";

constexpr const char *rgba_gl3_fsh =
R"SHADER(#version 300 es

precision highp float;
in vec2 vTextureCoord;
uniform sampler2D sTexture;
uniform vec2 uTextureSz;
uniform vec2 uFrameSz;
layout(location = 0) out vec4 o_FragColor;

void main() {
	o_FragColor = texture(sTexture, vTextureCoord);
}
)SHADER";

constexpr const char *rgba_gl3_ext_fsh =
R"SHADER(#version 300 es
#extension GL_OES_EGL_image_external_essl3 : require
precision highp float;
in vec2 vTextureCoord;
uniform samplerExternalOES sTexture;
uniform vec2 uTextureSz;
uniform vec2 uFrameSz;
layout(location = 0) out vec4 o_FragColor;

void main() {
	o_FragColor = texture(sTexture, vTextureCoord);
}
)SHADER";
#else	// #if __ANDROID__
constexpr const char *rgba_gl2_fsh =
R"SHADER(#version 100

precision highp float;
varying vec2 vTextureCoord;
uniform sampler2D sTexture;
uniform vec2 uTextureSz;
uniform vec2 uFrameSz;

void main() {
	gl_FragColor = texture2D(sTexture, vTextureCoord);
}
)SHADER";

constexpr const char *rgba_gl3_fsh=
R"SHADER(#version 330

precision highp float;
in vec2 vTextureCoord;
uniform sampler2D sTexture;
uniform vec2 uTextureSz;
uniform vec2 uFrameSz;
layout(location = 0) out vec4 o_FragColor;

void main() {
	o_FragColor = texture(sTexture, vTextureCoord);
}
)SHADER";
#endif

#endif //AANDUSB_RGBA_FSH_H
