/*
 * aAndUsb
 * Copyright (c) 2014-2023 saki t_saki@serenegiant.com
 * Distributed under the terms of the GNU Lesser General Public License (LGPL v3.0) License.
 * License details are in the file license.txt, distributed as part of this software.
 */

#if 1	// デバッグ情報を出さない時は1
	#ifndef LOG_NDEBUG
		#define	LOG_NDEBUG		// LOGV/LOGD/MARKを出力しない時
	#endif
	#undef USE_LOGALL			// 指定したLOGxだけを出力
#else
//	#define USE_LOGALL
//	#define USE_LOGD
	#undef LOG_NDEBUG
	#undef NDEBUG
#endif

#include <cstdio>
#include <string>
#include <cstring>	// memcpy
#include <csetjmp>

#include <jpeglib.h>

#include "utilbase.h"
// core
#include "core/video_frame_interface.h"
#include "core/video_frame_base.h"
#include "core/video_frame_utils.h"
// usb
#include "usb/aandusb.h"
// uvc
#include "uvc/aanduvc.h"

namespace serenegiant::core {

/**libjpeg/libjpeg-turboのマーカーコールバックを使うかどうか(でもうまく動かない)*/
#define USE_MARKER_CALLBACK 0

/**
 * FOURCCとraw_frame_tのペアを保持するためのホルダークラス
 */
struct raw_frame_fource {
	const  uint8_t guid[4];
	raw_frame_t frame_type;
};

/*
 * ここのFOURCCの値はlibyuvのvideo_common.hをベースにしている
 *
 *  Copyright 2011 The LibYuv Project Authors. All rights reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS. All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

// フレームフォーマット識別GUID
static struct raw_frame_fource RAW_FRAME_FOURCC[] = {
	// 9 Primary YUV formats: 5 planar, 2 biplanar, 2 packed.
	{{/*I420*/ 'I', '4', '2', '0'}, RAW_FRAME_UNCOMPRESSED_I420},
	{{/*I422*/ 'I', '4', '2', '2'}, RAW_FRAME_UNCOMPRESSED_422p},
	{{/*I444*/ 'I', '4', '4', '4'}, RAW_FRAME_UNCOMPRESSED_444p},
	{{/*I400*/ 'I', '4', '0', '0'}, RAW_FRAME_UNCOMPRESSED_GRAY8},
	{{/*NV21*/ 'N', 'V', '2', '1'}, RAW_FRAME_UNCOMPRESSED_NV21},
	{{/*NV12*/ 'N', 'V', '1', '2'}, RAW_FRAME_UNCOMPRESSED_NV12},
	{{/*YUY2*/ 'Y', 'U', 'Y', '2'}, RAW_FRAME_UNCOMPRESSED_YUYV},
	{{/*UYVY*/ 'U', 'Y', 'V', 'Y'}, RAW_FRAME_UNCOMPRESSED_UYVY},
//	{{/*H010*/ 'H', '0', '1', '0'}, RAW_FRAME_UNCOMPRESSED_H010},  // unofficial fourcc. 10 bit lsb

// 1 Secondary YUV format: row biplanar.
	/*@deprecated*/
	{{/*M420*/ 'M', '4', '2', '0'}, RAW_FRAME_UNCOMPRESSED_M420},

// 11 Primary RGB formats: 4 32 bpp, 2 24 bpp, 3 16 bpp, 1 10 bpc
	{{/*ARGB*/ 'A', 'R', 'G', 'B'}, RAW_FRAME_UNCOMPRESSED_XRGB},
//	{{/*BGRA*/ 'B', 'G', 'R', 'A'}, RAW_FRAME_UNCOMPRESSED_BGRA},
//	{{/*ABGR*/ 'A', 'B', 'G', 'R'}, RAW_FRAME_UNCOMPRESSED_ABGR},
//	{{/*AR30*/ 'A', 'R', '3', '0'}, RAW_FRAME_UNCOMPRESSED_AR30},  // 10 bit per channel. 2101010.
//	{{/*AB30*/ 'A', 'B', '3', '0'}, RAW_FRAME_UNCOMPRESSED_AB30},  // ABGR version of 10 bit
	{{/*24BG*/ '2', '4', 'B', 'G'}, RAW_FRAME_UNCOMPRESSED_BGR},
