/*
 * aAndUsb
 * Copyright (c) 2014-2023 saki t_saki@serenegiant.com
 * Distributed under the terms of the GNU Lesser General Public License (LGPL v3.0) License.
 * License details are in the file license.txt, distributed as part of this software.
 */

#ifndef AANDUSB_BGR_FSH_H
#define AANDUSB_BGR_FSH_H

// rgbのテクスチャを描画するフラグメントシェーダー

#if __ANDROID__ || defined(ENABLE_GLES)
constexpr const char *bgr_gl2_fsh =
R"SHADER(#version 100

precision highp float;
varying vec2 vTextureCoord;
uniform sampler2D sTexture;
uniform vec2 uTextureSz;
uniform vec2 uFrameSz;

void main() {
	gl_FragColor = vec4(texture2D(sTexture, vTextureCoord).bgr, 1.0);
}
)SHADER";

constexpr const char *bgr_gl3_fsh =
R"SHADER(#version 300 es

precision highp float;
in vec2 vTextureCoord;
uniform sampler2D sTexture;
uniform vec2 uTextureSz;
uniform vec2 uFrameSz;
layout(location = 0) out vec4 o_FragColor;

void main() {
	o_FragColor = vec4(texture(sTexture, vTextureCoord).bgr, 1.0);
}
)SHADER";
#else	// #if __ANDROID__
constexpr const char *bgr_gl2_fsh =
R"SHADER(#version 100

precision highp float;
varying vec2 vTextureCoord;
uniform sampler2D sTexture;
uniform vec2 uTextureSz;
uniform vec2 uFrameSz;

void main() {
	gl_FragColor = vec4(texture2D(sTexture, vTextureCoord).bgr, 1.0);
}
)SHADER";

constexpr const char *bgr_gl3_fsh =
R"SHADER(#version 330

precision highp float;
in vec2 vTextureCoord;
uniform sampler2D sTexture;
uniform vec2 uTextureSz;
uniform vec2 uFrameSz;
layout(location = 0) out vec4 o_FragColor;

void main() {
	o_FragColor = vec4(texture(sTexture, vTextureCoord).bgr, 1.0);
}
)SHADER";
#endif

#endif //AANDUSB_BGR_FSH_H
