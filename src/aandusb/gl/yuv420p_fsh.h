/*
 * aAndUsb
 * Copyright (c) 2014-2023 saki t_saki@serenegiant.com
 * Distributed under the terms of the GNU Lesser General Public License (LGPL v3.0) License.
 * License details are in the file license.txt, distributed as part of this software.
 */

#ifndef AANDUSB_YUV420p_FSH_H
#define AANDUSB_YUV420p_FSH_H

// YUV420pの3プレーンのデータをGL_LUMINANCEの1つのテクスチャとして渡して
// RGBAとして描画するためのフラグメントシェーダー
#if __ANDROID__
constexpr const char *yuv420p_as_lumi_gl2_fsh =
R"SHADER(#version 100

precision highp float;
varying vec2 vTextureCoord;
uniform sampler2D sTexture;
uniform vec2 uTextureSz;
uniform vec2 uFrameSz;
uniform float uBrightness;

const mat3 MAT_YUV2RGB = mat3 (
//  r      g      b
	1.0,    1.0,    1.0,    // y
	0.0,    -0.39173,  2.017,    // u
	1.5958,    -0.81290,  0.0      // v
);

const vec3 YUV_OFFSET = vec3(0.0625, 0.5, 0.5);
const float Y_SCALE = 2.0 / 3.0;
const vec2 UV_SZ_DIV = vec2(2.0, 6.0);

#define EPS 0.00001

// YUV420pの3プレーンのデータをGL_LUMINANCEの1つのテクスチャとしてまとめて渡して換算する場合(元の1ピクセル分のデータがテクセル1個に対応)
void main() {
	vec2 uv_sz = (uFrameSz / uTextureSz) / UV_SZ_DIV;
	vec2 uv_tex = vec2(vTextureCoord.x / 2.0, vTextureCoord.y / 6.0 + uv_sz.y * 4.0);
	float y, u, v;
	y = texture2D(sTexture, vec2(vTextureCoord.x, vTextureCoord.y * Y_SCALE)).r;
	float u1 = texture2D(sTexture, vec2(uv_tex.x,           uv_tex.y)).r;
	float u2 = texture2D(sTexture, vec2(uv_tex.x + uv_sz.x, uv_tex.y)).r;
	u = (u1 + u2) / 2.0;
	float v1 = texture2D(sTexture, vec2(uv_tex.x,           uv_tex.y + uv_sz.y)).r;
	float v2 = texture2D(sTexture, vec2(uv_tex.x + uv_sz.x, uv_tex.y + uv_sz.y)).r;
	v = (v1 + v2) / 2.0;
	// 輝度がEPS未満なら黒を返す(プレビューを停止した時に画面が緑になるのを防ぐため)
	if (y > EPS) {
		vec3 yuv = vec3(clamp(y + uBrightness, 0.0, 1.0), u, v) - YUV_OFFSET;
		vec3 rgb = clamp((MAT_YUV2RGB * yuv), 0.0, 1.0);
		gl_FragColor = vec4(rgb, 1.0);
	} else {
		gl_FragColor = vec4(0.0, 0.0, 0.0, 1.0);
	}
}
)SHADER";

constexpr const char *yuv420p_as_lumi_gl3_fsh =
R"SHADER(#version 300 es

precision highp float;
in vec2 vTextureCoord;
uniform sampler2D sTexture;
uniform vec2 uTextureSz;
uniform vec2 uFrameSz;
uniform float uBrightness;
layout(location = 0) out vec4 o_FragColor;

const mat3 MAT_YUV2RGB = mat3 (
//  r      g      b
	1.0,    1.0,    1.0,    // y
	0.0,    -0.39173,  2.017,    // u
	1.5958,    -0.81290,  0.0      // v
);

const vec3 YUV_OFFSET = vec3(0.0625, 0.5, 0.5);
const float Y_SCALE = 2.0 / 3.0;
const vec2 UV_SZ_DIV = vec2(2.0, 6.0);

#define EPS 0.00001

// YUV420pの3プレーンのデータをGL_LUMINANCEの1つのテクスチャとしてまとめて渡して換算する場合(元の1ピクセル分のデータがテクセル1個に対応)
void main() {
	vec2 uv_sz = (uFrameSz / uTextureSz) / UV_SZ_DIV;
	vec2 uv_tex = vec2(vTextureCoord.x / 2.0, vTextureCoord.y / 6.0 + uv_sz.y * 4.0);
	float y, u, v;
	y = texture(sTexture, vec2(vTextureCoord.x, vTextureCoord.y * Y_SCALE)).r;
	float u1 = texture(sTexture, vec2(uv_tex.x,           uv_tex.y)).r;
	float u2 = texture(sTexture, vec2(uv_tex.x + uv_sz.x, uv_tex.y)).r;
	u = (u1 + u2) / 2.0;
	float v1 = texture(sTexture, vec2(uv_tex.x,           uv_tex.y + uv_sz.y)).r;
	float v2 = texture(sTexture, vec2(uv_tex.x + uv_sz.x, uv_tex.y + uv_sz.y)).r;
	v = (v1 + v2) / 2.0;
	// 輝度がEPS未満なら黒を返す(プレビューを停止した時に画面が緑になるのを防ぐため)
	if (y > EPS) {
		vec3 yuv = vec3(clamp(y + uBrightness, 0.0, 1.0), u, v) - YUV_OFFSET;
		vec3 rgb = clamp((MAT_YUV2RGB * yuv), 0.0, 1.0);
		o_FragColor = vec4(rgb, 1.0);
	} else {
		o_FragColor = vec4(0.0, 0.0, 0.0, 1.0);
	}
}
)SHADER";
#else	// #if __ANDROID__
constexpr const char *yuv420p_as_lumi_gl2_fsh =
R"SHADER(#version 100