//	{{/*RAW*/  'r', 'a', 'w', ' '}, RAW_FRAME_UNCOMPRESSED_RAW},
	{{/*RGBA*/ 'R', 'G', 'B', 'A'}, RAW_FRAME_UNCOMPRESSED_RGBX},
	{{/*RGBP*/ 'R', 'G', 'B', 'P'}, RAW_FRAME_UNCOMPRESSED_RGBP},  // rgb565 LE.
//	{{/*RGBO*/ 'R', 'G', 'B', 'O'}, RAW_FRAME_UNCOMPRESSED_ARGB1555},  // argb1555 LE.
//	{{/*R444*/ 'R', '4', '4', '4'}, RAW_FRAME_UNCOMPRESSED_ARGB444},  // argb4444 LE.

// 1 Primary Compressed YUV format.
	{{/*MJPG*/ 'M', 'J', 'P', 'G'}, RAW_FRAME_MJPEG},

// 8 Auxiliary YUV variations: 3 with U and V planes are swapped, 1 Alias.
	{{/*YV12*/ 'Y', 'V', '1', '2'}, RAW_FRAME_UNCOMPRESSED_YV12},
	{{/*YV16*/ 'Y', 'V', '1', '6'}, RAW_FRAME_UNCOMPRESSED_422p},
//	{{/*YV24*/ 'Y', 'V', '2', '4'}, RAW_FRAME_UNCOMPRESSED_YU24},
	{{/*YU12*/ 'Y', 'U', '1', '2'}, RAW_FRAME_UNCOMPRESSED_I420},  // Linux version of I420.
	{{/*J420*/ 'J', '4', '2', '0'}, RAW_FRAME_UNCOMPRESSED_I420},
	{{/*J400*/ 'J', '4', '0', '0'}, RAW_FRAME_UNCOMPRESSED_GRAY8},  // unofficial fourcc
//	{{/*H420*/ 'H', '4', '2', '0'}, RAW_FRAME_UNCOMPRESSED_H420},  // unofficial fourcc
//	{{/*H422*/ 'H', '4', '2', '2'}, RAW_FRAME_UNCOMPRESSED_H422},  // unofficial fourcc

// 14 Auxiliary aliases.  CanonicalFourCC() maps these to canonical fourcc.
	{{/*IYUV*/ 'I', 'Y', 'U', 'V'}, RAW_FRAME_UNCOMPRESSED_I420},  // Alias for I420.
	{{/*YU16*/ 'Y', 'U', '1', '6'}, RAW_FRAME_UNCOMPRESSED_422p},  // Alias for I422.
	{{/*YU24*/ 'Y', 'U', '2', '4'}, RAW_FRAME_UNCOMPRESSED_444p},  // Alias for I444.
	{{/*YUYV*/ 'Y', 'U', 'Y', 'V'}, RAW_FRAME_UNCOMPRESSED_YUYV},  // Alias for YUY2.
	{{/*YUVS*/ 'y', 'u', 'v', 's'}, RAW_FRAME_UNCOMPRESSED_YUYV},  // Alias for YUY2 on Mac.
	{{/*HDYC*/ 'H', 'D', 'Y', 'C'}, RAW_FRAME_UNCOMPRESSED_UYVY},  // Alias for UYVY.
	{{/*2VUY*/ '2', 'v', 'u', 'y'}, RAW_FRAME_UNCOMPRESSED_UYVY},  // Alias for UYVY on Mac.
	{{/*JPEG*/ 'J', 'P', 'E', 'G'}, RAW_FRAME_MJPEG},  // Alias for MJPG.
	{{/*DMB1*/ 'd', 'm', 'b', '1'}, RAW_FRAME_MJPEG},  // Alias for MJPG on Mac.
