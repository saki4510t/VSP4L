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

#include <stdio.h>
#include <string>
#include <cstring>

#include "utilbase.h"

#include "settings.h"

namespace serenegiant::app {

AppSettings::AppSettings()
:	modified(false)
{
	ENTER();
	EXIT();
}

AppSettings::~AppSettings() {
	ENTER();
	EXIT();
}

//--------------------------------------------------------------------------------
CameraSettings::CameraSettings()
:	modified(false)
{
	ENTER();
	EXIT();
}

CameraSettings::~CameraSettings() {
	ENTER();
	EXIT();
}

//--------------------------------------------------------------------------------
/**
 * @brief アプリ設定を保存
 *
 * @param settings
 * @return int
 */
int save(const AppSettings &settings) {
	ENTER();

	// FIXME 未実装

	RETURN(0, int);
}

/**
 * @brief アプリ設定を読み込み
 *
 * @param settings
 * @return int
 */
int load(AppSettings &settings) {
	ENTER();

	// FIXME 未実装

	RETURN(0, int);
}

/**
 * @brief カメラ設定を保存
 *
 * @param settings
 * @return int
 */
int save(const CameraSettings &settings) {
	ENTER();

	// FIXME 未実装

	RETURN(0, int);
}

/**
 * @brief カメラ設定を読み込み
 *
 * @param settings
 * @return int
 */
int load(CameraSettings &settings) {
	ENTER();

	// FIXME 未実装

	RETURN(0, int);
}

}   // namespace serenegiant::app
