/*
 * aAndUsb
 * Copyright (c) 2014-2023 saki t_saki@serenegiant.com
 * Distributed under the terms of the GNU Lesser General Public License (LGPL v3.0) License.
 * License details are in the file license.txt, distributed as part of this software.
 */

#ifndef AANDUSB_VIDEO_H
#define AANDUSB_VIDEO_H

// FIXME このヘッダーファイル内の定義は全部はいらないので必要なUVC_VS_FORMAT_XXX等だけこのファイル内で定義するように変更する
#include <linux/usb/video.h>
#include "core/core.h"

namespace serenegiant::core {

// Transferの個数
#define NUM_TRANSFER_BUFS (10)	// 大きすぎても小さすぎても動かない

#define DEFAULT_PREVIEW_WIDTH (640)
#define DEFAULT_PREVIEW_HEIGHT (480)
#define DEFAULT_PREVIEW_FPS_MIN (1)
#define DEFAULT_PREVIEW_FPS_MAX (61)

//--------------------------------------------------------------------------------
#define UVC_VS_FORMAT_H264 (0x13)
#define UVC_VS_FRAME_H264 (0x14)
#define UVC_VS_FORMAT_H264_SIMULCAST (0x15)
#define UVC_VS_FORMAT_VP8 (0x16)
#define UVC_VS_FRAME_VP8 (0x17)
#define UVC_VS_FORMAT_VP8_SIMULCAST (0x18)
#define UVC_VS_FRAME_H265 (0x19)

/**
 * 非圧縮フレームフォーマットの種類
 * FIXME 整理する
 */
typedef enum uncompress_frame_format {
	/** 不明な非圧縮フレームフォーマット */
	UNCOMPRESSED_FRAME_UNKNOWN = 0,
	/** 1 YUY2/YUYV/V422/YUV422, インターリーブ */
	UNCOMPRESSED_FRAME_YUYV, // 1
	/** 2 YUYVの順序違い、インターリーブ */
	UNCOMPRESSED_FRAME_UYVY, // 2
	/** 3 8ビットグレースケール, Y800/Y8/I400 */
	UNCOMPRESSED_FRAME_GRAY8, // 3
	/** 4 8ビットグレースケール, ベイヤー配列 */
	UNCOMPRESSED_FRAME_BY8, // 4
	/** 5 YVU420 SemiPlanar(y->vu), YVU420sp */
	UNCOMPRESSED_FRAME_NV21, // 5
	/** 6 YVU420 Planar(y->v->u), YVU420p, I420とU/Vが逆 */
	UNCOMPRESSED_FRAME_YV12, // 6
	/** 7 YUV420 Planar(y->u->v), YUV420p, YV12とU/Vが逆 */
	UNCOMPRESSED_FRAME_I420, // 7
	/** 8 16ビットグレースケール */
	UNCOMPRESSED_FRAME_Y16, // 8
	/** 9 16ビットインターリーブRGB, RGB565と並びが逆, RGB565 LE, BGR565 */
	UNCOMPRESSED_FRAME_RGBP, // 9
	/** 10 YUV420でY2の次に1/2UVが並ぶのを繰り返す, YYYY/YYYY/UVUV->YYYY/YYYY/UVUV->..., @deprecated */
	UNCOMPRESSED_FRAME_M420, // 0x0a
	/** 11 YUV420 SemiPlanar(y->uv) NV21とU/Vの並びが逆 */
	UNCOMPRESSED_FRAME_NV12, // 0x0b
	/** 12 Y->Cb->Cr, インターリーブ(色空間を別にすればY->U->Vと同じ並び) */
	UNCOMPRESSED_FRAME_YCbCr, // 0x0c
	/** 13 8ビットインターリーブRGB(16ビット/5+6+5カラー) */
	UNCOMPRESSED_FRAME_RGB565, // 0x0d
	/** 14 8ビットインターリーブRGB(24ビットカラー), RGB24 */
	UNCOMPRESSED_FRAME_RGB, // 0x0e
	/** 15 8ビットインターリーブBGR(24ビットカラー), BGR24 */
	UNCOMPRESSED_FRAME_BGR, // 0x0f
	/** 16 8ビットインターリーブRGBX(32ビットカラー), RGBX32 */
	UNCOMPRESSED_FRAME_RGBX, // 0x10
	/** 17 YUV444 Planar(y->u->v) */
	UNCOMPRESSED_FRAME_444p, // 0x11
	/** 18 YUV444 SemiPlanar(y->uv), YUV444sp */
	UNCOMPRESSED_FRAME_444sp, // 0x12
	/** 19 YUV422 Planar(y->u->v), YUV422p */
	UNCOMPRESSED_FRAME_422p, // 0x13
	/** 20 YUV422 SemiPlanar(y->uv), NV16/YUV422sp */
	UNCOMPRESSED_FRAME_422sp, // 0x14
	/** 21 YUV440 Planar(y->u->v), YUV440p */
	UNCOMPRESSED_FRAME_440p, // 0x15
	/** 22 YUV440 SemiPlanar(y->uv), YUV440sp */
	UNCOMPRESSED_FRAME_440sp, // 0x16
	/** 23 YUV411 Planar(y->u->v), YUV411p */
	UNCOMPRESSED_FRAME_411p, // 0x17
	/** 24 YUV411 SemiPlanar(y->uv), YUV411sp */
	UNCOMPRESSED_FRAME_411sp, // 0x18
	/** 25 MJPEGからYUVプラナー系への変換先指定用、実際はYUVのプランナーフォーマットのいずれかになる */
	UNCOMPRESSED_FRAME_YUV_ANY, // 0x18
	/** 26 8ビットインターリーブXRGB(32ビットカラー), XRGB32 */
	UNCOMPRESSED_FRAME_XRGB, // 0x19
	/** 26 8ビットインターリーブXBGR(32ビットカラー), XBGR32 */
	UNCOMPRESSED_FRAME_XBGR, // 0x1a
	/** 26 8ビットインターリーブBGRX(32ビットカラー), BGRX32 */
	UNCOMPRESSED_FRAME_BGRX, // 0x1b
	/** 非圧縮フレームフォーマットの個数 */
	UNCOMPRESSED_FRAME_COUNTS,
} uncompress_frame_format_t;

/**
 * カメラからの(生)フレームフォーマット
 * 下2バイトはUVC_VS_FRAME_XXと同じ
 * FIXME 整理する
 */
typedef enum raw_frame {
	/** 不明なフレームフォーマット */
	RAW_FRAME_UNKNOWN				= 0,
	/** 0x000003 静止画 */
	RAW_FRAME_STILL					= (0 << 16) | UVC_VS_STILL_IMAGE_FRAME,
	/** 0x000005 不明な非圧縮フレームフォーマット */
	RAW_FRAME_UNCOMPRESSED			= (UNCOMPRESSED_FRAME_UNKNOWN << 16) | UVC_VS_FRAME_UNCOMPRESSED,
	/** 0x010005, YUY2/YUYV/V422/YUV422, インターリーブ */
	RAW_FRAME_UNCOMPRESSED_YUYV		= (UNCOMPRESSED_FRAME_YUYV << 16) | UVC_VS_FRAME_UNCOMPRESSED,
	/** 0x020005, YUYVの順序違い、インターリーブ */
	RAW_FRAME_UNCOMPRESSED_UYVY		= (UNCOMPRESSED_FRAME_UYVY << 16) | UVC_VS_FRAME_UNCOMPRESSED,
	/** 0x030005, 8ビットグレースケール, Y800/Y8/I400 */
	RAW_FRAME_UNCOMPRESSED_GRAY8	= (UNCOMPRESSED_FRAME_GRAY8 << 16) | UVC_VS_FRAME_UNCOMPRESSED,
	/** 0x040005, 8ビットグレースケール, ベイヤー配列 */
	RAW_FRAME_UNCOMPRESSED_BY8		= (UNCOMPRESSED_FRAME_BY8 << 16) | UVC_VS_FRAME_UNCOMPRESSED,
	/** 0x050005, YVU420 SemiPlanar(y->vu), YVU420sp */
	RAW_FRAME_UNCOMPRESSED_NV21		= (UNCOMPRESSED_FRAME_NV21 << 16) | UVC_VS_FRAME_UNCOMPRESSED,
	/** 0x060005, YVU420 Planar(y->v->u), YVU420p, I420とU/Vが逆 */
	RAW_FRAME_UNCOMPRESSED_YV12		= (UNCOMPRESSED_FRAME_YV12 << 16) | UVC_VS_FRAME_UNCOMPRESSED,
	/** 0x070005, YUV420 Planar(y->u->v), YUV420p, YV12とU/Vが逆, YU12 */
	RAW_FRAME_UNCOMPRESSED_I420		= (UNCOMPRESSED_FRAME_I420 << 16) | UVC_VS_FRAME_UNCOMPRESSED,
	/** 0x080005, 16ビットグレースケール */
	RAW_FRAME_UNCOMPRESSED_Y16		= (UNCOMPRESSED_FRAME_Y16 << 16) | UVC_VS_FRAME_UNCOMPRESSED,
	/** 0x090005, 16ビットインターリーブRGB(16ビット/5+6+5カラー), RGB565と並びが逆, RGB565 LE, BGR565 */
	RAW_FRAME_UNCOMPRESSED_RGBP		= (UNCOMPRESSED_FRAME_RGBP << 16) | UVC_VS_FRAME_UNCOMPRESSED,
	/** 0x0a0005, YUV420でY2の次に1/2UVが並ぶのを繰り返す, YYYY/YYYY/UVUV->YYYY/YYYY/UVUV->... @deprecated */
	RAW_FRAME_UNCOMPRESSED_M420		= (UNCOMPRESSED_FRAME_M420 << 16) | UVC_VS_FRAME_UNCOMPRESSED,
	/** 0x0b0005, YUV420 SemiPlanar(y->uv) NV21とU/Vが逆 */
	RAW_FRAME_UNCOMPRESSED_NV12		= (UNCOMPRESSED_FRAME_NV12 << 16) | UVC_VS_FRAME_UNCOMPRESSED,
	/** 0x0c0005, Y->Cb->Cr, インターリーブ(色空間を別にすればY->U->Vと同じ並び) */
	RAW_FRAME_UNCOMPRESSED_YCbCr	= (UNCOMPRESSED_FRAME_YCbCr << 16) | UVC_VS_FRAME_UNCOMPRESSED,
	/** 0x0d0005, 8ビットインターリーブRGB(16ビット/5+6+5カラー) */
	RAW_FRAME_UNCOMPRESSED_RGB565	= (UNCOMPRESSED_FRAME_RGB565 << 16) | UVC_VS_FRAME_UNCOMPRESSED,
	/** 0x0e0005, 8ビットインターリーブRGB(24ビットカラー), RGB24 */
	RAW_FRAME_UNCOMPRESSED_RGB		= (UNCOMPRESSED_FRAME_RGB << 16) | UVC_VS_FRAME_UNCOMPRESSED,
	/** 0x0f0005, 8ビットインターリーブBGR(24ビットカラー), BGR24 */
	RAW_FRAME_UNCOMPRESSED_BGR		= (UNCOMPRESSED_FRAME_BGR << 16) | UVC_VS_FRAME_UNCOMPRESSED,
	/** 0x100005, 8ビットインターリーブRGBX(32ビットカラー), RGBX32 */
	RAW_FRAME_UNCOMPRESSED_RGBX		= (UNCOMPRESSED_FRAME_RGBX << 16) | UVC_VS_FRAME_UNCOMPRESSED,
	/** 0x110005, YUV444 Planar(y->u->v), YUV444p */
	RAW_FRAME_UNCOMPRESSED_444p		= (UNCOMPRESSED_FRAME_444p << 16) | UVC_VS_FRAME_UNCOMPRESSED,
	/** 0x120005, YUV444 semi Planar(y->uv), NV24/YUV444sp */
	RAW_FRAME_UNCOMPRESSED_444sp	= (UNCOMPRESSED_FRAME_444sp << 16) | UVC_VS_FRAME_UNCOMPRESSED,
	/** 0x130005, YUV422 Planar(y->u->v), YUV422p, YV16, YU16 */
	RAW_FRAME_UNCOMPRESSED_422p		= (UNCOMPRESSED_FRAME_422p << 16) | UVC_VS_FRAME_UNCOMPRESSED,
	/** 0x140005, YUV422 SemiPlanar(y->uv), NV16/YUV422sp */
	RAW_FRAME_UNCOMPRESSED_422sp	= (UNCOMPRESSED_FRAME_422sp << 16) | UVC_VS_FRAME_UNCOMPRESSED,
	/** 0x150005, YUV440 Planar(y->u->v), YUV440p */
	RAW_FRAME_UNCOMPRESSED_440p		= (UNCOMPRESSED_FRAME_440p << 16) | UVC_VS_FRAME_UNCOMPRESSED,
	/** 0x160005, YUV440 Planar(y->u->v), YUV440sp */
	RAW_FRAME_UNCOMPRESSED_440sp	= (UNCOMPRESSED_FRAME_440sp << 16) | UVC_VS_FRAME_UNCOMPRESSED,
	/** 0x170005, YUV411 Planar(y->u->v), YUV411p */
	RAW_FRAME_UNCOMPRESSED_411p		= (UNCOMPRESSED_FRAME_411p << 16) | UVC_VS_FRAME_UNCOMPRESSED,
	/** 0x180005, YUV411 SemiPlanar(y->uv), YUV411sp */
	RAW_FRAME_UNCOMPRESSED_411sp	= (UNCOMPRESSED_FRAME_411sp << 16) | UVC_VS_FRAME_UNCOMPRESSED,
	/** 0x190005, MJPEGからYUVプラナー系への変換先指定用、実際はYUVのプランナーフォーマットのいずれかになる */
	RAW_FRAME_UNCOMPRESSED_YUV_ANY	= (UNCOMPRESSED_FRAME_YUV_ANY << 16) | UVC_VS_FRAME_UNCOMPRESSED,
	/** 0x1a0005, 8ビットインターリーブXRGB(32ビットカラー), XRGB32 */
	RAW_FRAME_UNCOMPRESSED_XRGB		= (UNCOMPRESSED_FRAME_XRGB << 16) | UVC_VS_FRAME_UNCOMPRESSED,
	/** 0x1b0005, 8ビットインターリーブXBGR(32ビットカラー), XBGR32 */
	RAW_FRAME_UNCOMPRESSED_XBGR		= (UNCOMPRESSED_FRAME_XBGR << 16) | UVC_VS_FRAME_UNCOMPRESSED,
	/** 0x1c0005, 8ビットインターリーブBGRX(32ビットカラー), BGRX32 */
	RAW_FRAME_UNCOMPRESSED_BGRX		= (UNCOMPRESSED_FRAME_BGRX << 16) | UVC_VS_FRAME_UNCOMPRESSED,
//--------------------------------------------------------------------------------
	/** mjpeg */
	RAW_FRAME_MJPEG					= (0 << 16) | UVC_VS_FRAME_MJPEG,			// 0x000007
	/** 不明なフレームベースのフレームフォーマット */
	RAW_FRAME_FRAME_BASED			= (0 << 16) | UVC_VS_FRAME_FRAME_BASED,		// 0x000011
	/** MPEG2TS */
	RAW_FRAME_MPEG2TS				= (1 << 16) | UVC_VS_FRAME_FRAME_BASED,		// 0x010011
	/** DV */
	RAW_FRAME_DV					= (2 << 16) | UVC_VS_FRAME_FRAME_BASED,		// 0x020011
	/** フレームベースのH.264 */
	RAW_FRAME_FRAME_H264			= (3 << 16) | UVC_VS_FRAME_FRAME_BASED,		// 0x030011
	/** フレームベースのVP8 */
	RAW_FRAME_FRAME_VP8				= (4 << 16) | UVC_VS_FRAME_FRAME_BASED,		// 0x040011
	/** 不明なストリームベースのフレームフォーマット */
	RAW_FRAME_STREAM_BASED			= (0 << 16) | UVC_VS_FORMAT_STREAM_BASED,	// 0x000012
	/** H.264単独フレーム */
	RAW_FRAME_H264					= (0 << 16) | UVC_VS_FRAME_H264,			// 0x000014
	/** H.264 SIMULCAST */
	RAW_FRAME_H264_SIMULCAST		= (1 << 16) | UVC_VS_FRAME_H264,			// 0x010014
	/** VP8単独フレーム */
	RAW_FRAME_VP8					= (0 << 16) | UVC_VS_FRAME_VP8,				// 0x000017
	/** VP8 SIMULCAST */
	RAW_FRAME_VP8_SIMULCAST			= (1 << 16) | UVC_VS_FRAME_VP8,				// 0x010017,
	/** H.265単独フレーム */
	RAW_FRAME_H265					= (0 << 16) | UVC_VS_FRAME_H265,			// 0x000019
} raw_frame_t;

#define RAW_FRAME_UNCOMPRESSED_YV21 (RAW_FRAME_UNCOMPRESSED_I420)

typedef enum dct_mode {
	DCT_MODE_ISLOW,
	DCT_MODE_IFAST,
	DCT_MODE_FLOAT,
} dct_mode_t;

#define DEFAULT_DCT_MODE DCT_MODE_IFAST

/**
 * 1ピクセルあたりのバイト数を保持するためのホルダークラス
 */
class PixelBytes {
public:
	/** 分子 */
	int32_t _numerator;
	/** 分母 */
	int32_t _denominator;