//	{{/*BA81*/ 'B', 'A', '8', '1'}, RAW_FRAME_UNCOMPRESSED_BGGR},  // Alias for BGGR.
	{{/*RGB3*/ 'R', 'G', 'B', '3'}, RAW_FRAME_UNCOMPRESSED_RGB},  // Alias for RAW.
	{{/*BGR3*/ 'B', 'G', 'R', '3'}, RAW_FRAME_UNCOMPRESSED_BGR},  // Alias for 24BG.
	{{/*CM32*/ 0, 0, 0, 32}, RAW_FRAME_UNCOMPRESSED_XRGB},  // Alias for BGRA kCMPixelFormat_32ARGB
	{{/*CM24*/ 0, 0, 0, 24}, RAW_FRAME_UNCOMPRESSED_RGB},  // Alias for RAW kCMPixelFormat_24RGB
//	{{/*L555*/ 'L', '5', '5', '5'}, RAW_FRAME_UNCOMPRESSED_RGB0},  // Alias for RGBO.
	{{/*L565*/ 'L', '5', '6', '5'}, RAW_FRAME_UNCOMPRESSED_RGBP},  // Alias for RGBP.
//	{{/*5551*/ '5', '5', '5', '1'}, RAW_FRAME_UNCOMPRESSED_RGB0},  // Alias for RGBO.

// deprecated formats.  Not supported, but defined for backward compatibility.
	{{/*I411*/ 'I', '4', '1', '1'}, RAW_FRAME_UNCOMPRESSED_411p},
//	{{/*Q420*/ 'Q', '4', '2', '0'}, RAW_FRAME_UNCOMPRESSED_Q420},
//	{{/*RGGB*/ 'R', 'G', 'G', 'B'}, RAW_FRAME_UNCOMPRESSED_RGGB},
//	{{/*BGGR*/ 'B', 'G', 'G', 'R'}, RAW_FRAME_UNCOMPRESSED_BGGR},
//	{{/*GRBG*/ 'G', 'R', 'B', 'G'}, RAW_FRAME_UNCOMPRESSED_GRBG},
//	{{/*GBRG*/ 'G', 'B', 'R', 'G'}, RAW_FRAME_UNCOMPRESSED_GBRG},

	{{/*H264*/ 'H', '2', '6', '4'}, RAW_FRAME_H264},
	{{/*H265*/ 'H', '2', '6', '5'}, RAW_FRAME_H265},
	{{/*VP8*/ 'V', 'P', '8', ' '}, RAW_FRAME_VP8},
	{{/*VP8*/ 'V', 'P', '8', 0}, RAW_FRAME_VP8},
};

/**
 * FOURCCからraw_frame_tを取得する。見つからなければRAW_FRAME_UNKNOWN
 * @param fourcc
 * @return
 */
raw_frame_t get_raw_frame_type_from_fourcc(const uint8_t *fourcc) {
	ENTER();

	raw_frame_t result = RAW_FRAME_UNKNOWN;
	for (auto & i : RAW_FRAME_FOURCC) {
		if (!memcmp(i.guid, fourcc, 4)) {
			result = i.frame_type;
			break;
		}
	}

	RETURN(result, raw_frame_t);
}

/**
 * GUIDとraw_frame_tのペアを保持するためのホルダークラス
 */
struct raw_frame_guid {
	const  uint8_t guid[16];
	raw_frame_t frame_type;
};

