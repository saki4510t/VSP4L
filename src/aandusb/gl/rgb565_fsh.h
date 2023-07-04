/*
 * aAndUsb
 * Copyright (c) 2014-2023 saki t_saki@serenegiant.com
 * Distributed under the terms of the GNU Lesser General Public License (LGPL v3.0) License.
 * License details are in the file license.txt, distributed as part of this software.
 */

#ifndef AANDUSB_RGB565_FSH_H
#define AANDUSB_RGB565_FSH_H

// yuyvのデータをrgbaのテクスチャとして渡して変換しながら描画するフラグメントシェーダー

#if __ANDROID__ || defined(ENABLE_GLES)
constexpr const char *rgb565_gl2_fsh =
R"SHADER(#version 100

precision highp float;
varying vec2 vTextureCoord;
uniform sampler2D sTexture;
uniform vec2 uTextureSz;
uniform vec2 uFrameSz;
uniform float uBrightness;

#define EPS 0.00001

void main() {
	float rg = texture2D(sTexture, vTextureCoord).a * 255.0;
	float gb = texture2D(sTexture, vTextureCoord).r * 255.0;
	float r = clamp(rg / 8.0, 0.0, 31.0) / 31.0;
	float g = (mod(rg, 8.0) * 8.0 + clamp(gb / 32.0, 0.0, 31.0)) / 63.0;
	float b = clamp(mod(gb, 32.0), 0.0, 31.0) / 31.0;
	gl_FragColor = clamp(vec4(r, g, b, 1.0), EPS, 1.0);
}
)SHADER";

constexpr const char *rgb565_gl3_fsh =
R"SHADER(#version 300 es

precision highp float;
varying vec2 vTextureCoord;
uniform sampler2D sTexture;
uniform vec2 uTextureSz;
uniform vec2 uFrameSz;
uniform float uBrightness;

#define EPS 0.00001

void main() {
	float rg = texture(sTexture, vTextureCoord).a * 255.0;
	float gb = texture(sTexture, vTextureCoord).r * 255.0;
	float r = clamp(rg / 8.0, 0.0, 31.0) / 31.0;
	float g = (mod(rg, 8.0) * 8.0 + clamp(gb / 32.0, 0.0, 31.0)) / 63.0;
	float b = clamp(mod(gb, 32.0), 0.0, 31.0) / 31.0;
	o_FragColor = clamp(vec4(r, g, b, 1.0), EPS, 1.0);
}
)SHADER";
#else	// #if __ANDROID__
constexpr const char *rgb565_gl2_fsh =
R"SHADER(#version 100

precision highp float;
varying vec2 vTextureCoord;
uniform sampler2D sTexture;
uniform vec2 uTextureSz;
uniform vec2 uFrameSz;
uniform float uBrightness;

#define EPS 0.00001

void main() {
	float rg = texture2D(sTexture, vTextureCoord).a * 255.0;
	float gb = texture2D(sTexture, vTextureCoord).r * 255.0;
	float r = clamp(rg / 8.0, 0.0, 31.0) / 31.0;
	float g = (mod(rg, 8.0) * 8.0 + clamp(gb / 32.0, 0.0, 31.0)) / 63.0;
	float b = clamp(mod(gb, 32.0), 0.0, 31.0) / 31.0;
	gl_FragColor = clamp(vec4(r, g, b, 1.0), EPS, 1.0);
}
)SHADER";

constexpr const char *rgb565_gl3_fsh =
R"SHADER(#version 330

precision highp float;
varying vec2 vTextureCoord;
uniform sampler2D sTexture;
uniform vec2 uTextureSz;
uniform vec2 uFrameSz;
uniform float uBrightness;

#define EPS 0.00001

void main() {
	float rg = texture2D(sTexture, vTextureCoord).a * 255.0;
	float gb = texture2D(sTexture, vTextureCoord).r * 255.0;
	float r = clamp(rg / 8.0, 0.0, 31.0) / 31.0;
	float g = (mod(rg, 8.0) * 8.0 + clamp(gb / 32.0, 0.0, 31.0)) / 63.0;
	float b = clamp(mod(gb, 32.0), 0.0, 31.0) / 31.0;
	o_FragColor = clamp(vec4(r, g, b, 1.0), EPS, 1.0);
}
)SHADER";
#endif

#endif //AANDUSB_RGB565_FSH_H