	/**
	 * コンストラクタ
	 * @param numerator
	 * @param denominator
	 */
	PixelBytes(const int32_t numerator = 0, const int32_t denominator = 1)
	:	_numerator(numerator),
		_denominator(denominator)
	{
	}
	/**
	 * コピーコンストラクタ
	 * @param src
	 */
	PixelBytes(const PixelBytes &src)
	:	_numerator(src._numerator),
		_denominator(src._denominator)
	{
	}
	/**
	 * ムーブコンストラクタ
	 * @param src
	 */
	PixelBytes(const PixelBytes &&src)
	:	_numerator(src._numerator),
		_denominator(src._denominator)
	{
	}
	/**
	 * コピー代入演算子
	 * @param src
	 * @return
	 */
	PixelBytes &operator=(const PixelBytes &src) {
		if (this != &src) {
			_numerator = src._numerator;
			_denominator = src._denominator;
		}
		return *this;
	}

	/**
	 * ムーブ代入演算子
	 * @param src
	 * @return
	 */
	PixelBytes &operator=(const PixelBytes &&src) {
		if (this != &src) {
			_numerator = src._numerator;
			_denominator = src._denominator;
		}
		return *this;
	}

	/**
	 * 分子を取得
	 * @return
	 */
	inline const int32_t &numerator() const {return _numerator; };
	/**
	 * 分母を取得
	 * @return
	 */
	inline const int32_t &denominator() const {return _denominator; };
	/**
	 * 1フレームあたりの映像データサイズを計算
	 * @param width
	 * @param height
	 * @return
	 */
	inline uint32_t frame_bytes(const uint32_t &width, const uint32_t &height) const {
		return _denominator ? (width * height * _numerator) / _denominator : 0;
	}
};

}	// namespace serenegiant::core

#endif //AANDUSB_VIDEO_H