// フレームフォーマット識別GUID
static struct raw_frame_guid RAW_FRAME_GUID[] = {
	{{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71}, RAW_FRAME_UNKNOWN},
	{{ 'H',  '2',  '6',  '4', 0x00, 0x00, 0x10, 0x00, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71}, RAW_FRAME_FRAME_H264},
	{{ 'M',  'J',  'P',  'G', 0x00, 0x00, 0x10, 0x00, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71}, RAW_FRAME_MJPEG},
	{{ 'Y',  'U',  'Y',  '2', 0x00, 0x00, 0x10, 0x00, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71}, RAW_FRAME_UNCOMPRESSED_YUYV},
	{{ 'U',  'Y',  'V',  'Y', 0x00, 0x00, 0x10, 0x00, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71}, RAW_FRAME_UNCOMPRESSED_UYVY},
	{{ 'Y',  '8',  '0',  '0', 0x00, 0x00, 0x10, 0x00, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71}, RAW_FRAME_UNCOMPRESSED_GRAY8},	// Y800
	{{ 'B',  'Y',  '8',  ' ', 0x00, 0x00, 0x10, 0x00, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71}, RAW_FRAME_UNCOMPRESSED_BY8},
	{{ 'N',  'V',  '2',  '1', 0x00, 0x00, 0x10, 0x00, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71}, RAW_FRAME_UNCOMPRESSED_NV21},
	{{ 'Y',  'V',  '1',  '2', 0x00, 0x00, 0x10, 0x00, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71}, RAW_FRAME_UNCOMPRESSED_YV12},
	{{ 'I',  '4',  '2',  '0', 0x00, 0x00, 0x10, 0x00, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71}, RAW_FRAME_UNCOMPRESSED_I420},
	{{ 'Y',  '1',  '6',  ' ', 0x00, 0x00, 0x10, 0x00, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71}, RAW_FRAME_UNCOMPRESSED_Y16},
	{{ 'R',  'G',  'B',  'P', 0x00, 0x00, 0x10, 0x00, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71}, RAW_FRAME_UNCOMPRESSED_RGBP},
	{{ 'M',  '4',  '2',  '0', 0x00, 0x00, 0x10, 0x00, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71}, RAW_FRAME_UNCOMPRESSED_M420},
	{{ 'N',  'V',  '1',  '2', 0x00, 0x00, 0x10, 0x00, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71}, RAW_FRAME_UNCOMPRESSED_NV12},
};

/**
 * guidからraw_frame_tを取得する。見つからなければRAW_FRAME_UNKNOWN
 * @param guid
 * @return
 */
raw_frame_t get_raw_frame_type_from_guid(const uint8_t *guid) {
	ENTER();

	raw_frame_t result = RAW_FRAME_UNKNOWN;
	for (auto & i : RAW_FRAME_GUID) {
		if (!memcmp(i.guid, guid, 16)) {
			result = i.frame_type;
			break;
		}
	}
	if (UNLIKELY(result == RAW_FRAME_UNKNOWN)) {
		// 全部一致するのがなければ最初の4バイト(FOURCC)のみを比較する
		for (auto & i : RAW_FRAME_GUID) {
			if (!memcmp(i.guid, guid, 4)) {
				result = i.frame_type;
				break;
			}
		}
	}
	if (UNLIKELY(result == RAW_FRAME_UNKNOWN)) {
		result = get_raw_frame_type_from_fourcc(guid);
	}

	RETURN(result, raw_frame_t);
}

//--------------------------------------------------------------------------------
/**
 * JPEGデータから指定したAPPマーカーを探してマーカー直後のデータへのポインタを返す
 * 見つからなければnullptrを返す
 * @param marker
 * @param data
 * @param size
 * @param len
 * @return
 */
