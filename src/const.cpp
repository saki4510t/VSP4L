#include "utilbase.h"
#include "const.h"

/**
 * @brief 拡大縮小倍率
 * 
 */
float ZOOM_FACTOR[] = {
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
	1.3f,	// 11
	1.4f,	// 12
	1.5f,	// 13
	1.6f,	// 14
	1.7f,	// 15
	1.8f,	// 16
	1.9f,	// 17
	2.0f,	// 18
	2.5f,	// 19
	3.0f,	// 20
};

const int NUM_ZOOM_FACTOR = NUM_ARRAY_ELEMENTS(ZOOM_FACTOR);

const int DEFAULT_ZOOM_IX = 8;