/*
 * aAndUsb
 * Copyright (c) 2014-2023 saki t_saki@serenegiant.com
 * Distributed under the terms of the GNU Lesser General Public License (LGPL v3.0) License.
 * License details are in the file license.txt, distributed as part of this software.
 */

#ifndef AANDUSB_YUV420SP_NV21_EXT_YUV_TARGET_FSH_H
#define AANDUSB_YUV420SP_NV21_EXT_YUV_TARGET_FSH_H

// ハードウエアバッファのYUV420sp(NV21)のセミプレーンのデータを
// EGLClientBuffer, eglCreateImageKHR, glEGLImageTargetTexture2DOESと
// 組み合わせてテクスチャにしたものを描画するためのフラグメントシェーダー
#if __ANDROID__
constexpr const char *yuv420sp_nv21_ext_yuv_target_gl3_fsh =
R"SHADER(#version 300 es
#extension GL_EXT_YUV_target : require
#extension GL_OES_EGL_image_external_essl3 : require
precision highp float;
in vec2 vTextureCoord;
uniform __samplerExternal2DY2YEXT sTexture;
uniform vec2 uTextureSz;
uniform vec2 uFrameSz;
uniform float uBrightness;
layout(location = 0) out vec4 o_FragColor;

// ハードウエアバッファーを割り当てたYUV420sp(NV21)テクスチャを描画
void main() {
	vec3 srcYuv = texture(sTexture, vTextureCoord).xyz;
	o_FragColor = vec4(yuv_2_rgb(srcYuv, itu_601), 1.0);
}
)SHADER";

#else	// #if __ANDROID__
constexpr const char *yuv420sp_nv21_ext_yuv_target_gl3_fsh =
R"SHADER(#version 330
#extension GL_EXT_YUV_target : require
#extension GL_OES_EGL_image_external_essl3 : require
precision highp float;
in vec2 vTextureCoord;
uniform __samplerExternal2DY2YEXT sTexture;
uniform vec2 uTextureSz;
uniform vec2 uFrameSz;
uniform float uBrightness;
layout(location = 0) out vec4 o_FragColor;

// ハードウエアバッファーを割り当てたYUV420spテクスチャを描画
void main() {
	vec3 srcYuv = texture(sTexture, vTextureCoord).xyz;
	o_FragColor = vec4(yuv_2_rgb(srcYuv, itu_601), 1.0);
}
)SHADER";

#endif

#endif //AANDUSB_YUV420SP_NV21_EXT_YUV_TARGET_FSH_H