precision highp float;
varying vec2 vTextureCoord;
uniform sampler2D sTexture;
uniform vec2 uTextureSz;
uniform vec2 uFrameSz;
uniform float uBrightness;

const mat3 MAT_YUV2RGB = mat3 (
//  r      g      b
	1.0,    1.0,    1.0,    // y
	0.0,    -0.39173,  2.017,    // u
	1.5958,    -0.81290,  0.0      // v
);

const vec3 YUV_OFFSET = vec3(0.0625, 0.5, 0.5);
const float Y_SCALE = 2.0 / 3.0;
const vec2 UV_SZ_DIV = vec2(2.0, 6.0);

#define EPS 0.00001

// YUV420pの3プレーンのデータをGL_LUMINANCEの1つのテクスチャとしてまとめて渡して換算する場合(元の1ピクセル分のデータがテクセル1個に対応)
void main() {
	vec2 uv_sz = (uFrameSz / uTextureSz) / UV_SZ_DIV;
	vec2 uv_tex = vec2(vTextureCoord.x / 2.0, vTextureCoord.y / 6.0 + uv_sz.y * 4.0);
	float y, u, v;
	y = texture2D(sTexture, vec2(vTextureCoord.x, vTextureCoord.y * Y_SCALE)).r;
	float u1 = texture2D(sTexture, vec2(uv_tex.x,           uv_tex.y)).r;
	float u2 = texture2D(sTexture, vec2(uv_tex.x + uv_sz.x, uv_tex.y)).r;
	u = (u1 + u2) / 2.0;
	float v1 = texture2D(sTexture, vec2(uv_tex.x,           uv_tex.y + uv_sz.y)).r;
	float v2 = texture2D(sTexture, vec2(uv_tex.x + uv_sz.x, uv_tex.y + uv_sz.y)).r;
	v = (v1 + v2) / 2.0;
	// 輝度がEPS未満なら黒を返す(プレビューを停止した時に画面が緑になるのを防ぐため)
	if (y > EPS) {
		vec3 yuv = vec3(clamp(y + uBrightness, 0.0, 1.0), u, v) - YUV_OFFSET;
		vec3 rgb = clamp((MAT_YUV2RGB * yuv), 0.0, 1.0);
		gl_FragColor = vec4(rgb, 1.0);
	} else {
		gl_FragColor = vec4(0.0, 0.0, 0.0, 1.0);
	}
}
)SHADER";

constexpr const char *yuv420p_as_lumi_gl3_fsh =
R"SHADER(#version 330

precision highp float;
in vec2 vTextureCoord;
uniform sampler2D sTexture;
uniform vec2 uTextureSz;
uniform vec2 uFrameSz;
uniform float uBrightness;
layout(location = 0) out vec4 o_FragColor;

const mat3 MAT_YUV2RGB = mat3 (
//  r      g      b
	1.0,    1.0,    1.0,    // y
	0.0,    -0.39173,  2.017,    // u
	1.5958,    -0.81290,  0.0      // v
);

const vec3 YUV_OFFSET = vec3(0.0625, 0.5, 0.5);
const float Y_SCALE = 2.0 / 3.0;
const vec2 UV_SZ_DIV = vec2(2.0, 6.0);

#define EPS 0.00001

// YUV420pの3プレーンのデータをGL_LUMINANCEの1つのテクスチャとしてまとめて渡して換算する場合(元の1ピクセル分のデータがテクセル1個に対応)
void main() {
	vec2 uv_sz = (uFrameSz / uTextureSz) / UV_SZ_DIV;
	vec2 uv_tex = vec2(vTextureCoord.x / 2.0, vTextureCoord.y / 6.0 + uv_sz.y * 4.0);
	float y, u, v;
	y = texture(sTexture, vec2(vTextureCoord.x, vTextureCoord.y * Y_SCALE)).r;
	float u1 = texture(sTexture, vec2(uv_tex.x,           uv_tex.y)).r;
	float u2 = texture(sTexture, vec2(uv_tex.x + uv_sz.x, uv_tex.y)).r;
	u = (u1 + u2) / 2.0;
	float v1 = texture(sTexture, vec2(uv_tex.x,           uv_tex.y + uv_sz.y)).r;
	float v2 = texture(sTexture, vec2(uv_tex.x + uv_sz.x, uv_tex.y + uv_sz.y)).r;
	v = (v1 + v2) / 2.0;
	// 輝度がEPS未満なら黒を返す(プレビューを停止した時に画面が緑になるのを防ぐため)
	if (y > EPS) {
		vec3 yuv = vec3(clamp(y + uBrightness, 0.0, 1.0), u, v) - YUV_OFFSET;
		vec3 rgb = clamp((MAT_YUV2RGB * yuv), 0.0, 1.0);
		o_FragColor = vec4(rgb, 1.0);
	} else {
		o_FragColor = vec4(0.0, 0.0, 0.0, 1.0);
	}
}
)SHADER";
#endif

#endif //AANDUSB_YUV422p_FSH_H
