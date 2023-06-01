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
{
    ENTER();
    EXIT();
}

CameraSettings::~CameraSettings() {
    ENTER();
    EXIT();
}

}   // namespace serenegiant::app