const uint8_t *find_app_marker(const uint8_t &marker,
	const uint8_t *data, const size_t &size, size_t &len) {
//	ENTER();

	LOGV("marker=0x%02x,szi=%" FMT_SIZE_T, marker, size);
	uint8_t *result = nullptr;
	// マーカーは2バイト(0xff+1バイト)
	// ffd0-ffd7(リスタートマーカー)とffd8(SOI), ffd9(EOI)はデータ部無し
	// それ以外のマーカーはマーカー直後の2バイトがデータ部のサイズが入る(ビッグエンディアン)。
	// 実際のデータはデータ部のサイズ-2バイト
	for (size_t i = 0; i < size - 1; ) {
		if (data[i] == JFIF_MARKER_MSB) {
			const uint8_t next = data[i + 1];
//			LOGV("JFIFマーカーかもnext=0x%02x @ %d", next, i);
			if (next == marker) {
				LOGV("指定したマーカーかも");
				if (i + 4 >= size) {
					LOGD("not enough data space,marker=0x%02x @ %" FMT_SIZE_T, next, i);
					i += 2;
					continue;
				}
				// データ部の長さを読み取る(2バイト), これはJPEGのデータなのでビッグエンディアン
				auto data_len = (uint32_t)betoh16(*((uint16_t *)(data + i + 2)));
				if (i + data_len + 2 >= size) {
					// セグメントがデータの終わりよりも長いのはおかしい
					LOGD("not enough data space,marker=0x%02x @ %" FMT_SIZE_T, next, i);
					i += 2;
					continue;
				}
				result = (uint8_t *)data + i;
				len = data_len;
				LOGV("指定したマーカーが見つかった:%p,len=%d,%s",
					result, data_len, bin2hex(result, data_len < 64 ? data_len : 64).c_str());
				break;
			} else if (next == JFIF_MARKER_SOS) {
				// APPxマーカーは必ずSOSマーカーよりも前に存在するのでSOSマーカー以降は解析しない
				LOGD("found SOSマーカー");
				break;
			} else {
				switch (next) {
				case JFIF_MARKER_FF:	// 0xff00 is 0xff as data, not a maker
					i += 2;
					break;
				case JFIF_MARKER_RST0:	// RST0
				case JFIF_MARKER_RST1:	// RST1
				case JFIF_MARKER_RST2:	// RST2
				case JFIF_MARKER_RST3:	// RST3
				case JFIF_MARKER_RST4:	// RST4
				case JFIF_MARKER_RST5:	// RST5
				case JFIF_MARKER_RST6:	// RST6
				case JFIF_MARKER_RST7:	// RST7
				case JFIF_MARKER_SOI:	// SOI
				case JFIF_MARKER_EOI:	// EOI
					LOGV("segment less marker,0x%02x @ %" FMT_SIZE_T, next, i);
					i += 2;	// データ部の無いマーカーの場合
					break;
				default:
					LOGV("other marker,0x%02x @ %" FMT_SIZE_T, next, i);
					if (i + 4 >= size) {
						LOGD("not enough data space,marker=0x%02x @ %" FMT_SIZE_T, marker, i);
						i += 2;
						continue;
					}
					// データ部の長さを読み取る(2バイト), これはJPEGのデータなのでビッグエンディアン
					const auto data_len = (uint32_t)betoh16(*((uint16_t *)(data + i + 2)));
					LOGV("other marker,0x%02x @ %" FMT_SIZE_T ",len=%d", next, i, data_len);
					if (i + data_len + 2 >= size) {
						// セグメントがデータの終わりよりも長いのはおかしい
						LOGD("not enough data space,marker=0x%02x @ %" FMT_SIZE_T, next, i);
						i += 2;
						continue;
					}
					i += (data_len + 2);
					break;
				}
			}
		} else {
			i++;
		}
	}

	return result; // RETURN(result, const uint8_t *);
}

/**
 * JFIFのマーカー情報のホルダークラス
 */
typedef struct marker_info {
	const int marker;
	const size_t ix;
	marker_info(const int &_marker, const size_t &_ix)
		: marker(_marker), ix(_ix) { };
} marker_info_t;

static uint32_t frame_total = 0;
static uint32_t frame_err = 0;
static uint32_t frame_eoi_over = 0;
static uint32_t frame_eoi_over2 = 0;
static uint32_t over_bytes = 0;

/**
 * JFIFマーカーを手繰って(m)jpegフォーマットが正しいかどうかを確認する
 * @param frame
 * @param data
 * @param size
 * @return
 */
