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

// コマンドラインオプション
#define OPT_DEBUG "debug"
#define OPT_DEVICE "device"
#define OPT_FONT "font"
#define OPT_UDMABUF "udmabuf"
#define OPT_BUF_NUMS "buf_nums"
// コマンドラインオプションのデフォルト値
#define OPT_DEVICE_DEFAULT "/dev/video0"
#define OPT_FONT_DEFAULT "./src/imgui/misc/fonts/DroidSans.ttf"
#define OPT_UDMABUF_DEFAULT "/dev/udmabuf0"
#define OPT_BUF_NUMS_DEFAULT "4"

const struct option LONG_OPTS[] = {
	{ OPT_DEBUG,	no_argument,		nullptr,	'D' },
	{ OPT_DEVICE,	required_argument,	nullptr,	'd' },
	{ OPT_FONT,		required_argument,	nullptr,	'f' },
	{ OPT_UDMABUF,	required_argument,	nullptr,	'u' },
	{ OPT_BUF_NUMS,	required_argument,	nullptr,	'n' },
	{ 0,			0,					0,			0  },
};

/**
 * オプションを初期化
 * 
 * @return std::unordered_map<std::string, std::string> 
 */
std::unordered_map<std::string, std::string> init_options();

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

/**
 * @brief 測光モード
 * 
 */
typedef enum {
	EXP_MODE_AVERAGE = 0,	// 平均測光
	EXP_MODE_CENTER,		// 中央測光
} exp_mode_t;

/**
 * @brief レンズ補正係数
 * 
 */
extern const float LENSE_FACTOR;

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

/**
 * @brief シリアル番号を取得
 * 
 * @return std::string 
 */
std::string get_serial();

#endif
