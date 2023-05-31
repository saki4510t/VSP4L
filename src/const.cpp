#include "utilbase.h"
#include "const.h"

/**
 * @brief 拡大縮小倍率
 * 
 */
float ZOOM_FACTORS[] = {
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

const int NUM_ZOOM_FACTORS = NUM_ARRAY_ELEMENTS(ZOOM_FACTORS);

const int DEFAULT_ZOOM_IX = 8;