int sanitaryMJPEG(IVideoFrame &frame, const uint8_t *data, const size_t &size) {

	ENTER();

	int result = USB_SUCCESS;
	size_t pos = 0;
	uint8_t marker = 0;
	int count_soi = 0;
	std::vector<marker_info_t *> markers(40);

	frame_total++;
	if (UNLIKELY((size < 2)
		|| (data[0] != JFIF_MARKER_MSB)
		|| (data[1] != JFIF_MARKER_SOI))) {

		LOGW("frame does not start with SOI!");
		result = VIDEO_ERROR_FRAME_NO_SOI;
		goto SKIP;
	}
	for (size_t i = 0; !result && (i < size - 1); ) {
		if (data[i] == JFIF_MARKER_MSB) {
			LOGV("JFIFマーカーかも");
			const uint8_t next = data[i + 1];
			switch (next) {
			case JFIF_MARKER_FF:	// 0xff00 is 0xff as data, not a maker
				i += 2;
				break;
			case JFIF_MARKER_RST0:	// RST0
			case JFIF_MARKER_RST1:	// RST1
			case JFIF_MARKER_RST2:	// RST2
			case JFIF_MARKER_RST3:	// RST3
			case JFIF_MARKER_RST4:	// RST4
			case JFIF_MARKER_RST5:	// RST5
			case JFIF_MARKER_RST6:	// RST6
			case JFIF_MARKER_RST7:	// RST7
				LOGV("found marker %d", next);
				markers.push_back(new marker_info(next, i));
				i += 2;	// データ部の無いマーカー(セグメント)の場合
				break;
			case JFIF_MARKER_SOI:	// SOI
				LOGV("found SOI,ix=%" FMT_SIZE_T, i);
				if (!count_soi++) {
					markers.push_back(new marker_info(next, i));
					i += 2;	// データ部の無いマーカーの場合
				} else {
					// EOIが来る前にSOIがもう一度来た
					LOGW("duplicated SOI");
					result = VIDEO_ERROR_FRAME_DUPLICATED_SOI;
					pos = i;
					marker = next;
					goto SKIP;
				}
				break;
			case JFIF_MARKER_EOI:	// EOI
				LOGV("found EOI,ix=%" FMT_SIZE_T, i);
				markers.push_back(new marker_info(next, i));
				pos = i;
				marker = next;
				// EOIはイメージデータの最後で後ろにはデータは無いはずなのに
				// 後ろにデータがくっついてくるカメラがある
				if (size > i + 10) {		// 8バイト以上のパディング?
					frame_eoi_over++;
					over_bytes += size - (i + 2);
					LOGD("padding?,frames=%d,end=%" FMT_SIZE_T ",size=%" FMT_SIZE_T,
						frame_total, i + 2, size);
				} else if (size > i + 2) {	// パディング?
					frame_eoi_over2++;
					LOGD("padding?,frames=%d,end=%" FMT_SIZE_T ",size=%" FMT_SIZE_T,
						frame_total, i + 2, size);
				}
				// EOIマーカー以降を切り捨てる
				frame.resize(i + 2);
				goto SKIP;
			default:
				// データ部のあるマーカー(セグメント)
				LOGV("found marker 0x%02x", next);
				if (UNLIKELY(i + 4 >= size)) {
					// データ部があるはずなのに無い
					LOGD("not enough data space");
					result = VIDEO_ERROR_FRAME_WRONG_SEG_LENGTH;
					pos = i;
					marker = next;
					goto SKIP;
				}
				// データ部の長さを読み取る(2バイト), これはJPEGのデータなのでビッグエンディアン
				const auto data_len = (uint32_t)betoh16(*((uint16_t *)(data + i + 2)));
				if (UNLIKELY(i + data_len + 2 >= size)) {
					// セグメントがデータの終わりよりも長いのはおかしい
					LOGD("truncated,marker=0x%02x,end=%" FMT_SIZE_T ",size=%" FMT_SIZE_T ",0x%02x%02x",
						next, i + data_len + 2, size, data[i+2], data[i+3]);
					result = VIDEO_ERROR_FRAME_TRUNCATED;
					pos = i;
					marker = next;
					goto SKIP;
				}
				markers.push_back(new marker_info(next, i));
				i += (data_len + 2);
				break;
			}
		} else {
			// パディングがあるかも
			i++;
		}
	}
	if (UNLIKELY(!markers.empty())) {
		result = VIDEO_ERROR_FRAME_NO_MARKER;
	}
SKIP:
#ifndef NDEBUG
	if ((frame_total % 2000) == 0) {
		LOGW("markerNum=%" FMT_SIZE_T ",pos=%" FMT_SIZE_T ",size=%" FMT_SIZE_T ",err=%d/%d,over=%d/%d(%d)",
			markers.size(), pos, size,
			frame_err, frame_total, frame_eoi_over, frame_eoi_over2,
			frame_eoi_over ? over_bytes / frame_eoi_over : 0);
	}
#endif
	if (LIKELY(!result && !markers.empty())) {
		// FIXME 未実装 マーカーをチェック
		if (LIKELY(markers[markers.size()-1]->marker == JFIF_MARKER_EOI)) {
			// 最後はEOIでないとだめ, 先頭のSOIはチェック済みなのでここではしない
			for (auto iter: markers) {
				LOGV("found marker 0x%02x\n", iter->marker);
			}
		} else {
			result = VIDEO_ERROR_FRAME_NO_EOI;
		}
	}
	if (UNLIKELY(result)) {
		frame_err++;
		LOGW("wrong frame,r=%d,err=%d/%d(%d/%d),markerNum=%zu,pos=%" FMT_SIZE_T ",marker=0x%02x,size=%zu",
			result, frame_err, frame_eoi_over, frame_eoi_over2, frame_total,
			markers.size(), pos, marker, size);
	}

	// 中間データを削除
	if (!markers.empty()) {
		for (auto iter: markers) {
			SAFE_DELETE(iter);
		}
		markers.clear();
	}
	RETURN(result, int);
}

