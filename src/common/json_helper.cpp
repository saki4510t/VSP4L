/*
 * aAndUsb
 * Copyright (c) 2014-2023 saki t_saki@serenegiant.com
 * Distributed under the terms of the GNU Lesser General Public License (LGPL v3.0) License.
 * License details are in the file license.txt, distributed as part of this software.
 */
 
#if 1	// set 1 if you don't need debug message
	#ifndef LOG_NDEBUG
		#define	LOG_NDEBUG		// ignore LOGV/LOGD/MARK
	#endif
	#undef USE_LOGALL
#else
//	#define USE_LOGALL
	#define USE_LOGD
	#undef LOG_NDEBUG
	#undef NDEBUG		// depends on definition in Android.mk and Application.mk
#endif

#include <cstdio>

#include "json_helper.h"

/**
 * rapidjsonのヘルパー関数
 * 指定したuint8_t配列を16進文字列として書き込む
 * @param writer
 * @param key
 * @param ptr
 * @param size
 */
void write_nbytes(Writer<StringBuffer> &writer, const char *key, const uint8_t *ptr, int size) {
	char buf[64 + 3];
	char *p = &buf[0];
	for (int i = 0; i < size; i++) {
		snprintf(p, 64 - i * 2, "%02x", ptr[i]);
		p += 2;
	}
	*p = '\0';
	writer.String(key);
	writer.String(size ? buf : "---");
}

/**
 * rapidjsonのヘルパー関数
 * guidを保持したuint8_t配列をguid文字列として書き込む
 * @param writer
 * @param key
 * @param guid 領域チェックしていないので16バイト以上確保しておくこと
 */
void write_guid(Writer<StringBuffer> &writer, const char *key, const uint8_t *guid) {
	char buf[64];
	snprintf(buf, sizeof(buf), "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
		guid[0], guid[1], guid[2], guid[3], guid[4], guid[5], guid[6], guid[7],
		guid[8], guid[9], guid[10], guid[11], guid[12], guid[13], guid[14], guid[15]);
	buf[sizeof(buf)-1] = '\0';
	writer.String(key);
	writer.String(buf);
}
