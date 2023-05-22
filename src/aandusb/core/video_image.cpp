/*
 * aAndUsb
 * Copyright (c) 2014-2023 saki t_saki@serenegiant.com
 * Distributed under the terms of the GNU Lesser General Public License (LGPL v3.0) License.
 * License details are in the file license.txt, distributed as part of this software.
 */

#define LOG_TAG "VideoImage"

#if 1	// デバッグ情報を出さない時は1
	#ifndef LOG_NDEBUG
		#define	LOG_NDEBUG		// LOGV/LOGD/MARKを出力しない時
	#endif
	#undef USE_LOGALL			// 指定したLOGxだけを出力
#else
//	#define USE_LOGALL
	#define USE_LOGD
	#undef LOG_NDEBUG
	#undef NDEBUG
//	#define DEBUG_GL_CHECK			// GL関数のデバッグメッセージを表示する時
#endif

#include <cstdio>
#include <cstring>	// memcpy

#include <turbojpeg.h>
#include "libyuv.h"

#include "utilbase.h"

// core
#include "core/video_image.h"
#include "core/image_buffer.h"
// usb
#include "usb/aandusb.h"

namespace serenegiant::core {

/**
 * VideoImage_tの内容をログへ出力
 * @param image
 */
void dump(const VideoImage_t &image) {
	LOGI("frame_type=0x%08x", image.frame_type);
	LOGI("width=%u", image.width);
	LOGI("height=%u", image.height);
	LOGI("actual_bytes=%" FMT_SIZE_T, image.actual_bytes);
	LOGI("ptr=%p", image.ptr);
	LOGI("  y=%p", image.ptr_y);
	LOGI("  u=%p", image.ptr_u);
	LOGI("  v=%p", image.ptr_v);
	LOGI("  uv/vu=%p", MIN(image.ptr_v, image.ptr_u));
	LOGI("stride=%u", image.stride);
	LOGI("  y=%u", image.stride_y);
	LOGI("  u=%u", image.stride_u);
	LOGI("  v=%u", image.stride_v);
	LOGI("pixel_stride=%u", image.pixel_stride);
	LOGI("  y=%u", image.pixel_stride_y);
	LOGI("  u=%u", image.pixel_stride_u);
	LOGI("  v=%u", image.pixel_stride_v);
}

static inline bool is_uv(const VideoImage_t &image) {
	return image.ptr_u && image.ptr_v
		&& (image.ptr_u - image.ptr_v == -1);
}

static inline bool is_vu(const VideoImage_t &image) {
	return image.ptr_u && image.ptr_v
		&& (image.ptr_u - image.ptr_v == +1);
}

/**
 * フレームタイプがYUV planar/semi planarかどうかを取得
 * YUYVやUYVYはYUVだけどインターリーブ形式なのでfalseを返す
 * @param frame_type
 * @return
 */
bool is_yuv_format_type(const raw_frame_t &frame_type) {
	switch (frame_type) {
	case RAW_FRAME_UNCOMPRESSED_BY8:	/** 0x040005, 8ビットグレースケール, ベイヤー配列 */
	case RAW_FRAME_UNCOMPRESSED_NV21:	/** 0x050005, YVU420 SemiPlanar(y->vu), YVU420sp */
	case RAW_FRAME_UNCOMPRESSED_YV12:	/** 0x060005, YVU420 Planar(y->v->u), YVU420p, I420とU/Vが逆 */
	case RAW_FRAME_UNCOMPRESSED_I420:	/** 0x070005, YUV420 Planar(y->u->v), YUV420p, YV12とU/Vが逆, YU12 */
	case RAW_FRAME_UNCOMPRESSED_NV12:	/** 0x0b0005, YUV420 SemiPlanar(y->uv) NV21とU/Vが逆 */
	case RAW_FRAME_UNCOMPRESSED_444p:	/** 0x110005, YUV444 Planar(y->u->v), YUV444p */
	case RAW_FRAME_UNCOMPRESSED_444sp:	/** 0x120005, YUV444 semi Planar(y->uv), NV24/YUV444sp */
	case RAW_FRAME_UNCOMPRESSED_422p:	/** 0x130005, YUV422 Planar(y->u->v), YUV422p, YV16, YU16 */
	case RAW_FRAME_UNCOMPRESSED_422sp:	/** 0x140005, YUV422 SemiPlanar(y->uv), NV16/YUV422sp */
	case RAW_FRAME_UNCOMPRESSED_440p:	/** 0x150005, YUV440 Planar(y->u->v), YUV440p */
	case RAW_FRAME_UNCOMPRESSED_440sp:	/** 0x160005, YUV440 Planar(y->u->v), YUV440sp */
	case RAW_FRAME_UNCOMPRESSED_411p:	/** 0x170005, YUV411 Planar(y->u->v), YUV411p */
	case RAW_FRAME_UNCOMPRESSED_411sp:	/** 0x180005, YUV411 SemiPlanar(y->uv), YUV411sp */
		return true;
//	case RAW_FRAME_UNKNOWN:				/** 不明なフレームフォーマット */
//	case RAW_FRAME_STILL:				/** 0x000003 静止画 */
//	case RAW_FRAME_UNCOMPRESSED:		/** 0x000005 不明な非圧縮フレームフォーマット */
//	case RAW_FRAME_UNCOMPRESSED_YUYV:	/** 0x010005, YUY2/YUYV/V422/YUV422, インターリーブ */
//	case RAW_FRAME_UNCOMPRESSED_UYVY:	/** 0x020005, YUYVの順序違い、インターリーブ */
//	case RAW_FRAME_UNCOMPRESSED_GRAY8:	/** 0x030005, 8ビットグレースケール, Y800/Y8/I400 */
//	case RAW_FRAME_UNCOMPRESSED_Y16:	/** 0x080005, 16ビットグレースケール */
//	case RAW_FRAME_UNCOMPRESSED_RGBP:	/** 0x090005, 16ビットインターリーブRGB(16ビット/5+6+5カラー), RGB565と並びが逆, RGB565 LE, BGR565 */
//	case RAW_FRAME_UNCOMPRESSED_M420:	/** 0x0a0005, YUV420でY2の次に1/2UVが並ぶのを繰り返す, YYYY/YYYY/UVUV->YYYY/YYYY/UVUV->... @deprecated */
//	case RAW_FRAME_UNCOMPRESSED_YCbCr:	/** 0x0c0005, Y->Cb->Cr, インターリーブ(色空間を別にすればY->U->Vと同じ並び) */
//	case RAW_FRAME_UNCOMPRESSED_RGB565:	/** 0x0d0005, 8ビットインターリーブRGB(16ビット/5+6+5カラー) */
//	case RAW_FRAME_UNCOMPRESSED_RGB:	/** 0x0e0005, 8ビットインターリーブRGB(24ビットカラー), RGB24 */
//	case RAW_FRAME_UNCOMPRESSED_BGR:	/** 0x0f0005, 8ビットインターリーブBGR(24ビットカラー), BGR24 */
//	case RAW_FRAME_UNCOMPRESSED_RGBX:	/** 0x100005, 8ビットインターリーブRGBX(32ビットカラー), RGBX32 */
//	case RAW_FRAME_UNCOMPRESSED_YUV_ANY:/** 0x190005, MJPEGからYUVプラナー系への変換先指定用、実際はYUVのプランナーフォーマットのいずれかになる */
//	case RAW_FRAME_UNCOMPRESSED_XRGB:	/** 0x1a0005, 8ビットインターリーブXRGB(32ビットカラー), XRGB32 */
//	case RAW_FRAME_UNCOMPRESSED_XBGR:	/** 0x1b0005, 8ビットインターリーブXBGR(32ビットカラー), XBGR32 */
//	case RAW_FRAME_UNCOMPRESSED_BGRX:	/** 0x1c0005, 8ビットインターリーブBGRX(32ビットカラー), BGRX32 */
//	case RAW_FRAME_MJPEG:				/** mjpeg */
//	case RAW_FRAME_FRAME_BASED:			/** 不明なフレームベースのフレームフォーマット */
//	case RAW_FRAME_MPEG2TS:				/** MPEG2TS */
//	case RAW_FRAME_DV:					/** DV */
//	case RAW_FRAME_FRAME_H264:			/** フレームベースのH.264 */
//	case RAW_FRAME_FRAME_VP8:			/** フレームベースのVP8 */
//	case RAW_FRAME_H264:				/** H.264単独フレーム */
//	case RAW_FRAME_H264_SIMULCAST:		/** H.264 SIMULCAST */
//	case RAW_FRAME_VP8:					/** VP8単独フレーム */
//	case RAW_FRAME_VP8_SIMULCAST:		/** VP8 SIMULCAST */
	default:
		return false;
	}
}

/**
 * 引数が同じフォーマットかどうかを確認
 * (ポインタが指しているメモリーが十分かどうかは確認できないので注意)
 * @param a
 * @param b
 * @param uv_check セミプラナー形式でuvとvuを区別するかどうか, デフォルトはtrueで区別する
 * @return
 */
int equal_format(const VideoImage_t &a, const VideoImage_t &b, const bool &uv_check) {
	if ((a.frame_type != RAW_FRAME_UNKNOWN)
		&& (a.frame_type == b.frame_type)
		&& (a.ptr) && (b.ptr)
		&& (a.width == b.width)
		&& (a.height == b.height)
		&& (a.pixel_stride == b.pixel_stride)) {
		
		switch (a.frame_type) {
		//--------------------------------------------------------------------------------
		// インターリーブ形式
		case RAW_FRAME_UNCOMPRESSED_YUYV:	/** 0x010005, YUY2/YUYV/V422/YUV422, インターリーブ */
		case RAW_FRAME_UNCOMPRESSED_UYVY:	/** 0x020005, YUYVの順序違い、インターリーブ */
		case RAW_FRAME_UNCOMPRESSED_BY8:	/** 0x040005, 8ビットグレースケール, ベイヤー配列 */
		case RAW_FRAME_UNCOMPRESSED_Y16:	/** 0x080005, 16ビットグレースケール */
		case RAW_FRAME_UNCOMPRESSED_RGBP:	/** 0x090005, 16ビットインターリーブRGB(16ビット/5+6+5カラー), RGB565と並びが逆, RGB565 LE, BGR565 */
		case RAW_FRAME_UNCOMPRESSED_RGB565:	/** 0x0d0005, 8ビットインターリーブRGB(16ビット/5+6+5カラー) */
		case RAW_FRAME_UNCOMPRESSED_YCbCr:	/** 0x0c0005, Y->Cb->Cr, インターリーブ(色空間を別にすればY->U->Vと同じ並び) */
		case RAW_FRAME_UNCOMPRESSED_RGB:	/** 0x0e0005, 8ビットインターリーブRGB(24ビットカラー), RGB24 */
		case RAW_FRAME_UNCOMPRESSED_BGR:	/** 0x0f0005, 8ビットインターリーブBGR(24ビットカラー), BGR24 */
		case RAW_FRAME_UNCOMPRESSED_RGBX:	/** 0x100005, 8ビットインターリーブRGBX(32ビットカラー), RGBX32 */
		case RAW_FRAME_UNCOMPRESSED_XRGB:	/** 0x1a0005, 8ビットインターリーブXRGB(32ビットカラー), XRGB32 */
		case RAW_FRAME_UNCOMPRESSED_XBGR:	/** 0x1b0005, 8ビットインターリーブXBGR(32ビットカラー), XBGR32 */
		case RAW_FRAME_UNCOMPRESSED_BGRX:	/** 0x1c0005, 8ビットインターリーブBGRX(32ビットカラー), BGRX32 */
			return (a.ptr) && (b.ptr)
				&& (a.stride >= a.width) && (b.stride >= b.width)
				&& (a.actual_bytes >= (a.width * a.height * a.pixel_stride))
				&& (b.actual_bytes >= (b.width * b.height * b.pixel_stride));
		//--------------------------------------------------------------------------------
		case RAW_FRAME_UNCOMPRESSED_GRAY8:	/** 0x030005, 8ビットグレースケール, Y800/Y8/I400 */
			return (a.ptr_y) && (b.ptr_y);
		// YUVセミプラナー系
		case RAW_FRAME_UNCOMPRESSED_NV21:	/** 0x050005, YVU420 SemiPlanar(y->vu), YVU420sp */
		case RAW_FRAME_UNCOMPRESSED_444sp:	/** 0x120005, YUV444 semi Planar(y->uv), NV24/YUV444sp */
		case RAW_FRAME_UNCOMPRESSED_NV12:	/** 0x0b0005, YUV420 SemiPlanar(y->uv) NV21とU/Vが逆 */
		case RAW_FRAME_UNCOMPRESSED_422sp:	/** 0x140005, YUV422 SemiPlanar(y->uv), NV16/YUV422sp */
			return (a.ptr_y) && (b.ptr_y)
				&& (!uv_check || ((is_vu(a) && is_vu(b)) || (is_uv(a) && is_uv(b)))); // uvの順番が同じ
		// YUVプラナー系
		case RAW_FRAME_UNCOMPRESSED_YV12:	/** 0x060005, YVU420 Planar(y->v->u), YVU420p, I420とU/Vが逆 */
		case RAW_FRAME_UNCOMPRESSED_I420:	/** 0x070005, YUV420 Planar(y->u->v), YUV420p, YV12とU/Vが逆 */
		case RAW_FRAME_UNCOMPRESSED_444p:	/** 0x110005, YUV444 Planar(y->u->v), YUV444p */
		case RAW_FRAME_UNCOMPRESSED_422p:	/** 0x130005, YUV422 Planar(y->u->v), YUV422p */
			return (a.ptr_y) && (a.ptr_u) && (a.ptr_v)
				&& (b.ptr_y) && (b.ptr_u) && (b.ptr_v);
		//--------------------------------------------------------------------------------
		// フレームベース
		case RAW_FRAME_MJPEG:				/** mjpeg */
		case RAW_FRAME_FRAME_BASED:			/** 不明なフレームベースのフレームフォーマット */
		case RAW_FRAME_MPEG2TS:				/** MPEG2TS */
		case RAW_FRAME_DV:					/** DV */
		case RAW_FRAME_FRAME_H264:			/** フレームベースのH.264 */
		case RAW_FRAME_FRAME_VP8:			/** フレームベースのVP8 */
		case RAW_FRAME_H264:				/** H.264単独フレーム */
		case RAW_FRAME_H264_SIMULCAST:		/** H.264 SIMULCAST */
		case RAW_FRAME_VP8:					/** VP8単独フレーム */
		case RAW_FRAME_VP8_SIMULCAST:		/** VP8 SIMULCAST */
		case RAW_FRAME_H265:				/** H.265単独フレーム */
			return (a.actual_bytes == b.actual_bytes);
		//--------------------------------------------------------------------------------
		// その他未対応
		case RAW_FRAME_UNKNOWN:				/** 不明なフレームフォーマット */
		case RAW_FRAME_UNCOMPRESSED:		/** 0x000005 不明な非圧縮フレームフォーマット */
		case RAW_FRAME_STILL:				/** 0x000003 静止画 */
		case RAW_FRAME_UNCOMPRESSED_M420:	/** 0x0a0005, YUV420でY2の次に1/2UVが並ぶのを繰り返す, YYYY/YYYY/UVUV->YYYY/YYYY/UVUV->... @deprecated */
		case RAW_FRAME_UNCOMPRESSED_440p:	/** 0x150005, YUV440 Planar(y->u->v), YUV440p */
		case RAW_FRAME_UNCOMPRESSED_440sp:	/** 0x160005, YUV440 Planar(y->u->v), YUV440sp */
		case RAW_FRAME_UNCOMPRESSED_411p:	/** 0x170005, YUV411 Planar(y->u->v), YUV411p */
		case RAW_FRAME_UNCOMPRESSED_411sp:	/** 0x180005, YUV411 SemiPlanar(y->uv), YUV411sp */
		case RAW_FRAME_UNCOMPRESSED_YUV_ANY:	/** 0x190005, MJPEGからYUVプラナー系への変換先指定用、実際はYUVのプランナーフォーマットのいずれかになる */
		default:
			break;
		}
	}
	return false;
}

/**
 * 単純コピー
 * XXX サブサンプリングが同じならセミプラナー⇔プラナー、u⇔vが入れ替わっててもコピーできるけど
 *     とりあえずは単純コピーのみを実装する
 * @param src
 * @param dst
 * @param work
 * @param uv_check セミプラナー形式でuvとvuを区別するかどうか, デフォルトはtrueで区別する
 * @return
 */
int copy(
	const VideoImage_t &src, VideoImage_t &dst,
	std::vector<uint8_t> &work,
	const bool &uv_check) {

	ENTER();
	
	int result = USB_ERROR_NOT_SUPPORTED;

	if (equal_format(src, dst, uv_check)) {
		switch (dst.frame_type) {
		//--------------------------------------------------------------------------------
		// インターリーブ形式
		case RAW_FRAME_UNCOMPRESSED_YUYV:	/** 0x010005, YUY2/YUYV/V422/YUV422, インターリーブ */
		case RAW_FRAME_UNCOMPRESSED_UYVY:	/** 0x020005, YUYVの順序違い、インターリーブ */
		case RAW_FRAME_UNCOMPRESSED_GRAY8:	/** 0x030005, 8ビットグレースケール, Y800/Y8/I400 */
		case RAW_FRAME_UNCOMPRESSED_BY8:	/** 0x040005, 8ビットグレースケール, ベイヤー配列 */
		case RAW_FRAME_UNCOMPRESSED_Y16:	/** 0x080005, 16ビットグレースケール */
		case RAW_FRAME_UNCOMPRESSED_RGBP:	/** 0x090005, 16ビットインターリーブRGB(16ビット/5+6+5カラー), RGB565と並びが逆, RGB565 LE, BGR565 */
		case RAW_FRAME_UNCOMPRESSED_RGB565:	/** 0x0d0005, 8ビットインターリーブRGB(16ビット/5+6+5カラー) */
		case RAW_FRAME_UNCOMPRESSED_YCbCr:	/** 0x0c0005, Y->Cb->Cr, インターリーブ(色空間を別にすればY->U->Vと同じ並び) */
		case RAW_FRAME_UNCOMPRESSED_RGB:	/** 0x0e0005, 8ビットインターリーブRGB(24ビットカラー), RGB24 */
		case RAW_FRAME_UNCOMPRESSED_BGR:	/** 0x0f0005, 8ビットインターリーブBGR(24ビットカラー), BGR24 */
		case RAW_FRAME_UNCOMPRESSED_RGBX:	/** 0x100005, 8ビットインターリーブRGBX(32ビットカラー), RGBX32 */
		case RAW_FRAME_UNCOMPRESSED_XRGB:	/** 0x1a0005, 8ビットインターリーブXRGB(32ビットカラー), XRGB32 */
		case RAW_FRAME_UNCOMPRESSED_XBGR:	/** 0x1b0005, 8ビットインターリーブXBGR(32ビットカラー), XBGR32 */
		case RAW_FRAME_UNCOMPRESSED_BGRX:	/** 0x1c0005, 8ビットインターリーブBGRX(32ビットカラー), BGRX32 */
		{
			const uint8_t *src_ptr = src.ptr;
			auto dst_ptr = dst.ptr;
			const auto w = dst.width * dst.pixel_stride;			// 映像の横幅[バイト数]
			const auto src_stride = src.stride * src.pixel_stride;	// 転送元映像のメモリ上の横幅(パディング込み)[バイト数]
			const auto dst_stride = dst.stride * dst.pixel_stride;	// 転送先映像のメモリ上の横幅(パディング込み[バイト数]
			for (int y = 0; y < dst.height; y++) {
				memcpy(dst_ptr, src_ptr, w);
				src_ptr += src_stride;
				dst_ptr += dst_stride;
			}
			result = USB_SUCCESS;
			break;
		}
		//--------------------------------------------------------------------------------
		// YUVセミプラナー系
		case RAW_FRAME_UNCOMPRESSED_NV21:	/** 0x050005, YVU420 SemiPlanar(y->vu), YVU420sp */
		case RAW_FRAME_UNCOMPRESSED_NV12:	/** 0x0b0005, YUV420 SemiPlanar(y->uv) NV21とU/Vが逆 */
		{
			const int ds = src.ptr_u - src.ptr_v;	// +1か-1のはず
			const uint8_t *src_uv;
			if (ds == 1) {
				// y→vu
				src_uv = src.ptr_v;
			} else if (ds == -1) {
				// y→uv
				src_uv = src.ptr_u;
			} else {
				LOGD("予期しないポインタ値,ds=%d", ds);
				RETURN(result, int);
			}
			const int dd = dst.ptr_u - dst.ptr_v;
			uint8_t *dst_uv;
			if (dd == 1) {
				// y→vu
				dst_uv = dst.ptr_v;
			} else if (dd == -1) {
				// y→uv
				dst_uv = dst.ptr_u;
			} else {
				LOGD("予期しないポインタ値,dd=%d", dd);
				RETURN(result, int);
			}

			const auto w = dst.width;
			const uint8_t *src_y = src.ptr_y;
			auto dst_y = dst.ptr_y;
			// yプレーンをコピー
			for (int y = 0; y < dst.height; y++) {
				memcpy(dst_y, src_y, w);
				src_y += src.stride_y;
				dst_y += dst.stride_y;
			}
			// uv/vuプレーンをコピー
			// uとvはwidth/2だけどuv/vuペアなので1行の幅は(width/2)x2=width
			const auto src_stride_uv = src.stride_u ? src.stride_u : (src.stride_v ? src.stride_v : w);
			const auto dst_stride_uv = dst.stride_u ? dst.stride_u : (dst.stride_v ? dst.stride_v : w);
			for (int y = 0; y < dst.height / 2; y++) {	// 420はuvの高さがheight/2
				memcpy(dst_uv, src_uv, w);
				src_uv += src_stride_uv;
				dst_uv += dst_stride_uv;
			}
			result = USB_SUCCESS;
			break;
		}
		case RAW_FRAME_UNCOMPRESSED_422sp:	/** 0x140005, YUV422 SemiPlanar(y->uv), NV16/YUV422sp */
		{
			const int ds = src.ptr_u - src.ptr_v;	// +1か-1のはず
			const uint8_t *src_uv;
			if (ds == 1) {
				// y→vu
				src_uv = src.ptr_v;
			} else if (ds == -1) {
				// y→uv
				src_uv = src.ptr_u;
			} else {
				LOGD("予期しないポインタ値,ds=%d", ds);
				RETURN(result, int);
			}
			const int dd = dst.ptr_u - dst.ptr_v;
			uint8_t *dst_uv;
			if (dd == 1) {
				// y→vu
				dst_uv = dst.ptr_v;
			} else if (dd == -1) {
				// y→uv
				dst_uv = dst.ptr_u;
			} else {
				LOGD("予期しないポインタ値,dd=%d", dd);
				RETURN(result, int);
			}

			const auto w = dst.width;
			const uint8_t *src_y = src.ptr_y;
			auto dst_y = dst.ptr_y;
			// yプレーンをコピー
			for (int y = 0; y < dst.height; y++) {
				memcpy(dst_y, src_y, w);
				src_y += src.stride_y;
				dst_y += dst.stride_y;
			}
			// uv/vuプレーンをコピー
			// uとvはwidth/2だけどuv/vuペアなので1行の幅は(width/2)x2=width
			const auto src_stride_uv = src.stride_u ? src.stride_u : (src.stride_v ? src.stride_v : w);
			const auto dst_stride_uv = dst.stride_u ? dst.stride_u : (dst.stride_v ? dst.stride_v : w);
			for (int y = 0; y < dst.height; y++) {
				memcpy(dst_uv, src_uv, w);
				src_uv += src_stride_uv;
				dst_uv += dst_stride_uv;
			}
			result = USB_SUCCESS;
			break;
		}
		case RAW_FRAME_UNCOMPRESSED_444sp:	/** 0x120005, YUV444 semi Planar(y->uv), NV24/YUV444sp */
		{
			const int ds = src.ptr_u - src.ptr_v;	// +1か-1のはず
			const uint8_t *src_uv;
			if (ds == 1) {
				// y→vu
				src_uv = src.ptr_v;
			} else if (ds == -1) {
				// y→uv
				src_uv = src.ptr_u;
			} else {
				LOGD("予期しないポインタ値,ds=%d", ds);
				RETURN(result, int);
			}
			const int dd = dst.ptr_u - dst.ptr_v;
			uint8_t *dst_uv;
			if (dd == 1) {
				// y→vu
				dst_uv = dst.ptr_v;
			} else if (dd == -1) {
				// y→uv
				dst_uv = dst.ptr_u;
			} else {
				LOGD("予期しないポインタ値,dd=%d", dd);
				RETURN(result, int);
			}

			const auto w = dst.width;
			const uint8_t *src_y = src.ptr_y;
			auto dst_y = dst.ptr_y;
			// yプレーンをコピー
			for (int y = 0; y < dst.height; y++) {
				memcpy(dst_y, src_y, w);
				src_y += src.stride_y;
				dst_y += dst.stride_y;
			}
			// uv/vuプレーンをコピー
			// uとvはwidthでuv/vuペアなので1行の幅はwidth*2
			const auto w2 = dst.width * 2;
			const auto src_stride_uv = src.stride_u ? src.stride_u : (src.stride_v ? src.stride_v : w2);
			const auto dst_stride_uv = dst.stride_u ? dst.stride_u : (dst.stride_v ? dst.stride_v : w2);
			for (int y = 0; y < dst.height; y++) {
				memcpy(dst_uv, src_uv, w2);
				src_uv += src_stride_uv;
				dst_uv += dst_stride_uv;
			}
			result = USB_SUCCESS;
			break;
		}
		//--------------------------------------------------------------------------------
		// YUVプラナー系
		case RAW_FRAME_UNCOMPRESSED_YV12:	/** 0x060005, YVU420 Planar(y->v->u), YVU420p, I420とU/Vが逆 */
		case RAW_FRAME_UNCOMPRESSED_I420:	/** 0x070005, YUV420 Planar(y->u->v), YUV420p, YV12とU/Vが逆 */
		{
			const auto w = dst.width;
			const uint8_t *src_y = src.ptr_y;
			auto dst_y = dst.ptr_y;
			// yプレーンをコピー
			for (int y = 0; y < dst.height; y++) {
				memcpy(dst_y, src_y, w);
				src_y += src.stride_y;
				dst_y += dst.stride_y;
			}
			const uint8_t *src_u = src.ptr_u;
			const uint8_t *src_v = src.ptr_v;
			auto dst_u = dst.ptr_u;
			auto dst_v = dst.ptr_v;
			// u, vプレーンをコピー
			// uとvはwidth/2
			const auto w2 = w / 2;
			for (int y = 0; y < dst.height / 2; y++) {
				memcpy(dst_u, src_u, w2);
				src_u += src.stride_u;
				dst_u += dst.stride_u;
				memcpy(dst_v, src_v, w2);
				src_v += src.stride_v;
				dst_v += dst.stride_v;
			}
			result = USB_SUCCESS;
			break;
		}
		case RAW_FRAME_UNCOMPRESSED_422p:	/** 0x130005, YUV422 Planar(y->u->v), YUV422p */
		{
			const auto w = dst.width;
			const uint8_t *src_y = src.ptr_y;
			auto dst_y = dst.ptr_y;
			// yプレーンをコピー
			for (int y = 0; y < dst.height; y++) {
				memcpy(dst_y, src_y, w);
				src_y += src.stride_y;
				dst_y += dst.stride_y;
			}
			const uint8_t *src_u = src.ptr_u;
			const uint8_t *src_v = src.ptr_v;
			auto dst_u = dst.ptr_u;
			auto dst_v = dst.ptr_v;
			const auto w2 = w / 2;
			// u, vプレーンをコピー
			// uとvはwidth/2
			for (int y = 0; y < dst.height; y++) {
				memcpy(dst_u, src_u, w2);
				src_u += src.stride_u;
				dst_u += dst.stride_u;
				memcpy(dst_v, src_v, w2);
				src_v += src.stride_v;
				dst_v += dst.stride_v;
			}
			result = USB_SUCCESS;
			break;
		}
		case RAW_FRAME_UNCOMPRESSED_444p:	/** 0x110005, YUV444 Planar(y->u->v), YUV444p */
		{
			const auto w = dst.width;
			const uint8_t *src_y = src.ptr_y;
			auto dst_y = dst.ptr_y;
			// yプレーンをコピー
			for (int y = 0; y < dst.height; y++) {
				memcpy(dst_y, src_y, w);
				src_y += src.stride_y;
				dst_y += dst.stride_y;
			}
			const uint8_t *src_u = src.ptr_u;
			const uint8_t *src_v = src.ptr_v;
			auto dst_u = dst.ptr_u;
			auto dst_v = dst.ptr_v;
			// u, vプレーンをコピー
			// uとvはwidth
			for (int y = 0; y < dst.height; y++) {
				memcpy(dst_u, src_u, w);
				src_u += src.stride_u;
				dst_u += dst.stride_u;
				memcpy(dst_v, src_v, w);
				src_v += src.stride_v;
				dst_v += dst.stride_v;
			}
			result = USB_SUCCESS;
			break;
		}
		//--------------------------------------------------------------------------------
		// フレームベース
		case RAW_FRAME_MJPEG:				/** mjpeg */
		case RAW_FRAME_FRAME_BASED:			/** 不明なフレームベースのフレームフォーマット */
		case RAW_FRAME_MPEG2TS:				/** MPEG2TS */
		case RAW_FRAME_DV:					/** DV */
		case RAW_FRAME_FRAME_H264:			/** フレームベースのH.264 */
		case RAW_FRAME_FRAME_VP8:			/** フレームベースのVP8 */
		case RAW_FRAME_H264:				/** H.264単独フレーム */
		case RAW_FRAME_H264_SIMULCAST:		/** H.264 SIMULCAST */
		case RAW_FRAME_VP8:					/** VP8単独フレーム */
		case RAW_FRAME_VP8_SIMULCAST:		/** VP8 SIMULCAST */
		case RAW_FRAME_H265:				/** H.265単独フレーム */
		{	// 単純にまるごとコピー
			auto actual_bytes = MIN(src.actual_bytes, dst.actual_bytes);
			if (actual_bytes) {
				memcpy(dst.ptr, src.ptr, actual_bytes);
				result = USB_SUCCESS;
			}
			break;
		}
		//--------------------------------------------------------------------------------
		// その他未対応
		case RAW_FRAME_UNKNOWN:				/** 不明なフレームフォーマット */
		case RAW_FRAME_UNCOMPRESSED:		/** 0x000005 不明な非圧縮フレームフォーマット */
		case RAW_FRAME_STILL:				/** 0x000003 静止画 */
		case RAW_FRAME_UNCOMPRESSED_M420:	/** 0x0a0005, YUV420でY2の次に1/2UVが並ぶのを繰り返す, YYYY/YYYY/UVUV->YYYY/YYYY/UVUV->... @deprecated */
		case RAW_FRAME_UNCOMPRESSED_440p:	/** 0x150005, YUV440 Planar(y->u->v), YUV440p */
		case RAW_FRAME_UNCOMPRESSED_440sp:	/** 0x160005, YUV440 Planar(y->u->v), YUV440sp */
		case RAW_FRAME_UNCOMPRESSED_411p:	/** 0x170005, YUV411 Planar(y->u->v), YUV411p */
		case RAW_FRAME_UNCOMPRESSED_411sp:	/** 0x180005, YUV411 SemiPlanar(y->uv), YUV411sp */
		case RAW_FRAME_UNCOMPRESSED_YUV_ANY:	/** 0x190005, MJPEGからYUVプラナー系への変換先指定用、実際はYUVのプランナーフォーマットのいずれかになる */
		default:
			LOGD("unknown frame_type 0x%08x->0x%08x", src.frame_type, dst.frame_type);
			break;
		}
	} else {
		LOGD("different format! 0x%08x->0x%08x", src.frame_type, dst.frame_type);
		result = yuv_copy(src, dst, work);
	}
#if !defined(LOG_NDEBUG)
	if (UNLIKELY(result)) {
		dump(src);
		dump(dst);
	}
#endif

	RETURN(result, int);
}


/**
 * YUV系の映像を別のフレームタイプに変換する
 * @param src
 * @param dst
 * @param work
 * @return
 */
int yuv_copy(
	const VideoImage_t &src, VideoImage_t &dst,
	std::vector<uint8_t> &work) {

	int result = USB_ERROR_NOT_SUPPORTED;
	// ワークへのデコードと出力先のバッファサイズ調整が成功した時
	// 各プレーンの先頭ポインタとサイズを取得
	const uint8_t *src_y = src.ptr_y;
	const uint8_t *src_u = src.ptr_u;
	const uint8_t *src_v = src.ptr_v;
	const int src_w_y = (int)src.stride_y;
	const int src_w_u = (int)src.stride_u;
	const int src_w_v = (int)src.stride_v;
	const int in_width = (int)src.width;
	const int in_height = (int)src.height;

	switch (dst.frame_type) {
	case RAW_FRAME_UNCOMPRESSED_NV21:
	{	// 出力先がNV21の時
//		const size_t sz_y = tjPlaneSizeYUV(0, in_width, 0, in_height, TJSAMP_420);
		const auto dst_stride_y = (int)dst.stride_y;
		const auto dst_stride_uv = (int)MIN(dst.stride_u, dst.stride_v);
		uint8_t *dst_y = dst.ptr_y;
		uint8_t *dst_vu = MIN(dst.ptr_u, dst.ptr_v);
		// 入力映像のフレームタイプで分岐
		switch (src.frame_type) {
		case RAW_FRAME_UNCOMPRESSED_444p:
		{
			result = libyuv::I444ToNV21(
				src_y, src_w_y,
				src_u, src_w_u,
				src_v, src_w_v,
				dst_y, dst_stride_y,
				dst_vu, dst_stride_uv,
				in_width, in_height);
			break;
		}
		case RAW_FRAME_UNCOMPRESSED_422p:
		{
			result = libyuv::I422ToNV21(
				src_y, src_w_y,
				src_u, src_w_u,
				src_v, src_w_v,
				dst_y, dst_stride_y,
				dst_vu, dst_stride_uv,
				in_width, in_height);
			break;
		}
		case RAW_FRAME_UNCOMPRESSED_I420:
		{
			result = libyuv::I420ToNV21(
				src_y, src_w_y,
				src_u, src_w_u,
				src_v, src_w_v,
			  dst_y, dst_stride_y,
					dst_vu, dst_stride_uv,
				in_width, in_height);
			break;
		}
		case RAW_FRAME_UNCOMPRESSED_GRAY8:
		{
			result = libyuv::I400ToNV21(
				src_y, src_w_y,
				dst_y, dst_stride_y,
				dst_vu, dst_stride_uv,
				in_width, in_height);
			break;
		}
		case RAW_FRAME_UNCOMPRESSED_440p:
		case RAW_FRAME_UNCOMPRESSED_411p:
		default:
			break;
		}
		break;
	} // RAW_FRAME_UNCOMPRESSED_NV21
	case RAW_FRAME_UNCOMPRESSED_YV12:
	{	// 出力先がYV12の時
//		const size_t sz_y = tjPlaneSizeYUV(0, in_width, 0, in_height, TJSAMP_420);
//		const size_t sz_u = tjPlaneSizeYUV(1, in_width, 0, in_height, TJSAMP_420);
//		const size_t sz_v = tjPlaneSizeYUV(2, in_width, 0, in_height, TJSAMP_420);
		const auto dst_stride_y = (int)dst.stride_y;
		const auto dst_stride_u = (int)dst.stride_u;
		const auto dst_stride_v = (int)dst.stride_v;
		uint8_t *dst_y = dst.ptr_y;
		uint8_t *dst_v = dst.ptr_v;
		uint8_t *dst_u = dst.ptr_u;
		// 入力映像のフレームタイプで分岐
		switch (src.frame_type) {
		case RAW_FRAME_UNCOMPRESSED_444p:
		{
			result = libyuv::I444ToI420(
				src_y, src_w_y,
				src_u, src_w_u,
				src_v, src_w_v,
				dst_y, dst_stride_y,
				dst_u, dst_stride_u,
				dst_v, dst_stride_v,
				in_width, in_height);
			break;
		}
		case RAW_FRAME_UNCOMPRESSED_422p:
		{
			result = libyuv::I422ToI420(
				src_y, src_w_y,
				src_u, src_w_u,
				src_v, src_w_v,
				dst_y, dst_stride_y,
				dst_u, dst_stride_u,
				dst_v, dst_stride_v,
				in_width, in_height);
			break;
		}
		case RAW_FRAME_UNCOMPRESSED_I420:
		{
			result = libyuv::I420Copy(
				src_y, src_w_y,
				src_u, src_w_u,
				src_v, src_w_v,
				dst_y, dst_stride_y,
				dst_u, dst_stride_u,
				dst_v, dst_stride_v,
				in_width, in_height);
			break;
		}
		case RAW_FRAME_UNCOMPRESSED_GRAY8:
		{
			result = libyuv::I400ToI420(
				src_y, src_w_y,
				dst_y, dst_stride_y,
				dst_u, dst_stride_u,
				dst_v, dst_stride_v,
				in_width, in_height);
			break;
		}
		case RAW_FRAME_UNCOMPRESSED_440p:
		case RAW_FRAME_UNCOMPRESSED_411p:
		default:
			break;
		}
		break;
	} // RAW_FRAME_UNCOMPRESSED_YV12
	case RAW_FRAME_UNCOMPRESSED_I420:
	{	// 出力先がI420の時
//		const size_t sz_y = tjPlaneSizeYUV(0, in_width, 0, in_height, TJSAMP_420);
//		const size_t sz_u = tjPlaneSizeYUV(1, in_width, 0, in_height, TJSAMP_420);
//		const size_t sz_v = tjPlaneSizeYUV(2, in_width, 0, in_height, TJSAMP_420);
		const auto dst_stride_y = (int)dst.stride_y;
		const auto dst_stride_u = (int)dst.stride_u;
		const auto dst_stride_v = (int)dst.stride_v;
		uint8_t *dst_y = dst.ptr_y;
		uint8_t *dst_u = dst.ptr_u;
		uint8_t *dst_v = dst.ptr_v;
		// 入力映像のフレームタイプで分岐
		switch (src.frame_type) {
		case RAW_FRAME_UNCOMPRESSED_444p:
		{
			result = libyuv::I444ToI420(
				src_y, src_w_y,
				src_u, src_w_u,
				src_v, src_w_v,
				dst_y, dst_stride_y,
				dst_u, dst_stride_u,
				dst_v, dst_stride_v,
				in_width, in_height);
			break;
		}
		case RAW_FRAME_UNCOMPRESSED_422p:
		{
			result = libyuv::I422ToI420(
				src_y, src_w_y,
				src_u, src_w_u,
				src_v, src_w_v,
				dst_y, dst_stride_y,
				dst_u, dst_stride_u,
				dst_v, dst_stride_v,
				in_width, in_height);
			break;
		}
		case RAW_FRAME_UNCOMPRESSED_I420:
		{
			result = libyuv::I420Copy(
				src_y, src_w_y,
				src_u, src_w_u,
				src_v, src_w_v,
				dst_y, dst_stride_y,
				dst_u, dst_stride_u,
				dst_v, dst_stride_v,
				in_width, in_height);
			break;
		}
		case RAW_FRAME_UNCOMPRESSED_GRAY8:
		{
			result = libyuv::I400ToI420(
				src_y, src_w_y,
				dst_y, dst_stride_y,
				dst_u, dst_stride_u,
				dst_v, dst_stride_v,
				in_width, in_height);
			break;
		}
		case RAW_FRAME_UNCOMPRESSED_440p:
		case RAW_FRAME_UNCOMPRESSED_411p:
		default:
			break;
		}
		break;
	} // RAW_FRAME_UNCOMPRESSED_I420
//	case RAW_FRAME_UNCOMPRESSED_M420:
//	{	// FIXME 未実装　出力先がM420の時
//		const size_t sz_y = tjPlaneSizeYUV(0, in_width, 0, in_height, TJSAMP_420);
//		const size_t sz_u = tjPlaneSizeYUV(1, in_width, 0, in_height, TJSAMP_420);
//		const size_t sz_v = tjPlaneSizeYUV(2, in_width, 0, in_height, TJSAMP_420);
//		const int w_y = tjPlaneWidth(0, in_width, TJSAMP_420);
//		const int w_u = tjPlaneWidth(1, in_width, TJSAMP_420);
//		const int w_v = tjPlaneWidth(2, in_width, TJSAMP_420);
//		uint8_t *y = dst.ptr_y;
//		uint8_t *v = dst.ptr_v;
//		uint8_t *u = dst.ptr_u;
//		// 入力映像のフレームタイプで分岐
//		switch (src.frame_type) {
//		case RAW_FRAME_UNCOMPRESSED_444p:
//		case RAW_FRAME_UNCOMPRESSED_422p:
//		case RAW_FRAME_UNCOMPRESSED_I420:
//		case RAW_FRAME_UNCOMPRESSED_GRAY8:
//		case RAW_FRAME_UNCOMPRESSED_440p:
//		case RAW_FRAME_UNCOMPRESSED_411p:
//		default:
//			break;
//		}
//		break;
//	} // RAW_FRAME_UNCOMPRESSED_M420
	case RAW_FRAME_UNCOMPRESSED_NV12:
	{	// 出力先がNV12の時
		const size_t sz_y = tjPlaneSizeYUV(0, in_width, 0, in_height, TJSAMP_420);
		const size_t sz_u = tjPlaneSizeYUV(1, in_width, 0, in_height, TJSAMP_420);
		const size_t sz_v = tjPlaneSizeYUV(2, in_width, 0, in_height, TJSAMP_420);
		const size_t sz = sz_y + sz_u + sz_y + sz_v;
		const int w_y = tjPlaneWidth(0, in_width, TJSAMP_420);
		const int w_uv = tjPlaneWidth(1, in_width, TJSAMP_420) * 2;
		const auto dst_stride_y = (int)dst.stride_y;
		const auto dst_stride_uv = (int)MIN(dst.stride_u, dst.stride_v);
		uint8_t *dst_y = dst.ptr_y;
		uint8_t *dst_uv = MIN(dst.ptr_u, dst.ptr_v);
		// 入力映像のフレームタイプで分岐
		switch (src.frame_type) {
		case RAW_FRAME_UNCOMPRESSED_444p:
		{
			work.resize(sz);
			if (work.size() >= sz) {
				uint8_t *wy = &work[0];
				uint8_t *wvu = wy + sz_y;
				libyuv::I444ToNV21(
					src_y, src_w_y,
					src_u, src_w_u,
					src_v, src_w_v,
					wy, w_y,
					wvu, w_uv,
					in_width, in_height);
				result = libyuv::NV21ToNV12(
					wy, w_y,
					wvu, w_uv,
					dst_y, dst_stride_y,
					dst_uv, dst_stride_uv,
					in_width, in_height);
			} else {
				result = USB_ERROR_NO_MEM;
			}
			break;
		}
		case RAW_FRAME_UNCOMPRESSED_422p:
		{
			work.resize(sz);
			if (work.size() >= sz) {
				uint8_t *wy = &work[0];
				uint8_t *wvu = wy + sz_y;
				libyuv::I422ToNV21(
					src_y, src_w_y,
					src_u, src_w_u,
					src_v, src_w_v,
					wy, w_y,
					wvu, w_uv,
					in_width, in_height);
				result = libyuv::NV21ToNV12(
					wy, w_y,
					wvu, w_uv,
					dst_y, dst_stride_y,
					dst_uv, dst_stride_uv,
					in_width, in_height);
			} else {
				result = USB_ERROR_NO_MEM;
			}
			break;
		}
		case RAW_FRAME_UNCOMPRESSED_I420:
		{
			result = libyuv::I420ToNV12(
				src_y, src_w_y,
				src_u, src_w_u,
				src_v, src_w_v,
				dst_y, dst_stride_y,
				dst_uv, dst_stride_uv,
				in_width, in_height);
			break;
		}
		case RAW_FRAME_UNCOMPRESSED_GRAY8:
		{
			work.resize(sz);
			if (work.size() >= sz) {
				uint8_t *wy = &work[0];
				uint8_t *wvu = wy + sz_y;
				libyuv::I400ToNV21(
					src_y, src_w_y,
					wy, w_y,
					wvu, w_uv,
					in_width, in_height);
				result = libyuv::NV21ToNV12(
					wy, w_y,
					wvu, w_uv,
					dst_y, dst_stride_y,
					dst_uv, dst_stride_uv,
					in_width, in_height);
			} else {
				result = USB_ERROR_NO_MEM;
			}
			break;
		}
		case RAW_FRAME_UNCOMPRESSED_440p:
		case RAW_FRAME_UNCOMPRESSED_411p:
		default:
			break;
		}
		break;
	} // RAW_FRAME_UNCOMPRESSED_NV12
	case RAW_FRAME_UNCOMPRESSED_444p:
	{	// 出力先が444pの時
//		const size_t sz_y = tjPlaneSizeYUV(0, in_width, 0, in_height, TJSAMP_444);
//		const size_t sz_u = tjPlaneSizeYUV(1, in_width, 0, in_height, TJSAMP_444);
//		const size_t sz_v = tjPlaneSizeYUV(2, in_width, 0, in_height, TJSAMP_444);
		const int w_y = tjPlaneWidth(0, in_width, TJSAMP_444);
		const int w_u = tjPlaneWidth(1, in_width, TJSAMP_444);
		const int w_v = tjPlaneWidth(2, in_width, TJSAMP_444);
		const auto dst_stride_y = (int)dst.stride_y;
		const auto dst_stride_u = (int)dst.stride_u;
		const auto dst_stride_v = (int)dst.stride_v;
		uint8_t *dst_y = dst.ptr_y;
		uint8_t *dst_u = dst.ptr_u;
		uint8_t *dst_v = dst.ptr_v;
#if 0
		static int cnt = 0;
		if ((++cnt % 100) == 0) {
			LOGI("sz(%" FMT_SIZE_T ",%" FMT_SIZE_T ",%" FMT_SIZE_T "),w(%d,%d,%d)",
				sz_y, sz_u, sz_v, w_y, w_u, w_v);
		}
#endif
		// 入力映像のフレームタイプで分岐
		switch (src.frame_type) {
		case RAW_FRAME_UNCOMPRESSED_444p:
		{
			result = libyuv::I444Copy(
				src_y, src_w_y,
				src_u, src_w_u,
				src_v, src_w_v,
				dst_y, dst_stride_y,
				dst_u, dst_stride_u,
				dst_v, dst_stride_v,
				in_width, in_height);
			break;
		}
		case RAW_FRAME_UNCOMPRESSED_422p:
		{
			const size_t sz_argb = in_width * in_height * 4;
			work.resize(sz_argb);
			if (work.size() >= sz_argb) {
				uint8_t *argb = &work[0];
				libyuv::I422ToARGB(
					src_y, src_w_y,
					src_u, src_w_u,
					src_v, src_w_v,
					argb, in_width * 4,
					in_width, in_height);
				result = libyuv::ARGBToI444(
					argb, in_width * 4,
					dst_y, dst_stride_y,
					dst_u, dst_stride_u,
					dst_v, dst_stride_v,
					in_width, in_height);
			} else {
				result = USB_ERROR_NO_MEM;
			}
			break;
		}
		case RAW_FRAME_UNCOMPRESSED_I420:
		{
			result = libyuv::I420ToI444(
				src_y, src_w_y,
				src_u, src_w_u,
				src_v, src_w_v,
				dst_y, dst_stride_y,
				dst_u, dst_stride_u,
				dst_v, dst_stride_v,
				in_width, in_height);
			break;
		}
		case RAW_FRAME_UNCOMPRESSED_GRAY8:
		{
			const size_t sz_y0 = tjPlaneSizeYUV(0, in_width, 0, in_height, TJSAMP_420);
			const size_t sz_u0 = tjPlaneSizeYUV(1, in_width, 0, in_height, TJSAMP_420);
			const size_t sz_v0 = tjPlaneSizeYUV(2, in_width, 0, in_height, TJSAMP_420);
			const size_t sz0 = sz_y0 + sz_u0 + sz_v0;
			const int w_y0 = tjPlaneWidth(0, in_width, TJSAMP_420);
			const int w_u0 = tjPlaneWidth(1, in_width, TJSAMP_420);
			const int w_v0 = tjPlaneWidth(2, in_width, TJSAMP_420);
			work.resize(sz0);
			if (work.size() >= sz0) {
				uint8_t *y0 = &work[0];
				uint8_t *u0 = y0 + sz_y0;
				uint8_t *v0 = u0 + sz_u0;
				libyuv::I400ToI420(
					src_y, src_w_y,
					y0, w_y0,
					u0, w_u0,
					v0, w_v0,
					in_width, in_height);
				result = libyuv::I420ToI444(
					y0, w_y0,
					u0, w_u0,
					v0, w_v0,
					dst_y, dst_stride_y,
					dst_u, dst_stride_u,
					dst_v, dst_stride_v,
					in_width, in_height);
			} else {
				result = USB_ERROR_NO_MEM;
			}
			break;
		}
		case RAW_FRAME_UNCOMPRESSED_440p:
		case RAW_FRAME_UNCOMPRESSED_411p:
		default:
			break;
		}
		break;
	} // RAW_FRAME_UNCOMPRESSED_444p
	case RAW_FRAME_UNCOMPRESSED_444sp:
	{	// 出力先が444spの時
//		const size_t sz_y = tjPlaneSizeYUV(0, in_width, 0, in_height, TJSAMP_444);
//		const size_t sz_u = tjPlaneSizeYUV(1, in_width, 0, in_height, TJSAMP_444);
//		const size_t sz_v = tjPlaneSizeYUV(2, in_width, 0, in_height, TJSAMP_444);
		const auto dst_stride_y = (int)dst.stride_y;
		const auto dst_stride_u = (int)dst.stride_u;
		const auto dst_stride_v = (int)dst.stride_v;
		uint8_t *dst_y = dst.ptr_y;
		uint8_t *dst_u = dst.ptr_u;
		uint8_t *dst_v = dst.ptr_v;
		// 入力映像のフレームタイプで分岐
		switch (src.frame_type) {
		case RAW_FRAME_UNCOMPRESSED_444p:
		{
			result = libyuv::I444Copy(
				src_y, src_w_y,
				src_u, src_w_u,
				src_v, src_w_v,
				dst_y, dst_stride_y,
				dst_u, dst_stride_u,
				dst_v, dst_stride_v,
				in_width, in_height);
			break;
		}
		case RAW_FRAME_UNCOMPRESSED_422p:
		{
			const size_t sz_argb = in_width * in_height * 4;
			work.resize(sz_argb);
			if (work.size() >= sz_argb) {
				uint8_t *argb = &work[0];
				libyuv::I422ToARGB(
					src_y, src_w_y,
					src_u, src_w_u,
					src_v, src_w_v,
					argb, in_width * 4,
					in_width, in_height);
				result = libyuv::ARGBToI444(
					argb, in_width * 4,
					dst_y, dst_stride_y,
					dst_u, dst_stride_u,
					dst_v, dst_stride_v,
					in_width, in_height);
			} else {
				result = USB_ERROR_NO_MEM;
			}
			break;
		}
		case RAW_FRAME_UNCOMPRESSED_I420:
		{
			result = libyuv::I420ToI444(
				src_y, src_w_y,
				src_u, src_w_u,
				src_v, src_w_v,
				dst_y, dst_stride_y,
				dst_u, dst_stride_u,
				dst_v, dst_stride_v,
				in_width, in_height);
			break;
		}
		case RAW_FRAME_UNCOMPRESSED_GRAY8:
		{
			const size_t sz_y0 = tjPlaneSizeYUV(0, in_width, 0, in_height, TJSAMP_420);
			const size_t sz_u0 = tjPlaneSizeYUV(1, in_width, 0, in_height, TJSAMP_420);
			const size_t sz_v0 = tjPlaneSizeYUV(2, in_width, 0, in_height, TJSAMP_420);
			const size_t sz0 = sz_y0 + sz_u0 + sz_v0;
			const int w_y0 = tjPlaneWidth(0, in_width, TJSAMP_420);
			const int w_u0 = tjPlaneWidth(1, in_width, TJSAMP_420);
			const int w_v0 = tjPlaneWidth(2, in_width, TJSAMP_420);
			work.resize(sz0);
			if (work.size() >= sz0) {
				uint8_t *y0 = &work[0];
				uint8_t *u0 = y0 + sz_y0;
				uint8_t *v0 = u0 + sz_u0;
				libyuv::I400ToI420(
					src_y, src_w_y,
					y0, w_y0,
					u0, w_u0,
					v0, w_v0,
					in_width, in_height);
				result = libyuv::I420ToI444(
					y0, w_y0,
					u0, w_u0,
					v0, w_v0,
					dst_y, dst_stride_y,
					dst_u, dst_stride_u,
					dst_v, dst_stride_v,
					in_width, in_height);
			} else {
				result = USB_ERROR_NO_MEM;
			}
			break;
		}
		case RAW_FRAME_UNCOMPRESSED_440p:
		case RAW_FRAME_UNCOMPRESSED_411p:
		default:
			break;
		}
		break;
	} // RAW_FRAME_UNCOMPRESSED_444sp
	case RAW_FRAME_UNCOMPRESSED_422p:
	{	// 出力先が422pの時
//		const size_t sz_y = tjPlaneSizeYUV(0, in_width, 0, in_height, TJSAMP_422);
//		const size_t sz_u = tjPlaneSizeYUV(1, in_width, 0, in_height, TJSAMP_422);
//		const size_t sz_v = tjPlaneSizeYUV(2, in_width, 0, in_height, TJSAMP_422);
		const auto dst_stride_y = (int)dst.stride_y;
		const auto dst_stride_u = (int)dst.stride_u;
		const auto dst_stride_v = (int)dst.stride_v;
		uint8_t *dst_y = dst.ptr_y;
		uint8_t *dst_u = dst.ptr_u;
		uint8_t *dst_v = dst.ptr_v;
		// 入力映像のフレームタイプで分岐
		switch (src.frame_type) {
		case RAW_FRAME_UNCOMPRESSED_444p:
		{
#if 0
			// これは結構遅い
			const size_t sz_argb = in_width * in_height * 4;
			work.resize(sz_argb);
			if (work.size() >= sz_argb) {
				uint8_t *argb = &work[0];
				libyuv::I444ToARGB(
					src_y, src_w_y,
					src_u, src_w_u,
					src_v, src_w_v,
					argb, in_width * 4,
					in_width, in_height);
				result = libyuv::ARGBToI422(
					argb, in_width * 4,
					dst_y, w_y,
					dst_u, w_u,
					dst_v, w_v,
					in_width, in_height);
			} else {
				result = USB_ERROR_NO_MEM;
			}
#else
			// 多少uvの解像度が下がるのかもしれないけどARGBを経由するより速い
			const size_t sz_y0 = tjPlaneSizeYUV(0, in_width, 0, in_height, TJSAMP_420);
			const size_t sz_u0 = tjPlaneSizeYUV(1, in_width, 0, in_height, TJSAMP_420);
			const size_t sz_v0 = tjPlaneSizeYUV(2, in_width, 0, in_height, TJSAMP_420);
			const size_t sz0 = sz_y0 + sz_u0 + sz_v0;
			const int w_y0 = tjPlaneWidth(0, in_width, TJSAMP_420);
			const int w_u0 = tjPlaneWidth(1, in_width, TJSAMP_420);
			const int w_v0 = tjPlaneWidth(2, in_width, TJSAMP_420);
			work.resize(sz0);
			if (work.size() >= sz0) {
				uint8_t *y0 = &work[0];
				uint8_t *u0 = y0 + sz_y0;
				uint8_t *v0 = u0 + sz_u0;
				libyuv::I444ToI420(
					src_y, src_w_y,
					src_u, src_w_u,
					src_v, src_w_v,
					y0, w_y0,
					u0, w_u0,
					v0, w_v0,
					in_width, in_height);
				result = libyuv::I420ToI422(
					y0, w_y0,
					u0, w_u0,
					v0, w_v0,
					dst_y, dst_stride_y,
					dst_u, dst_stride_u,
					dst_v, dst_stride_v,
					in_width, in_height);
			} else {
				result = USB_ERROR_NO_MEM;
			}
#endif
			break;
		}
		case RAW_FRAME_UNCOMPRESSED_422p:
		{
			result = libyuv::I422Copy(
				src_y, src_w_y,
				src_u, src_w_u,
				src_v, src_w_v,
				dst_y, dst_stride_y,
				dst_u, dst_stride_u,
				dst_v, dst_stride_v,
				in_width, in_height);
			break;
		}
		case RAW_FRAME_UNCOMPRESSED_I420:
		{
			result = libyuv::I420ToI422(
				src_y, src_w_y,
				src_u, src_w_u,
				src_v, src_w_v,
				dst_y, dst_stride_y,
				dst_u, dst_stride_u,
				dst_v, dst_stride_v,
				in_width, in_height);
			break;
		}
		case RAW_FRAME_UNCOMPRESSED_GRAY8:
		{
			const size_t sz_y0 = tjPlaneSizeYUV(0, in_width, 0, in_height, TJSAMP_420);
			const size_t sz_u0 = tjPlaneSizeYUV(1, in_width, 0, in_height, TJSAMP_420);
			const size_t sz_v0 = tjPlaneSizeYUV(2, in_width, 0, in_height, TJSAMP_420);
			const size_t sz0 = sz_y0 + sz_u0 + sz_v0;
			const int w_y0 = tjPlaneWidth(0, in_width, TJSAMP_420);
			const int w_u0 = tjPlaneWidth(1, in_width, TJSAMP_420);
			const int w_v0 = tjPlaneWidth(2, in_width, TJSAMP_420);
			work.resize(sz0);
			if (work.size() >= sz0) {
				uint8_t *y0 = &work[0];
				uint8_t *u0 = y0 + sz_y0;
				uint8_t *v0 = u0 + sz_u0;
				libyuv::I400ToI420(
					src_y, src_w_y,
					y0, w_y0,
					u0, w_u0,
					v0, w_v0,
					in_width, in_height);
				result = libyuv::I420ToI422(
					y0, w_y0,
					u0, w_u0,
					v0, w_v0,
					dst_y, dst_stride_y,
					dst_u, dst_stride_u,
					dst_v, dst_stride_v,
					in_width, in_height);
			} else {
				result = USB_ERROR_NO_MEM;
			}
			break;
		}
		case RAW_FRAME_UNCOMPRESSED_440p:
		case RAW_FRAME_UNCOMPRESSED_411p:
		default:
			break;
		}
		break;
	} // RAW_FRAME_UNCOMPRESSED_422p
	case RAW_FRAME_UNCOMPRESSED_422sp:
	{	// 出力先が422spの時
//		const size_t sz_y = tjPlaneSizeYUV(0, in_width, 0, in_height, TJSAMP_422);
//		const size_t sz_u = tjPlaneSizeYUV(1, in_width, 0, in_height, TJSAMP_422);
//		const size_t sz_v = tjPlaneSizeYUV(2, in_width, 0, in_height, TJSAMP_422);
		const int w_y = tjPlaneWidth(0, in_width, TJSAMP_422);
		const int w_u = tjPlaneWidth(1, in_width, TJSAMP_422);
		const int w_v = tjPlaneWidth(2, in_width, TJSAMP_422);
		const auto dst_stride_y = (int)dst.stride_y;
		const auto dst_stride_u = (int)dst.stride_u;
		const auto dst_stride_v = (int)dst.stride_v;
		uint8_t *dst_y = dst.ptr_y;
		uint8_t *dst_u = dst.ptr_u;
		uint8_t *dst_v = dst.ptr_v;
		// 入力映像のフレームタイプで分岐
		switch (src.frame_type) {
		case RAW_FRAME_UNCOMPRESSED_444p:
		{
			const size_t sz_argb = in_width * in_height * 4;
			work.resize(sz_argb);
			if (work.size() >= sz_argb) {
				uint8_t *argb = &work[0];
				libyuv::I444ToARGB(
					src_y, src_w_y,
					src_u, src_w_u,
					src_v, src_w_v,
					argb, in_width * 4,
					in_width, in_height);
				result = libyuv::ARGBToI422(
					argb, in_width * 4,
					dst_y, dst_stride_y,
					dst_u, dst_stride_u,
					dst_v, dst_stride_v,
					in_width, in_height);
			} else {
				result = USB_ERROR_NO_MEM;
			}
			break;
		}
		case RAW_FRAME_UNCOMPRESSED_422p:
		{	// yuv422p -> yuv422sp
			// yプレーンだけをコピーする
			libyuv::CopyPlane(
				src_y, src_w_y,
				dst_y, w_y,
				in_width, in_height);
			// u planeとv planeをuv planeにまとめてコピー
			libyuv::MergeUVPlane(
				src_u, w_u,
				src_v, w_v,
				dst_u, dst_stride_u,
				in_width, in_height);
			result = USB_SUCCESS;
			break;
		}
		case RAW_FRAME_UNCOMPRESSED_I420:
		{
			result = libyuv::I420ToI422(
				src_y, src_w_y,
				src_u, src_w_u,
				src_v, src_w_v,
				dst_y, dst_stride_y,
				dst_u, dst_stride_u,
				dst_v, dst_stride_v,
				in_width, in_height);
			break;
		}
		case RAW_FRAME_UNCOMPRESSED_GRAY8:
		{
			const size_t sz_y0 = tjPlaneSizeYUV(0, in_width, 0, in_height, TJSAMP_420);
			const size_t sz_u0 = tjPlaneSizeYUV(1, in_width, 0, in_height, TJSAMP_420);
			const size_t sz_v0 = tjPlaneSizeYUV(2, in_width, 0, in_height, TJSAMP_420);
			const size_t sz0 = sz_y0 + sz_u0 + sz_v0;
			const int w_y0 = tjPlaneWidth(0, in_width, TJSAMP_420);
			const int w_u0 = tjPlaneWidth(1, in_width, TJSAMP_420);
			const int w_v0 = tjPlaneWidth(2, in_width, TJSAMP_420);
			work.resize(sz0);
			if (work.size() >= sz0) {
				uint8_t *y0 = &work[0];
				uint8_t *u0 = y0 + sz_y0;
				uint8_t *v0 = u0 + sz_u0;
				libyuv::I400ToI420(
					src_y, src_w_y,
					y0, w_y0,
					u0, w_u0,
					v0, w_v0,
					in_width, in_height);
				result = libyuv::I420ToI422(
					y0, w_y0,
					u0, w_u0,
					v0, w_v0,
					dst_y, dst_stride_y,
					dst_u, dst_stride_u,
					dst_v, dst_stride_v,
					in_width, in_height);
			} else {
				result = USB_ERROR_NO_MEM;
			}
			break;
		}
		case RAW_FRAME_UNCOMPRESSED_440p:
		case RAW_FRAME_UNCOMPRESSED_411p:
		default:
			break;
		}
		break;
	} // RAW_FRAME_UNCOMPRESSED_422sp
	case RAW_FRAME_UNCOMPRESSED_GRAY8:
	{	// 出力先がgray8の時
		const auto dst_stride_y = (int)dst.stride_y;
		uint8_t *y = dst.ptr_y;
		// yプレーンだけをコピーする
		libyuv::CopyPlane(
			src_y, src_w_y,
			y, dst_stride_y,
			in_width, in_height);
		result = USB_SUCCESS;
		break;
	} // RAW_FRAME_UNCOMPRESSED_GRAY8
	case RAW_FRAME_UNCOMPRESSED_440p:
	case RAW_FRAME_UNCOMPRESSED_440sp:
	case RAW_FRAME_UNCOMPRESSED_411p:
	case RAW_FRAME_UNCOMPRESSED_411sp:
	default:
		result = USB_ERROR_NOT_SUPPORTED;
		break;
	}

	RETURN(result, int);
}

/**
 * 単純コピー
 * @param src
 * @param dst
 * @param work
 * @param uv_check セミプラナー形式でuvとvuを区別するかどうか, デフォルトはtrueで区別する
 * @return
 */
int copy(
	const ImageBuffer &src, ImageBuffer &dst,
	std::vector<uint8_t> &work,
	const bool &uv_check) {

	ENTER();

	int result = USB_ERROR_NOT_SUPPORTED;

	auto _src = const_cast<ImageBuffer &>(src);
	auto src_image = _src.lock();
	auto dst_image = dst.lock();
	if ((src_image.frame_type != RAW_FRAME_UNKNOWN)
		&& (dst_image.frame_type != RAW_FRAME_UNKNOWN)) {
		result = copy(src_image, dst_image, work, uv_check);
	}
	_src.unlock();
	dst.unlock();

	RETURN(result, int);
}

/**
 * 映像データのポインターをVideoImage_tに割り当てる
 * @param image 映像データのポインターを割り当てるVideoImage_t
 * @param frame_type フレームタイプ
 * @param width 映像幅
 * @param height 映像高さ
 * @param data 映像データへのポインター, プラナー/セミプラナーの場合は各プレーンが連続していること
 * @param bytes 映像データのフレームサイズ
 * @return
 */
int assign(
	VideoImage_t &image,
	const raw_frame_t  &frame_type, const uint32_t &width, const uint32_t &height,
	uint8_t *data, const size_t &bytes) {

	ENTER();

	int result = USB_ERROR_NOT_SUPPORTED;

	const auto plane_bytes = width * height;	// 1plane当たりのピクセル数
	memset(&image, 0, sizeof(VideoImage_t));
	image.frame_type = frame_type;
	image.width = width;
	image.height = height;
	image.actual_bytes = bytes;
	image.ptr = data;
	image.stride = width;
	image.pixel_stride = 0;

	switch (frame_type) {
	// インターリーブ形式
	case RAW_FRAME_UNCOMPRESSED_YUYV:	/** 0x010005, YUY2/YUYV/V422/YUV422, インターリーブ */
	case RAW_FRAME_UNCOMPRESSED_UYVY:	/** 0x020005, YUYVの順序違い、インターリーブ */
		image.pixel_stride = 2;
		result = USB_SUCCESS;
		break;
	case RAW_FRAME_UNCOMPRESSED_GRAY8:	/** 0x030005, 8ビットグレースケール, Y800/Y8/I400 */
	case RAW_FRAME_UNCOMPRESSED_BY8:	/** 0x040005, 8ビットグレースケール, ベイヤー配列 */
		image.pixel_stride = 1;
		result = USB_SUCCESS;
		break;
	case RAW_FRAME_UNCOMPRESSED_Y16:	/** 0x080005, 16ビットグレースケール */
	case RAW_FRAME_UNCOMPRESSED_RGBP:	/** 0x090005, 16ビットインターリーブRGB(16ビット/5+6+5カラー), RGB565と並びが逆, RGB565 LE, BGR565 */
	case RAW_FRAME_UNCOMPRESSED_RGB565:	/** 0x0d0005, 8ビットインターリーブRGB(16ビット/5+6+5カラー) */
		image.pixel_stride = 2;
		result = USB_SUCCESS;
		break;
	case RAW_FRAME_UNCOMPRESSED_YCbCr:	/** 0x0c0005, Y->Cb->Cr, インターリーブ(色空間を別にすればY->U->Vと同じ並び) */
	case RAW_FRAME_UNCOMPRESSED_RGB:	/** 0x0e0005, 8ビットインターリーブRGB(24ビットカラー), RGB24 */
	case RAW_FRAME_UNCOMPRESSED_BGR:	/** 0x0f0005, 8ビットインターリーブBGR(24ビットカラー), BGR24 */
		image.pixel_stride = 3;
		result = USB_SUCCESS;
		break;
	case RAW_FRAME_UNCOMPRESSED_RGBX:	/** 0x100005, 8ビットインターリーブRGBX(32ビットカラー), RGBX32 */
	case RAW_FRAME_UNCOMPRESSED_XRGB:	/** 0x1a0005, 8ビットインターリーブXRGB(32ビットカラー), XRGB32 */
	case RAW_FRAME_UNCOMPRESSED_XBGR:	/** 0x1b0005, 8ビットインターリーブXBGR(32ビットカラー), XBGR32 */
	case RAW_FRAME_UNCOMPRESSED_BGRX:	/** 0x1c0005, 8ビットインターリーブBGRX(32ビットカラー), BGRX32 */
		image.pixel_stride = 4;
		result = USB_SUCCESS;
		break;
	// YUVプラナー/セミプラナー系
	case RAW_FRAME_UNCOMPRESSED_NV21:	/** 0x050005, YVU420 SemiPlanar(y->vu), YVU420sp */
		// y:  width x height
		// u:  width/2 x height/2
		// v:  width/2 x height/2
		image.ptr_y = data;
		image.ptr_v = data + plane_bytes;
		image.ptr_u = image.ptr_v + 1;
		image.stride_y = width;
		image.stride_u = image.stride_v = width; // width/2 + width/2 = width
		image.pixel_stride_y = 1;
		image.pixel_stride_u = image.pixel_stride_v = 2;
		result = USB_SUCCESS;
		break;
	case RAW_FRAME_UNCOMPRESSED_NV12:	/** 0x0b0005, YUV420 SemiPlanar(y->uv) NV21とU/Vが逆 */
		// y:  width x height
		// u:  width/2 x height/2
		// v:  width/2 x height/2
		image.ptr_y = data;
		image.ptr_u = data + plane_bytes;
		image.ptr_v = image.ptr_u + 1;
		image.stride_y = width;
		image.stride_u = image.stride_v = width; // width/2 + width/2 = width
		image.pixel_stride_y = 1;
		image.pixel_stride_u = image.pixel_stride_v = 2;
		result = USB_SUCCESS;
		break;
	case RAW_FRAME_UNCOMPRESSED_YV12:	/** 0x060005, YVU420 Planar(y->v->u), YVU420p, I420とU/Vが逆 */
		// y:  width x height
		// u:  width/2 x height/2
		// v:  width/2 x height/2
		image.ptr_y = data;
		image.ptr_v = data + plane_bytes;
		image.ptr_u = image.ptr_v + (width / 2) * (height / 2);
		image.stride_y = width;
		image.stride_v = image.stride_u = width / 2;
		image.pixel_stride_y = image.pixel_stride_u = image.pixel_stride_v = 1;
		result = USB_SUCCESS;
		break;
	case RAW_FRAME_UNCOMPRESSED_I420:	/** 0x070005, YUV420 Planar(y->u->v), YUV420p, YV12とU/Vが逆 */
		// y:  width x height
		// u:  width/2 x height/2
		// v:  width/2 x height/2
		image.ptr_y = data;
		image.ptr_u = data + plane_bytes;
		image.ptr_v = image.ptr_u + (width / 2) * (height / 2);
		image.stride_y = width;
		image.stride_u = image.stride_v = width / 2;
		image.pixel_stride_y = image.pixel_stride_u = image.pixel_stride_v = 1;
		result = USB_SUCCESS;
		break;
	case RAW_FRAME_UNCOMPRESSED_444p:	/** 0x110005, YUV444 Planar(y->u->v), YUV444p */
		// y:  width x height
		// u:  width x height
		// v:  width x height
		image.ptr_y = data;
		image.ptr_u = data + plane_bytes;
		image.ptr_v = data + plane_bytes * 2;
		image.stride_y = image.stride_u = image.stride_v = width;
		image.pixel_stride_y = image.pixel_stride_u = image.pixel_stride_v = 1;
		result = USB_SUCCESS;
		break;
	case RAW_FRAME_UNCOMPRESSED_444sp:	/** 0x120005, YUV444 semi Planar(y->uv), NV24/YUV444sp */
		// y:  width x height
		// u:  width x height
		// v:  width x height
		image.ptr_y = data;
		image.ptr_u = data + plane_bytes;
		image.ptr_v = image.ptr_u + 1;
		image.stride_y = width;
		image.stride_u = image.stride_v = width * 2;
		image.pixel_stride_y = 1;
		image.pixel_stride_u = image.pixel_stride_v = 2;
		result = USB_SUCCESS;
		break;
	case RAW_FRAME_UNCOMPRESSED_422p:	/** 0x130005, YUV422 Planar(y->u->v), YUV422p */
		// y:  width x height
		// u:  width/2 x height
		// v:  width/2 x height
		image.ptr_y = data;
		image.ptr_u = data + plane_bytes;
		image.ptr_v = data + (plane_bytes * 3) / 2;
		image.stride_y = width;
		image.stride_u = image.stride_v = width / 2;
		image.pixel_stride_y = image.pixel_stride_u = image.pixel_stride_v = 1;
		result = USB_SUCCESS;
		break;
	case RAW_FRAME_UNCOMPRESSED_422sp:	/** 0x140005, YUV422 SemiPlanar(y->uv), NV16/YUV422sp */
		// y:  width x height
		// u:  width/2 x height
		// v:  width/2 x height
		image.ptr_y = data;
		image.ptr_u = data + plane_bytes;
		image.ptr_v = image.ptr_u + 1;
		image.stride_y = width;
		image.stride_u = image.stride_v = width;
		image.pixel_stride_y = 1;
		image.pixel_stride_u = image.pixel_stride_v = 2;
		result = USB_SUCCESS;
		break;
	//--------------------------------------------------------------------------------
	// フレームベース
	case RAW_FRAME_MJPEG:				/** mjpeg */
	case RAW_FRAME_FRAME_BASED:			/** 不明なフレームベースのフレームフォーマット */
	case RAW_FRAME_MPEG2TS:				/** MPEG2TS */
	case RAW_FRAME_DV:					/** DV */
	case RAW_FRAME_FRAME_H264:			/** フレームベースのH.264 */
	case RAW_FRAME_FRAME_VP8:			/** フレームベースのVP8 */
	case RAW_FRAME_H264:				/** H.264単独フレーム */
	case RAW_FRAME_H264_SIMULCAST:		/** H.264 SIMULCAST */
	case RAW_FRAME_VP8:					/** VP8単独フレーム */
	case RAW_FRAME_VP8_SIMULCAST:		/** VP8 SIMULCAST */
	case RAW_FRAME_H265:				/** H.265単独フレーム */
		image.stride = 0;
		result = USB_SUCCESS;
		break;
	// その他未対応
	case RAW_FRAME_UNKNOWN:				/** 不明なフレームフォーマット */
	case RAW_FRAME_UNCOMPRESSED:		/** 0x000005 不明な非圧縮フレームフォーマット */
	case RAW_FRAME_STILL:				/** 0x000003 静止画 */
	case RAW_FRAME_UNCOMPRESSED_M420:	/** 0x0a0005, YUV420でY2の次に1/2UVが並ぶのを繰り返す, YYYY/YYYY/UVUV->YYYY/YYYY/UVUV->... @deprecated */
	case RAW_FRAME_UNCOMPRESSED_440p:	/** 0x150005, YUV440 Planar(y->u->v), YUV440p */
	case RAW_FRAME_UNCOMPRESSED_440sp:	/** 0x160005, YUV440 Planar(y->u->v), YUV440sp */
	case RAW_FRAME_UNCOMPRESSED_411p:	/** 0x170005, YUV411 Planar(y->u->v), YUV411p */
	case RAW_FRAME_UNCOMPRESSED_411sp:	/** 0x180005, YUV411 SemiPlanar(y->uv), YUV411sp */
	case RAW_FRAME_UNCOMPRESSED_YUV_ANY:	/** 0x190005, MJPEGからYUVプラナー系への変換先指定用、実際はYUVのプランナーフォーマットのいずれかになる */
	default:
		image.frame_type = RAW_FRAME_UNKNOWN;
		break;
	}

	RETURN(result, int);
}

#if defined(__ANDROID__)
/**
 * ハードウエアバッファをロックして映像データのポインターをVideoImage_tへ割り当てる
 * @param image
 * @param frame_type フレームタイプ
 * @param width 映像幅
 * @param height 映像高さ
 * @param graphicBuffer
 * @param lock_usage
 * @return
 */
int lock_and_assign(
	VideoImage_t &image,
	const raw_frame_t  &frame_type, const uint32_t &width, const uint32_t &height,
	AHardwareBuffer *graphicBuffer, const uint64_t &lock_usage) {

	ENTER();

	int result = USB_ERROR_NOT_SUPPORTED;

	memset(&image, 0, sizeof(VideoImage_t));
	image.frame_type = RAW_FRAME_UNKNOWN;

	if (LIKELY(graphicBuffer)) {
		if (is_hardware_buffer_supported_v29()) {
			LOGV("AAHardwareBuffer_lockPlanes");
			AHardwareBuffer_Desc desc;
			AHardwareBuffer_Planes info;
			AAHardwareBuffer_describe(graphicBuffer, &desc);
			AAHardwareBuffer_acquire(graphicBuffer);	// API>=26
			int r = AAHardwareBuffer_lockPlanes(graphicBuffer, lock_usage, -1, nullptr, &info);	// API>=29
			if (!r) {
				const auto planes = info.planes;
				image.frame_type = frame_type;
				image.width = width;
				image.height = height;
				image.stride = width;
				image.pixel_stride = 0;
				switch (frame_type) {
				case RAW_FRAME_UNCOMPRESSED_GRAY8:
					if (info.planeCount >= 1) {
						image.ptr_y = static_cast<uint8_t *>(planes[0].data);
						image.stride_y = planes[0].rowStride;
						image.pixel_stride_y = 1;
						image.actual_bytes = height * image.stride_y;
						result = USB_SUCCESS;
					}
					break;
				case RAW_FRAME_UNCOMPRESSED_NV21:	/** 0x050005, YVU420 SemiPlanar(y->vu), YVU420sp */ // 動作確認OK
					if (info.planeCount == 3) {
						image.ptr_y = static_cast<uint8_t *>(planes[0].data);
						image.ptr_v = static_cast<uint8_t *>(planes[1].data);
						image.ptr_u = static_cast<uint8_t *>(planes[2].data);
						image.stride_y = planes[0].rowStride;
						image.stride_v = planes[1].rowStride;
						image.stride_u = planes[2].rowStride;
						image.pixel_stride_y = 1;
						image.pixel_stride_u = image.pixel_stride_v = 2;
						image.actual_bytes = height * (image.stride_y + (image.stride_v + image.stride_u) / 2);
						result = USB_SUCCESS;
					}
					break;
				case RAW_FRAME_UNCOMPRESSED_NV12:	/** 0x0b0005, YUV420 SemiPlanar(y->uv) NV21とU/Vが逆 */
					if (info.planeCount == 3) {
						image.ptr_y = static_cast<uint8_t *>(planes[0].data);
						image.ptr_u = static_cast<uint8_t *>(planes[1].data);
						image.ptr_v = static_cast<uint8_t *>(planes[2].data);
						image.stride_y = planes[0].rowStride;
						image.stride_u = planes[1].rowStride;
						image.stride_v = planes[2].rowStride;
						image.pixel_stride_y = 1;
						image.pixel_stride_u = image.pixel_stride_v = 2;
						image.actual_bytes = height * (image.stride_y + (image.stride_v + image.stride_u) / 2);
						result = USB_SUCCESS;
					}
					break;
				case RAW_FRAME_UNCOMPRESSED_YV12:	/** 0x060005, YVU420 Planar(y->v->u), YVU420p, I420とU/Vが逆 */ // 動作確認OK
					if (info.planeCount == 3) {
						image.ptr_y = static_cast<uint8_t *>(planes[0].data);
						image.ptr_v = static_cast<uint8_t *>(planes[1].data);
						image.ptr_u = static_cast<uint8_t *>(planes[2].data);
						image.stride_y = planes[0].rowStride;
						image.stride_v = planes[1].rowStride;
						image.stride_u = planes[2].rowStride;
						image.pixel_stride_y = image.pixel_stride_u = image.pixel_stride_v = 1;
						image.actual_bytes = height * (image.stride_y + (image.stride_v + image.stride_u) / 2);
						result = USB_SUCCESS;
					}
					break;
				case RAW_FRAME_UNCOMPRESSED_I420:	/** 0x070005, YUV420 Planar(y->u->v), YUV420p, YV12とU/Vが逆 */ // 動作確認OK
					if (info.planeCount == 3) {
						image.ptr_y = static_cast<uint8_t *>(planes[0].data);
						image.ptr_u = static_cast<uint8_t *>(planes[1].data);
						image.ptr_v = static_cast<uint8_t *>(planes[2].data);
						image.stride_y = planes[0].rowStride;
						image.stride_u = planes[1].rowStride;
						image.stride_v = planes[2].rowStride;
						image.pixel_stride_y = image.pixel_stride_u = image.pixel_stride_v = 1;
						image.actual_bytes = height * (image.stride_y + (image.stride_v + image.stride_u) / 2);
						result = USB_SUCCESS;
					}
					break;
				case RAW_FRAME_UNCOMPRESSED_RGB565:	/** 0x0d0005, 8ビットインターリーブRGB(16ビット/5+6+5カラー) */
				case RAW_FRAME_UNCOMPRESSED_RGBP:	/** 0x090005, 16ビットインターリーブRGB(16ビット/5+6+5カラー), RGB565と並びが逆, RGB565 LE, BGR565 */
				case RAW_FRAME_UNCOMPRESSED_RGB:	/** 0x0e0005, 8ビットインターリーブRGB(24ビットカラー), RGB24 */
				case RAW_FRAME_UNCOMPRESSED_BGR:	/** 0x0f0005, 8ビットインターリーブBGR(24ビットカラー), BGR24 */
				case RAW_FRAME_UNCOMPRESSED_RGBX:	/** 0x100005, 8ビットインターリーブRGBX(32ビットカラー), RGBX32 */
				case RAW_FRAME_UNCOMPRESSED_XRGB:	/** 0x1a0005, 8ビットインターリーブXRGB(32ビットカラー), XRGB32 */
				case RAW_FRAME_UNCOMPRESSED_XBGR:	/** 0x1b0005, 8ビットインターリーブXBGR(32ビットカラー), XBGR32 */
				case RAW_FRAME_UNCOMPRESSED_BGRX:	/** 0x1c0005, 8ビットインターリーブBGRX(32ビットカラー), BGRX32 */
				default:
					if (info.planeCount == 1) {
						// RGB系インターリーブ形式
						image.ptr = static_cast<uint8_t *>(planes[0].data);
						// planes[0].rowStrideの値はバイト数なのでpixelStrideで割ってピクセル数にする
						image.stride = planes[0].rowStride / planes[0].pixelStride;
						image.pixel_stride = planes[0].pixelStride;
						image.actual_bytes = height * image.stride * image.pixel_stride;
						result = USB_SUCCESS;
					} else {
						result = USB_ERROR_NOT_SUPPORTED;
					}
					break;
				}
				if (UNLIKELY(result)) {
					image.frame_type = RAW_FRAME_UNKNOWN;
					AAHardwareBuffer_unlock(graphicBuffer, nullptr);	// API>=26
					AAHardwareBuffer_release(graphicBuffer);			// API>=26
				}
			}	// if (!r)
		}	// if (is_hardware_buffer_supported_v29())

		if (result && init_hardware_buffer()) {
			// フォールバック用を兼ねているけど一度でもAAHardwareBuffer_lockPlanesを呼んでると
			// AAHardwareBuffer_lockが必ず失敗するみたい？
			uint8_t *ptr = nullptr;
			AAHardwareBuffer_acquire(graphicBuffer);	// API>=26
			int r = AAHardwareBuffer_lock(graphicBuffer, lock_usage, -1, nullptr, (void **)&ptr);	// API>=26
			if (!r && ptr) {
				// プラナー/セミプラナーの時もyプレーンとuv/u,vプレーンが連続している前提で割り振る
				result = assign(image,
					frame_type, width, height,
					ptr, get_pixel_bytes(frame_type).frame_bytes(width, height));
				if (UNLIKELY(result)) {
					LOGW("may not support now for frame type 0x%08x, unlock AHardwareBuffer", frame_type);
					AAHardwareBuffer_unlock(graphicBuffer, nullptr);	// API>=26
					AAHardwareBuffer_release(graphicBuffer);			// API>=26
					image.frame_type = RAW_FRAME_UNKNOWN;
				}
			}
		}	// if (init_hardware_buffer())
	}

	RETURN(result, int);
}

#endif

}	// namespace serenegiant::usb