/**
 * (m)jpegのヘッダーを解析してサブサンプリングや解像度情報を取得する
 * tjDecompressHeader2呼び出しのヘルパー関数
 * @param jpegDecompressor
 * @param in
 * @param jpeg_bytes
 * @param jpeg_subsamp
 * @param jpeg_width
 * @param jpeg_height
 * @return
 */
int parse_mjpeg_header(
	tjhandle &jpegDecompressor,
	const IVideoFrame &in,
	size_t &jpeg_bytes, int &jpeg_subsamp,
	int &jpeg_width, int &jpeg_height) {

	ENTER();

	jpeg_bytes = in.actual_bytes();
	if ((in.frame_type() != RAW_FRAME_MJPEG) || !jpeg_bytes || !jpegDecompressor) {
		RETURN(USB_ERROR_INVALID_PARAM, int);
	}

	auto compressed = (uint8_t *)in.frame();
	// MJPEGのヘッダーを解析してサイズとサブサンプリングの種類を取得する
	// 元のMJPEGがYUV 4:2:2じゃないとうまく動かないかも
	// MD-T1040はjpegSubsampが2(TJSAMP_420)、それ以外は1(TJSAMP_422)が来てる
	const auto r = tjDecompressHeader2(jpegDecompressor, compressed,
		jpeg_bytes, &jpeg_width, &jpeg_height, &jpeg_subsamp);
	if (UNLIKELY(r)) {
		LOGD("not a mjpeg frame:%d", r);
		RETURN(USB_ERROR_NOT_SUPPORTED, int);
	}

	RETURN(USB_SUCCESS, int);
}

/**
 * libjpeg-turboのJ_COLOR_SPACEに対応するraw_frame_tを取得
 * 対応するものがなければRAW_FRAME_UNKNOWNを返す
 * @param color_space
 * @return
 */
raw_frame_t tj_color_space2raw_frame(const int/*J_COLOR_SPACE*/ &color_space) {
	raw_frame_t frame_type = RAW_FRAME_UNKNOWN;
	switch (color_space) {
	case JCS_GRAYSCALE:	frame_type = RAW_FRAME_UNCOMPRESSED_GRAY8; break;
	case JCS_YCbCr:		frame_type = RAW_FRAME_UNCOMPRESSED_YCbCr; break;
	case JCS_RGB565:	frame_type = RAW_FRAME_UNCOMPRESSED_RGB565; break;
	case JCS_RGB:
	case JCS_EXT_RGB:	frame_type = RAW_FRAME_UNCOMPRESSED_RGB; break;
	case JCS_EXT_BGR:	frame_type = RAW_FRAME_UNCOMPRESSED_BGR; break;
	case JCS_EXT_RGBA:
	case JCS_EXT_RGBX:	frame_type = RAW_FRAME_UNCOMPRESSED_RGBX; break;
	case JCS_EXT_ARGB:
	case JCS_EXT_XRGB:	frame_type = RAW_FRAME_UNCOMPRESSED_XRGB; break;
	case JCS_EXT_BGRA:
	case JCS_EXT_BGRX:	frame_type = RAW_FRAME_UNCOMPRESSED_BGRX; break;
	case JCS_EXT_ABGR:
	case JCS_EXT_XBGR:	frame_type = RAW_FRAME_UNCOMPRESSED_XBGR; break;
	default:
		LOGD("unknown out_color_space %d", out_color_space);
		break;
	}
	return frame_type;
}

