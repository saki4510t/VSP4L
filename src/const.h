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

#ifndef CONST_H_
#define CONST_H_

#include <unistd.h>
#include <inttypes.h>
#include <cinttypes>
#include <stdint.h>
#include <string>
#include <memory>
#include <unordered_map>

#include <getopt.h>
#include <linux/v4l2-controls.h>

namespace serenegiant::app {

// Open GL3を使うかどうか, 1: GL3を使う, 0: 使わない(GL2を使う)
#define USE_GL3 (0)

//--------------------------------------------------------------------------------
// コマンドラインオプション
// ESCキーでアプリを終了させるかどうか
#define OPT_DEBUG_EXIT_ESC "debug_exit_esc"
// FPS表示をするかどうか
#define OPT_DEBUG_SHOW_FPS "debug_show_fps"
// 接続するV4L2機器のデバイスファイルを指定, デフォルトはOPT_DEVICE_DEFAULT="/dev/video0"
#define OPT_DEVICE "device"
// V4L2機器から映像データを受け取る際に使うUDMABUFのデバイスファイル名, デフォルトはOPT_UDMABUF_DEFAULT="/dev/udmabuf0"
#define OPT_UDMABUF "udmabuf"
// V4L2機器から映像データを受け取る際に使うバッファの数、デフォルトはOPT_BUF_NUMS_DEFAULT="4"
#define OPT_BUF_NUMS "buf_nums"
// V4L2機器から受け取る映像データの幅, デフォルトはOPT_WIDTH_DEFAULT="1920"
#define OPT_WIDTH "width"
// V4L2機器から受け取る映像データの高さ, デフォルトはOPT_HEIGHT_DEFAULT="1080"
#define OPT_HEIGHT "height"

// コマンドラインオプションのデフォルト値
#define OPT_DEVICE_DEFAULT "/dev/video0"
#define OPT_UDMABUF_DEFAULT "/dev/udmabuf0"
#define OPT_BUF_NUMS_DEFAULT "4"
#define OPT_WIDTH_DEFAULT "1920"
#define OPT_HEIGHT_DEFAULT "1080"

// 短い形式のコマンドラインオプション(-オプション、うまく動かない)
#define SHORT_OPTS "efd:u:n:w:h"
// 長い形式のコマンドラインオプション定義(--オプション)
const struct option LONG_OPTS[] = {
	{ OPT_DEBUG_EXIT_ESC,	no_argument,		nullptr,	'e' },
	{ OPT_DEBUG_SHOW_FPS,	no_argument,		nullptr,	'f' },
	{ OPT_DEVICE,			required_argument,	nullptr,	'd' },
	{ OPT_UDMABUF,			required_argument,	nullptr,	'u' },
	{ OPT_BUF_NUMS,			required_argument,	nullptr,	'n' },
	{ OPT_WIDTH,			required_argument,	nullptr,	'w' },
	{ OPT_HEIGHT,			required_argument,	nullptr,	'h' },
	{ 0,					0,					0,			0  },
};

/**
 * オプションを初期化
 * 
 * @return std::unordered_map<std::string, std::string> 
 */
std::unordered_map<std::string, std::string> init_options();

//--------------------------------------------------------------------------------
/**
 * @brief モデル文字列
 * 
 */
extern const char *MODEL;
/**
 * @brief バージョン文字列
 * 
 */
extern const char *VERSION;

//--------------------------------------------------------------------------------
/**
 * @brief キー操作モード定数
 * 
 */
typedef enum {
	KEY_MODE_NORMAL = 0,	// 通常モード
	KEY_MODE_BRIGHTNESS,	// 輝度調整モード
	KEY_MODE_ZOOM,			// 拡大縮小モード
	KEY_MODE_OSD,			// OSD操作モード
} key_mode_t;

//--------------------------------------------------------------------------------
/**
 * @brief 映像効果定数
 * 
 */
typedef enum {
	EFFECT_NON = 0,			// 映像効果なし
	EFFECT_GRAY,			// グレースケール
	EFFECT_GRAY_REVERSE,	// グレースケール反転
	EFFECT_BIN,				// 白黒二値
	EFFECT_BIN_REVERSE,		// 白黒二値反転
	EFFECT_NUM,				// 映像効果の数
} effect_t;

//--------------------------------------------------------------------------------
/**
 * @brief 測光モード
 * 
 */
typedef enum {
	EXP_MODE_AVERAGE = 0,	// 平均測光
	EXP_MODE_CENTER,		// 中央測光
} exp_mode_t;

//--------------------------------------------------------------------------------
/**
 * @brief レンズ補正係数
 * 
 */
extern const float LENSE_FACTOR;

//--------------------------------------------------------------------------------
/**
 * @brief 拡大縮小倍率配列
 * 
 */
extern const float ZOOM_FACTORS[];
/**
 * @brief 拡大縮小倍率の要素数
 * 
 */
extern const int NUM_ZOOM_FACTORS;
/**
 * @brief デフォルトの拡大縮小率のインデックス
 * 
 */
extern const int DEFAULT_ZOOM_IX;

//--------------------------------------------------------------------------------
/**
 * @brief シリアル番号を取得
 * 
 * @return std::string 
 */
std::string get_serial();

//--------------------------------------------------------------------------------
// ノイズ低減コントロール(カーネルドライバー側で決まっている値)
#define V4L2_CID_DENOISE	(0x00980922)
// フレームレートコントロール(カーネルドライバー側で決まっている値)
#define V4L2_CID_FRAMERATE	(0x00980923)
// 設定を初期値に戻す
#define V4L2_CID_RESTORE_SETTINGS (0x00980924)

/**
 * OSD画面で対応可能なカメラコントロール
 * XXX apply_settingsで前から順にカメラ側へ適用するので
 *     自動設定(自動露出等)のidは対応する手動設定よりも前に置くこと
 *     フレームレートは30fps固定にするためにここには含めないこと
*/
const uint32_t SUPPORTED_CTRLS[] {
	// 調整１
	V4L2_CID_BRIGHTNESS,
	V4L2_CID_HUE,
	V4L2_CID_SATURATION,
	V4L2_CID_CONTRAST,
	V4L2_CID_SHARPNESS,
	V4L2_CID_GAMMA,
	// 調整2
	V4L2_CID_DENOISE,
	V4L2_CID_AUTOGAIN,
	V4L2_CID_GAIN,
	V4L2_CID_EXPOSURE_AUTO,
	V4L2_CID_EXPOSURE_ABSOLUTE,
	V4L2_CID_AUTO_N_PRESET_WHITE_BALANCE,
	// 調整3
	V4L2_CID_POWER_LINE_FREQUENCY,
	V4L2_CID_ZOOM_ABSOLUTE,
	0,
	0,
	0,
	0,

//	V4L2_CID_BG_COLOR,	// V4L2_CID_BG_COLORのIDで返ってくるけどこれはフレームレート(V4L2_CID_FRAMERATE)
//	V4L2_CID_MIN_BUFFER_FOR_CAPTURE
//	Skipping Frames Enable/Disable(0x009819e0)
};

/**
 * 自動調整の設定をするカメラコントロール
*/
const uint32_t AUTO_CTRLS[] {
	V4L2_CID_AUTOBRIGHTNESS,
	V4L2_CID_HUE_AUTO,
	V4L2_CID_AUTOGAIN,
	V4L2_CID_EXPOSURE_AUTO,
	V4L2_CID_AUTO_WHITE_BALANCE,
};

}	// namespace serenegiant::app

#endif	// #ifndef CONST_H_
