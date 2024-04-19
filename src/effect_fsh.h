// Vision Support Program for Linux ... EyeApp
// Copyright (C) 2023-2024 saki t_saki@serenegiant.com
// 
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#ifndef EFFECT_FSH_H_
#define EFFECT_FSH_H_

//--------------------------------------------------------------------------------
// グレースケース変換
constexpr const char *rgba_gray_gl2_fsh =
R"SHADER(#version 100

precision highp float;
varying vec2 vTextureCoord;
uniform sampler2D sTexture;
uniform vec2 uTextureSz;
uniform vec2 uFrameSz;
const vec3 conv = vec3(0.3, 0.59, 0.11);

void main() {
    vec4 tc = texture2D(sTexture, vTextureCoord);
    float color = dot(tc.rgb, conv);
    vec3 cl3 = vec3(color, color, color);
    gl_FragColor = vec4(cl3, 1.0);
}
)SHADER";

constexpr const char *rgba_gray_gl3_fsh =
R"SHADER(#version 330

precision highp float;
in vec2 vTextureCoord;
uniform sampler2D sTexture;
uniform vec2 uTextureSz;
uniform vec2 uFrameSz;
const vec3 conv = vec3(0.3, 0.59, 0.11);
layout(location = 0) out vec4 o_FragColor;

void main() {
    vec4 tc = texture(sTexture, vTextureCoord);
    float color = dot(tc.rgb, conv);
    vec3 cl3 = vec3(color, color, color);
    o_FragColor = vec4(cl3, 1.0);
}
)SHADER";

//--------------------------------------------------------------------------------
// グレースケール反転変換
constexpr const char *rgba_gray_reverse_gl2_fsh =
R"SHADER(#version 100

precision highp float;
varying vec2 vTextureCoord;
uniform sampler2D sTexture;
uniform vec2 uTextureSz;
uniform vec2 uFrameSz;
const vec3 conv = vec3(0.3, 0.59, 0.11);

void main() {
    vec4 tc = texture2D(sTexture, vTextureCoord);
    float color = dot(tc.rgb, conv);
    vec3 cl3 = vec3(color, color, color);
    gl_FragColor = vec4(clamp(vec3(1.0, 1.0, 1.0) - cl3, 0.0, 1.0), 1.0);
}
)SHADER";

constexpr const char *rgba_gray_reverse_gl3_fsh =
R"SHADER(#version 330

precision highp float;
in vec2 vTextureCoord;
uniform sampler2D sTexture;
uniform vec2 uTextureSz;
uniform vec2 uFrameSz;
const vec3 conv = vec3(0.3, 0.59, 0.11);
layout(location = 0) out vec4 o_FragColor;

void main() {
    vec4 tc = texture(sTexture, vTextureCoord);
    float color = dot(tc.rgb, conv);
    vec3 cl3 = vec3(color, color, color);
    o_FragColor = vec4(clamp(vec3(1.0, 1.0, 1.0) - cl3, 0.0, 1.0), 1.0);
}
)SHADER";

//--------------------------------------------------------------------------------
// 白黒二値変換
constexpr const char *rgba_bin_gl2_fsh =
R"SHADER(#version 100

precision highp float;
varying vec2 vTextureCoord;
uniform sampler2D sTexture;
uniform vec2 uTextureSz;
uniform vec2 uFrameSz;
const vec3 conv = vec3(0.3, 0.59, 0.11);
const vec3 cl = vec3(1.0, 1.0, 1.0);

void main() {
    vec4 tc = texture2D(sTexture, vTextureCoord);
    float color = dot(tc.rgb, conv);
    vec3 bin = step(0.3, vec3(color, color, color));
    gl_FragColor = vec4(cl * bin, 1.0);
}
)SHADER";

constexpr const char *rgba_bin_gl3_fsh =
R"SHADER(#version 330

precision highp float;
in vec2 vTextureCoord;
uniform sampler2D sTexture;
uniform vec2 uTextureSz;
uniform vec2 uFrameSz;
const vec3 conv = vec3(0.3, 0.59, 0.11);
const vec3 cl = vec3(1.0, 1.0, 1.0);
layout(location = 0) out vec4 o_FragColor;

void main() {
    vec4 tc = texture(sTexture, vTextureCoord);
    float color = dot(tc.rgb, conv);
    vec3 bin = step(0.3, vec3(color, color, color));
    o_FragColor = vec4(cl * bin, 1.0);
}
)SHADER";

//--------------------------------------------------------------------------------
// 白黒二値反転変換
constexpr const char *rgba_bin_reverse_gl2_fsh =
R"SHADER(#version 100

precision highp float;
varying vec2 vTextureCoord;
uniform sampler2D sTexture;
uniform vec2 uTextureSz;
uniform vec2 uFrameSz;
const vec3 conv = vec3(0.3, 0.59, 0.11);
const vec3 cl = vec3(1.0, 1.0, 1.0);

void main() {
    vec4 tc = texture2D(sTexture, vTextureCoord);
    float color = dot(tc.rgb, conv);
    vec3 bin = step(0.3, vec3(color, color, color));
    gl_FragColor = vec4(cl * (vec3(1.0, 1.0, 1.0) - bin), 1.0);
}
)SHADER";

constexpr const char *rgba_bin_reverse_gl3_fsh =
R"SHADER(#version 330

precision highp float;
in vec2 vTextureCoord;
uniform sampler2D sTexture;
uniform vec2 uTextureSz;
uniform vec2 uFrameSz;
const vec3 conv = vec3(0.3, 0.59, 0.11);
const vec3 cl = vec3(1.0, 1.0, 1.0);
layout(location = 0) out vec4 o_FragColor;

void main() {
    vec4 tc = texture(sTexture, vTextureCoord);
    float color = dot(tc.rgb, conv);
    vec3 bin = step(0.3, vec3(color, color, color));
    o_FragColor = vec4(cl * (vec3(1.0, 1.0, 1.0) - bin), 1.0);
}
)SHADER";

#endif  // EFFECT_FSH_H_