/**
 * libjpeg-turboのピクセルフォーマットからraw_frame_tへ変換
 * 対応するものがなければRAW_FRAME_UNKNOWNを返す
 * @param tj_pixel_format
 * @return
 */
raw_frame_t tj_pixel_format2raw_frame(const int &tj_pixel_format) {
	switch (tj_pixel_format) {
	case TJPF_RGB:
		return RAW_FRAME_UNCOMPRESSED_RGB;
	case TJPF_BGR:
		return RAW_FRAME_UNCOMPRESSED_BGR;
	case TJPF_RGBA:
	case TJPF_RGBX:
		return RAW_FRAME_UNCOMPRESSED_RGBX;
	case TJPF_BGRA:
	case TJPF_BGRX:
		return RAW_FRAME_UNCOMPRESSED_BGRX;
	case TJPF_ABGR:
	case TJPF_XBGR:
		return RAW_FRAME_UNCOMPRESSED_XBGR;
	case TJPF_ARGB:
	case TJPF_XRGB:
		return RAW_FRAME_UNCOMPRESSED_XRGB;
	case TJPF_GRAY:
		return RAW_FRAME_UNCOMPRESSED_GRAY8;
	case TJPF_CMYK:
	// case TJPF_UNKNOWN:
	default:
		return RAW_FRAME_UNKNOWN;
	}
}

/**
 * turbo-jpegのサブサンプリングからraw_frame_tを取得
 * @param jpeg_subsamp
 * @return
 */
raw_frame_t tjsamp2raw_frame(const int &jpeg_subsamp) {
	switch (jpeg_subsamp) {
	case TJSAMP_444:
		return RAW_FRAME_UNCOMPRESSED_444p;
	case TJSAMP_422:
		return RAW_FRAME_UNCOMPRESSED_422p;
	case TJSAMP_420:
		return RAW_FRAME_UNCOMPRESSED_I420;
	case TJSAMP_GRAY:
		return RAW_FRAME_UNCOMPRESSED_GRAY8;
	case TJSAMP_440:
		return RAW_FRAME_UNCOMPRESSED_440p;
	case TJSAMP_411:
		return RAW_FRAME_UNCOMPRESSED_411p;
	default:
		return RAW_FRAME_UNKNOWN;
	}
}

/**
 * mjpegフレームデータをデコードした時のフレームタイプを取得
 * 指定したIVideoFrameがmjpegフレームでないときなどはRAW_FRAME_UNKNOWNを返す
 * @param src
 * @return
 */
/*public*/
raw_frame_t get_mjpeg_decode_type(tjhandle &jpegDecompressor, const IVideoFrame &src) {
	ENTER();

	raw_frame_t result = RAW_FRAME_UNKNOWN;
	if (src.frame_type() == RAW_FRAME_MJPEG) {
		size_t jpeg_bytes;
		int jpeg_subsamp, jpeg_width, jpeg_height;
		int r = parse_mjpeg_header(jpegDecompressor, src, jpeg_bytes, jpeg_subsamp, jpeg_width, jpeg_height);
		if (!r) {
			result = tjsamp2raw_frame(jpeg_subsamp);
		} else {
			LOGW("parse_mjpeg_header failed,err=%d", r);
		}
	} else {
		LOGD("Not a mjpeg frame!");
	}

	RETURN(result, raw_frame_t);
}

/**
 * FOURCCをチェックして対応するraw_frame_tを返す
 * サポートしているものがなければRAW_FRAME_UNKNOWNを返す
 */
raw_frame_t checkFOURCC(const uint8_t *data, const size_t &size) {
	ENTER();

	raw_frame_t ret = RAW_FRAME_UNKNOWN;
	if (size >= 4) {
		ret = get_raw_frame_type_from_fourcc(data);
	}
	RETURN(ret, raw_frame_t);
}

}	// namespace serenegiant::usb
