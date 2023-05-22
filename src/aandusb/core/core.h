/*
 * aAndUsb
 * Copyright (c) 2014-2023 saki t_saki@serenegiant.com
 * Distributed under the terms of the GNU Lesser General Public License (LGPL v3.0) License.
 * License details are in the file license.txt, distributed as part of this software.
 */

#ifndef AANDUSB_CORE_H
#define AANDUSB_CORE_H

#include <unistd.h>
#include <inttypes.h>
#include <cinttypes>
#include <stdint.h>

#include "endian_unaligned.h"
#include "mutex.h"
#include "condition.h"
#include "semaphore.h"
#include "times.h"

#if defined(__ANDROID__)
	#include "core/core_android.h"
#elif defined(__APPLE__)
	#include "core/core_osx.h"
#else
	#include "core/core_linux.h"
#endif

using namespace android;
namespace sere = serenegiant;

namespace serenegiant::core {

typedef enum usb_error {
	// 成功(エラー無し)
	USB_SUCCESS = 0,
	// 入出力エラー
	USB_ERROR_IO = -1,
	// パラメータが不正
	USB_ERROR_INVALID_PARAM = -2,
	// アクセス不能(パーミッションが足りない)
	USB_ERROR_ACCESS = -3,
	// 存在しない
	USB_ERROR_NO_DEVICE = -4,
	// 見つからない
	USB_ERROR_NOT_FOUND = -5,
	// 使用中
	USB_ERROR_BUSY = -6,
	// タイムアウト
	USB_ERROR_TIMEOUT = -7,
	// オーバーフロー
	USB_ERROR_OVERFLOW = -8,
	// パイプエラー
	USB_ERROR_PIPE = -9,
	// 割り込み
	USB_ERROR_INTERRUPTED = -10,
	// メモリ不足
	USB_ERROR_NO_MEM = -11,
	// サポートされていない
	USB_ERROR_NOT_SUPPORTED = -12,
	// 既にclimeされている
	USB_ERROR_ALREADY_CLAIMED = -13,
	// モード設定が不正
	USB_ERROR_INVALID_MODE = -14,
	// 想定した状態と異なる
	USB_ERROR_INVALID_STATE = -15,
	//
	USB_ERROR_NO_SPACE = -28,
	// 想定される状態と異なる
	USB_ERROR_ILLEGAL_STATE = -29,
	// パラメータが不正
	USB_ERROR_ILLEGAL_ARGS = -30,
	// その他のエラー
	USB_ERROR_OTHER = -99,

	// 不正なJavaオブジェクト
	ERROR_INVALID_JAVA_OBJECT = -150,
	// コールバックメソッドが見つからない
	ERROR_CALLBACK_NOT_FOUND = -160,
	// コールバックのJava側で例外生成
	ERROR_CALLBACK_EXCEPTION = -161,
} usb_error_t;

// -100以下
typedef enum video_error {
	VIDEO_ERROR_DUPLICATE_ENDPOINT = -100,
	// フレームデータがおかしい
	VIDEO_ERROR_FRAME = -110,
	// フレームデータがおかしい, セグメント長がフレームサイズの終わりより長い
	VIDEO_ERROR_FRAME_WRONG_SEG_LENGTH = -111,
	// フレームデータがおかしい, データ長がフレームサイズの終わりより長い
	VIDEO_ERROR_FRAME_TRUNCATED = -112,
	// EOIが来る前にSOIが来た、SOIが複数含まれている
	VIDEO_ERROR_FRAME_DUPLICATED_SOI = -113,
	// JFIFマーカーが見つからなかった
	VIDEO_ERROR_FRAME_NO_MARKER = -114,
	// SOIで始まっていない
	VIDEO_ERROR_FRAME_NO_SOI = -115,
	// EOIが見つからなかった
	VIDEO_ERROR_FRAME_NO_EOI = -116,
	// フレームデータが存在しない
	VIDEO_ERROR_NO_DATA = -170,
} video_error_t;

}	// namespace serenegiant::core

#endif //AANDUSB_CORE_H
