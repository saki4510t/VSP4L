#ifndef CONST_H_
#define CONST_H_

#include <string>

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
