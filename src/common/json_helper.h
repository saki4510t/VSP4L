/*
 * aAndUsb
 * Copyright (c) 2014-2023 saki t_saki@serenegiant.com
 * Distributed under the terms of the GNU Lesser General Public License (LGPL v3.0) License.
 * License details are in the file license.txt, distributed as part of this software.
 */

#ifndef USBWEBCAMERAPROJ_JSON_HELPER_H
#define USBWEBCAMERAPROJ_JSON_HELPER_H

#include "rapidjson/rapidjson.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"

using namespace rapidjson;

//#define JSON_DEBUG

#ifdef JSON_DEBUG
// FIXME ここはターゲットOSに依存しないように書き換えないといけない
#include <android/log.h>
#include <unistd.h>
#include <libgen.h>
#define JSON_LOGD(FMT, ...) __android_log_print(ANDROID_LOG_DEBUG, "json_helper", "[%d*%s:%d:%s]:" FMT,	\
							gettid(), basename(__FILE__), __LINE__, __FUNCTION__, ## __VA_ARGS__)
#else
#define JSON_LOGD(FMT, ...)
#endif

/**
 * rapidjsonのヘルパー関数
 * 指定したuint8_t配列を16進文字列として書き込む
 * @param writer
 * @param key
 * @param ptr
 * @param size
 */
void write_nbytes(Writer<StringBuffer> &writer, const char *key, const uint8_t *ptr, int size);

/**
 * rapidjsonのヘルパー関数
 * guidを保持したuint8_t配列をguid文字列として書き込む
 * @param writer
 * @param key
 * @param guid 領域チェックしていないので16バイト以上確保しておくこと
 */
void write_guid(Writer<StringBuffer> &writer, const char *key, const uint8_t *guid);

/**
 * 指定したkeyにnullを書き込むためのrapidjsonのヘルパー関数
 * @param writer
 * @param key
 */
inline void write(Writer<StringBuffer> &writer, const char *key) {
	writer.String(key);
	writer.Null();
	JSON_LOGD("%s=%s", key, "null");
}

/**
 * 指定したkeyにbool値を書き込むためのrapidjsonのヘルパー関数
 * @param writer
 * @param key
 * @param value
 */
inline void write(Writer<StringBuffer> &writer, const char *key, const bool &value) {
	writer.String(key);
	writer.Bool(value);
	JSON_LOGD("%s=%s", key, value ? "true" : "false");
}

/**
 * 指定したkeyに文字を書き込むためのrapidjsonのヘルパー関数
 * @param writer
 * @param key
 * @param value
 */
inline void write(Writer<StringBuffer> &writer, const char *key, const char *value) {
	writer.String(key);
	writer.String(value);
	JSON_LOGD("%s=%s", key, value);
}

/**
 * 指定したkeyにint8_t値を書き込むためのrapidjsonのヘルパー関数
 * @param writer
 * @param key
 * @param value
 */
inline void write(Writer<StringBuffer> &writer, const char *key, const int8_t &value) {
	writer.String(key);
	writer.Int(value);
	JSON_LOGD("%s=%d", key, value);
}

/**
 * 指定したkeyにuint8_t値を書き込むためのrapidjsonのヘルパー関数
 * @param writer
 * @param key
 * @param value
 */
inline void write(Writer<StringBuffer> &writer, const char *key, const uint8_t &value) {
	writer.String(key);
	writer.Uint(value);
	JSON_LOGD("%s=%u", key, value);
}

/**
 * 指定したkeyにint16_t値を書き込むためのrapidjsonのヘルパー関数
 * @param writer
 * @param key
 * @param value
 */
inline void write(Writer<StringBuffer> &writer, const char *key, const int16_t &value) {
	writer.String(key);
	writer.Int(value);
	JSON_LOGD("%s=%d", key, value);
}

/**
 * 指定したkeyにuint16_t値を書き込むためのrapidjsonのヘルパー関数
 * @param writer
 * @param key
 * @param value
 */
inline void write(Writer<StringBuffer> &writer, const char *key, const uint16_t &value) {
	writer.String(key);
	writer.Uint(value);
	JSON_LOGD("%s=%u", key, value);
}

/**
 * 指定したkeyにint32_t値を書き込むためのrapidjsonのヘルパー関数
 * @param writer
 * @param key
 * @param value
 */
inline void write(Writer<StringBuffer> &writer, const char *key, const int32_t &value) {
	writer.String(key);
	writer.Int(value);
	JSON_LOGD("%s=%d", key, value);
}

/**
 * 指定したkeyにuint32_t値を書き込むためのrapidjsonのヘルパー関数
 * @param writer
 * @param key
 * @param value
 */
inline void write(Writer<StringBuffer> &writer, const char *key, const uint32_t &value) {
	writer.String(key);
	writer.Uint(value);
	JSON_LOGD("%s=%u", key, value);
}

/**
 * 指定したkeyにint64_t値を書き込むためのrapidjsonのヘルパー関数
 * @param writer
 * @param key
 * @param value
 */
inline void write(Writer<StringBuffer> &writer, const char *key, const int64_t &value) {
	writer.String(key);
	writer.Int64(value);
	JSON_LOGD("%s=%lld", key, value);
}

/**
 * 指定したkeyにuint64_t値を書き込むためのrapidjsonのヘルパー関数
 * @param writer
 * @param key
 * @param value
 */
inline void write(Writer<StringBuffer> &writer, const char *key, const uint64_t &value) {
	writer.String(key);
	writer.Uint64(value);
	JSON_LOGD("%s=%llu", key, value);
}

/**
 * 指定したkeyにfloat値を書き込むためのrapidjsonのヘルパー関数
 * @param writer
 * @param key
 * @param value
 */
inline void write(Writer<StringBuffer> &writer, const char *key, const float &value) {
	writer.String(key);
	writer.Double(value);
	JSON_LOGD("%s=%f", key, value);
}

/**
 * 指定したkeyにdouble値を書き込むためのrapidjsonのヘルパー関数
 * @param writer
 * @param key
 * @param value
 */
inline void write(Writer<StringBuffer> &writer, const char *key, const double &value) {
	writer.String(key);
	writer.Double(value);
	JSON_LOGD("%s=%f", key, value);
}

#endif //USBWEBCAMERAPROJ_JSON_HELPER_H
