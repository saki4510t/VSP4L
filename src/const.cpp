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

#include "utilbase.h"
#include "const.h"

namespace serenegiant::app {

/**
 * @brief モデル文字列
 * 
 */
const char *MODEL = "VSP4L";
/**
 * @brief バージョン文字列
 * 
 */
const char *VERSION = "v1.0.0";

/**
 * @brief レンズ補正係数
 * 
 */
const float LENSE_FACTOR = 1.0f;

/**
 * @brief 拡大縮小倍率
 * 
 */
const float ZOOM_FACTORS[] = {
	0.2f,	// 0
	0.3f,	// 1
	0.4f,	// 2
	0.5f,	// 3
	0.6f,	// 4
	0.7f,	// 5
	0.8f,	// 6
	0.9f,	// 7
	1.0f,	// 8	デフォルト
	1.1f,	// 9
	1.2f,	// 10
	1.4f,	// 11
	1.6f,	// 12
	1.8f,	// 13
	2.0f,	// 14
	2.5f,	// 15
	3.0f,	// 16
	3.5f,	// 17
	4.0f,	// 18
};

/**
 * @brief 拡大縮小率配列の要素数
 * 
 */
const int NUM_ZOOM_FACTORS = NUM_ARRAY_ELEMENTS(ZOOM_FACTORS);

/**
 * @brief デフォルトの拡大縮小率のインデックス
 * 
 */
const int DEFAULT_ZOOM_IX = 8;

/**
 * @brief シリアル番号を取得
 * 
 * @return std::string 
 */
std::string get_serial() {
	// FIXME 未実装
	return "0123456789";
}

/**
 * @brief オプションを初期化
 * 
 * @return std::unordered_map<std::string, std::string> 
 */
std::unordered_map<std::string, std::string> init_options() {
	std::unordered_map<std::string, std::string> options;
//	options[OPT_DEBUG_EXIT_ESC] = "";
//	options[OPT_DEBUG_SHOW_FPS] = "";
	options[OPT_DEVICE] = OPT_DEVICE_DEFAULT;
	options[OPT_UDMABUF] = OPT_UDMABUF_DEFAULT;
	options[OPT_BUF_NUMS] = OPT_BUF_NUMS_DEFAULT;
	options[OPT_WIDTH] = OPT_WIDTH_DEFAULT;
	options[OPT_HEIGHT] = OPT_HEIGHT_DEFAULT;

	return options;
}

}	// namespace serenegiant::app
