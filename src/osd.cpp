#if 0    // set 0 if you need debug log, otherwise set 1
	#ifndef LOG_NDEBUG
	#define LOG_NDEBUG
	#endif
	#undef USE_LOGALL
#else
//	#define USE_LOGALL
	#define USE_LOGD
	#undef LOG_NDEBUG
	#undef NDEBUG
#endif

#include "utilbase.h"

#include "window.h"
#include "osd.h"

namespace serenegiant::app {

/**
 * @brief コンストラクタ
 *
 */
/*public*/
OSD::OSD()
:	page(0)
{
	ENTER();

	// FIXME 未実装

	EXIT();
}

/**
 * @brief デストラクタ
 *
 */
/*public*/
OSD::~OSD() {
	ENTER();

	// FIXME 未実装

	EXIT();
}

/**
 * @brief OSD表示中のキー入力処理
 *
 * @param event
 */
void OSD::on_key(const KeyEvent &event) {
	ENTER();

	LOGD("key=%d,scancode=%d/%s,action=%d,mods=%d",
		event.key, event.scancode, glfwGetKeyName(event.key, event.scancode),
		event.action, event.mods);
	// FIXME 未実装

	EXIT();
}

/**
 * @brief OSD表示の描画処理
 *
 */
/*public*/
void OSD::draw() {
	ENTER();

	// FIXME 未実装

	EXIT();
}

}   // namespace serenegiant::app
