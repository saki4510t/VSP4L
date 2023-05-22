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

#include "libyuv.h"

#include "utilbase.h"
#include "binutils.h"
// core
#include "core/video_frame_base.h"
#include "core/video_converter.h"
#if defined(__ANDROID__)
#include "core/video_frame_hw_buffer.h"
#endif
// usb
#include "usb/aandusb.h"
// uvc
#include "uvc/aanduvc.h"

namespace serenegiant::core {

static inline uint8_t sat(int i) {
	return (uint8_t) (i >= 255 ? 255 : (i < 0 ? 0 : i));
}

#define YCbCr_YUYV_2(YCbCr, yuyv) \
	{ \
		*((yuyv)++) = *((YCbCr)+0); \
		*((yuyv)++) = (*((YCbCr)+1) + *((YCbCr)+4)) >> 1u; \
		*((yuyv)++) = *((YCbCr)+3); \
		*((yuyv)++) = (*((YCbCr)+2) + *((YCbCr)+5)) >> 1u; \
	}

#define PIXEL_RGB565		2
#define PIXEL_UYVY			2
#define PIXEL_YUYV			2
#define PIXEL_RGB			3
#define PIXEL_BGR			3
#define PIXEL_RGBX			4

#define PIXEL2_RGB565		(PIXEL_RGB565 * 2)
#define PIXEL2_UYVY			(PIXEL_UYVY * 2)
#define PIXEL2_YUYV			(PIXEL_YUYV * 2)
#define PIXEL2_RGB			(PIXEL_RGB * 2)
#define PIXEL2_BGR			(PIXEL_BGR * 2)
#define PIXEL2_RGBX			(PIXEL_RGBX * 2)

#define PIXEL4_RGB565		(PIXEL_RGB565 * 4)
#define PIXEL4_UYVY			(PIXEL_UYVY * 4)
#define PIXEL4_YUYV			(PIXEL_YUYV * 4)
#define PIXEL4_RGB			(PIXEL_RGB * 4)
#define PIXEL4_BGR			(PIXEL_BGR * 4)
#define PIXEL4_RGBX			(PIXEL_RGBX * 4)

#define PIXEL8_RGB565		(PIXEL_RGB565 * 8)
#define PIXEL8_UYVY			(PIXEL_UYVY * 8)
#define PIXEL8_YUYV			(PIXEL_YUYV * 8)
#define PIXEL8_RGB			(PIXEL_RGB * 8)
#define PIXEL8_BGR			(PIXEL_BGR * 8)
#define PIXEL8_RGBX			(PIXEL_RGBX * 8)

#define PIXEL16_RGB565		(PIXEL_RGB565 * 16)
#define PIXEL16_UYVY		(PIXEL_UYVY * 16)
#define PIXEL16_YUYV		(PIXEL_YUYV * 16)
#define PIXEL16_RGB			(PIXEL_RGB * 16)
#define PIXEL16_BGR			(PIXEL_BGR * 16)
#define PIXEL16_RGBX		(PIXEL_RGBX * 16)

#define RGB2RGBX_2(prgb, prgbx, ax, bx) { \
		(prgbx)[(bx)+0] = (prgb)[(ax)+0]; \
		(prgbx)[(bx)+1] = (prgb)[(ax)+1]; \
		(prgbx)[(bx)+2] = (prgb)[(ax)+2]; \
		(prgbx)[(bx)+3] = 0xff; \
		(prgbx)[(bx)+4] = (prgb)[(ax)+3]; \
		(prgbx)[(bx)+5] = (prgb)[(ax)+4]; \
		(prgbx)[(bx)+6] = (prgb)[(ax)+5]; \
		(prgbx)[(bx)+7] = 0xff; \
	}

#define RGB2RGBX_4(prgb, prgbx, ax, bx) \
	RGB2RGBX_2(prgb, prgbx, ax, bx) \
	RGB2RGBX_2(prgb, prgbx, (ax) + PIXEL2_RGB, (bx) + PIXEL2_RGBX);
#define RGB2RGBX_8(prgb, prgbx, ax, bx) \
	RGB2RGBX_4(prgb, prgbx, ax, bx) \
	RGB2RGBX_4(prgb, prgbx, (ax) + PIXEL4_RGB, (bx) + PIXEL4_RGBX);
#define RGB2RGBX_16(prgb, prgbx, ax, bx) \
	RGB2RGBX_8(prgb, prgbx, (ax), (bx)) \
	RGB2RGBX_8(prgb, prgbx, (ax) + PIXEL8_RGB, (bx) +PIXEL8_RGBX);

/**
 * RGB888 => RGBX8888
 * RGB888は1ピクセル3バイトで遅くなるので可能な限り使わない方がいい
 * @param src
 * @param dst
 * @return
 */
static int rgb2rgbx(const IVideoFrame &src, IVideoFrame &dst) {
	if (UNLIKELY(src.frame_type() != RAW_FRAME_UNCOMPRESSED_RGB))
		return USB_ERROR_INVALID_PARAM;

	if (UNLIKELY(dst.resize(src, RAW_FRAME_UNCOMPRESSED_RGBX)))
		return USB_ERROR_NO_MEM;

	const uint8_t *src_ptr = src.frame();
	const uint8_t *src_end = src_ptr + src.size() - PIXEL8_RGB;
	uint8_t *dst_ptr = dst.frame();
	const uint8_t *dst_end = dst_ptr + dst.size() - PIXEL8_RGBX;
	const auto in_step = src.step();
	const auto out_step = dst.step();

	// RGB888 to RGBX8888
	if (in_step && out_step && (in_step != out_step)) {
		const auto hh = src.height() < dst.height() ? src.height() : dst.height();
		const auto ww = src.width() < dst.width() ? src.width() : dst.width();
		for (int h = 0; h < hh; h++) {
			int w = 0;
			src_ptr = &src[in_step * h];
			dst_ptr = &dst[out_step * h];
			for (; (dst_ptr <= dst_end) && (src_ptr <= src_end) && (w < ww) ;) {
				RGB2RGBX_8(src_ptr, dst_ptr, 0, 0);

				src_ptr += PIXEL8_RGB;
				dst_ptr += PIXEL8_RGBX;
				w += 8;
			}
		}
	} else {
		// compressed format? XXX どちらか一方のstepがwidthと異なっていればクラッシュするかも
		for (; (dst_ptr <= dst_end) && (src_ptr <= src_end) ;) {
			RGB2RGBX_8(src_ptr, dst_ptr, 0, 0);

			src_ptr += PIXEL8_RGB;
			dst_ptr += PIXEL8_RGBX;
		}
	}
	return USB_SUCCESS;
}

// prgb565[0] = ((g << 3) & 0b11100000) | ((b >> 3) & 0b00011111);	// low byte
// prgb565[1] = ((r & 0b11111000) | ((g >> 5) & 0b00000111)); 		// high byte
#define RGB2RGB565_2(prgb, prgb565, ax, bx) { \
		(prgb565)[(bx)+0] = (((prgb)[(ax)+1] << 3) & 0b11100000) | (((prgb)[(ax)+2] >> 3u) & 0b00011111); \
		(prgb565)[(bx)+1] = (((prgb)[(ax)+0] & 0b11111000) | (((prgb)[(ax)+1] >> 5u) & 0b00000111)); \
		(prgb565)[(bx)+2] = (((prgb)[(ax)+4] << 3) & 0b11100000) | (((prgb)[(ax)+5] >> 3u) & 0b00011111); \
		(prgb565)[(bx)+3] = (((prgb)[(ax)+3] & 0b11111000) | (((prgb)[(ax)+4] >> 5u) & 0b00000111)); \
    }
#define RGB2RGB565_4(prgb, prgb565, ax, bx) \
	RGB2RGB565_2(prgb, prgb565, (ax), (bx)) \
	RGB2RGB565_2(prgb, prgb565, (ax) + PIXEL2_RGB, (bx) + PIXEL2_RGB565);
#define RGB2RGB565_8(prgb, prgb565, ax, bx) \
	RGB2RGB565_4(prgb, prgb565, (ax), (bx)) \
	RGB2RGB565_4(prgb, prgb565, (ax) + PIXEL4_RGB, (bx) + PIXEL4_RGB565);
#define RGB2RGB565_16(prgb, prgb565, ax, bx) \
	RGB2RGB565_8(prgb, prgb565, (ax), (bx)) \
	RGB2RGB565_8(prgb, prgb565, (ax) + PIXEL8_RGB, (bx) + PIXEL8_RGB565);

// RGB888 => RGB565
// RGB888は1ピクセル3バイトで遅くなるので可能な限り使わない方がいい
static int rgb2rgb565(const IVideoFrame &src, IVideoFrame &dst) {
	if (UNLIKELY(src.frame_type() != RAW_FRAME_UNCOMPRESSED_RGB))
		return USB_ERROR_INVALID_PARAM;

	if (UNLIKELY(dst.resize(src, RAW_FRAME_UNCOMPRESSED_RGB565)))
		return USB_ERROR_NO_MEM;

	const uint8_t *src_ptr = src.frame();
	const uint8_t *src_end = src_ptr + src.size() - PIXEL8_RGB;
	uint8_t *dst_ptr = dst.frame();
	const uint8_t *dst_end = dst_ptr + dst.size() - PIXEL8_RGB565;
	const auto in_step = src.step();
	const auto out_step = dst.step();

	// RGB888 to RGB565
	if (in_step && out_step && (in_step != out_step)) {
		// ストライドが異なる時
		const auto hh = src.height() < dst.height() ? src.height() : dst.height();
		const auto ww = src.width() < dst.width() ? src.width() : dst.width();
		for (int h = 0; h < hh; h++) {
			int w = 0;
			src_ptr = &src[in_step * h];
			dst_ptr = &dst[out_step * h];
			for (; (dst_ptr <= dst_end) && (src_ptr <= src_end) && (w < ww) ;) {
				RGB2RGB565_8(src_ptr, dst_ptr, 0, 0);

				src_ptr += PIXEL8_RGB;
				dst_ptr += PIXEL8_RGB565;
				w += 8;
			}
		}
	} else {
		// compressed format? XXX どちらか一方のstepがwidthと異なっていればクラッシュするかも
		for (; (dst_ptr <= dst_end) && (src_ptr <= src_end) ;) {
			RGB2RGB565_8(src_ptr, dst_ptr, 0, 0);

			src_ptr += PIXEL8_RGB;
			dst_ptr += PIXEL8_RGB565;
		}
	}
	return USB_SUCCESS;
}

#define IYUYV2RGB_2(pyuv, prgb, ax, bx) { \
		const int d1 = (pyuv)[(ax)+1]; \
		const int d3 = (pyuv)[(ax)+3]; \
		const int r = (22987 * (d3 - 128)) >> 14u; \
		const int g = (-5636 * (d1 - 128) - 11698 * (d3 - 128)) >> 14u; \
		const int b = (29049 * (d1 - 128)) >> 14u; \
		const int y0 = (pyuv)[(ax)+0]; \
		(prgb)[(bx)+0] = sat(y0 + r); \
		(prgb)[(bx)+1] = sat(y0 + g); \
		(prgb)[(bx)+2] = sat(y0 + b); \
		const int y2 = (pyuv)[(ax)+2]; \
		(prgb)[(bx)+3] = sat(y2 + r); \
		(prgb)[(bx)+4] = sat(y2 + g); \
		(prgb)[(bx)+5] = sat(y2 + b); \
    }
#define IYUYV2RGB_4(pyuv, prgb, ax, bx) \
	IYUYV2RGB_2(pyuv, prgb, (ax), (bx)) \
	IYUYV2RGB_2(pyuv, prgb, (ax) + PIXEL2_YUYV, (bx) + PIXEL2_RGB)
#define IYUYV2RGB_8(pyuv, prgb, ax, bx) \
	IYUYV2RGB_4(pyuv, prgb, (ax), (bx)) \
	IYUYV2RGB_4(pyuv, prgb, (ax) + PIXEL4_YUYV, (bx) + PIXEL4_RGB)
#define IYUYV2RGB_16(pyuv, prgb, ax, bx) \
	IYUYV2RGB_8(pyuv, prgb, (ax), (bx)) \
	IYUYV2RGB_8(pyuv, prgb, (ax) + PIXEL8_YUYV, (bx) + PIXEL8_RGB)

// YUYV => RGB888
// RGB888は1ピクセル3バイトで遅くなるので可能な限り使わない方がいい
static int yuyv2rgb(const IVideoFrame &src, IVideoFrame &dst) {
	if (UNLIKELY(src.frame_type() != RAW_FRAME_UNCOMPRESSED_YUYV))
		return USB_ERROR_INVALID_PARAM;

	if (UNLIKELY(dst.resize(src, RAW_FRAME_UNCOMPRESSED_RGB)))
		return USB_ERROR_NO_MEM;

	const uint8_t *src_yuv = src.frame();
	const uint8_t *src_end = src_yuv + src.size() - PIXEL8_YUYV;
	uint8_t *dst_ptr = dst.frame();
	const uint8_t *dst_end = dst_ptr + dst.size() - PIXEL8_RGB;
	const auto in_step = src.step();
	const auto out_step = dst.step();

	if (in_step && out_step && (in_step != out_step)) {
		const auto hh = src.height() < dst.height() ? src.height() : dst.height();
		const auto ww = src.width() < dst.width() ? src.width() : dst.width();
		for (int h = 0; h < hh; h++) {
			int w = 0;
			src_yuv = &src[in_step * h];
			dst_ptr = &dst[out_step * h];
			for (; (dst_ptr <= dst_end) && (src_yuv <= src_end) && (w < ww) ;) {
				IYUYV2RGB_8(src_yuv, dst_ptr, 0, 0);

				dst_ptr += PIXEL8_RGB;
				src_yuv += PIXEL8_YUYV;
				w += 8;
			}
		}
	} else {
		// compressed format? XXX どちらか一方のstepがwidthと異なっていればクラッシュするかも
		for (; (dst_ptr <= dst_end) && (src_yuv <= src_end) ;) {
			IYUYV2RGB_8(src_yuv, dst_ptr, 0, 0);

			dst_ptr += PIXEL8_RGB;
			src_yuv += PIXEL8_YUYV;
		}
	}
	return USB_SUCCESS;
}

/**
 * YUYV => RGB565
 * @param src
 * @param dst
 * @return
 */
static int yuyv2rgb565(const IVideoFrame &src, IVideoFrame &dst) {
	if (UNLIKELY(src.frame_type() != RAW_FRAME_UNCOMPRESSED_YUYV))
		return USB_ERROR_INVALID_PARAM;

	if (UNLIKELY(dst.resize(src, RAW_FRAME_UNCOMPRESSED_RGB565)))
		return USB_ERROR_NO_MEM;

	const uint8_t *src_ptr = src.frame();
	const uint8_t *src_end = src_ptr + src.size() - PIXEL8_YUYV;
	auto dst_ptr = dst.frame();
	const auto dst_end = dst_ptr + dst.size() - PIXEL8_RGB565;
	const auto in_step = src.step();
	const auto out_step = dst.step();

	uint8_t tmp[PIXEL8_RGB];	// for temporary rgb888 data(8pixel)

	if (in_step && out_step && (in_step != out_step)) {
		const auto hh = src.height() < dst.height() ? src.height() : dst.height();
		const auto ww = src.width() < dst.width() ? src.width() : dst.width();
		for (int h = 0; h < hh; h++) {
			int w = 0;
			src_ptr = &src[in_step * h];
			dst_ptr = &dst[out_step * h];
			for (; (dst_ptr <= dst_end) && (src_ptr <= src_end) && (w < ww) ;) {
				IYUYV2RGB_8(src_ptr, tmp, 0, 0);
				RGB2RGB565_8(tmp, dst_ptr, 0, 0);

				dst_ptr += PIXEL8_YUYV;
				src_ptr += PIXEL8_RGB565;
				w += 8;
			}
		}
	} else {
		// compressed format? XXX どちらか一方のstepがwidthと異なっていればクラッシュするかも
		for (; (dst_ptr <= dst_end) && (src_ptr <= src_end) ;) {
			IYUYV2RGB_8(src_ptr, tmp, 0, 0);
			RGB2RGB565_8(tmp, dst_ptr, 0, 0);

			dst_ptr += PIXEL8_YUYV;
			src_ptr += PIXEL8_RGB565;
		}
	}
	return USB_SUCCESS;
}

#define IYUYV2RGBX_2(pyuv, prgbx, ax, bx) { \
		const int d1 = (pyuv)[(ax)+1]; \
		const int d3 = (pyuv)[(ax)+3]; \
		const int r = (22987 * (d3 - 128)) >> 14u; \
		const int g = (-5636 * (d1 - 128) - 11698 * (d3 - 128)) >> 14u; \
		const int b = (29049 * (d1 - 128)) >> 14u; \
		const int y0 = (pyuv)[(ax)+0]; \
		(prgbx)[(bx)+0] = sat(y0 + r); \
		(prgbx)[(bx)+1] = sat(y0 + g); \
		(prgbx)[(bx)+2] = sat(y0 + b); \
		(prgbx)[(bx)+3] = 0xff; \
		const int y2 = (pyuv)[(ax)+2]; \
		(prgbx)[(bx)+4] = sat(y2 + r); \
		(prgbx)[(bx)+5] = sat(y2 + g); \
		(prgbx)[(bx)+6] = sat(y2 + b); \
		(prgbx)[(bx)+7] = 0xff; \
    }
#define IYUYV2RGBX_4(pyuv, prgbx, ax, bx) \
	IYUYV2RGBX_2(pyuv, prgbx, (ax), (bx)) \
	IYUYV2RGBX_2(pyuv, prgbx, (ax) + PIXEL2_YUYV, (bx) + PIXEL2_RGBX);
#define IYUYV2RGBX_8(pyuv, prgbx, ax, bx) \
	IYUYV2RGBX_4(pyuv, prgbx, (ax), (bx)) \
	IYUYV2RGBX_4(pyuv, prgbx, (ax) + PIXEL4_YUYV, (bx) + PIXEL4_RGBX);
#define IYUYV2RGBX_16(pyuv, prgbx, ax, bx) \
	IYUYV2RGBX_8(pyuv, prgbx, (ax), (bx)) \
	IYUYV2RGBX_8(pyuv, prgbx, (ax) + PIXEL8_YUYV, (bx) + PIXEL8_RGBX);

/**
 * YUYV => RGBX8888
 * @param src
 * @param dst
 * @return
 */
static int yuyv2rgbx(const IVideoFrame &src, IVideoFrame &dst) {
	if (UNLIKELY(src.frame_type() != RAW_FRAME_UNCOMPRESSED_YUYV))
		return USB_ERROR_INVALID_PARAM;

	if (UNLIKELY(dst.resize(src, RAW_FRAME_UNCOMPRESSED_RGBX)))
		return USB_ERROR_NO_MEM;

	const uint8_t *src_ptr = src.frame();
	const uint8_t *src_end = src_ptr + src.size() - PIXEL8_YUYV;
	uint8_t *dst_ptr = dst.frame();
	const uint8_t *dst_end = dst_ptr + dst.size() - PIXEL8_RGBX;
	const auto in_step = src.step();
	const auto out_step = dst.step();

	// YUYV => RGBX8888
	if (in_step && out_step && (in_step != out_step)) {
		const auto hh = src.height() < dst.height() ? src.height() : dst.height();
		const auto ww = src.width() < dst.width() ? src.width() : dst.width();
		for (int h = 0; h < hh; h++) {
			int w = 0;
			src_ptr = &src[in_step * h];
			dst_ptr = &dst[out_step * h];
			for (; (dst_ptr <= dst_end) && (src_ptr <= src_end) && (w < ww) ;) {
				IYUYV2RGBX_8(src_ptr, dst_ptr, 0, 0);

				dst_ptr += PIXEL8_RGBX;
				src_ptr += PIXEL8_YUYV;
				w += 8;
			}
		}
	} else {
		// compressed format? XXX どちらか一方のstepがwidthと異なっていればクラッシュするかも
		for (; (dst_ptr <= dst_end) && (src_ptr <= src_end) ;) {
			IYUYV2RGBX_8(src_ptr, dst_ptr, 0, 0);

			dst_ptr += PIXEL8_RGBX;
			src_ptr += PIXEL8_YUYV;
		}
	}
	return USB_SUCCESS;
}

#define IYUYV2BGR_2(pyuv, pbgr, ax, bx) { \
		const int d1 = (pyuv)[1]; \
		const int d3 = (pyuv)[3]; \
	    const int r = (22987 * (d3 - 128)) >> 14u; \
	    const int g = (-5636 * (d1 - 128) - 11698 * (d3 - 128)) >> 14u; \
	    const int b = (29049 * (d1 - 128)) >> 14u; \
		const int y0 = (pyuv)[(ax)+0]; \
		(pbgr)[(bx)+0] = sat(y0 + b); \
		(pbgr)[(bx)+1] = sat(y0 + g); \
		(pbgr)[(bx)+2] = sat(y0 + r); \
		const int y2 = (pyuv)[(ax)+2]; \
		(pbgr)[(bx)+3] = sat(y2 + b); \
		(pbgr)[(bx)+4] = sat(y2 + g); \
		(pbgr)[(bx)+5] = sat(y2 + r); \
    }
#define IYUYV2BGR_4(pyuv, pbgr, ax, bx) \
	IYUYV2BGR_2(pyuv, pbgr, (ax), (bx)) \
	IYUYV2BGR_2(pyuv, pbgr, (ax) + PIXEL2_YUYV, (bx) + PIXEL2_BGR)
#define IYUYV2BGR_8(pyuv, pbgr, ax, bx) \
	IYUYV2BGR_4(pyuv, pbgr, (ax), (bx)) \
	IYUYV2BGR_4(pyuv, pbgr, (ax) + PIXEL4_YUYV, (bx) + PIXEL4_BGR)
#define IYUYV2BGR_16(pyuv, pbgr, ax, bx) \
	IYUYV2BGR_8(pyuv, pbgr, (ax), (bx)) \
	IYUYV2BGR_8(pyuv, pbgr, (ax) + PIXEL8_YUYV, (bx) + PIXEL8_BGR)

/**
 * YUYV => BGR888
 * @param src
 * @param dst
 * @return
 */
static int yuyv2bgr(const IVideoFrame &src, IVideoFrame &dst) {
	if (UNLIKELY(src.frame_type() != RAW_FRAME_UNCOMPRESSED_YUYV))
		return USB_ERROR_INVALID_PARAM;

	if (UNLIKELY(dst.resize(src, RAW_FRAME_UNCOMPRESSED_BGR)))
		return USB_ERROR_NO_MEM;

	const uint8_t *src_ptr = src.frame();
	const uint8_t *src_end = src_ptr + src.size() - PIXEL8_YUYV;
	uint8_t *dst_ptr = dst.frame();
	uint8_t *dst_end = dst_ptr + dst.size() - PIXEL8_BGR;
	const auto in_step = src.step();
	const auto out_step = dst.step();

	// YUYV => BGR888
	if (in_step && out_step && (in_step != out_step)) {
		const auto hh = src.height() < dst.height() ? src.height() : dst.height();
		const auto ww = src.width() < dst.width() ? src.width() : dst.width();
		for (int h = 0; h < hh; h++) {
			int w = 0;
			src_ptr = &src[in_step * h];
			dst_ptr = &dst[out_step * h];
			for (; (dst_ptr <= dst_end) && (src_ptr <= src_end) && (w < ww) ;) {
				IYUYV2BGR_8(src_ptr, dst_ptr, 0, 0);

				dst_ptr += PIXEL8_BGR;
				src_ptr += PIXEL8_YUYV;
				w += 8;
			}
		}
	} else {
		// compressed format? XXX どちらか一方のstepがwidthと異なっていればクラッシュするかも
		for (; (dst_ptr <= dst_end) && (src_ptr <= src_end) ;) {
			IYUYV2BGR_8(src_ptr, dst_ptr, 0, 0);

			dst_ptr += PIXEL8_BGR;
			src_ptr += PIXEL8_YUYV;
		}
	}
	return USB_SUCCESS;
}

#define IUYVY2RGB_2(pyuv, prgb, ax, bx) { \
		const int d0 = (pyuv)[(ax)+0]; \
		const int d2 = (pyuv)[(ax)+2]; \
	    const int r = (22987 * (d2 - 128)) >> 14u; \
	    const int g = (-5636 * (d0 - 128) - 11698 * (d2 - 128)) >> 14u; \
	    const int b = (29049 * (d0 - 128)) >> 14u; \
		const int y1 = (pyuv)[(ax)+1]; \
		(prgb)[(bx)+0] = sat(y1 + r); \
		(prgb)[(bx)+1] = sat(y1 + g); \
		(prgb)[(bx)+2] = sat(y1 + b); \
		const int y3 = (pyuv)[(ax)+3]; \
		(prgb)[(bx)+3] = sat(y3 + r); \
		(prgb)[(bx)+4] = sat(y3 + g); \
		(prgb)[(bx)+5] = sat(y3 + b); \
    }
#define IUYVY2RGB_4(pyuv, prgb, ax, bx) \
	IUYVY2RGB_2(pyuv, prgb, (ax), (bx)) \
	IUYVY2RGB_2(pyuv, prgb, (ax) + 4, (bx) + 6)
#define IUYVY2RGB_8(pyuv, prgb, ax, bx) \
	IUYVY2RGB_4(pyuv, prgb, (ax), (bx)) \
	IUYVY2RGB_4(pyuv, prgb, (ax) + 8, (bx) + 12)
#define IUYVY2RGB_16(pyuv, prgb, ax, bx) \
	IUYVY2RGB_8(pyuv, prgb, (ax), (bx)) \
	IUYVY2RGB_8(pyuv, prgb, (ax) + 16, (bx) + 24)

/**
 * UYVY => RGB888
 * RGB888は1ピクセル3バイトで遅くなるので可能な限り使わない方がいい
 * @param src
 * @param dst
 * @return
 */
static int uyvy2rgb(const IVideoFrame &src, IVideoFrame &dst) {
	if (UNLIKELY(src.frame_type() != RAW_FRAME_UNCOMPRESSED_UYVY))
		return USB_ERROR_INVALID_PARAM;

	if (UNLIKELY(dst.resize(src, RAW_FRAME_UNCOMPRESSED_RGB)))
		return USB_ERROR_NO_MEM;

	const uint8_t *src_ptr = src.frame();
	const uint8_t *src_end = src_ptr + src.size() - PIXEL8_UYVY;
	uint8_t *dst_ptr = dst.frame();
	const uint8_t *dst_end = dst_ptr + dst.size() - PIXEL8_RGB;
	const auto src_step = src.step();
	const auto dst_step = dst.step();

	// UYVY => RGB888
	if (src_step && dst_step && (src_step != dst_step)) {
		const auto hh = src.height() < dst.height() ? src.height() : dst.height();
		const auto ww = src.width() < dst.width() ? src.width() : dst.width();
		for (int h = 0; h < hh; h++) {
			int w = 0;
			src_ptr = &src[src_step * h];
			dst_ptr = &dst[dst_step * h];
			for (; (dst_ptr <= dst_end) && (src_ptr <= src_end) && (w < ww) ;) {
				IUYVY2RGB_8(src_ptr, dst_ptr, 0, 0);

				dst_ptr += PIXEL8_RGB;
				src_ptr += PIXEL8_UYVY;
				w += 8;
			}
		}
	} else {
		// compressed format? XXX どちらか一方のstepがwidthと異なっていればクラッシュするかも
		for (; (dst_ptr <= dst_end) && (src_ptr <= src_end) ;) {
			IUYVY2RGB_8(src_ptr, dst_ptr, 0, 0);

			dst_ptr += PIXEL8_RGB;
			src_ptr += PIXEL8_UYVY;
		}
	}
	return USB_SUCCESS;
}

/**
 * UYVY => RGB565
 * @param src
 * @param dst
 * @return
 */
static int uyvy2rgb565(const IVideoFrame &src, IVideoFrame &dst) {
	if (UNLIKELY(src.frame_type() != RAW_FRAME_UNCOMPRESSED_UYVY))
		return USB_ERROR_INVALID_PARAM;

	if (UNLIKELY(dst.resize(src, RAW_FRAME_UNCOMPRESSED_RGB565)))
		return USB_ERROR_NO_MEM;

	const uint8_t *src_ptr = src.frame();
	const uint8_t *src_end = src_ptr + src.size() - PIXEL8_UYVY;
	uint8_t *dst_ptr = dst.frame();
	const uint8_t *dst_end = dst_ptr + dst.size() - PIXEL8_RGB565;
	const auto src_step = src.step();
	const auto dst_step = dst.step();

	uint8_t tmp[PIXEL8_RGB];		// for temporary rgb888 data(8pixel)

	// UYVY => RGB565
	if (src_step && dst_step && (src_step != dst_step)) {
		const auto hh = src.height() < dst.height() ? src.height() : dst.height();
		const auto ww = src.width() < dst.width() ? src.width() : dst.width();
		for (int h = 0; h < hh; h++) {
			int w = 0;
			src_ptr = &src[src_step * h];
			dst_ptr = &dst[dst_step * h];
			for (; (dst_ptr <= dst_end) && (src_ptr <= src_end) && (w < ww) ;) {
				IUYVY2RGB_8(src_ptr, tmp, 0, 0);
				RGB2RGB565_8(tmp, dst_ptr, 0, 0);

				dst_ptr += PIXEL8_RGB565;
				src_ptr += PIXEL8_UYVY;
				w += 8;
			}
		}
	} else {
		// compressed format? XXX どちらか一方のstepがwidthと異なっていればクラッシュするかも
		for (; (dst_ptr <= dst_end) && (src_ptr <= src_end) ;) {
			IUYVY2RGB_8(src_ptr, tmp, 0, 0);
			RGB2RGB565_8(tmp, dst_ptr, 0, 0);

			dst_ptr += PIXEL8_RGB565;
			src_ptr += PIXEL8_UYVY;
		}
	}
	return USB_SUCCESS;
}

#define IUYVY2RGBX_2(pyuv, prgbx, ax, bx) { \
		const int d0 = (pyuv)[(ax)+0]; \
		const int d2 = (pyuv)[(ax)+2]; \
	    const int r = (22987 * (d2/*(pyuv)[ax+2]*/ - 128)) >> 14u; \
	    const int g = (-5636 * (d0/*(pyuv)[ax+0]*/ - 128) - 11698 * (d2/*(pyuv)[ax+2]*/ - 128)) >> 14u; \
	    const int b = (29049 * (d0/*(pyuv)[ax+0]*/ - 128)) >> 14u; \
		const int y1 = (pyuv)[(ax)+1]; \
		(prgbx)[(bx)+0] = sat(y1 + r); \
		(prgbx)[(bx)+1] = sat(y1 + g); \
		(prgbx)[(bx)+2] = sat(y1 + b); \
		(prgbx)[(bx)+3] = 0xff; \
		const int y3 = (pyuv)[(ax)+3]; \
		(prgbx)[(bx)+4] = sat(y3 + r); \
		(prgbx)[(bx)+5] = sat(y3 + g); \
		(prgbx)[(bx)+6] = sat(y3 + b); \
		(prgbx)[(bx)+7] = 0xff; \
    }
#define IUYVY2RGBX_4(pyuv, prgbx, ax, bx) \
	IUYVY2RGBX_2(pyuv, prgbx, (ax), (bx)) \
	IUYVY2RGBX_2(pyuv, prgbx, (ax) + PIXEL2_UYVY, (bx) + PIXEL2_RGBX)
#define IUYVY2RGBX_8(pyuv, prgbx, ax, bx) \
	IUYVY2RGBX_4(pyuv, prgbx, (ax), (bx)) \
	IUYVY2RGBX_4(pyuv, prgbx, (ax) + PIXEL4_UYVY, (bx) + PIXEL4_RGBX)
#define IUYVY2RGBX_16(pyuv, prgbx, ax, bx) \
	IUYVY2RGBX_8(pyuv, prgbx, (ax), (bx) \
	IUYVY2RGBX_8(pyuv, prgbx, (ax) + PIXEL8_UYVY, (bx) + PIXEL8_RGBX)

/**
 * UYVY => RGBX8888
 * @param src
 * @param dst
 * @return
 */
static int uyvy2rgbx(const IVideoFrame &src, IVideoFrame &dst) {
	if (UNLIKELY(src.frame_type() != RAW_FRAME_UNCOMPRESSED_UYVY))
		return USB_ERROR_INVALID_PARAM;

	if (UNLIKELY(dst.resize(src, RAW_FRAME_UNCOMPRESSED_RGBX)))
		return USB_ERROR_NO_MEM;

	const uint8_t *src_ptr = src.frame();
	const uint8_t *src_end = src_ptr + src.size() - PIXEL8_UYVY;
	uint8_t *dst_ptr = dst.frame();
	const uint8_t *dst_end = dst_ptr + dst.size() - PIXEL8_RGBX;
	const auto src_step = src.step();
	const auto dst_step = dst.step();

	// UYVY => RGBX8888
	if (src_step && dst_step && (src_step != dst_step)) {
		const auto hh = src.height() < dst.height() ? src.height() : dst.height();
		const auto ww = src.width() < dst.width() ? src.width() : dst.width();
		for (int h = 0; h < hh; h++) {
			int w = 0;
			src_ptr = &src[src_step * h];
			dst_ptr = &dst[dst_step * h];
			for (; (dst_ptr <= dst_end) && (src_ptr <= src_end) && (w < ww) ;) {
				IUYVY2RGBX_8(src_ptr, dst_ptr, 0, 0);

				dst_ptr += PIXEL8_RGBX;
				src_ptr += PIXEL8_UYVY;
				w += 8;
			}
		}
	} else {
		// compressed format? XXX どちらか一方のstepがwidthと異なっていればクラッシュするかも
		for (; (dst_ptr <= dst_end) && (src_ptr <= src_end) ;) {
			IUYVY2RGBX_8(src_ptr, dst_ptr, 0, 0);

			dst_ptr += PIXEL8_RGBX;
			src_ptr += PIXEL8_UYVY;
		}
	}
	return USB_SUCCESS;
}

#define IUYVY2BGR_2(pyuv, pbgr, ax, bx) { \
		const int d0 = (pyuv)[(ax)+0]; \
		const int d2 = (pyuv)[(ax)+2]; \
	    const int r = (22987 * (d2/*(pyuv)[ax+2]*/ - 128)) >> 14u; \
	    const int g = (-5636 * (d0/*(pyuv)[ax+0]*/ - 128) - 11698 * (d2/*(pyuv)[ax+2]*/ - 128)) >> 14u; \
	    const int b = (29049 * (d0/*(pyuv)[ax+0]*/ - 128)) >> 14u; \
		const int y1 = (pyuv)[(ax)+1]; \
		(pbgr)[(bx)+0] = sat(y1 + b); \
		(pbgr)[(bx)+1] = sat(y1 + g); \
		(pbgr)[(bx)+2] = sat(y1 + r); \
		const int y3 = (pyuv)[(ax)+3]; \
		(pbgr)[(bx)+3] = sat(y3 + b); \
		(pbgr)[(bx)+4] = sat(y3 + g); \
		(pbgr)[(bx)+5] = sat(y3 + r); \
    }
#define IUYVY2BGR_4(pyuv, pbgr, ax, bx) \
	IUYVY2BGR_2(pyuv, pbgr, (ax), (bx)) \
	IUYVY2BGR_2(pyuv, pbgr, (ax) + PIXEL2_UYVY, (bx) + PIXEL2_BGR)
#define IUYVY2BGR_8(pyuv, pbgr, ax, bx) \
	IUYVY2BGR_4(pyuv, pbgr, (ax), (bx)) \
	IUYVY2BGR_4(pyuv, pbgr, (ax) + PIXEL4_UYVY, (bx) + PIXEL4_BGR)
#define IUYVY2BGR_16(pyuv, pbgr, ax, bx) \
	IUYVY2BGR_8(pyuv, pbgr, (ax), (bx)) \
	IUYVY2BGR_8(pyuv, pbgr, (ax) + PIXEL8_UYVY, (bx) + PIXEL8_BGR)

/**
 * UYVY => RGB888
 * RGB888は1ピクセル3バイトで遅くなるので可能な限り使わない方がいい
 * @param src
 * @param dst
 * @return
 */
static int uyvy2bgr(const IVideoFrame &src, IVideoFrame &dst) {
	if (UNLIKELY(src.frame_type() != RAW_FRAME_UNCOMPRESSED_UYVY))
		return USB_ERROR_INVALID_PARAM;

	if (UNLIKELY(dst.resize(src, RAW_FRAME_UNCOMPRESSED_BGR)))
		return USB_ERROR_NO_MEM;

	const uint8_t *src_ptr = src.frame();
	const uint8_t *src_end = src_ptr + src.size() - PIXEL8_UYVY;
	uint8_t *dst_ptr = dst.frame();
	const uint8_t *dst_end = dst_ptr + dst.size() - PIXEL8_BGR;
	const auto src_step = src.step();
	const auto dst_step = dst.step();

	// UYVY => BGR888
	if (src_step && dst_step && (src_step != dst_step)) {
		const auto hh = src.height() < dst.height() ? src.height() : dst.height();
		const auto ww = src.width() < dst.width() ? src.width() : dst.width();
		for (int h = 0; h < hh; h++) {
			int w = 0;
			src_ptr = &src[src_step * h];
			dst_ptr = &dst[dst_step * h];
			for (; (dst_ptr <= dst_end) && (src_ptr <= src_end) && (w < ww) ;) {
				IUYVY2BGR_8(src_ptr, dst_ptr, 0, 0);

				dst_ptr += PIXEL8_BGR;
				src_ptr += PIXEL8_UYVY;
				w += 8;
			}
		}
	} else {
		// compressed format? XXX どちらか一方のstepがwidthと異なっていればクラッシュするかも
		for (; (dst_ptr <= dst_end) && (src_ptr <= src_end) ;) {
			IUYVY2BGR_8(src_ptr, dst_ptr, 0, 0);

			dst_ptr += PIXEL8_BGR;
			src_ptr += PIXEL8_UYVY;
		}
	}
	return USB_SUCCESS;
}

/**
 * yuv444semi planerからyuyvインターリーブへ変換
 * @param width
 * @param height
 * @param y
 * @param u
 * @param v
 * @param py
 * @param pu
 * @param pv
 * @return
 */
static int yuv444sp_to_yuy2(const uint32_t &width, const uint32_t &height,
	uint8_t *y, uint8_t *u, uint8_t *v, const uint8_t *py, uint8_t *pu, uint8_t *pv) {

	if (LIKELY(!(width % 8))) {
		// 横幅が8の倍数の時
		for (uint32_t h = 0; h < height; h += 1) {
			for (uint32_t w = 0; w < width; w += 8) {
				*(y) = *(py++);
				*(y+2) = *(py++);
				*(y+4) = *(py++);
				*(y+6) = *(py++);
				*(y+8) = *(py++);
				*(y+10) = *(py++);
				*(y+12) = *(py++);
				*(y+14) = *(py++);
				uint32_t temp = *(pu++);	temp += *(pu++);
				*(u) = (uint8_t)(temp >> 1u);
				temp = *(pu++);	temp += *(pu++);
				*(u+4) = (uint8_t)(temp >> 1u);
				temp = *(pu++);	temp += *(pu++);
				*(u+8) = (uint8_t)(temp >> 1u);
				temp = *(pu++);	temp += *(pu++);
				*(u+12) = (uint8_t)(temp >> 1u);
				temp = *(pu++);	temp += *(pv++);
				*(v) = (uint8_t)(temp >> 1u);
				temp = *(pu++);	temp += *(pv++);
				*(v+4) = (uint8_t)(temp >> 1u);
				temp = *(pu++);	temp += *(pv++);
				*(v+8) = (uint8_t)(temp >> 1u);
				temp = *(pu++);	temp += *(pv++);
				*(v+12) = (uint8_t)(temp >> 1u);
				// 出力位置を8ピクセル分ずらす
				y += 16;
				u += 16;
				v += 16;
			}
		}
	} else {
		// 横幅が4の倍数の時
		for (uint32_t h = 0; h < height; h += 1) {
			for (uint32_t w = 0; w < width; w += 4) {
				*(y) = *(py++);
				*(y+2) = *(py++);
				*(y+4) = *(py++);
				*(y+6) = *(py++);
				uint32_t temp = *(pu++);	temp += *(pu++);
				*(u) = (uint8_t)(temp >> 1u);
				temp = *(pu++);	temp += *(pu++);
				*(u+4) = (uint8_t)(temp >> 1u);
				temp = *(pv++);	temp += *(pv++);
				*(v) = (uint8_t)(temp >> 1u);
				temp = *(pv++);	temp += *(pv++);
				*(v+4) = (uint8_t)(temp >> 1u);
				// 出力位置を4ピクセル分ずらす
				y += 8;
				u += 8;
				v += 8;
			}
		}
	}
	return USB_SUCCESS;
}

/**
 * yuv422semi planerからyuyvインターリーブへ変換
 * @param width
 * @param height
 * @param y
 * @param u
 * @param v
 * @param py
 * @param pu
 * @param pv
 * @return
 */
static int yuv422sp_to_yuy2(const uint32_t &width, const uint32_t &height,
	uint8_t *y, uint8_t *u, uint8_t *v, const uint8_t *py, uint8_t *pu, uint8_t *pv) {

	if (LIKELY(!(width % 8))) {
		// 横幅が8の倍数の時
		for (uint32_t h = 0; h < height; h += 1) {
			for (uint32_t w = 0; w < width; w += 8) {
				*(y) = *(py++);
				*(y+2) = *(py++);
				*(y+4) = *(py++);
				*(y+6) = *(py++);
				*(y+8) = *(py++);
				*(y+10) = *(py++);
				*(y+12) = *(py++);
				*(y+14) = *(py++);
				*(u) = *(pu++);
				*(u+4) = *(pu++);
				*(u+8) = *(pu++);
				*(u+12) = *(pu++);
				*(v) = *(pv++);
				*(v+4) = *(pv++);
				*(v+8) = *(pv++);
				*(v+12) = *(pv++);
				// 出力位置を8ピクセル分ずらす
				y += 16;
				u += 16;
				v += 16;
			}
		}
	} else {
		// 横幅が4の倍数の時
		for (int h = 0; h < height; h += 1) {
			for (int w = 0; w < width; w += 4) {
				*(y) = *(py++);
				*(y+2) = *(py++);
				*(y+4) = *(py++);
				*(y+6) = *(py++);
				*(u) = *(pu++);
				*(u+4) = *(pu++);
				*(v) = *(pv++);
				*(v+4) = *(pv++);
				// 出力位置を4ピクセル分ずらす
				y += 8;
				u += 8;
				v += 8;
			}
		}
	}
	return USB_SUCCESS;
}

/**
 * yuv420semi planer(NV12)からyuyvインターリーブへ変換 FIXME うまく動かない
 * u/vは2x2のサブサンプリングのはずなんだけど
 * @param width
 * @param height
 * @param y
 * @param u
 * @param v
 * @param py
 * @param pu
 * @param pv
 * @return
 */
static int yuv420sp_to_yuy2(const uint32_t &width, const uint32_t &height,
	uint8_t *y, uint8_t *u, uint8_t *v, const uint8_t *py, uint8_t *pu, uint8_t *pv) {

	const uint32_t w2 = width << 1u;	// 1行下の出力バッファへのオフセット
	if (LIKELY(!(width % 8))) {
		// 横幅が8の倍数の時
		for (uint32_t h = 0; h < height; h += 1) {
			for (uint32_t w = 0; w < width; w += 8) {
				*(y) = *(py++);
				*(y+2) = *(py++);
				*(y+4) = *(py++);
				*(y+6) = *(py++);
				*(y+8) = *(py++);
				*(y+10) = *(py++);
				*(y+12) = *(py++);
				*(y+14) = *(py++);
				if (!(h & 0x01u)) {	// u, vは縦にもサブサンプリングされている
					// 偶数行  奇数行		  データ
					*(u)	= *(u+w2)	= *(pu++);
					*(u+4)	= *(u+w2+4)	= *(pu++);
					*(u+8)	= *(u+w2+8)	= *(pu++);
					*(u+12)	= *(u+w2+12)= *(pu++);
					*(v)	= *(v+w2)	= *(pv++);
					*(v+4)	= *(v+w2+4)	= *(pv++);
					*(v+8)	= *(v+w2+8)	= *(pv++);
					*(v+12)	= *(v+w2+12)= *(pv++);
				}
				// 出力位置を8ピクセル分ずらす
				y += 16;
				u += 16;
				v += 16;
			}
		}
	} else {
		// 横幅が4の倍数の時
		for (uint32_t h = 0; h < height; h += 1) {
			for (uint32_t w = 0; w < width; w += 4) {
				*(y) = *(py++);
				*(y+2) = *(py++);
				*(y+4) = *(py++);
				*(y+6) = *(py++);
				if (!(h & 0x01u)) {	// u, vは縦にもサブサンプリングされている
					// 偶数行  奇数行		  データ
					*(u)	= *(u+w2)	= *(pu++);
					*(u+4)	= *(u+w2+4)	= *(pu++);
					*(v)	= *(v+w2)	= *(pv++);
					*(v+4)	= *(v+w2+4)	= *(pv++);
				}
				// 出力位置を4ピクセル分ずらす
				y += 8;
				u += 8;
				v += 8;
			}
		}
	}
	return USB_SUCCESS;
}

//--------------------------------------------------------------------------------
// mjpeg関係
//--------------------------------------------------------------------------------
/**
 * デコードのスピードアップのために複数行をまとめて読み込み変換する
 * 行数の公約数でないとダメ。2592x1944があるので1,2,4,8のどれか
 * (1, 2, 4, 5, 6, 8, 10, 12, 20, 40...for 720p&1080p)
 */
#define MAX_READLINE 4

struct error_mgr {
	struct jpeg_error_mgr super;
	jmp_buf jmp;
};

/**
 * libjpeg/libjpeg-turboのエラー処理関数
 */
static void _error_exit(j_common_ptr dinfo) {
	auto my_err = (struct error_mgr *) dinfo->err;
#ifndef NDEBUG
	char err_msg[1024];
	(*dinfo->err->format_message)(dinfo, err_msg);
	err_msg[1023] = 0;
	LOGW("err=%s", err_msg);
#endif
	longjmp(my_err->jmp, 1);
}

#if USE_MARKER_CALLBACK // 0
// libjpeg/libjpeg-turboがAPP/COMマーカーを処理する時のコールバック関数
// typedef boolean (*jpeg_marker_parser_method) (j_decompress_ptr cinfo);
static boolean app4_parser(j_decompress_ptr cinfo) {
	ENTER();

	jpeg_saved_marker_ptr marker = cinfo->marker_list;

	for ( ; marker ; ) {
		LOGI("marker=%d,original_length=%d,data_length=%d",
			marker->marker, marker->original_length, marker->data_length);
		// marker->data(unsigned charへのポインタ)
		marker = marker->next;
	}
	RET(true);
}

static boolean app12_parser(j_decompress_ptr cinfo) {
	ENTER();

	jpeg_saved_marker_ptr marker = cinfo->marker_list;

	for ( ; marker ; ) {
		LOGI("marker=%d,original_length=%d,data_length=%d",
			marker->marker, marker->original_length, marker->data_length);
		// marker->data(unsigned charへのポインタ)
		marker = marker->next;
	}
	RET(true);
}

static boolean com_parser(j_decompress_ptr cinfo) {
	ENTER();

	jpeg_saved_marker_ptr marker = cinfo->marker_list;

	for ( ; marker ; ) {
		LOGI("marker=%d,original_length=%d,data_length=%d",
			marker->marker, marker->original_length, marker->data_length);
		// marker->data(unsigned charへのポインタ)
		marker = marker->next;
	}
	RET(true);
}
#endif // USE_MARKER_CALLBACK

/**
 * dct_mode_tをlibjpeg/libjpegturboのJ_DCT_METHODへ変換する
 * @param dct_mode
 * @return
 */
static J_DCT_METHOD getJDCTMethod(const dct_mode_t &dct_mode) {
	switch (dct_mode) {
	case DCT_MODE_ISLOW:
		return JDCT_ISLOW;
	case DCT_MODE_FLOAT:
		return JDCT_FLOAT;
    case DCT_MODE_IFAST:
	default:
		return JDCT_IFAST;
	}
}

//--------------------------------------------------------------------------------
// ハフマンテーブルなしのフレームデータが来た時のための
// ISO/IEC 10918-1:1993(E) K.3.3.で定義されているデフォルトのハフマンテーブル
static const uint8_t dc_lumi_len[] = {
	0, 0, 1, 5, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0 };
static const uint8_t dc_lumi_val[] = {
	0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 };

static const uint8_t dc_chromi_len[] = {
	0, 0, 3, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0 };
static const uint8_t dc_chromi_val[] = {
	0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 };

static const uint8_t ac_lumi_len[] = {
	0, 0, 2, 1, 3, 3, 2, 4, 3, 5, 5, 4, 4, 0, 0, 1, 0x7d };
static const uint8_t ac_lumi_val[] = {
	0x01, 0x02, 0x03, 0x00, 0x04, 0x11,	0x05, 0x12,
	0x21, 0x31, 0x41, 0x06, 0x13, 0x51, 0x61, 0x07,
	0x22, 0x71,	0x14, 0x32, 0x81, 0x91, 0xa1, 0x08,
	0x23, 0x42, 0xb1, 0xc1, 0x15, 0x52, 0xd1, 0xf0,
	0x24, 0x33, 0x62, 0x72, 0x82, 0x09, 0x0a, 0x16,
	0x17, 0x18, 0x19, 0x1a, 0x25, 0x26, 0x27, 0x28,
	0x29, 0x2a, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39,
	0x3a, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49,
	0x4a, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59,
	0x5a, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69,
	0x6a, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79,
	0x7a, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89,
	0x8a, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98,
	0x99, 0x9a, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7,
	0xa8, 0xa9, 0xaa, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6,
	0xb7, 0xb8, 0xb9, 0xba, 0xc2, 0xc3, 0xc4, 0xc5,
	0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xd2, 0xd3, 0xd4,
	0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda, 0xe1, 0xe2,
	0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea,
	0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8,
	0xf9, 0xfa
};

static const uint8_t ac_chromi_len[] = {
	0, 0, 2, 1, 2, 4, 4, 3, 4, 7, 5, 4, 4, 0, 1, 2, 0x77 };

static const uint8_t ac_chromi_val[] = {
	0x00, 0x01, 0x02, 0x03, 0x11, 0x04, 0x05, 0x21,
	0x31, 0x06, 0x12, 0x41, 0x51, 0x07, 0x61, 0x71,
	0x13, 0x22, 0x32, 0x81, 0x08, 0x14, 0x42, 0x91,
	0xa1, 0xb1, 0xc1, 0x09, 0x23, 0x33, 0x52, 0xf0,
	0x15, 0x62, 0x72, 0xd1, 0x0a, 0x16, 0x24, 0x34,
	0xe1, 0x25, 0xf1, 0x17, 0x18, 0x19, 0x1a, 0x26,
	0x27, 0x28, 0x29, 0x2a, 0x35, 0x36, 0x37, 0x38,
	0x39, 0x3a, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48,
	0x49, 0x4a, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58,
	0x59, 0x5a, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68,
	0x69, 0x6a, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78,
	0x79, 0x7a, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87,
	0x88, 0x89, 0x8a, 0x92, 0x93, 0x94, 0x95, 0x96,
	0x97, 0x98, 0x99, 0x9a, 0xa2, 0xa3, 0xa4, 0xa5,
	0xa6, 0xa7, 0xa8, 0xa9, 0xaa, 0xb2, 0xb3, 0xb4,
	0xb5, 0xb6, 0xb7, 0xb8, 0xb9, 0xba, 0xc2, 0xc3,
	0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xd2,
	0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda,
	0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9,
	0xea, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8,
	0xf9, 0xfa
};

#define COPY_HUFF_TABLE(dinfo,tbl,name) do { \
	if ((dinfo)->tbl == nullptr) (dinfo)->tbl = jpeg_alloc_huff_table((j_common_ptr)(dinfo)); \
		memcpy((dinfo)->tbl->bits, name##_len, sizeof(name##_len)); \
		memset((dinfo)->tbl->huffval, 0, sizeof((dinfo)->tbl->huffval)); \
		memcpy((dinfo)->tbl->huffval, name##_val, sizeof(name##_val)); \
	} while(0)

static inline void insert_huff_tables(j_decompress_ptr dinfo) {
	COPY_HUFF_TABLE(dinfo, dc_huff_tbl_ptrs[0], dc_lumi);
	COPY_HUFF_TABLE(dinfo, dc_huff_tbl_ptrs[1], dc_chromi);
	COPY_HUFF_TABLE(dinfo, ac_huff_tbl_ptrs[0], ac_lumi);
	COPY_HUFF_TABLE(dinfo, ac_huff_tbl_ptrs[1], ac_chromi);
}

/**
 * MJPEG => RGB各種
 * RGB/BGRに変換すると1ピクセル3バイトなので4バイト境界にアライメント出来なくて
 * 遅くなるのでRGBX/XRGB/BGRX/XBGR/RGB565/YUVに変換すべき
 * @param src
 * @param dst
 * @param dst_color_space
 * @param dct_mode
 * @return
 */
static int mjpeg2rgb(
	const IVideoFrame &src, VideoImage_t &dst,
	const J_COLOR_SPACE &dst_color_space, const dct_mode_t &dct_mode) {

	struct jpeg_decompress_struct dinfo{};
	struct error_mgr jerr{};
	uint32_t lines_read = 0;

	uint32_t num_scanlines, i, ret_header;
	int result;
	uint8_t *buffer[MAX_READLINE];

	auto frame_type = tj_color_space2raw_frame(dst_color_space);
	if (UNLIKELY((RAW_FRAME_UNKNOWN == frame_type) || (dst.frame_type != frame_type))) {
		RETURN(USB_ERROR_INVALID_PARAM, int);
	}
	const auto pixel_bytes = get_pixel_bytes(frame_type);
	// local copy
	const auto width = src.width();
	const auto height = src.height();
	
	const auto dst_step = pixel_bytes.frame_bytes(width, 1);
	uint8_t *dst_ptr = dst.ptr;
	dinfo.err = jpeg_std_error(&jerr.super);
	jerr.super.error_exit = _error_exit;

	if (setjmp(jerr.jmp)) {
		result = VIDEO_ERROR_FRAME;
		goto fail;
	}

	jpeg_create_decompress(&dinfo);

#if USE_MARKER_CALLBACK // 0
	// COM/APPxのコールバック関数をセット...でも一度も呼ばれたことがない(T T)
	jpeg_set_marker_processor(&dinfo, JFIF_MARKER_APP4, app4_parser);	// APP4の追加処理関数を追加
	jpeg_set_marker_processor(&dinfo, JFIF_MARKER_APP12, app12_parser);	// APP12の追加処理関数を追加
	jpeg_set_marker_processor(&dinfo, JFIF_MARKER_COM, com_parser);		// COMの追加処理関数を追加
#endif // USE_MARKER_CALLBACK

	jpeg_mem_src(&dinfo, (uint8_t *)src.frame(), src.actual_bytes());

	ret_header = jpeg_read_header(&dinfo, TRUE);
	if (UNLIKELY(ret_header != JPEG_HEADER_OK)) {
		LOGD("jpeg_read_header returned error %d", ret_header);
		jpeg_abort_decompress(&dinfo);	// 後でjpeg_destroy_decompressを呼ぶからこれは呼ばなくてもいい
		result = VIDEO_ERROR_FRAME;
		goto fail;
	}

	if (UNLIKELY((dinfo.image_width != src.width()) || (dinfo.image_height != src.height()))) {
		LOGD("unexpected image size:expected(%dx%d), actual(%dx%d)",
			src.width(), src.height(), dinfo.image_width, dinfo.image_height);
		jpeg_abort_decompress(&dinfo);	// 後でjpeg_destroy_decompressを呼ぶからこれは呼ばなくてもいい
		result = VIDEO_ERROR_FRAME;
		goto fail;
	}

	if (dinfo.dc_huff_tbl_ptrs[0] == nullptr) {
		// フレームデータ内にハフマンテーブルが無いので標準のハフマンテーブルを使用する
		insert_huff_tables(&dinfo);
	}

	dinfo.out_color_space = dst_color_space;
	dinfo.dct_method = getJDCTMethod(dct_mode);

	jpeg_start_decompress(&dinfo);

	if (LIKELY(dinfo.output_height == dst.height)) {
		for (; dinfo.output_scanline < dinfo.output_height ;) {
			buffer[0] = dst_ptr + (lines_read) * dst_step;
			for (i = 1; i < MAX_READLINE; i++) {
				buffer[i] = buffer[i-1] + dst_step;
			}
			num_scanlines = jpeg_read_scanlines(&dinfo, buffer, MAX_READLINE);
			lines_read += num_scanlines;
		}
	}

	jpeg_finish_decompress(&dinfo);
	jpeg_destroy_decompress(&dinfo);
	RETURN(lines_read == dst.height ? USB_SUCCESS : VIDEO_ERROR_FRAME, int);

fail:
	jpeg_destroy_decompress(&dinfo);
	RETURN(result, int);
}

/**
 * MJPEG => RGB各種
 * RGB/BGRに変換すると1ピクセル3バイトなので4バイト境界にアライメント出来なくて
 * 遅くなるのでRGBX/XRGB/BGRX/XBGR/RGB565/YUVに変換すべき
 * @param src
 * @param dst
 * @param dst_color_space
 * @param dct_mode
 * @return
 */
static int mjpeg2rgb(
	const IVideoFrame &src, IVideoFrame &dst,
	const J_COLOR_SPACE &dst_color_space, const dct_mode_t &dct_mode) {


	auto frame_type = tj_color_space2raw_frame(dst_color_space);
	if (UNLIKELY(RAW_FRAME_UNKNOWN == frame_type)) {
		RETURN(USB_ERROR_INVALID_PARAM, int);
	}

	if (UNLIKELY(dst.resize(src, frame_type))) {
		RETURN(USB_ERROR_NO_MEM, int);
	}

	VideoImage_t dst_image {
		.frame_type = frame_type,
		.width = src.width(),
		.height = src.height(),
		.ptr = dst.frame(),
	};

	int result = mjpeg2rgb(src, dst_image, dst_color_space, dct_mode);

	RETURN(result, int);
}

/**
 * mjpegデコード用に各プレーンを示すポインタを初期化してy/u/vの各サイズを取得するためのヘルパー関数
 * @param jpeg_bytes 元の(m)jpegのサイズ[バイト数]
 * @param jpeg_subsamp サブサンプリングの種類
 * @param jpeg_width 映像サイズ(幅)
 * @param jpeg_height 映像サイズ(高さ)
 * @param dst 出力先のバッファ
 * @param planes  インデックスが0:y, 1:u, 2:vの出力先プレーンを受け取るためのuint8_tポインタ配列
 * @param y_bytes yプレーンのサイズ[バイト数]
 * @param u_bytes uプレーンのサイズ[バイト数]
 * @param v_bytes vプレーンのサイズ[バイト数]
 * @return
 */
static int prepare_mjpeg_plane(
	const size_t &jpeg_bytes, const int &jpeg_subsamp,
	const int &jpeg_width, const int &jpeg_height,
	IVideoFrame &dst, uint8_t *planes[3],
	size_t &y_bytes, size_t &u_bytes, size_t &v_bytes) {

	ENTER();

	y_bytes = tjPlaneSizeYUV(0, jpeg_width, 0, jpeg_height, jpeg_subsamp);	// = width * height;
	u_bytes = tjPlaneSizeYUV(1, jpeg_width, 0, jpeg_height, jpeg_subsamp);	// = y_bytes >> 1;
	v_bytes = tjPlaneSizeYUV(2, jpeg_width, 0, jpeg_height, jpeg_subsamp);	// = y_bytes >> 1;

	const raw_frame_t jpeg_frame_type = tjsamp2raw_frame(jpeg_subsamp);
	if (LIKELY(jpeg_frame_type != RAW_FRAME_UNKNOWN)) {
		dst.resize(jpeg_width, jpeg_height, jpeg_frame_type);
		if (dst.actual_bytes() >= y_bytes + u_bytes + v_bytes) {
			uint8_t *y = planes[0] = &dst[0];	// y
			planes[1] = y + y_bytes;			// u
			planes[2] = y + y_bytes + u_bytes;	// v
			RETURN(USB_SUCCESS, int);
		} else {
			LOGW("Failed to resize output frame");
			RETURN(USB_ERROR_NO_MEM, int);
		}
	}

	RETURN(USB_ERROR_NOT_SUPPORTED, int);
}

/**
 * mjpegデコード用に各プレーンを示すポインタを初期化してy/u/vの各サイズを取得するためのヘルパー関数
 * @param jpeg_bytes 元の(m)jpegのサイズ[バイト数]
 * @param jpeg_subsamp サブサンプリングの種類
 * @param jpeg_width 映像サイズ(幅)
 * @param jpeg_height 映像サイズ(高さ)
 * @param dst 出力先のバッファ
 * @param planes  インデックスが0:y, 1:u, 2:vの出力先プレーンを受け取るためのuint8_tポインタ配列
 * @param y_bytes yプレーンのサイズ[バイト数]
 * @param u_bytes uプレーンのサイズ[バイト数]
 * @param v_bytes vプレーンのサイズ[バイト数]
 * @return
 */
static int prepare_mjpeg_plane(
	const size_t &jpeg_bytes, const int &jpeg_subsamp,
	const int &jpeg_width, const int &jpeg_height,
	std::vector<uint8_t> &dst, uint8_t *planes[3],
	size_t &y_bytes, size_t &u_bytes, size_t &v_bytes) {

	ENTER();

	y_bytes = tjPlaneSizeYUV(0, jpeg_width, 0, jpeg_height, jpeg_subsamp);	// = width * height;
	u_bytes = tjPlaneSizeYUV(1, jpeg_width, 0, jpeg_height, jpeg_subsamp);	// = y_bytes >> 1;
	v_bytes = tjPlaneSizeYUV(2, jpeg_width, 0, jpeg_height, jpeg_subsamp);	// = y_bytes >> 1;

	const raw_frame_t jpeg_frame_type = tjsamp2raw_frame(jpeg_subsamp);
	if (LIKELY(jpeg_frame_type != RAW_FRAME_UNKNOWN)) {
		const size_t bytes = y_bytes + u_bytes + v_bytes;
		dst.resize(bytes);
		if (dst.size() >= bytes) {
			uint8_t *dst_y = planes[0] = &dst[0];	// y
			planes[1] = dst_y + y_bytes;			// u
			planes[2] = dst_y + y_bytes + u_bytes;	// v
			RETURN(USB_SUCCESS, int);
		} else {
			LOGW("Failed to resize output frame");
			RETURN(USB_ERROR_NO_MEM, int);
		}
	}

	RETURN(USB_ERROR_NOT_SUPPORTED, int);
}

/**
 * turbo-jpeg APIを使ってRGB系に変換する
 * Nexus5で1920x1080でも30ミリ秒ぐらい
 * @param jpegDecompressor libjpeg-turboによるmjpeg展開用
 * @param src 映像入力
 * @param dst 映像出力
 * @param tj_pixel_format 変換するピクセルフォーマット
 * @param dct_mode dctモード
 * @return
 */
static int mjpeg2rgb_turbo(
	tjhandle &jpegDecompressor,
	const IVideoFrame &src, uint8_t *dst,
	const int &tj_pixel_format, const dct_mode_t &dct_mode) {

	const auto frame_type = tj_pixel_format2raw_frame(tj_pixel_format);
	if (UNLIKELY(frame_type == RAW_FRAME_UNKNOWN)) {
		RETURN(USB_ERROR_INVALID_PARAM, int);
	}

	size_t jpeg_bytes;
	int jpeg_subsamp, jpeg_width, jpeg_height;
	int result = parse_mjpeg_header(jpegDecompressor, src, jpeg_bytes, jpeg_subsamp, jpeg_width, jpeg_height);
	if (result) {
		RETURN(result, int);
	}
	if (UNLIKELY((jpeg_width != src.width()) || (jpeg_height != src.height()))) {
		LOGD("unexpected image size:expected(%dx%d), actual(%dx%d)",
			src.width(), src.height(), jpeg_width, jpeg_height);
		RETURN(VIDEO_ERROR_FRAME, int);
	}
	// libjpegturbo側関数が本来はconst uint8_t *のところがuint8_t *を受け取るのでキャスト
	auto jpeg_src = const_cast<uint8_t *>(src.frame());
	result = tjDecompress2(jpegDecompressor,
		jpeg_src, jpeg_bytes,
		dst, jpeg_width, 0, jpeg_height,
		tj_pixel_format,
		(dct_mode == DCT_MODE_ISLOW || dct_mode == DCT_MODE_FLOAT)
			? TJFLAG_ACCURATEDCT : (TJFLAG_FASTDCT | TJFLAG_FASTUPSAMPLE));

	RETURN(result, int);
}

/**
 * turbo-jpeg APIを使ってRGB系に変換する
 * Nexus5で1920x1080でも30ミリ秒ぐらい
 * @param jpegDecompressor libjpeg-turboによるmjpeg展開用
 * @param src 映像入力
 * @param dst 映像出力
 * @param tj_pixel_format 変換するピクセルフォーマット
 * @param dct_mode dctモード
 * @return
 */
static int mjpeg2rgb_turbo(
	tjhandle &jpegDecompressor,
	const IVideoFrame &src, IVideoFrame &dst,
	const int &tj_pixel_format, const dct_mode_t &dct_mode) {

	ENTER();

	const auto frame_type = tj_pixel_format2raw_frame(tj_pixel_format);
	if (UNLIKELY(frame_type == RAW_FRAME_UNKNOWN)) {
		RETURN(USB_ERROR_INVALID_PARAM, int);
	}
	if (UNLIKELY(dst.resize(src, frame_type))) {
		RETURN(USB_ERROR_NO_MEM, int);
	}
	int result = mjpeg2rgb_turbo(jpegDecompressor, src, &dst[0], tj_pixel_format, dct_mode);
	RETURN(result, int);
}

/**
 * turbo-jpeg APIを使ってYUVのプランナーフォーマットに変換する
 * サブサンプリングは元のjpegに従う(YUV444, YUV422, YUV420, GRAY(Y), YUV440, YUV411のいずれか)
 *
 *           width
 *   --------------------
 *   |                  |
 *   |         Y        | height
 *   |                  |
 *   |                  |
 *   --------------------
 *   |    U    |
 *   |         |
 *   -----------
 *   |    V    |
 *   |         |
 *   -----------
 * @param jpegDecompressor
 * @param src
 * @param dst
 * @param dct_mode
 * @return
 */
static int mjpeg2YUVAnyPlanner_turbo(tjhandle &jpegDecompressor,
	const IVideoFrame &src, IVideoFrame &dst,
	const dct_mode_t &dct_mode) {

	size_t jpeg_bytes;
	int jpeg_subsamp, jpeg_width, jpeg_height;
	int result = parse_mjpeg_header(jpegDecompressor, src, jpeg_bytes, jpeg_subsamp, jpeg_width, jpeg_height);
	if (UNLIKELY(result)) {
		LOGD("parse_mjpeg_header failed,err=%d", result);
		RETURN(result, int);
	}
	if (UNLIKELY((jpeg_width != src.width()) || (jpeg_height != src.height()))) {
		// 入力映像フレームに指定されている映像サイズとjpegのヘッダーから取得した映像サイズが異なる場合はエラー
		LOGD("unexpected image size:expected(%dx%d), actual(%dx%d)",
			src.width(), src.height(), jpeg_width, jpeg_height);
		RETURN(VIDEO_ERROR_FRAME, int);
	}
	// libjpegturbo側関数が本来はconst uint8_t *のところがuint8_t *を受け取るのでキャスト
	auto compressed = (uint8_t *)src.frame();
#if 0
	LOGI("sz_yuv=(%lu,%lu,%lu),subsamp=%d",
		tjPlaneSizeYUV(0, jpeg_width, 0, jpeg_height, jpeg_subsamp),
		tjPlaneSizeYUV(1, jpeg_width, 0, jpeg_height, jpeg_subsamp),
		tjPlaneSizeYUV(2, jpeg_width, 0, jpeg_height, jpeg_subsamp),
		jpeg_subsamp);
#endif
	// yuv-planerへデコードする
	size_t sz_y, sz_u, sz_v;
	uint8_t *dst_planes[3];
	result = prepare_mjpeg_plane(
		jpeg_bytes, jpeg_subsamp,
		jpeg_width, jpeg_height,
		dst, dst_planes,
		sz_y, sz_u, sz_v);
	if (!result) {
		result = tjDecompressToYUVPlanes(jpegDecompressor, compressed,
			jpeg_bytes, dst_planes, jpeg_width, nullptr/*strides*/, jpeg_height,
				(dct_mode == DCT_MODE_ISLOW || dct_mode == DCT_MODE_FLOAT)
					? TJFLAG_ACCURATEDCT : TJFLAG_FASTDCT);
		if (UNLIKELY(result)) {
			// has error
			const int err = tjGetErrorCode(jpegDecompressor);
#if defined(__ANDROID__)
			LOGD("tjDecompressToYUVPlanes failed,result=%d,err=%d,%s",
				result,
				err, tjGetErrorStr2(jpegDecompressor));
#else
            LOGD("tjDecompressToYUVPlanes failed,result=%d", result);
#endif
			result = err == TJERR_WARNING ? USB_SUCCESS : USB_ERROR_OTHER;
		}
	} else {
		LOGW("failed to prepare output planes,result=%d", result);
		result = VIDEO_ERROR_FRAME;
	}
	RETURN(result, int);
}

/**
 * mjpegを展開して必要であれば指定されたフレームフォーマットになるように変換する
 * @param jpegDecompressor
 * @param src
 * @param dst
 * @param work
 * @param dct_mode
 * @return
 */
static int mjpeg2YUVxxx_turbo(tjhandle &jpegDecompressor,
	const IVideoFrame &src, IVideoFrame &dst,
	std::vector<uint8_t> &work1,
	std::vector<uint8_t> &work2,
	const dct_mode_t &dct_mode) {

	// (m)jpegのサイズやサブサンプリングを取得する
	size_t jpeg_bytes;
	int jpeg_subsamp, jpeg_width, jpeg_height;
	int result = parse_mjpeg_header(jpegDecompressor, src, jpeg_bytes, jpeg_subsamp, jpeg_width, jpeg_height);
	if (UNLIKELY(result)) {
		LOGD("parse_mjpeg_header failed,err=%d", result);
		RETURN(result, int);
	}
	if (UNLIKELY((jpeg_width != src.width()) || (jpeg_height != src.height()))) {
		// 入力映像フレームに指定されている映像サイズとjpegのヘッダーから取得した映像サイズが異なる場合はエラー
		LOGD("unexpected image size:expected(%dx%d), actual(%dx%d)",
			src.width(), src.height(), jpeg_width, jpeg_height);
		RETURN(VIDEO_ERROR_FRAME, int);
	}
	// サブサンプリングからraw_frame_tを取得
	const raw_frame_t jpeg_frame_type = tjsamp2raw_frame(jpeg_subsamp);
	if (jpeg_frame_type == RAW_FRAME_UNKNOWN) {
		// 未対応のjpegサブサンプリングの場合
		RETURN(VIDEO_ERROR_FRAME, int);
	} else if ((jpeg_frame_type == dst.frame_type()
		|| (dst.frame_type() == RAW_FRAME_UNCOMPRESSED_YUV_ANY))) {
		// mjpegを展開するだけでOKな場合
		RETURN(mjpeg2YUVAnyPlanner_turbo(jpegDecompressor, src, dst, dct_mode), int);
	}
	// mjepgを展開後フォーマットを変換する場合
	// libjpegturbo側関数が本来はconst uint8_t *のところがuint8_t *を受け取るのでキャスト
	auto compressed = (uint8_t *)src.frame();
#if 0
	LOGI("sz_yuv=(%lu,%lu,%lu)",
		tjPlaneSizeYUV(0, width, 0, height, jpegSubsamp),
		tjPlaneSizeYUV(1, width, 0, height, jpegSubsamp),
		tjPlaneSizeYUV(2, width, 0, height, jpegSubsamp));
#endif
	// ワーク用バッファへyuv-planerとしてデコードする
	size_t src_sz_y, src_sz_u, src_sz_v;
	uint8_t *dst_planes[3];
	result = prepare_mjpeg_plane(
		jpeg_bytes, jpeg_subsamp,
		jpeg_width, jpeg_height,
		work1, dst_planes,
		src_sz_y, src_sz_u, src_sz_v);
	if (LIKELY(!result)) {
		result = tjDecompressToYUVPlanes(jpegDecompressor, compressed,
			jpeg_bytes, dst_planes, jpeg_width, nullptr/*strides*/, jpeg_height,
				(dct_mode == DCT_MODE_ISLOW || dct_mode == DCT_MODE_FLOAT)
					? TJFLAG_ACCURATEDCT : TJFLAG_FASTDCT);
		if (result) {
			// failed
			const int err = tjGetErrorCode(jpegDecompressor);
#if defined(__ANDROID__)
			LOGD("tjDecompressToYUVPlanes failed,result=%d,err=%d,%s",
				result,
				err, tjGetErrorStr2(jpegDecompressor));
#else
			LOGD("tjDecompressToYUVPlanes failed,result=%d", result);
#endif
			result = err == TJERR_WARNING ? USB_SUCCESS : USB_ERROR_OTHER;
		}
	} else {
		LOGW("failed to prepare output planes,result=%d", result);
		result = VIDEO_ERROR_FRAME;
	}
	if (!result) {
		// 出力先のバッファサイズを調整
		result = dst.resize(jpeg_width, jpeg_height, dst.frame_type());
		if (result) {
			RETURN(USB_ERROR_NO_MEM, int);
		}
	}
	if (!result) {
		// ワークへのデコードと出力先のバッファサイズ調整が成功した時
		// 各プレーンの先頭ポインタとサイズを取得
		const uint8_t *src_y = dst_planes[0];
		const uint8_t *src_u = dst_planes[1];
		const uint8_t *src_v = dst_planes[2];
		const int src_w_y = tjPlaneWidth(0, jpeg_width, jpeg_subsamp);
		const int src_w_u = tjPlaneWidth(1, jpeg_width, jpeg_subsamp);
		const int src_w_v = tjPlaneWidth(2, jpeg_width, jpeg_subsamp);
		// FIXME 未実装 目的のフレームフォーマットに変換する
		switch (dst.frame_type()) {
		case RAW_FRAME_UNCOMPRESSED_NV21:
		{	// 出力先がNV21の時
			const size_t sz_y = tjPlaneSizeYUV(0, jpeg_width, 0, jpeg_height, TJSAMP_420);
			const int w_y = tjPlaneWidth(0, jpeg_width, TJSAMP_420);
			const int w_vu = tjPlaneWidth(1, jpeg_width, TJSAMP_420) * 2;
			uint8_t *dst_y = &dst[0];
			uint8_t *dst_vu = dst_y + sz_y;
			// mjepg側のサブサンプリングで分岐
			switch (jpeg_frame_type) {
			case RAW_FRAME_UNCOMPRESSED_444p:
			{
				result = libyuv::I444ToNV21(
					src_y, src_w_y,
					src_u, src_w_u,
					src_v, src_w_v,
					dst_y, w_y,
					dst_vu, w_vu,
					jpeg_width, jpeg_height);
				break;
			}
			case RAW_FRAME_UNCOMPRESSED_422p:
			{
				result = libyuv::I422ToNV21(
					src_y, src_w_y,
					src_u, src_w_u,
					src_v, src_w_v,
					dst_y, w_y,
					dst_vu, w_vu,
					jpeg_width, jpeg_height);
				break;
			}
			case RAW_FRAME_UNCOMPRESSED_I420:
			{
				result = libyuv::I420ToNV21(
					src_y, src_w_y,
					src_u, src_w_u,
					src_v, src_w_v,
					dst_y, w_y,
				  	dst_vu, w_vu,
					jpeg_width, jpeg_height);
				break;
			}
			case RAW_FRAME_UNCOMPRESSED_GRAY8:
			{
				result = libyuv::I400ToNV21(
					src_y, src_w_y,
					dst_y, w_y,
					dst_vu, w_vu,
					jpeg_width, jpeg_height);
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
			const size_t sz_y = tjPlaneSizeYUV(0, jpeg_width, 0, jpeg_height, TJSAMP_420);
			const size_t sz_u = tjPlaneSizeYUV(1, jpeg_width, 0, jpeg_height, TJSAMP_420);
			const size_t sz_v = tjPlaneSizeYUV(2, jpeg_width, 0, jpeg_height, TJSAMP_420);
			const int w_y = tjPlaneWidth(0, jpeg_width, TJSAMP_420);
			const int w_u = tjPlaneWidth(1, jpeg_width, TJSAMP_420);
			const int w_v = tjPlaneWidth(2, jpeg_width, TJSAMP_420);
			uint8_t *dst_y = &dst[0];
			uint8_t *dst_v = dst_y + sz_y;	// uとvを逆にして呼び出す
			uint8_t *dst_u = dst_v + sz_v;
			// mjepg側のサブサンプリングで分岐
			switch (jpeg_frame_type) {
			case RAW_FRAME_UNCOMPRESSED_444p:
			{
				result = libyuv::I444ToI420(
					src_y, src_w_y,
					src_u, src_w_u,
					src_v, src_w_v,
					dst_y, w_y,
					dst_u, w_u,
					dst_v, w_v,
					jpeg_width, jpeg_height);
				break;
			}
			case RAW_FRAME_UNCOMPRESSED_422p:
			{
				result = libyuv::I422ToI420(
					src_y, src_w_y,
					src_u, src_w_u,
					src_v, src_w_v,
					dst_y, w_y,
					dst_u, w_u,
					dst_v, w_v,
					jpeg_width, jpeg_height);
				break;
			}
			case RAW_FRAME_UNCOMPRESSED_I420:
			{
				result = libyuv::I420Copy(
					src_y, src_w_y,
					src_u, src_w_u,
					src_v, src_w_v,
					dst_y, w_y,
					dst_u, w_u,
					dst_v, w_v,
					jpeg_width, jpeg_height);
				break;
			}
			case RAW_FRAME_UNCOMPRESSED_GRAY8:
			{
				result = libyuv::I400ToI420(
					src_y, src_w_y,
					dst_y, w_y,
					dst_u, w_u,
					dst_v, w_v,
					jpeg_width, jpeg_height);
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
			const size_t sz_y = tjPlaneSizeYUV(0, jpeg_width, 0, jpeg_height, TJSAMP_420);
			const size_t sz_u = tjPlaneSizeYUV(1, jpeg_width, 0, jpeg_height, TJSAMP_420);
			const size_t sz_v = tjPlaneSizeYUV(2, jpeg_width, 0, jpeg_height, TJSAMP_420);
			const int w_y = tjPlaneWidth(0, jpeg_width, TJSAMP_420);
			const int w_u = tjPlaneWidth(1, jpeg_width, TJSAMP_420);
			const int w_v = tjPlaneWidth(2, jpeg_width, TJSAMP_420);
			uint8_t *dst_y = &dst[0];
			uint8_t *dst_u = dst_y + sz_y;
			uint8_t *dst_v = dst_u + sz_u;
			// mjepg側のサブサンプリングで分岐
			switch (jpeg_frame_type) {
			case RAW_FRAME_UNCOMPRESSED_444p:
			{
				result = libyuv::I444ToI420(
					src_y, src_w_y,
					src_u, src_w_u,
					src_v, src_w_v,
					dst_y, w_y,
					dst_u, w_u,
					dst_v, w_v,
					jpeg_width, jpeg_height);
				break;
			}
			case RAW_FRAME_UNCOMPRESSED_422p:
			{
				result = libyuv::I422ToI420(
					src_y, src_w_y,
					src_u, src_w_u,
					src_v, src_w_v,
					dst_y, w_y,
					dst_u, w_u,
					dst_v, w_v,
					jpeg_width, jpeg_height);
				break;
			}
			case RAW_FRAME_UNCOMPRESSED_I420:
			{
				result = libyuv::I420Copy(
					src_y, src_w_y,
					src_u, src_w_u,
					src_v, src_w_v,
					dst_y, w_y,
					dst_u, w_u,
					dst_v, w_v,
					jpeg_width, jpeg_height);
				break;
			}
			case RAW_FRAME_UNCOMPRESSED_GRAY8:
			{
				result = libyuv::I400ToI420(
					src_y, src_w_y,
					dst_y, w_y,
					dst_u, w_u,
					dst_v, w_v,
					jpeg_width, jpeg_height);
				break;
			}
			case RAW_FRAME_UNCOMPRESSED_440p:
			case RAW_FRAME_UNCOMPRESSED_411p:
			default:
				break;
			}
			break;
		} // RAW_FRAME_UNCOMPRESSED_I420
		case RAW_FRAME_UNCOMPRESSED_M420:
		{	// FIXME 未実装　出力先がM420の時
			const size_t sz_y = tjPlaneSizeYUV(0, jpeg_width, 0, jpeg_height, TJSAMP_420);
			const size_t sz_u = tjPlaneSizeYUV(1, jpeg_width, 0, jpeg_height, TJSAMP_420);
			const size_t sz_v = tjPlaneSizeYUV(2, jpeg_width, 0, jpeg_height, TJSAMP_420);
			const int w_y = tjPlaneWidth(0, jpeg_width, TJSAMP_420);
			const int w_u = tjPlaneWidth(1, jpeg_width, TJSAMP_420);
			const int w_v = tjPlaneWidth(2, jpeg_width, TJSAMP_420);
			uint8_t *dst_y = &dst[0];
			uint8_t *dst_v = dst_y + sz_y;
			uint8_t *dst_u = dst_v + sz_v;
			// mjepg側のサブサンプリングで分岐
			switch (jpeg_frame_type) {
			case RAW_FRAME_UNCOMPRESSED_444p:
			case RAW_FRAME_UNCOMPRESSED_422p:
			case RAW_FRAME_UNCOMPRESSED_I420:
			case RAW_FRAME_UNCOMPRESSED_GRAY8:
			case RAW_FRAME_UNCOMPRESSED_440p:
			case RAW_FRAME_UNCOMPRESSED_411p:
			default:
				break;
			}
			break;
		} // RAW_FRAME_UNCOMPRESSED_M420
		case RAW_FRAME_UNCOMPRESSED_NV12:
		{	// 出力先がNV12の時
			const size_t sz_y = tjPlaneSizeYUV(0, jpeg_width, 0, jpeg_height, TJSAMP_420);
			const size_t sz_u = tjPlaneSizeYUV(1, jpeg_width, 0, jpeg_height, TJSAMP_420);
			const size_t sz_v = tjPlaneSizeYUV(2, jpeg_width, 0, jpeg_height, TJSAMP_420);
			const size_t sz = sz_y + sz_u + sz_y + sz_v;
			const int w_y = tjPlaneWidth(0, jpeg_width, TJSAMP_420);
			const int w_uv = tjPlaneWidth(1, jpeg_width, TJSAMP_420) * 2;
			uint8_t *dst_y = &dst[0];
			uint8_t *dst_uv = dst_y + sz_y;
			// mjepg側のサブサンプリングで分岐
			switch (jpeg_frame_type) {
			case RAW_FRAME_UNCOMPRESSED_444p:
			{
				work2.resize(sz);
				if (work2.size() >= sz) {
					uint8_t *wy = &work2[0];
					uint8_t *wvu = wy + sz_y;
					libyuv::I444ToNV21(
						src_y, src_w_y,
						src_u, src_w_u,
						src_v, src_w_v,
						wy, w_y,
						wvu, w_uv,
						jpeg_width, jpeg_height);
					result = libyuv::NV21ToNV12(
						wy, w_y,
						wvu, w_uv,
						dst_y, w_y,
						dst_uv, w_uv,
						jpeg_width, jpeg_height);
				} else {
					result = USB_ERROR_NO_MEM;
				}
				break;
			}
			case RAW_FRAME_UNCOMPRESSED_422p:
			{
				work2.resize(sz);
				if (work2.size() >= sz) {
					uint8_t *wy = &work2[0];
					uint8_t *wvu = wy + sz_y;
					libyuv::I422ToNV21(
						src_y, src_w_y,
						src_u, src_w_u,
						src_v, src_w_v,
						wy, w_y,
						wvu, w_uv,
						jpeg_width, jpeg_height);
					result = libyuv::NV21ToNV12(
						wy, w_y,
						wvu, w_uv,
						dst_y, w_y,
						dst_uv, w_uv,
						jpeg_width, jpeg_height);
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
					dst_y, w_y,
					dst_uv, w_uv,
					jpeg_width, jpeg_height);
				break;
			}
			case RAW_FRAME_UNCOMPRESSED_GRAY8:
			{
				work2.resize(sz);
				if (work2.size() >= sz) {
					uint8_t *wy = &work2[0];
					uint8_t *wvu = wy + sz_y;
					libyuv::I400ToNV21(
						src_y, src_w_y,
						dst_y, w_y,
						dst_uv, w_uv,
						jpeg_width, jpeg_height);
					result = libyuv::NV21ToNV12(
						wy, w_y,
						wvu, w_uv,
						dst_y, w_y,
						dst_uv, w_uv,
						jpeg_width, jpeg_height);
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
			const size_t sz_y = tjPlaneSizeYUV(0, jpeg_width, 0, jpeg_height, TJSAMP_444);
			const size_t sz_u = tjPlaneSizeYUV(1, jpeg_width, 0, jpeg_height, TJSAMP_444);
			const size_t sz_v = tjPlaneSizeYUV(2, jpeg_width, 0, jpeg_height, TJSAMP_444);
			const int w_y = tjPlaneWidth(0, jpeg_width, TJSAMP_444);
			const int w_u = tjPlaneWidth(1, jpeg_width, TJSAMP_444);
			const int w_v = tjPlaneWidth(2, jpeg_width, TJSAMP_444);
			uint8_t *dst_y = &dst[0];
			uint8_t *dst_u = dst_y + sz_y;
			uint8_t *dst_v = dst_u + sz_u;
#if 0
			static int cnt = 0;
			if ((++cnt % 100) == 0) {
				LOGI("sz(%" FMT_SIZE_T ",%" FMT_SIZE_T ",%" FMT_SIZE_T "),w(%d,%d,%d)",
					sz_y, sz_u, sz_v, w_y, w_u, w_v);
			}
#endif
			// mjepg側のサブサンプリングで分岐
			switch (jpeg_frame_type) {
			case RAW_FRAME_UNCOMPRESSED_444p:
			{
				result = libyuv::I444Copy(
					src_y, src_w_y,
					src_u, src_w_u,
					src_v, src_w_v,
					dst_y, w_y,
					dst_u, w_u,
					dst_v, w_v,
					jpeg_width, jpeg_height);
				break;
			}
			case RAW_FRAME_UNCOMPRESSED_422p:
			{
				const size_t sz_argb = jpeg_width * jpeg_height * 4;
				work2.resize(sz_argb);
				if (work2.size() >= sz_argb) {
					uint8_t *argb = &work2[0];
					libyuv::I422ToARGB(
						src_y, src_w_y,
						src_u, src_w_u,
						src_v, src_w_v,
						argb, jpeg_width * 4,
						jpeg_width, jpeg_height);
					result = libyuv::ARGBToI444(
						argb, jpeg_width * 4,
						dst_y, w_y,
						dst_u, w_u,
						dst_v, w_v,
						jpeg_width, jpeg_height);
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
					dst_y, w_y,
					dst_u, w_u,
					dst_v, w_v,
					jpeg_width, jpeg_height);
				break;
			}
			case RAW_FRAME_UNCOMPRESSED_GRAY8:
			{
				const size_t sz_y0 = tjPlaneSizeYUV(0, jpeg_width, 0, jpeg_height, TJSAMP_420);
				const size_t sz_u0 = tjPlaneSizeYUV(1, jpeg_width, 0, jpeg_height, TJSAMP_420);
				const size_t sz_v0 = tjPlaneSizeYUV(2, jpeg_width, 0, jpeg_height, TJSAMP_420);
				const size_t sz0 = sz_y0 + sz_u0 + sz_v0;
				const int w_y0 = tjPlaneWidth(0, jpeg_width, TJSAMP_420);
				const int w_u0 = tjPlaneWidth(1, jpeg_width, TJSAMP_420);
				const int w_v0 = tjPlaneWidth(2, jpeg_width, TJSAMP_420);
				work2.resize(sz0);
				if (work2.size() >= sz0) {
					uint8_t *y0 = &work2[0];
					uint8_t *u0 = y0 + sz_y0;
					uint8_t *v0 = u0 + sz_u0;
					libyuv::I400ToI420(
						src_y, src_w_y,
						y0, w_y0,
						u0, w_u0,
						v0, w_v0,
						jpeg_width, jpeg_height);
					result = libyuv::I420ToI444(
						y0, w_y0,
						u0, w_u0,
						v0, w_v0,
						dst_y, w_y,
						dst_u, w_u,
						dst_v, w_v,
						jpeg_width, jpeg_height);
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
			const size_t sz_y = tjPlaneSizeYUV(0, jpeg_width, 0, jpeg_height, TJSAMP_444);
			const size_t sz_u = tjPlaneSizeYUV(1, jpeg_width, 0, jpeg_height, TJSAMP_444);
			const size_t sz_v = tjPlaneSizeYUV(2, jpeg_width, 0, jpeg_height, TJSAMP_444);
			const int w_y = tjPlaneWidth(0, jpeg_width, TJSAMP_444);
			const int w_u = tjPlaneWidth(1, jpeg_width, TJSAMP_444);
			const int w_v = tjPlaneWidth(2, jpeg_width, TJSAMP_444);
			uint8_t *dst_y = &dst[0];
			uint8_t *dst_u = dst_y + sz_y;
			uint8_t *dst_v = dst_u + 1;
			// mjepg側のサブサンプリングで分岐
			switch (jpeg_frame_type) {
			case RAW_FRAME_UNCOMPRESSED_444p:
			{
				result = libyuv::I444Copy(
					src_y, src_w_y,
					src_u, src_w_u,
					src_v, src_w_v,
					dst_y, w_y,
					dst_u, w_u,
					dst_v, w_v,
					jpeg_width, jpeg_height);
				break;
			}
			case RAW_FRAME_UNCOMPRESSED_422p:
			{
				const size_t sz_argb = jpeg_width * jpeg_height * 4;
				work2.resize(sz_argb);
				if (work2.size() >= sz_argb) {
					uint8_t *argb = &work2[0];
					libyuv::I422ToARGB(
						src_y, src_w_y,
						src_u, src_w_u,
						src_v, src_w_v,
						argb, jpeg_width * 4,
						jpeg_width, jpeg_height);
					result = libyuv::ARGBToI444(
						argb, jpeg_width * 4,
						dst_y, w_y,
						dst_u, w_u,
						dst_v, w_v,
						jpeg_width, jpeg_height);
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
					dst_y, w_y,
					dst_u, w_u,
					dst_v, w_v,
					jpeg_width, jpeg_height);
				break;
			}
			case RAW_FRAME_UNCOMPRESSED_GRAY8:
			{
				const size_t sz_y0 = tjPlaneSizeYUV(0, jpeg_width, 0, jpeg_height, TJSAMP_420);
				const size_t sz_u0 = tjPlaneSizeYUV(1, jpeg_width, 0, jpeg_height, TJSAMP_420);
				const size_t sz_v0 = tjPlaneSizeYUV(2, jpeg_width, 0, jpeg_height, TJSAMP_420);
				const size_t sz0 = sz_y0 + sz_u0 + sz_v0;
				const int w_y0 = tjPlaneWidth(0, jpeg_width, TJSAMP_420);
				const int w_u0 = tjPlaneWidth(1, jpeg_width, TJSAMP_420);
				const int w_v0 = tjPlaneWidth(2, jpeg_width, TJSAMP_420);
				work2.resize(sz0);
				if (work2.size() >= sz0) {
					uint8_t *y0 = &work2[0];
					uint8_t *u0 = y0 + sz_y0;
					uint8_t *v0 = u0 + sz_u0;
					libyuv::I400ToI420(
						src_y, src_w_y,
						y0, w_y0,
						u0, w_u0,
						v0, w_v0,
						jpeg_width, jpeg_height);
					result = libyuv::I420ToI444(
						y0, w_y0,
						u0, w_u0,
						v0, w_v0,
						dst_y, w_y,
						dst_u, w_u,
						dst_v, w_v,
						jpeg_width, jpeg_height);
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
			const size_t sz_y = tjPlaneSizeYUV(0, jpeg_width, 0, jpeg_height, TJSAMP_422);
			const size_t sz_u = tjPlaneSizeYUV(1, jpeg_width, 0, jpeg_height, TJSAMP_422);
			const size_t sz_v = tjPlaneSizeYUV(2, jpeg_width, 0, jpeg_height, TJSAMP_422);
			const int w_y = tjPlaneWidth(0, jpeg_width, TJSAMP_422);
			const int w_u = tjPlaneWidth(1, jpeg_width, TJSAMP_422);
			const int w_v = tjPlaneWidth(2, jpeg_width, TJSAMP_422);
			uint8_t *dst_y = &dst[0];
			uint8_t *dst_u = dst_y + sz_y;
			uint8_t *dst_v = dst_u + sz_u;
			// mjepg側のサブサンプリングで分岐
			switch (jpeg_frame_type) {
			case RAW_FRAME_UNCOMPRESSED_444p:
			{
#if 0
				// これは結構遅い
				const size_t sz_argb = jpeg_width * jpeg_height * 4;
				work2.resize(sz_argb);
				if (work2.size() >= sz_argb) {
					uint8_t *argb = &work2[0];
					libyuv::I444ToARGB(
						src_y, src_w_y,
						src_u, src_w_u,
						src_v, src_w_v,
						argb, jpeg_width * 4,
						jpeg_width, jpeg_height);
					result = libyuv::ARGBToI422(
						argb, jpeg_width * 4,
						dst_y, w_y,
						dst_u, w_u,
						dst_v, w_v,
						jpeg_width, jpeg_height);
				} else {
					result = USB_ERROR_NO_MEM;
				}
#else
				// 多少uvの解像度が下がるのかもしれないけどARGBを経由するより速い
				const size_t sz_y0 = tjPlaneSizeYUV(0, jpeg_width, 0, jpeg_height, TJSAMP_420);
				const size_t sz_u0 = tjPlaneSizeYUV(1, jpeg_width, 0, jpeg_height, TJSAMP_420);
				const size_t sz_v0 = tjPlaneSizeYUV(2, jpeg_width, 0, jpeg_height, TJSAMP_420);
				const size_t sz0 = sz_y0 + sz_u0 + sz_v0;
				const int w_y0 = tjPlaneWidth(0, jpeg_width, TJSAMP_420);
				const int w_u0 = tjPlaneWidth(1, jpeg_width, TJSAMP_420);
				const int w_v0 = tjPlaneWidth(2, jpeg_width, TJSAMP_420);
				work2.resize(sz0);
				if (work2.size() >= sz0) {
					uint8_t *y0 = &work2[0];
					uint8_t *u0 = y0 + sz_y0;
					uint8_t *v0 = u0 + sz_u0;
					libyuv::I444ToI420(
						src_y, src_w_y,
						src_u, src_w_u,
						src_v, src_w_v,
						y0, w_y0,
						u0, w_u0,
						v0, w_v0,
						jpeg_width, jpeg_height);
					result = libyuv::I420ToI422(
						y0, w_y0,
						u0, w_u0,
						v0, w_v0,
						dst_y, w_y,
						dst_u, w_u,
						dst_v, w_v,
						jpeg_width, jpeg_height);
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
					dst_y, w_y,
					dst_u, w_u,
					dst_v, w_v,
					jpeg_width, jpeg_height);
				break;
			}
			case RAW_FRAME_UNCOMPRESSED_I420:
			{
				result = libyuv::I420ToI422(
					src_y, src_w_y,
					src_u, src_w_u,
					src_v, src_w_v,
					dst_y, w_y,
					dst_u, w_u,
					dst_v, w_v,
					jpeg_width, jpeg_height);
				break;
			}
			case RAW_FRAME_UNCOMPRESSED_GRAY8:
			{
				const size_t sz_y0 = tjPlaneSizeYUV(0, jpeg_width, 0, jpeg_height, TJSAMP_420);
				const size_t sz_u0 = tjPlaneSizeYUV(1, jpeg_width, 0, jpeg_height, TJSAMP_420);
				const size_t sz_v0 = tjPlaneSizeYUV(2, jpeg_width, 0, jpeg_height, TJSAMP_420);
				const size_t sz0 = sz_y0 + sz_u0 + sz_v0;
				const int w_y0 = tjPlaneWidth(0, jpeg_width, TJSAMP_420);
				const int w_u0 = tjPlaneWidth(1, jpeg_width, TJSAMP_420);
				const int w_v0 = tjPlaneWidth(2, jpeg_width, TJSAMP_420);
				work2.resize(sz0);
				if (work2.size() >= sz0) {
					uint8_t *y0 = &work2[0];
					uint8_t *u0 = y0 + sz_y0;
					uint8_t *v0 = u0 + sz_u0;
					libyuv::I400ToI420(
						src_y, src_w_y,
						y0, w_y0,
						u0, w_u0,
						v0, w_v0,
						jpeg_width, jpeg_height);
					result = libyuv::I420ToI422(
						y0, w_y0,
						u0, w_u0,
						v0, w_v0,
						dst_y, w_y,
						dst_u, w_u,
						dst_v, w_v,
						jpeg_width, jpeg_height);
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
			const size_t sz_y = tjPlaneSizeYUV(0, jpeg_width, 0, jpeg_height, TJSAMP_422);
			const size_t sz_u = tjPlaneSizeYUV(1, jpeg_width, 0, jpeg_height, TJSAMP_422);
			const size_t sz_v = tjPlaneSizeYUV(2, jpeg_width, 0, jpeg_height, TJSAMP_422);
			const int w_y = tjPlaneWidth(0, jpeg_width, TJSAMP_422);
			const int w_u = tjPlaneWidth(1, jpeg_width, TJSAMP_422);
			const int w_v = tjPlaneWidth(2, jpeg_width, TJSAMP_422);
			uint8_t *dst_y = &dst[0];
			uint8_t *dst_u = dst_y + sz_y;
			uint8_t *dst_v = dst_u + 1;
			// mjepg側のサブサンプリングで分岐
			switch (jpeg_frame_type) {
			case RAW_FRAME_UNCOMPRESSED_444p:
			{
				const size_t sz_argb = jpeg_width * jpeg_height * 4;
				work2.resize(sz_argb);
				if (work2.size() >= sz_argb) {
					uint8_t *argb = &work2[0];
					libyuv::I444ToARGB(
						src_y, src_w_y,
						src_u, src_w_u,
						src_v, src_w_v,
						argb, jpeg_width * 4,
						jpeg_width, jpeg_height);
					result = libyuv::ARGBToI422(
						argb, jpeg_width * 4,
						dst_y, w_y,
					  	dst_u, w_u,
					  	dst_v, w_v,
						jpeg_width, jpeg_height);
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
					jpeg_width, jpeg_height);
				// u planeとv planeをuv planeにまとめてコピー
				libyuv::MergeUVPlane(
					src_u, w_u,
					src_v, w_v,
					dst_u, w_u + w_v,
					jpeg_width, jpeg_height);
				result = USB_SUCCESS;
				break;
			}
			case RAW_FRAME_UNCOMPRESSED_I420:
			{
				result = libyuv::I420ToI422(
					src_y, src_w_y,
					src_u, src_w_u,
					src_v, src_w_v,
					dst_y, w_y,
					dst_u, w_u,
					dst_v, w_v,
					jpeg_width, jpeg_height);
				break;
			}
			case RAW_FRAME_UNCOMPRESSED_GRAY8:
			{
				const size_t sz_y0 = tjPlaneSizeYUV(0, jpeg_width, 0, jpeg_height, TJSAMP_420);
				const size_t sz_u0 = tjPlaneSizeYUV(1, jpeg_width, 0, jpeg_height, TJSAMP_420);
				const size_t sz_v0 = tjPlaneSizeYUV(2, jpeg_width, 0, jpeg_height, TJSAMP_420);
				const size_t sz0 = sz_y0 + sz_u0 + sz_v0;
				const int w_y0 = tjPlaneWidth(0, jpeg_width, TJSAMP_420);
				const int w_u0 = tjPlaneWidth(1, jpeg_width, TJSAMP_420);
				const int w_v0 = tjPlaneWidth(2, jpeg_width, TJSAMP_420);
				work2.resize(sz0);
				if (work2.size() >= sz0) {
					uint8_t *y0 = &work2[0];
					uint8_t *u0 = y0 + sz_y0;
					uint8_t *v0 = u0 + sz_u0;
					libyuv::I400ToI420(
						src_y, src_w_y,
						y0, w_y0,
						u0, w_u0,
						v0, w_v0,
						jpeg_width, jpeg_height);
					result = libyuv::I420ToI422(
						y0, w_y0,
						u0, w_u0,
						v0, w_v0,
						dst_y, w_y,
						dst_u, w_u,
						dst_v, w_v,
						jpeg_width, jpeg_height);
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
			const int w_y = tjPlaneWidth(0, jpeg_width, TJSAMP_GRAY);
			uint8_t *y = &dst[0];
			// yプレーンだけをコピーする
			libyuv::CopyPlane(
				src_y, src_w_y,
				y, w_y,
				jpeg_width, jpeg_height);
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
	}

	RETURN(result, int);
}

/**
 * Nexus5で1920x1080でも合計45ミリ秒ぐらい。たぶんカラースペースの変換を２回してしまうのが遅くなる原因じゃないかと
 * mjpeg2yuyv_turboでサブサンプリングが444,422, 420じゃないときにフォールバックとして呼ばれる
 * @param src
 * @param dst
 * @param dct_mode
 * @return
 */
static int mjpeg2yuyv(
	const IVideoFrame &src, VideoImage_t &dst,
	const dct_mode_t &dct_mode) {

	uint32_t lines_read = 0;
	int i;
	uint32_t j;
	uint32_t num_scanlines;
	int result;
	uint8_t *yuyv, *ycbcr;

	struct jpeg_decompress_struct dinfo{};
	struct error_mgr jerr{};
	dinfo.err = jpeg_std_error(&jerr.super);
	jerr.super.error_exit = _error_exit;

	JDIMENSION row_stride;
	int ret_header;
	JSAMPARRAY buffer;
	// local copy
	uint8_t *data = dst.ptr;
	const auto out_step = src.width() * 2;

	if (setjmp(jerr.jmp)) {
		result = VIDEO_ERROR_FRAME;
		goto fail;
	}

	jpeg_create_decompress(&dinfo);
	jpeg_mem_src(&dinfo, (uint8_t *)src.frame(), src.actual_bytes());
	ret_header = jpeg_read_header(&dinfo, TRUE);
	if (UNLIKELY(ret_header != JPEG_HEADER_OK)) {
		LOGD("jpeg_read_header returned error %d", ret_header);
		jpeg_abort_decompress(&dinfo);	// 後でjpeg_destroy_decompressを呼ぶからこれは呼ばなくてもいい
		result = VIDEO_ERROR_FRAME;
		goto fail;
	}

	if (UNLIKELY((dinfo.image_width != src.width()) || (dinfo.image_height != src.height()))) {
		LOGD("unexpected image size:expected(%dx%d), actual(%dx%d)",
			src.width(), src.height(), dinfo.image_width, dinfo.image_height);
		jpeg_abort_decompress(&dinfo);	// 後でjpeg_destroy_decompressを呼ぶからこれは呼ばなくてもいい
		result = VIDEO_ERROR_FRAME;
		goto fail;
	}

	if (dinfo.dc_huff_tbl_ptrs[0] == nullptr) {
		// フレームデータ内にハフマンテーブルが無いので標準のハフマンテーブルを使用する
		insert_huff_tables(&dinfo);
	}

	dinfo.out_color_space = JCS_YCbCr;
	dinfo.dct_method = getJDCTMethod(dct_mode);
	// 66.7msec @ 1920x1080x30fps on Nexus7(2013)
	// デコード開始
	jpeg_start_decompress(&dinfo);

	// dinfo.xxx変数はjpeg_start_decompressを呼ぶまでは有効じゃないので注意
	row_stride = dinfo.output_width * dinfo.output_components;

	// allocate buffer
	buffer = (*dinfo.mem->alloc_sarray)
		((j_common_ptr) &dinfo, JPOOL_IMAGE, row_stride, MAX_READLINE);

	if (LIKELY(dinfo.output_height == dst.height)) {
		for (; dinfo.output_scanline < dinfo.output_height ;) {
			// convert lines of mjpeg data to YCbCr
			num_scanlines = jpeg_read_scanlines(&dinfo, buffer, MAX_READLINE);
			// convert YCbCr to yuyv(YUV422)
			// ここはせずに上位でGPUで変換すればもう少し速くなるかも
			for (j = 0; j < num_scanlines; j++) {
				yuyv = data + (lines_read + j) * out_step;
				ycbcr = buffer[j];
				for (i = 0; i < row_stride; i += 24) {	// step by YCbCr x 8 pixels = 3 x 8 bytes
					YCbCr_YUYV_2(ycbcr + i, yuyv);
					YCbCr_YUYV_2(ycbcr + i + 6, yuyv);
					YCbCr_YUYV_2(ycbcr + i + 12, yuyv);
					YCbCr_YUYV_2(ycbcr + i + 18, yuyv);
				}
			}
			lines_read += num_scanlines;
		}
	}

	jpeg_finish_decompress(&dinfo);
	jpeg_destroy_decompress(&dinfo);
	RETURN(lines_read == dst.height ? USB_SUCCESS : VIDEO_ERROR_FRAME, int);

fail:
	LOGW("dinfo.output_width=%d,out_step=%d,actualBytes=%zu",
		dinfo.output_width, out_step, src.actual_bytes());
	jpeg_destroy_decompress(&dinfo);
	LOGW("fail:%d,%d", lines_read, dst.height);
	RETURN(result, int);
}

/**
 * turbo-jpeg APIを使ってYUYVに変換する
 * Nexus5で1920x1080でも30ミリ秒ぐらい
 * @param jpegDecompressor libjpeg-turboによるmjpeg展開用
 * @param src 映像入力
 * @param dst 映像出力
 * @param work 変換用ワーク
 * @param dct_mode dctモード
 * @param uyvy false: YUYV(YUV2), true: UYVY
 * @return
 */
static int mjpeg2yuyv_turbo(
	tjhandle &jpegDecompressor,
	const IVideoFrame &src,
	VideoImage_t &dst,
	std::vector<uint8_t> &work,
	const dct_mode_t &dct_mode,
	const bool &uyvy) {

	size_t jpeg_bytes;
	int jpeg_subsamp, jpeg_width, jpeg_height;
	int result = parse_mjpeg_header(jpegDecompressor, src, jpeg_bytes, jpeg_subsamp, jpeg_width, jpeg_height);
	if (result) {
		RETURN(result, int);
	}
	if (UNLIKELY((jpeg_width != src.width()) || (jpeg_height != src.height()))) {
		LOGD("unexpected image size:expected(%dx%d), actual(%dx%d)",
			src.width(), src.height(), jpeg_width, jpeg_height);
		RETURN(VIDEO_ERROR_FRAME, int);
	}
	if (UNLIKELY(result)) {
		LOGD("not a mjpeg frame:%d", result);
		RETURN(USB_ERROR_NOT_SUPPORTED, int);
	}
	if (UNLIKELY((jpeg_width != src.width()) || (jpeg_height != src.height()))) {
		LOGD("frame size is different from actual mjpeg size:(src:%dx%d,actual:%dx%d)",
			src.width(), src.height(), jpeg_width, jpeg_height);
		RETURN(VIDEO_ERROR_FRAME, int);
	}
	// libjpegturbo側関数が本来はconst uint8_t *のところがuint8_t *を受け取るのでキャスト
	auto compressed = (uint8_t *)src.frame();
	if (UNLIKELY(jpeg_subsamp != TJSAMP_422)
		&& (jpeg_subsamp != TJSAMP_420)
		&& (jpeg_subsamp != TJSAMP_444)) {

		// サブサンプリングが合わない時はフォールバックする
		RETURN(mjpeg2yuyv(src, dst, dct_mode), int);
	}
	size_t sz_y, sz_u, sz_v;
	uint8_t *dst_planes[3];
	result = prepare_mjpeg_plane(
		jpeg_bytes, jpeg_subsamp,
		jpeg_width, jpeg_height,
		work, dst_planes,
		sz_y, sz_u, sz_v);
	uint8_t *src_y = dst_planes[0];
	uint8_t *src_u = dst_planes[1];
	uint8_t *src_v = dst_planes[2];
	const size_t sz = sz_y + sz_u + sz_v;
	if (!result) {
		// ここで一旦yuv planerでworkへデコードする...tjDecompressToYUVPlanesからいつも-1が返ってくる
		result = tjDecompressToYUVPlanes(jpegDecompressor, compressed,
			jpeg_bytes, dst_planes, jpeg_width, nullptr/*strides*/, jpeg_height,
			(dct_mode == DCT_MODE_ISLOW || dct_mode == DCT_MODE_FLOAT)
				? TJFLAG_ACCURATEDCT : TJFLAG_FASTDCT);
		if (UNLIKELY(result)) {
			// has error
#if defined(__ANDROID__)
            const int err = tjGetErrorCode(jpegDecompressor);
    		LOGD("tjDecompressToYUVPlanes failed,result=%d,err=%d,%s",
				result,
				err, tjGetErrorStr2(jpegDecompressor));
            result = err == TJERR_WARNING ? USB_SUCCESS : USB_ERROR_OTHER;
#else
            LOGD("tjDecompressToYUVPlanes failed,result=%d", result);
			result = USB_SUCCESS;
#endif
		}
	}
	if (!result) {
		// ついでyuyv(インターリーブ)へ変換する
		// 各プレーンの幅を取得
		const int w_y = tjPlaneWidth(0, jpeg_width, jpeg_subsamp);
		const int w_u = tjPlaneWidth(1, jpeg_width, jpeg_subsamp);
		const int w_v = tjPlaneWidth(2, jpeg_width, jpeg_subsamp);

		switch (jpeg_subsamp) {
		case TJSAMP_444:
		{
			uint8_t *dst_y = &dst.ptr[uyvy ? 1 : 0];
			uint8_t *dst_u = &dst.ptr[uyvy ? 0 : 1];
			uint8_t *dst_v = &dst.ptr[uyvy ? 2 : 3];
			result = yuv444sp_to_yuy2(
				(uint32_t) jpeg_width, (uint32_t) jpeg_height,
				dst_y, dst_u, dst_v,
			  	src_y, src_u, src_v);	// これはテストできてない
			break;
		}
		case TJSAMP_422:	// C910HD, C920r, C922, C930e
		{
			uint8_t *dst_y = dst.ptr;
			if (uyvy) {
				result = libyuv::I422ToUYVY(
					src_y, w_y,
					src_u, w_u,
					src_v, w_v,
					dst_y, jpeg_width * 2,
					jpeg_width, jpeg_height);
			} else {
				result = libyuv::I422ToYUY2(
					src_y, w_y,
					src_u, w_u,
					src_v, w_v,
					dst_y, jpeg_width * 2,
					jpeg_width, jpeg_height);
			}
			break;
		}
		case TJSAMP_420:
		{
			uint8_t *dst_y = dst.ptr;
			if (uyvy) {
				libyuv::I420ToUYVY(
					src_y, w_y,
					src_u, w_u,
					src_v, w_v,
					dst_y, jpeg_width * 2,
					jpeg_width, jpeg_height);
			} else {
				result = libyuv::I420ToYUY2(
					src_y, w_y,
					src_u, w_u,
					src_v, w_v,
					dst_y, jpeg_width * 2,
					jpeg_width, jpeg_height);
			}
			break;
		}
		case TJSAMP_GRAY:
		case TJSAMP_440:
		case TJSAMP_411:
		default:	// jpegSubsampでフォールバックさせているのでここには来ない
		{
			uint8_t *dst_y = &dst.ptr[uyvy ? 1 : 0];
			uint8_t *dst_u = &dst.ptr[uyvy ? 0 : 1];
			uint8_t *dst_v = &dst.ptr[uyvy ? 2 : 3];
			result = yuv422sp_to_yuy2(
				(uint32_t)jpeg_width, (uint32_t)jpeg_height,
				dst_y, dst_u, dst_v,
				src_y, src_u, src_v);
			break;
		}
		}
	}

	RETURN(result, int);
}

/**
 * Nexus5で1920x1080でも合計45ミリ秒ぐらい。たぶんカラースペースの変換を２回してしまうのが遅くなる原因じゃないかと
 * mjpeg2yuyv_turboでサブサンプリングが444,422, 420じゃないときにフォールバックとして呼ばれる
 * @param src
 * @param dst
 * @param dct_mode
 * @return
 */
static int mjpeg2yuyv(
	const IVideoFrame &src, IVideoFrame &dst,
	const dct_mode_t &dct_mode) {

	if (UNLIKELY(dst.resize(src, RAW_FRAME_UNCOMPRESSED_YUYV)))
		RETURN(USB_ERROR_NO_MEM, int);

	VideoImage_t dst_image {
		.frame_type = dst.frame_type(),
		.width = dst.width(),
		.height = dst.height(),
		.ptr = dst.frame(),
	};
	int result = mjpeg2yuyv(src, dst_image, dct_mode);

	RETURN(result, int);
}

/**
 * turbo-jpeg APIを使ってYUYVに変換する
 * Nexus5で1920x1080でも30ミリ秒ぐらい
 * @param jpegDecompressor libjpeg-turboによるmjpeg展開用
 * @param src 映像入力
 * @param dst 映像出力
 * @param work 変換用ワーク
 * @param dct_mode dctモード
 * @param uyvy false: YUYV(YUV2), true: UYVY
 * @return
 */
static int mjpeg2yuyv_turbo(
	tjhandle &jpegDecompressor,
	const IVideoFrame &src,
	IVideoFrame &dst,
	std::vector<uint8_t> &work,
	const dct_mode_t &dct_mode,
	const bool &uyvy) {

	if (UNLIKELY(dst.resize(src, uyvy ? RAW_FRAME_UNCOMPRESSED_UYVY : RAW_FRAME_UNCOMPRESSED_YUYV))) {
		RETURN(USB_ERROR_NO_MEM, int);
	}

	size_t jpeg_bytes;
	int jpeg_subsamp, jpeg_width, jpeg_height;
	int result = parse_mjpeg_header(jpegDecompressor, src, jpeg_bytes, jpeg_subsamp, jpeg_width, jpeg_height);
	if (result) {
		RETURN(result, int);
	}
	if (UNLIKELY((jpeg_width != src.width()) || (jpeg_height != src.height()))) {
		LOGD("unexpected image size:expected(%dx%d), actual(%dx%d)",
			src.width(), src.height(), jpeg_width, jpeg_height);
		RETURN(VIDEO_ERROR_FRAME, int);
	}
	if (UNLIKELY(result)) {
		LOGD("not a mjpeg frame:%d", result);
		RETURN(USB_ERROR_NOT_SUPPORTED, int);
	}
	if (UNLIKELY((jpeg_width != src.width()) || (jpeg_height != src.height()))) {
		LOGD("frame size is different from actual mjpeg size:(src:%dx%d,actual:%dx%d)",
			src.width(), src.height(), jpeg_width, jpeg_height);
		RETURN(VIDEO_ERROR_FRAME, int);
	}
	// libjpegturbo側関数が本来はconst uint8_t *のところがuint8_t *を受け取るのでキャスト
	auto compressed = (uint8_t *)src.frame();
	if (UNLIKELY(jpeg_subsamp != TJSAMP_422)
		&& (jpeg_subsamp != TJSAMP_420)
		&& (jpeg_subsamp != TJSAMP_444)) {

		// サブサンプリングが合わない時はフォールバックする
		RETURN(mjpeg2yuyv(src, dst, dct_mode), int);
	}
#if 1
	size_t sz_y, sz_u, sz_v;
	uint8_t *dst_planes[3];
	result = prepare_mjpeg_plane(
		jpeg_bytes, jpeg_subsamp,
		jpeg_width, jpeg_height,
		work, dst_planes,
		sz_y, sz_u, sz_v);
	uint8_t *src_y = dst_planes[0];
	uint8_t *src_u = dst_planes[1];
	uint8_t *src_v = dst_planes[2];
	const size_t sz = sz_y + sz_u + sz_v;
	if (!result) {
		// ここで一旦yuv semi planerでworkへデコードする...tjDecompressToYUVPlanesからいつも-1が返ってくる
		result = tjDecompressToYUVPlanes(jpegDecompressor, compressed,
			jpeg_bytes, dst_planes, jpeg_width, nullptr/*strides*/, jpeg_height,
			(dct_mode == DCT_MODE_ISLOW || dct_mode == DCT_MODE_FLOAT)
				? TJFLAG_ACCURATEDCT : TJFLAG_FASTDCT);
		if (UNLIKELY(result)) {
			// has error
#if defined(__ANDROID__)
            const int err = tjGetErrorCode(jpegDecompressor);
    		LOGD("tjDecompressToYUVPlanes failed,result=%d,err=%d,%s",
				result,
				err, tjGetErrorStr2(jpegDecompressor));
            result = err == TJERR_WARNING ? USB_SUCCESS : USB_ERROR_OTHER;
#else
            LOGD("tjDecompressToYUVPlanes failed,result=%d", result);
			result = USB_SUCCESS;
#endif
		}
	}
#else
	// 各プレーンのサイズを取得
	const size_t sz_y = tjPlaneSizeYUV(0, jpeg_width, 0, jpeg_height, jpeg_subsamp);	// = width * height;
	const size_t sz_u = tjPlaneSizeYUV(1, jpeg_width, 0, jpeg_height, jpeg_subsamp);	// = sz_y >> 1;
	const size_t sz_v = tjPlaneSizeYUV(2, jpeg_width, 0, jpeg_height, jpeg_subsamp);	// = sz_y >> 1;
	const size_t sz_work = sz_y + sz_u + sz_v;
	if (UNLIKELY(sz_work > work.size())) {
		try {
			work.resize(sz_work);
		} catch (std::exception & e) {
			LOGE("failed to resize work buffer");
			RETURN(USB_ERROR_NO_MEM, int);
		}
	}
	uint8_t *py = &work[0], *pu = py + sz_y, *pv = py + sz_y + sz_u;
	if (LIKELY(work.size() >= sz_work)) {
		uint8_t *dst_planes[3] = { py, pu, pv };
		const size_t sz = ((uint32_t)jpeg_width * (uint32_t)jpeg_height) << 1;
		if (LIKELY(dst.resize(sz) >= sz)) {	// YUV4:2:2以外のサブサンプリングの時にクラッシュしないように
			// ここで一旦yuv semi planerでworkへデコードする...tjDecompressToYUVPlanesからいつも-1が返ってくる
			tjDecompressToYUVPlanes(jpegDecompressor, compressed,
				jpeg_bytes, dst_planes, jpeg_width, nullptr/*strides*/, jpeg_height,
				(dct_mode == DCT_MODE_ISLOW || dct_mode == DCT_MODE_FLOAT)
					? TJFLAG_ACCURATEDCT : TJFLAG_FASTDCT);
			result = USB_SUCCESS;
		} else {
			LOGW("failed to resize output VideoFrame");
			result = USB_ERROR_NO_MEM;
		}
	} else {
		LOGW("failed to resize work buffer");
		result = USB_ERROR_NO_MEM;
	}
#endif
	if (!result) {
		// ついでyuyv(インターリーブ)へ変換する
		// 各プレーンの幅を取得
		const int w_y = tjPlaneWidth(0, jpeg_width, jpeg_subsamp);
		const int w_u = tjPlaneWidth(1, jpeg_width, jpeg_subsamp);
		const int w_v = tjPlaneWidth(2, jpeg_width, jpeg_subsamp);
//		const int h_y = tjPlaneHeight(0, jpeg_height, jpeg_subsamp);
//		const int h_u = tjPlaneHeight(1, jpeg_height, jpeg_subsamp);
//		const int h_v = tjPlaneHeight(2, jpeg_height, jpeg_subsamp);
//		LOGW("jpegSubsamp=%d,w(%d,%d,%d),h(%d,%d,%d)", jpegSubsamp, w_y, w_u, w_v, h_y, h_u, h_v);

		switch (jpeg_subsamp) {
		case TJSAMP_444:
		{
			uint8_t *dst_y = &dst[uyvy ? 1 : 0];
			uint8_t *dst_u = &dst[uyvy ? 0 : 1];
			uint8_t *dst_v = &dst[uyvy ? 2 : 3];
			result = yuv444sp_to_yuy2(
				(uint32_t) jpeg_width, (uint32_t) jpeg_height,
				dst_y, dst_u, dst_v,
				src_y, src_u, src_v);	// これはテストできてない
			break;
		}
		case TJSAMP_422:	// C910HD, C920r, C922, C930e
		{
			uint8_t *dst_y = &dst[0];
			if (uyvy) {
				result = libyuv::I422ToUYVY(
					src_y, w_y,
					src_u, w_u,
					src_v, w_v,
					dst_y, jpeg_width * 2,
					jpeg_width, jpeg_height);
			} else {
				result = libyuv::I422ToYUY2(
					src_y, w_y,
					src_u, w_u,
					src_v, w_v,
					dst_y, jpeg_width * 2,
					jpeg_width, jpeg_height);
			}
			break;
		}
		case TJSAMP_420:
		{
			uint8_t *dst_y = &dst[0];
			if (uyvy) {
				libyuv::I420ToUYVY(
					src_y, w_y,
					src_u, w_u,
					src_v, w_v,
					dst_y, jpeg_width * 2,
					jpeg_width, jpeg_height);
			} else {
				result = libyuv::I420ToYUY2(
					src_y, w_y,
					src_u, w_u,
					src_v, w_v,
					dst_y, jpeg_width * 2,
					jpeg_width, jpeg_height);
			}
			break;
		}
		case TJSAMP_GRAY:
		case TJSAMP_440:
		case TJSAMP_411:
		default:	// jpegSubsampでフォールバックさせているのでここには来ない
		{
			uint8_t *dst_y = &dst[uyvy ? 1 : 0];
			uint8_t *dst_u = &dst[uyvy ? 0 : 1];
			uint8_t *dst_v = &dst[uyvy ? 2 : 3];
			result = yuv422sp_to_yuy2(
				(uint32_t)jpeg_width, (uint32_t)jpeg_height,
				dst_y, dst_u, dst_v,
				src_y, src_u, src_v);
			break;
		}
		}
	}

	RETURN(result, int);
}

//--------------------------------------------------------------------------------
/**
 * copy_toのヘルパー関数
 * yuyvをdest.frame_typeに変換
 * 変換できなかればUSB_ERROR_NOT_SUPPORTEDを返す
 * @param src
 * @param dst
 * @return
 */
static int yuyv2xxx(
	const IVideoFrame &src, IVideoFrame & dst,
	std::vector<uint8_t> &work) {		// 変換用ワーク

	ENTER();

	int result = USB_ERROR_NOT_SUPPORTED;
	const auto width = (int)src.width();
	const auto height = (int)src.height();
	const uint8_t *src_yuy2 = src.frame();

	switch (dst.frame_type()) {
	case RAW_FRAME_UNCOMPRESSED:
	case RAW_FRAME_UNCOMPRESSED_YUYV:
	case RAW_FRAME_UNCOMPRESSED_YUV_ANY: // OK
		src.copy_to(dst);
		break;
	case RAW_FRAME_UNCOMPRESSED_RGB565: // OK
		result = yuyv2rgb565(src, dst);
		break;
	case RAW_FRAME_UNCOMPRESSED_RGB: // OK
		result = yuyv2rgb(src, dst);
		break;
	case RAW_FRAME_UNCOMPRESSED_BGR: // OK
		result = yuyv2bgr(src, dst);
		break;
	case RAW_FRAME_UNCOMPRESSED_RGBX: // FIXME 赤くなる
		result = yuyv2rgbx(src, dst);
		break;
	case RAW_FRAME_UNCOMPRESSED_BGRX: // OK
	{
		uint8_t *dst_ptr = &dst[0];
		// libyuvのRGB系はリトルエンディアンなのでARGBは実際のRGBAになる
		result = libyuv::YUY2ToARGB(
			src_yuy2, width * 2,
			dst_ptr, width * 4,
			width, height);
		break;
	}
	case RAW_FRAME_UNCOMPRESSED_UYVY:
		break;
	case RAW_FRAME_UNCOMPRESSED_GRAY8: // OK
	{
		uint8_t *dst_y = &dst[0];
		result = libyuv::YUY2ToY(
			src_yuy2, width * 2,
			dst_y, width,
			width, height);
		break;
	}
	case RAW_FRAME_UNCOMPRESSED_BY8:
		break;
	case RAW_FRAME_UNCOMPRESSED_NV21: // OK
	{
		// YUY2 -> I420
		const size_t sz_y = tjPlaneSizeYUV(0, width, 0, height, TJSAMP_420);
		const size_t sz_u = tjPlaneSizeYUV(1, width, 0, height, TJSAMP_420);
		const int w_y = tjPlaneWidth(0, width, TJSAMP_420);
		const int w_u = tjPlaneWidth(1, width, TJSAMP_420);
		const int w_v = tjPlaneWidth(2, width, TJSAMP_420);
		work.resize(sz_y + sz_u * 2);
		uint8_t *y = &work[0];
		uint8_t *u = y + sz_y;
		uint8_t *v = u + sz_u;
		result = libyuv::YUY2ToI420(
			src_yuy2, width * 2,
			y, w_y,
			u, w_u,
			v, w_v,
			width, height);
		// I420 -> NV21
		uint8_t *dst_y = &dst[0];
		uint8_t *dst_uv = dst_y + sz_y;
		result = libyuv::I420ToNV21(
			y, w_y,
			u, w_u,
			v, w_v,
			dst_y, w_y,
			dst_uv, w_u * 2,
			width, height);
		break;
	}
	case RAW_FRAME_UNCOMPRESSED_YV12: // OK
	{
		const size_t sz_y = tjPlaneSizeYUV(0, width, 0, height, TJSAMP_420);
		const size_t sz_u = tjPlaneSizeYUV(1, width, 0, height, TJSAMP_420);
		const int w_y = tjPlaneWidth(0, width, TJSAMP_420);
		const int w_u = tjPlaneWidth(1, width, TJSAMP_420);
		const int w_v = tjPlaneWidth(2, width, TJSAMP_420);
		uint8_t *dst_y = &dst[0];
		uint8_t *dst_u = dst_y + sz_y;
		uint8_t *dst_v = dst_u + sz_u;
		result = libyuv::YUY2ToI420(
			src_yuy2, width * 2,
			dst_y, w_y,
			dst_v, w_v,	// i420のuとvを入れ替えて渡す
			dst_u, w_u,
			width, height);
		break;
	}
	case RAW_FRAME_UNCOMPRESSED_I420: // OK
	{
		const size_t sz_y = tjPlaneSizeYUV(0, width, 0, height, TJSAMP_420);
		const size_t sz_u = tjPlaneSizeYUV(1, width, 0, height, TJSAMP_420);
		const int w_y = tjPlaneWidth(0, width, TJSAMP_420);
		const int w_u = tjPlaneWidth(1, width, TJSAMP_420);
		const int w_v = tjPlaneWidth(2, width, TJSAMP_420);
		uint8_t *dst_y = &dst[0];
		uint8_t *dst_u = dst_y + sz_y;
		uint8_t *dst_v = dst_u + sz_u;
		result = libyuv::YUY2ToI420(
			src_yuy2, width * 2,
			dst_y, w_y,
			dst_u, w_u,
			dst_v, w_v,
			width, height);
		break;
	}
	case RAW_FRAME_UNCOMPRESSED_Y16: // ??
	{
		uint8_t *dst_y = &dst[0];
		result = libyuv::YUY2ToY(
			src_yuy2, width * 2,
			dst_y, width,
			width, height);
		break;
	}
	case RAW_FRAME_UNCOMPRESSED_NV12: // OK
	{
		const size_t sz_y = tjPlaneSizeYUV(0, width, 0, height, TJSAMP_420);
		const size_t sz_uv = tjPlaneSizeYUV(1, width, 0, height, TJSAMP_420) * 2;
		const int w_y = tjPlaneWidth(0, width, TJSAMP_420);
		const int w_uv = tjPlaneWidth(1, width, TJSAMP_420) * 2;
		uint8_t *dst_y = &dst[0];
		uint8_t *dst_uv = dst_y + sz_y;
		result = libyuv::YUY2ToNV12(
			src_yuy2, width * 2,
			dst_y, w_y,
			dst_uv, w_uv,
			width, height);
		break;
	}
	case RAW_FRAME_UNCOMPRESSED_RGBP:
	case RAW_FRAME_UNCOMPRESSED_M420:
	case RAW_FRAME_UNCOMPRESSED_YCbCr:
	case RAW_FRAME_UNCOMPRESSED_422sp:
	default:
		LOGW("unexpected dst frame type 0x%08x", dst.frame_type());
		break;
	}

	RETURN (result, int);
}

/**
 * copy_toのヘルパー関数
 * uyvyをdest.frame_typeに変換
 * 変換できなかればUSB_ERROR_NOT_SUPPORTEDを返す
 * @param src
 * @param dst
 * @return
 */
static int uyvy2xxx(
	const IVideoFrame &src, IVideoFrame & dst,
	std::vector<uint8_t> &work) {	// 変換用ワーク

	ENTER();

	int result = USB_ERROR_NOT_SUPPORTED;
	const auto width = (int)src.width();
	const auto height = (int)src.height();
	const uint8_t *src_uyvy = src.frame();

	switch (dst.frame_type()) {
	case RAW_FRAME_UNCOMPRESSED_RGB565: // OK
		result = uyvy2rgb565(src, dst);
		break;
	case RAW_FRAME_UNCOMPRESSED_RGB: // OK
		result = uyvy2rgb(src, dst);
		break;
	case RAW_FRAME_UNCOMPRESSED_BGR: // OK
		result = uyvy2bgr(src, dst);
		break;
	case RAW_FRAME_UNCOMPRESSED_RGBX: // OK
		result = uyvy2rgbx(src, dst);
		break;
	case RAW_FRAME_UNCOMPRESSED_XRGB:
		break;
	case RAW_FRAME_UNCOMPRESSED_BGRX: // OK
	{
		uint8_t *dst_ptr = &dst[0];
		result = libyuv::UYVYToARGB(
			src_uyvy, width * 2,
			dst_ptr, width * 4,
			width, height);
		break;
	}
	case RAW_FRAME_UNCOMPRESSED_YUYV:
		break;
	case RAW_FRAME_UNCOMPRESSED:
	case RAW_FRAME_UNCOMPRESSED_UYVY:
	case RAW_FRAME_UNCOMPRESSED_YUV_ANY: // OK
	{
		src.copy_to(dst);
		result = USB_SUCCESS;
		break;
	}
	case RAW_FRAME_UNCOMPRESSED_GRAY8: // OK
	{
		uint8_t *dst_y = &dst[0];
		// YUYVとUYVYのYの位置は1バイトずれてるので+1オフセットしたのを渡せばOK
		result = libyuv::YUY2ToY(
			src_uyvy + 1, width * 2,
			dst_y, width,
			width, height);
		break;
	}
	case RAW_FRAME_UNCOMPRESSED_BY8:
		break;
	case RAW_FRAME_UNCOMPRESSED_NV21: // OK
	{
		const size_t sz_y = tjPlaneSizeYUV(0, width, 0, height, TJSAMP_420);
		const size_t sz_uv = tjPlaneSizeYUV(1, width, 0, height, TJSAMP_420) * 2;
		const int w_y = tjPlaneWidth(0, width, TJSAMP_420);
		const int w_vu = tjPlaneWidth(1, width, TJSAMP_420) * 2;
		work.resize(sz_y + sz_uv);
		uint8_t *y = &work[0];
		uint8_t *vu = y + sz_y;
		result = libyuv::UYVYToNV12(
			src_uyvy, width * 2,
			y, w_y,
			vu, w_vu,
			width, height);
		if (!result) {
			uint8_t *dst_y = &dst[0];
			uint8_t *dst_uv = dst_y + sz_y;
			// vuをuvに入れ替えてコピーするのでNV21ToNV12でOK
			result = libyuv::NV21ToNV12(
				y, w_y,
				vu, w_vu,
				dst_y, w_y,
				dst_uv, w_vu,
				width, height);
		}
		break;
	}
	case RAW_FRAME_UNCOMPRESSED_NV12: // OK
	{
		const size_t sz_y = tjPlaneSizeYUV(0, width, 0, height, TJSAMP_420);
		const int w_y = tjPlaneWidth(0, width, TJSAMP_420);
		const int w_vu = tjPlaneWidth(1, width, TJSAMP_420) * 2;
		uint8_t *dst_y = &dst[0];
		uint8_t *dst_vu = dst_y + sz_y;
		result = libyuv::UYVYToNV12(
			src_uyvy, width * 2,
			dst_y, w_y,
			dst_vu, w_vu,
			width, height);
		break;
	}
	case RAW_FRAME_UNCOMPRESSED_YV12: // OK
	{
		const size_t sz_y = tjPlaneSizeYUV(0, width, 0, height, TJSAMP_420);
		const size_t sz_u = tjPlaneSizeYUV(1, width, 0, height, TJSAMP_420);
		const int w_y = tjPlaneWidth(0, width, TJSAMP_420);
		const int w_u = tjPlaneWidth(1, width, TJSAMP_420);
		const int w_v = tjPlaneWidth(2, width, TJSAMP_420);
		uint8_t *dst_y = &dst[0];
		uint8_t *dst_u = dst_y + sz_y;
		uint8_t *dst_v = dst_u + sz_u;
		result = libyuv::UYVYToI420(
			src_uyvy, width * 2,
			dst_y, w_y,
			dst_v, w_v,	// u,vを逆にする
			dst_u, w_u,
			width, height);
		break;
	}
	case RAW_FRAME_UNCOMPRESSED_I420: // OK
	{
		const size_t sz_y = tjPlaneSizeYUV(0, width, 0, height, TJSAMP_420);
		const size_t sz_u = tjPlaneSizeYUV(1, width, 0, height, TJSAMP_420);
		const int w_y = tjPlaneWidth(0, width, TJSAMP_420);
		const int w_u = tjPlaneWidth(1, width, TJSAMP_420);
		const int w_v = tjPlaneWidth(2, width, TJSAMP_420);
		uint8_t *dst_y = &dst[0];
		uint8_t *dst_u = dst_y + sz_y;
		uint8_t *dst_v = dst_u + sz_u;
		result = libyuv::UYVYToI420(
			src_uyvy, width * 2,
			dst_y, w_y,
			dst_u, w_u,
			dst_v, w_v,
			width, height);
		break;
	}
	case RAW_FRAME_UNCOMPRESSED_Y16:
	case RAW_FRAME_UNCOMPRESSED_RGBP:
	case RAW_FRAME_UNCOMPRESSED_M420:
	case RAW_FRAME_UNCOMPRESSED_YCbCr:
		break;
	case RAW_FRAME_UNCOMPRESSED_422p: // OK
	{
		const size_t sz_y = tjPlaneSizeYUV(0, width, 0, height, TJSAMP_422);
		const size_t sz_u = tjPlaneSizeYUV(1, width, 0, height, TJSAMP_422);
		const int w_y = tjPlaneWidth(0, width, TJSAMP_422);
		const int w_u = tjPlaneWidth(1, width, TJSAMP_422);
		const int w_v = tjPlaneWidth(2, width, TJSAMP_422);
		uint8_t *dst_y = &dst[0];
		uint8_t *dst_u = dst_y + sz_y;
		uint8_t *dst_v = dst_u + sz_u;
		result = libyuv::UYVYToI422(
			src_uyvy, width * 2,
			dst_y, w_y,
			dst_u, w_u,
			dst_v, w_v,
			width, height);
		break;
	}
	case RAW_FRAME_UNCOMPRESSED_422sp:
	default:
		LOGW("unexpected dst frame type 0x%08x", dst.frame_type());
		break;
	}

	RETURN(result, int);
}

static int nv21xxx(
	const IVideoFrame &src, IVideoFrame & dst,
	std::vector<uint8_t> &work) {	// 変換用ワーク

	ENTER();

	const auto width = (int)src.width();
	const auto height = (int)src.height();
	const size_t src_sz_y = tjPlaneSizeYUV(0, width, 0, height, TJSAMP_420);
	const size_t src_sz_u = tjPlaneSizeYUV(1, width, 0, height, TJSAMP_420);
	const size_t src_sz_v = tjPlaneSizeYUV(2, width, 0, height, TJSAMP_420);
	const int src_w_y = tjPlaneWidth(0, width, TJSAMP_420);
	const int src_w_u = tjPlaneWidth(1, width, TJSAMP_420);
	const int src_w_v = tjPlaneWidth(2, width, TJSAMP_420);
	const int src_w_vu = src_w_u + src_w_v;
	const uint8_t *src_y = src.frame();
	const uint8_t *src_vu = src_y + src_sz_y;

	int result = USB_ERROR_NOT_SUPPORTED;
	switch (dst.frame_type()) {
	case RAW_FRAME_UNCOMPRESSED_YUYV:
	case RAW_FRAME_UNCOMPRESSED_UYVY:
		break;
	case RAW_FRAME_UNCOMPRESSED_GRAY8: // OK
	{
		uint8_t *dst_y = &dst[0];
		// yプレーンだけをコピーする
		libyuv::CopyPlane(
			src_y, src_w_y,
			dst_y, src_w_y,
			width, height);
		result = USB_SUCCESS;
		break;
	}
	case RAW_FRAME_UNCOMPRESSED_BY8:
		break;
	case RAW_FRAME_UNCOMPRESSED:
	case RAW_FRAME_UNCOMPRESSED_YUV_ANY:
	case RAW_FRAME_UNCOMPRESSED_NV21:
		src.copy_to(dst);
		result = USB_SUCCESS;
		break;
	case RAW_FRAME_UNCOMPRESSED_NV12: // OK
	{
		uint8_t *dst_y = &dst[0];
		uint8_t *dst_uv = dst_y + src_sz_y;
		result = libyuv::NV21ToNV12(
			src_y, src_w_y,
			src_vu, src_w_vu,
			dst_y, src_w_y,
			dst_uv, src_w_vu,
			width, height);
		break;
	}
	case RAW_FRAME_UNCOMPRESSED_YV12: // OK
	{
		uint8_t *dst_y = &dst[0];
		uint8_t *dst_u = dst_y + src_sz_y;
		uint8_t *dst_v = dst_u + src_sz_u;
		result = libyuv::NV21ToI420(
			src_y, src_w_y,
			src_vu, src_w_vu,
			dst_y, src_w_y,
			dst_v, src_w_v, // u,vを逆にする
			dst_u, src_w_u,
			width, height);
		break;
	}
	case RAW_FRAME_UNCOMPRESSED_I420: // OK
	{
		uint8_t *dst_y = &dst[0];
		uint8_t *dst_u = dst_y + src_sz_y;
		uint8_t *dst_v = dst_u + src_sz_u;
		result = libyuv::NV21ToI420(
			src_y, src_w_y,
			src_vu, src_w_vu,
			dst_y, src_w_y,
			dst_u, src_w_u,
			dst_v, src_w_v,
			width, height);
		break;
	}
	case RAW_FRAME_UNCOMPRESSED_422p:
	{
		// FIXME 未実装
		break;
	}
	case RAW_FRAME_UNCOMPRESSED_444p:
	case RAW_FRAME_UNCOMPRESSED_Y16:
	case RAW_FRAME_UNCOMPRESSED_RGBP:
	case RAW_FRAME_UNCOMPRESSED_M420:
	case RAW_FRAME_UNCOMPRESSED_YCbCr:
	case RAW_FRAME_UNCOMPRESSED_RGB565:
		break;
	case RAW_FRAME_UNCOMPRESSED_BGR: // OK
	{	// LOGI("NV21ToRGB24");
		uint8_t *dst_rgb = &dst[0];
		// libyuvのRGB系はリトルエンディアンなのでRGB24は実際のBGRになる
		result = libyuv::NV21ToRGB24(
			src_y, src_w_y,
			src_vu, src_w_vu,
			dst_rgb, width * 3,
			width, height);
		break;
	}
	case RAW_FRAME_UNCOMPRESSED_RGB: // OK
	{	// LOGI("NV21ToRAW");
		uint8_t *dst_ptr = &dst[0];
		result = libyuv::NV21ToRAW(
			src_y, src_w_y,
			src_vu, src_w_vu,
			dst_ptr, width * 3,
			width, height);
		break;
	}
	case RAW_FRAME_UNCOMPRESSED_RGBX: // OK
	{	// LOGI("NV21ToABGR");
		uint8_t *dst_ptr = &dst[0];
		// libyuvのRGB系はリトルエンディアンなのでABGRは実際のRGBAになる...はずだけどおかしい
		result = libyuv::NV21ToABGR(
			src_y, src_w_y,
			src_vu, src_w_vu,
			dst_ptr, width * 4,
			width, height);
		break;
	}
	case RAW_FRAME_UNCOMPRESSED_BGRX: // OK
	{	// LOGI("NV21ToARGB");
		uint8_t *dst_ptr = &dst[0];
		// libyuvのRGB系はリトルエンディアンなのでARGBは実際のBGRAになる
		result = libyuv::NV21ToARGB(
			src_y, src_w_y,
			src_vu, src_w_vu,
			dst_ptr, width * 4,
			width, height);
		break;
	}
	case RAW_FRAME_UNCOMPRESSED_444sp:
	case RAW_FRAME_UNCOMPRESSED_422sp:
	case RAW_FRAME_UNCOMPRESSED_440p:
	case RAW_FRAME_UNCOMPRESSED_440sp:
	case RAW_FRAME_UNCOMPRESSED_411p:
	case RAW_FRAME_UNCOMPRESSED_411sp:
	default:
		LOGW("unexpected dst frame type 0x%08x", dst.frame_type());
		break;
	}
	RETURN(result, int);
}

static int nv12xxx(
	const IVideoFrame &src, IVideoFrame & dst,
	std::vector<uint8_t> &work) {	// 変換用ワーク

	ENTER();

	const auto width = (int)src.width();
	const auto height = (int)src.height();
	const size_t src_sz_y = tjPlaneSizeYUV(0, width, 0, height, TJSAMP_420);
	const size_t src_sz_u = tjPlaneSizeYUV(1, width, 0, height, TJSAMP_420);
	const size_t src_sz_v = tjPlaneSizeYUV(2, width, 0, height, TJSAMP_420);
	const int src_w_y = tjPlaneWidth(0, width, TJSAMP_420);
	const int src_w_u = tjPlaneWidth(1, width, TJSAMP_420);
	const int src_w_v = tjPlaneWidth(2, width, TJSAMP_420);
	const int src_w_uv = src_w_u + src_w_v;
	const uint8_t *src_y = src.frame();
	const uint8_t *src_uv = src_y + src_sz_y;

	int result = USB_ERROR_NOT_SUPPORTED;
	switch (dst.frame_type()) {
	case RAW_FRAME_UNCOMPRESSED_YUYV:
	case RAW_FRAME_UNCOMPRESSED_UYVY:
		break;
	case RAW_FRAME_UNCOMPRESSED_GRAY8: // OK
	{
		uint8_t *dst_y = &dst[0];
		// yプレーンだけをコピーする
		libyuv::CopyPlane(
			src_y, src_w_y,
			dst_y, src_w_y,
			width, height);
		result = USB_SUCCESS;
		break;
	}
	case RAW_FRAME_UNCOMPRESSED_BY8:
		break;
	case RAW_FRAME_UNCOMPRESSED_NV21: // OK
	{
		uint8_t *dst_y = &dst[0];
		uint8_t *dst_vu = dst_y + src_sz_y;
		// uvを入れ替えるだけなのでNV21ToNV12で大丈夫
		result = libyuv::NV21ToNV12(
			src_y, src_w_y,
			src_uv, src_w_uv,
			dst_y, src_w_y,
			dst_vu, src_w_uv,
			width, height);
		break;
	}
	case RAW_FRAME_UNCOMPRESSED:
	case RAW_FRAME_UNCOMPRESSED_YUV_ANY:
	case RAW_FRAME_UNCOMPRESSED_NV12: // OK
		src.copy_to(dst);
		result = USB_SUCCESS;
		break;
	case RAW_FRAME_UNCOMPRESSED_YV12: // OK
	{
		uint8_t *dst_y = &dst[0];
		uint8_t *dst_u = dst_y + src_sz_y;
		uint8_t *dst_v = dst_u + src_sz_u;
		result = libyuv::NV12ToI420(
			src_y, src_w_y,
			src_uv, src_w_uv,
			dst_y, src_w_y,
			dst_v, src_w_v, // u,vを逆にする
			dst_u, src_w_u,
			width, height);
		break;
	}
	case RAW_FRAME_UNCOMPRESSED_I420: // OK
	{
		uint8_t *dst_y = &dst[0];
		uint8_t *dst_u = dst_y + src_sz_y;
		uint8_t *dst_v = dst_u + src_sz_u;
		result = libyuv::NV12ToI420(
			src_y, src_w_y,
			src_uv, src_w_uv,
			dst_y, src_w_y,
			dst_u, src_w_u,
			dst_v, src_w_v,
			width, height);
		break;
	}
	case RAW_FRAME_UNCOMPRESSED_422p:
	{
		// FIXME 未実装
		break;
	}
	case RAW_FRAME_UNCOMPRESSED_444p:
	case RAW_FRAME_UNCOMPRESSED_Y16:
	case RAW_FRAME_UNCOMPRESSED_RGBP:
	case RAW_FRAME_UNCOMPRESSED_M420:
	case RAW_FRAME_UNCOMPRESSED_YCbCr:
		break;
	case RAW_FRAME_UNCOMPRESSED_RGB565: // OK
	{
		uint8_t *dst_ptr = &dst[0];
		result = libyuv::NV12ToRGB565(
			src_y, src_w_y,
			src_uv, src_w_uv,
			dst_ptr, width * 2,
			width, height);
		break;
	}
	case RAW_FRAME_UNCOMPRESSED_BGR: // OK
	{	// LOGI("NV12ToRGB24");
		uint8_t *dst_ptr = &dst[0];
		// libyuvのRGB系はリトルエンディアンなのでRGB24は実際のBGRになる
		result = libyuv::NV12ToRGB24(
			src_y, src_w_y,
			src_uv, src_w_uv,
			dst_ptr, width * 3,
			width, height);
		break;
	}
	case RAW_FRAME_UNCOMPRESSED_RGB: // OK
	{	// LOGI("NV12ToRAW");
		uint8_t *dst_ptr = &dst[0];
		result = libyuv::NV12ToRAW(
			src_y, src_w_y,
			src_uv, src_w_uv,
			dst_ptr, width * 3,
			width, height);
		break;
	}
	case RAW_FRAME_UNCOMPRESSED_RGBX: // OK
	{	// LOGI("NV12ToABGR");
		uint8_t *dst_ptr = &dst[0];
		// libyuvのRGB系はリトルエンディアンなのでABGRは実際のRGBAになる
		result = libyuv::NV12ToABGR(
			src_y, src_w_y,
			src_uv, src_w_uv,
			dst_ptr, width * 4,
			width, height);
		break;
	}
	case RAW_FRAME_UNCOMPRESSED_BGRX: // OK
	{	// LOGI("NV12ToARGB");
		uint8_t *dst_ptr = &dst[0];
		// libyuvのRGB系はリトルエンディアンなのでARGBは実際のBGRAになる
		result = libyuv::NV12ToARGB(
			src_y, src_w_y,
			src_uv, src_w_uv,
			dst_ptr, width * 4,
			width, height);
		break;
	}
	case RAW_FRAME_UNCOMPRESSED_444sp:
	case RAW_FRAME_UNCOMPRESSED_422sp:
	case RAW_FRAME_UNCOMPRESSED_440p:
	case RAW_FRAME_UNCOMPRESSED_440sp:
	case RAW_FRAME_UNCOMPRESSED_411p:
	case RAW_FRAME_UNCOMPRESSED_411sp:
	default:
		LOGW("unexpected dst frame type 0x%08x", dst.frame_type());
		break;
	}
	RETURN(result, int);
}

static int yv12xxx(
	const IVideoFrame &src, IVideoFrame &dst,
	std::vector<uint8_t> &work) {	// 変換用ワーク

	ENTER();

	const auto width = (int)src.width();
	const auto height = (int)src.height();
	const size_t src_sz_y = tjPlaneSizeYUV(0, width, 0, height, TJSAMP_420);
	const size_t src_sz_u = tjPlaneSizeYUV(1, width, 0, height, TJSAMP_420);
	const size_t src_sz_v = tjPlaneSizeYUV(2, width, 0, height, TJSAMP_420);
	const int src_w_y = tjPlaneWidth(0, width, TJSAMP_420);
	const int src_w_u = tjPlaneWidth(1, width, TJSAMP_420);
	const int src_w_v = tjPlaneWidth(2, width, TJSAMP_420);
	const uint8_t *src_y = src.frame();
	const uint8_t *src_v = src_y + src_sz_y;	// I420とU,Vプレーンの位置が逆
	const uint8_t *src_u = src_v + src_sz_v;

	int result = USB_ERROR_NOT_SUPPORTED;
	switch (dst.frame_type()) {
	//case RAW_FRAME_UNCOMPRESSED:
	case RAW_FRAME_UNCOMPRESSED_YUYV: // OK
	{
		uint8_t *dst_yuv = &dst[0];
		result = libyuv::I420ToYUY2(
			src_y, src_w_y,
			src_u, src_w_u,
			src_v, src_w_v,
			dst_yuv, width * 2,
			width, height);
		break;
	}
	case RAW_FRAME_UNCOMPRESSED_UYVY: // OK
	{
		uint8_t *dst_uyvy = &dst[0];
		result = libyuv::I420ToUYVY(
			src_y, src_w_y,
			src_u, src_w_u,
			src_v, src_w_v,
			dst_uyvy, width * 2,
			width, height);
		break;
	}
	case RAW_FRAME_UNCOMPRESSED_GRAY8: // OK
	{
		uint8_t *dst_y = &dst[0];
		// yプレーンだけをコピーする
		libyuv::CopyPlane(
			src_y, src_w_y,
			dst_y, src_w_y,
			width, height);
		result = USB_SUCCESS;
		break;
	}
	//case RAW_FRAME_UNCOMPRESSED_BY8:
	case RAW_FRAME_UNCOMPRESSED_NV21: // OK
	{
		uint8_t *dst_y = &dst[0];
		uint8_t *dst_vu = dst_y + src_sz_y;
		result = libyuv::I420ToNV21(
			src_y, src_w_y,
			src_u, src_w_u,
			src_v, src_w_v,
			dst_y, src_w_y,
			dst_vu, src_w_u * 2,
			width, height);
		break;
	}
	case RAW_FRAME_UNCOMPRESSED_NV12: // OK
	{
		uint8_t *dst_y = &dst[0];
		uint8_t *dst_uv = dst_y + src_sz_y;
		result = libyuv::I420ToNV12(
			src_y, src_w_y,
			src_u, src_w_u,
			src_v, src_w_v,
			dst_y, src_w_y,
			dst_uv, src_w_u * 2,
			width, height);
		break;
	}
	case RAW_FRAME_UNCOMPRESSED_I420: // OK
	{	// LOGI("I420Copy");
		uint8_t *dst_y = &dst[0];
		uint8_t *dst_v = dst_y + src_sz_y;	// uとvを入れ替える
		uint8_t *dst_u = dst_v + src_sz_v;
		result = libyuv::I420Copy(
			src_y, src_w_y,
			src_u, src_w_u,
			src_v, src_w_v,
			dst_y, src_w_y,
			dst_v, src_w_v,
			dst_u, src_w_u,
			width, height);
		break;
	}
	case RAW_FRAME_UNCOMPRESSED:
	case RAW_FRAME_UNCOMPRESSED_YV12:
	case RAW_FRAME_UNCOMPRESSED_YUV_ANY: // OK
		src.copy_to(dst);
		result = USB_SUCCESS;
		break;
	case RAW_FRAME_UNCOMPRESSED_422p: // OK
	{
		const size_t sz_y = tjPlaneSizeYUV(0, width, 0, height, TJSAMP_422);
		const size_t sz_u = tjPlaneSizeYUV(1, width, 0, height, TJSAMP_422);
		const int w_y = tjPlaneWidth(0, width, TJSAMP_422);
		const int w_u = tjPlaneWidth(1, width, TJSAMP_422);
		const int w_v = tjPlaneWidth(2, width, TJSAMP_422);

		uint8_t *dst_y = &dst[0];
		uint8_t *dst_u = dst_y + sz_y;
		uint8_t *dst_v = dst_u + sz_u;
		result = libyuv::I420ToI422(
			src_y, src_w_y,
			src_u, src_w_u,
			src_v, src_w_v,
			dst_y, w_y,
			dst_u, w_u,
			dst_v, w_v,
			width, height);
		break;
	}
	case RAW_FRAME_UNCOMPRESSED_444p: // OK
	{
		const size_t sz_y = tjPlaneSizeYUV(0, width, 0, height, TJSAMP_444);
		const size_t sz_u = tjPlaneSizeYUV(1, width, 0, height, TJSAMP_444);
		const int w_y = tjPlaneWidth(0, width, TJSAMP_444);
		const int w_u = tjPlaneWidth(1, width, TJSAMP_444);
		const int w_v = tjPlaneWidth(2, width, TJSAMP_444);

		uint8_t *dst_y = &dst[0];
		uint8_t *dst_u = dst_y + sz_y;
		uint8_t *dst_v = dst_u + sz_u;
		result = libyuv::I420ToI444(
			src_y, src_w_y,
			src_u, src_w_u,
			src_v, src_w_v,
			dst_y, w_y,
			dst_u, w_u,
			dst_v, w_v,
			width, height);
		break;
	}
	case RAW_FRAME_UNCOMPRESSED_Y16:
	case RAW_FRAME_UNCOMPRESSED_RGBP:
	case RAW_FRAME_UNCOMPRESSED_M420:
	case RAW_FRAME_UNCOMPRESSED_YCbCr:
		break;
	case RAW_FRAME_UNCOMPRESSED_RGB565: // OK
	{
		uint8_t *dst_ptr = &dst[0];
		result = libyuv::I420ToRGB565(
			src_y, src_w_y,
			src_u, src_w_u,
			src_v, src_w_v,
			dst_ptr, width * 2,
			width, height);
		break;
	}
	case RAW_FRAME_UNCOMPRESSED_RGB: // OK
	{	// LOGI("I420ToRAW");
		uint8_t *dst_ptr = &dst[0];
		result = libyuv::I420ToRAW(
			src_y, src_w_y,
			src_u, src_w_u,
			src_v, src_w_v,
			dst_ptr, width * 3,
			width, height);
		break;
	}
	case RAW_FRAME_UNCOMPRESSED_BGR: // OK
	{	// LOGI("I420ToRGB24");
		uint8_t *dst_ptr = &dst[0];
		// libyuvのRGB系はリトルエンディアンなのでBGRAは実際のARGBになる
		result = libyuv::I420ToRGB24(
			src_y, src_w_y,
			src_u, src_w_u,
			src_v, src_w_v,
			dst_ptr, width * 3,
			width, height);
		break;
	}
	case RAW_FRAME_UNCOMPRESSED_XRGB: // OK
	{	// LOGI("I420ToBGRA");
		uint8_t *dst_ptr = &dst[0];
		// libyuvのRGB系はリトルエンディアンなのでBGRAは実際のARGBになる
		result = libyuv::I420ToBGRA(
			src_y, src_w_y,
			src_u, src_w_u,
			src_v, src_w_v,
			dst_ptr, width * 4,
			width, height);
		break;
	}
	case RAW_FRAME_UNCOMPRESSED_XBGR: // OK
	{	// LOGI("I420ToRGBA");
		uint8_t *dst_ptr = &dst[0];
		// libyuvのRGB系はリトルエンディアンなのでRGBAは実際のABGRになる
		result = libyuv::I420ToRGBA(
			src_y, src_w_y,
			src_u, src_w_u,
			src_v, src_w_v,
			dst_ptr, width * 4,
			width, height);
		break;
	}
	case RAW_FRAME_UNCOMPRESSED_BGRX: // OK
	{	// LOGI("I420ToARGB");
		uint8_t *dst_ptr = &dst[0];
		// libyuvのRGB系はリトルエンディアンなのでARGBは実際のBGRAになる
		result = libyuv::I420ToARGB(
			src_y, src_w_y,
			src_u, src_w_u,
			src_v, src_w_v,
			dst_ptr, width * 4,
			width, height);
		break;
	}
	case RAW_FRAME_UNCOMPRESSED_RGBX: // OK
	{	// LOGI("I420ToABGR");
		uint8_t *dst_ptr = &dst[0];
		// libyuvのRGB系はリトルエンディアンなのでARGBは実際のRGBAになる
		result = libyuv::I420ToABGR(
			src_y, src_w_y,
			src_u, src_w_u,
			src_v, src_w_v,
			dst_ptr, width * 4,
			width, height);
		break;
	}
	case RAW_FRAME_UNCOMPRESSED_444sp:
	case RAW_FRAME_UNCOMPRESSED_422sp:
	case RAW_FRAME_UNCOMPRESSED_440p:
	case RAW_FRAME_UNCOMPRESSED_440sp:
	case RAW_FRAME_UNCOMPRESSED_411p:
	case RAW_FRAME_UNCOMPRESSED_411sp:
	default:
		LOGW("unexpected dst frame type 0x%08x", dst.frame_type());
		break;
	}
	RETURN(result, int);
}

static int i420xxx(
	const IVideoFrame &src, IVideoFrame & dst,
	std::vector<uint8_t> &work) {	// 変換用ワーク

	ENTER();

	const auto width = (int)src.width();
	const auto height = (int)src.height();
	const size_t src_sz_y = tjPlaneSizeYUV(0, width, 0, height, TJSAMP_420);
	const size_t src_sz_u = tjPlaneSizeYUV(1, width, 0, height, TJSAMP_420);
	const size_t src_sz_v = tjPlaneSizeYUV(2, width, 0, height, TJSAMP_420);
	const int src_w_y = tjPlaneWidth(0, width, TJSAMP_420);
	const int src_w_u = tjPlaneWidth(1, width, TJSAMP_420);
	const int src_w_v = tjPlaneWidth(2, width, TJSAMP_420);
	const uint8_t *src_y = src.frame();
	const uint8_t *src_u = src_y + src_sz_y;
	const uint8_t *src_v = src_u + src_sz_u;

	int result = USB_ERROR_NOT_SUPPORTED;
	switch (dst.frame_type()) {
	case RAW_FRAME_UNCOMPRESSED_YUYV: // OK
	{
		uint8_t *dst_yuv = &dst[0];
		result = libyuv::I420ToYUY2(
			src_y, src_w_y,
			src_u, src_w_u,
			src_v, src_w_v,
			dst_yuv, width * 2,
			width, height);
		break;
	}
	case RAW_FRAME_UNCOMPRESSED_UYVY: // OK
	{
		uint8_t *dst_uyvy = &dst[0];
		result = libyuv::I420ToUYVY(
			src_y, src_w_y,
			src_u, src_w_u,
			src_v, src_w_v,
			dst_uyvy, width * 2,
			width, height);
		break;
	}
	case RAW_FRAME_UNCOMPRESSED_GRAY8: // OK
	{
		uint8_t *dst_y = &dst[0];
		// yプレーンだけをコピーする
		libyuv::CopyPlane(
			src_y, src_w_y,
			dst_y, src_w_y,
			width, height);
		result = USB_SUCCESS;
		break;
	}
	//case RAW_FRAME_UNCOMPRESSED_BY8:
	case RAW_FRAME_UNCOMPRESSED_NV21: // OK
	{
		uint8_t *dst_y = &dst[0];
		uint8_t *dst_vu = dst_y + src_sz_y;
		result = libyuv::I420ToNV21(
			src_y, src_w_y,
			src_u, src_w_u,
			src_v, src_w_v,
			dst_y, src_w_y,
			dst_vu, src_w_u * 2,
			width, height);
		break;
	}
	case RAW_FRAME_UNCOMPRESSED_NV12: // OK
	{
		uint8_t *dst_y = &dst[0];
		uint8_t *dst_uv = dst_y + src_sz_y;
		result = libyuv::I420ToNV12(
			src_y, src_w_y,
			src_u, src_w_u,
			src_v, src_w_v,
			dst_y, src_w_y,
			dst_uv, src_w_u * 2,
			width, height);
		break;
	}
	case RAW_FRAME_UNCOMPRESSED_YV12: // OK
	{
		uint8_t *dst_y = &dst[0];
		uint8_t *dst_v = dst_y + src_sz_y;	// uとvを入れ替える
		uint8_t *dst_u = dst_v + src_sz_v;
		result = libyuv::I420Copy(
			src_y, src_w_y,
			src_u, src_w_u,
			src_v, src_w_v,
			dst_y, src_w_y,
			dst_u, src_w_u,
			dst_v, src_w_v,
			width, height);
		break;
	}
	case RAW_FRAME_UNCOMPRESSED:
	case RAW_FRAME_UNCOMPRESSED_I420: // OK
	case RAW_FRAME_UNCOMPRESSED_YUV_ANY: // OK
		src.copy_to(dst);
		result = USB_SUCCESS;
		break;
	case RAW_FRAME_UNCOMPRESSED_422p: // OK
	{
		const size_t sz_y = tjPlaneSizeYUV(0, width, 0, height, TJSAMP_422);
		const size_t sz_u = tjPlaneSizeYUV(1, width, 0, height, TJSAMP_422);
		const int w_y = tjPlaneWidth(0, width, TJSAMP_422);
		const int w_u = tjPlaneWidth(1, width, TJSAMP_422);
		const int w_v = tjPlaneWidth(2, width, TJSAMP_422);

		uint8_t *dst_y = &dst[0];
		uint8_t *dst_u = dst_y + sz_y;
		uint8_t *dst_v = dst_u + sz_u;
		result = libyuv::I420ToI422(
			src_y, src_w_y,
			src_u, src_w_u,
			src_v, src_w_v,
			dst_y, w_y,
			dst_u, w_u,
			dst_v, w_v,
			width, height);
		break;
	}
	case RAW_FRAME_UNCOMPRESSED_444p: // OK
	{
		const size_t sz_y = tjPlaneSizeYUV(0, width, 0, height, TJSAMP_444);
		const size_t sz_u = tjPlaneSizeYUV(1, width, 0, height, TJSAMP_444);
		const int w_y = tjPlaneWidth(0, width, TJSAMP_444);
		const int w_u = tjPlaneWidth(1, width, TJSAMP_444);
		const int w_v = tjPlaneWidth(2, width, TJSAMP_444);

		uint8_t *dst_y = &dst[0];
		uint8_t *dst_u = dst_y + sz_y;
		uint8_t *dst_v = dst_u + sz_u;
		result = libyuv::I420ToI444(
			src_y, src_w_y,
			src_u, src_w_u,
			src_v, src_w_v,
			dst_y, w_y,
			dst_u, w_u,
			dst_v, w_v,
			width, height);
		break;
	}
	case RAW_FRAME_UNCOMPRESSED_Y16:
	case RAW_FRAME_UNCOMPRESSED_RGBP:
	case RAW_FRAME_UNCOMPRESSED_M420:
	case RAW_FRAME_UNCOMPRESSED_YCbCr:
		break;
	case RAW_FRAME_UNCOMPRESSED_RGB565: // OK
	{
		uint8_t *dst_ptr = &dst[0];
		result = libyuv::I420ToRGB565(
			src_y, src_w_y,
			src_u, src_w_u,
			src_v, src_w_v,
			dst_ptr, width * 2,
			width, height);
		break;
	}
	case RAW_FRAME_UNCOMPRESSED_RGB: // OK
	{	// LOGI("I420ToRAW");
		uint8_t *dst_ptr = &dst[0];
		result = libyuv::I420ToRAW(
			src_y, src_w_y,
			src_u, src_w_u,
			src_v, src_w_v,
			dst_ptr, width * 3,
			width, height);
		break;
	}
	case RAW_FRAME_UNCOMPRESSED_BGR: // OK
	{	// LOGI("I420ToRGB24");
		uint8_t *dst_ptr = &dst[0];
		// libyuvのRGB系はリトルエンディアンなのでBGRAは実際のARGBになる
		result = libyuv::I420ToRGB24(
			src_y, src_w_y,
			src_u, src_w_u,
			src_v, src_w_v,
			dst_ptr, width * 3,
			width, height);
		break;
	}
	case RAW_FRAME_UNCOMPRESSED_XRGB: // OK
	{	// LOGI("I420ToBGRA");
		uint8_t *dst_ptr = &dst[0];
		// libyuvのRGB系はリトルエンディアンなのでBGRAは実際のARGBになる
		result = libyuv::I420ToBGRA(
			src_y, src_w_y,
			src_u, src_w_u,
			src_v, src_w_v,
			dst_ptr, width * 4,
			width, height);
		break;
	}
	case RAW_FRAME_UNCOMPRESSED_XBGR: // OK
	{	// LOGI("I420ToRGBA");
		uint8_t *dst_ptr = &dst[0];
		// libyuvのRGB系はリトルエンディアンなのでRGBAは実際のABGRになる
		result = libyuv::I420ToRGBA(
			src_y, src_w_y,
			src_u, src_w_u,
			src_v, src_w_v,
			dst_ptr, width * 4,
			width, height);
		break;
	}
	case RAW_FRAME_UNCOMPRESSED_BGRX: // OK
	{	// LOGI("I420ToARGB");
		uint8_t *dst_ptr = &dst[0];
		// libyuvのRGB系はリトルエンディアンなのでARGBは実際のBGRAになる
		result = libyuv::I420ToARGB(
			src_y, src_w_y,
			src_u, src_w_u,
			src_v, src_w_v,
			dst_ptr, width * 4,
			width, height);
		break;
	}
	case RAW_FRAME_UNCOMPRESSED_RGBX: // OK
	{	// LOGI("I420ToABGR");
		uint8_t *dst_ptr = &dst[0];
		// libyuvのRGB系はリトルエンディアンなのでABGRは実際のRGBAになる
		result = libyuv::I420ToABGR(
			src_y, src_w_y,
			src_u, src_w_u,
			src_v, src_w_v,
			dst_ptr, width * 4,
			width, height);
		break;
	}
	case RAW_FRAME_UNCOMPRESSED_444sp:
	case RAW_FRAME_UNCOMPRESSED_422sp:
	case RAW_FRAME_UNCOMPRESSED_440p:
	case RAW_FRAME_UNCOMPRESSED_440sp:
	case RAW_FRAME_UNCOMPRESSED_411p:
	case RAW_FRAME_UNCOMPRESSED_411sp:
	default:
		LOGW("unexpected dst frame type 0x%08x", dst.frame_type());
		break;
	}
	RETURN(result, int);
}

static int yuv422pxxx(
	const IVideoFrame &src, IVideoFrame &dst,
	std::vector<uint8_t> &work) {	// 変換用ワーク

	ENTER();

	const auto width = (int)src.width();
	const auto height = (int)src.height();
	// 各プレーンのサイズ
	const size_t src_sz_y = tjPlaneSizeYUV(0, width, 0, height, TJSAMP_422);
	const size_t src_sz_u = tjPlaneSizeYUV(1, width, 0, height, TJSAMP_422);
	const size_t src_sz_v = tjPlaneSizeYUV(2, width, 0, height, TJSAMP_422);
	// 各プレーンの幅
	const int src_w_y = tjPlaneWidth(0, width, TJSAMP_422);
	const int src_w_u = tjPlaneWidth(1, width, TJSAMP_422);
	const int src_w_v = tjPlaneWidth(2, width, TJSAMP_422);
	const uint8_t *src_y = src.frame();
	const uint8_t *src_u = src_y + src_sz_y;
	const uint8_t *src_v = src_u + src_sz_u;

	int result = USB_ERROR_NOT_SUPPORTED;
	switch (dst.frame_type()) {
	case RAW_FRAME_UNCOMPRESSED_YUYV: // OK
	{
		uint8_t *dst_y = &dst[0];
		result = libyuv::I422ToYUY2(
			src_y, src_w_y,
			src_u, src_w_u,
			src_v, src_w_v,
			dst_y, width * 2,
			width, height);
		break;
	}
	case RAW_FRAME_UNCOMPRESSED_UYVY: // OK
	{
		uint8_t *dst_y = &dst[0];
		result = libyuv::I422ToUYVY(
			src_y, src_w_y,
			src_u, src_w_u,
			src_v, src_w_v,
			dst_y, width * 2,
			width, height);
		break;
	}
	case RAW_FRAME_UNCOMPRESSED_GRAY8: // OK
	{
		uint8_t *dst_y = &dst[0];
		// yプレーンだけをコピーする
		libyuv::CopyPlane(
			src_y, src_w_y,
			dst_y, src_w_y,
			width, height);
		result = USB_SUCCESS;
		break;
	}
	//case RAW_FRAME_UNCOMPRESSED_BY8:
	case RAW_FRAME_UNCOMPRESSED_NV21: // OK
	{
		const size_t sz_y = tjPlaneSizeYUV(0, width, 0, height, TJSAMP_420);
		const int w_y = tjPlaneWidth(0, width, TJSAMP_420);
		const int w_vu = tjPlaneWidth(1, width, TJSAMP_420) * 2;
		uint8_t *dst_y = &dst[0];
		uint8_t *dst_vu = dst_y + sz_y;
		result = libyuv::I422ToNV21(
			src_y, src_w_y,
			src_u, src_w_u,
			src_v, src_w_v,
			dst_y, w_y,
			dst_vu, w_vu,
			width, height);
		break;
	}
	case RAW_FRAME_UNCOMPRESSED_NV12: // OK
	{
		const size_t sz_y = tjPlaneSizeYUV(0, width, 0, height, TJSAMP_420);
		const size_t sz_u = tjPlaneSizeYUV(1, width, 0, height, TJSAMP_420);
		const size_t sz_v = tjPlaneSizeYUV(2, width, 0, height, TJSAMP_420);
		const size_t sz_uv = sz_u + sz_v;
		const int w_y = tjPlaneWidth(0, width, TJSAMP_420);
		const int w_u = tjPlaneWidth(1, width, TJSAMP_420);
		const int w_v = tjPlaneWidth(2, width, TJSAMP_420);
		work.resize(sz_uv);
		if (work.size() >= sz_uv) {
			uint8_t *dst_y = &dst[0];
			uint8_t *plane_u = &work[0];
			uint8_t *plane_v = plane_u + sz_u;
			result = libyuv::I422ToI420(
				src_y, src_w_y,
				src_u, src_w_u,
				src_v, src_w_v,
				dst_y, w_y,
				plane_u, w_u,
				plane_v, w_v,
				width, height);
			uint8_t *dst_uv = dst_y + sz_y;
			libyuv::MergeUVPlane(
				plane_u, w_u,
				plane_v, w_v,
				dst_uv, w_u + w_v,
				w_u, height / 2);
		} else {
			result = USB_ERROR_NO_MEM;
		}
		break;
	}
	case RAW_FRAME_UNCOMPRESSED_YV12: // OK
	{
		const size_t sz_y = tjPlaneSizeYUV(0, width, 0, height, TJSAMP_420);
		const size_t sz_u = tjPlaneSizeYUV(1, width, 0, height, TJSAMP_420);
		const int w_y = tjPlaneWidth(0, width, TJSAMP_420);
		const int w_u = tjPlaneWidth(1, width, TJSAMP_420);
		const int w_v = tjPlaneWidth(2, width, TJSAMP_420);
		uint8_t *dst_y = &dst[0];
		uint8_t *dst_u = dst_y + sz_y;	// uとvを逆にして呼び出す
		uint8_t *dst_v = dst_u + sz_u;
		result = libyuv::I422ToI420(
			src_y, src_w_y,
			src_u, src_w_u,
			src_v, src_w_v,
			dst_y, w_y,
			dst_v, w_v, // uとvを逆にして呼び出す
			dst_u, w_u,
			width, height);
		break;
	}
	case RAW_FRAME_UNCOMPRESSED_I420: // OK
	{
		const size_t sz_y = tjPlaneSizeYUV(0, width, 0, height, TJSAMP_420);
		const size_t sz_u = tjPlaneSizeYUV(1, width, 0, height, TJSAMP_420);
		const int w_y = tjPlaneWidth(0, width, TJSAMP_420);
		const int w_u = tjPlaneWidth(1, width, TJSAMP_420);
		const int w_v = tjPlaneWidth(2, width, TJSAMP_420);
		uint8_t *dst_y = &dst[0];
		uint8_t *dst_u = dst_y + sz_y;
		uint8_t *dst_v = dst_u + sz_u;
		result = libyuv::I422ToI420(
			src_y, src_w_y,
			src_u, src_w_u,
			src_v, src_w_v,
			dst_y, w_y,
			dst_u, w_u,
			dst_v, w_v,
			width, height);
		break;
	}
	case RAW_FRAME_UNCOMPRESSED:
	case RAW_FRAME_UNCOMPRESSED_YUV_ANY: // OK
	case RAW_FRAME_UNCOMPRESSED_422p: // OK
	{
		src.copy_to(dst);
		result = USB_SUCCESS;
		break;
	}
	case RAW_FRAME_UNCOMPRESSED_444p: // OK
	{
		const size_t sz_y = tjPlaneSizeYUV(0, width, 0, height, TJSAMP_444);
		const size_t sz_u = tjPlaneSizeYUV(1, width, 0, height, TJSAMP_444);
		const int w_y = tjPlaneWidth(0, width, TJSAMP_444);
		const int w_u = tjPlaneWidth(1, width, TJSAMP_444);
		const int w_v = tjPlaneWidth(2, width, TJSAMP_444);
		uint8_t *dst_y = &dst[0];
		uint8_t *dst_u = dst_y + sz_y;
		uint8_t *dst_v = dst_u + sz_u;
		result = libyuv::I422ToI444(
			src_y, src_w_y,
			src_u, src_w_u,
			src_v, src_w_v,
			dst_y, w_y,
			dst_u, w_u,
			dst_v, w_v,
			width, height);
		break;
	}
	case RAW_FRAME_UNCOMPRESSED_Y16:
	case RAW_FRAME_UNCOMPRESSED_RGBP:
	case RAW_FRAME_UNCOMPRESSED_M420:
	case RAW_FRAME_UNCOMPRESSED_YCbCr:
		break;
	case RAW_FRAME_UNCOMPRESSED_RGB565: // OK
	{
		uint8_t *dst_ptr = &dst[0];
		result = libyuv::I422ToRGB565(
			src_y, src_w_y,
			src_u, src_w_u,
			src_v, src_w_v,
			dst_ptr, width * 2,
			width, height
		);
		break;
	}
	case RAW_FRAME_UNCOMPRESSED_RGB:
	{	// LOGI("I422ToARGB");
		work.resize(width * height * 4);
		uint8_t *work_ptr = &work[0];
		// libyuvのRGB系はリトルエンディアンなのでARGBは実際のBGRAになる
		result = libyuv::I422ToARGB(
			src_y, src_w_y,
			src_u, src_w_u,
			src_v, src_w_v,
			work_ptr, width * 4,
			width, height
		);
		// LOGI("ARGBToRAW");
		uint8_t *dst_ptr = &dst[0];
		result = libyuv::ARGBToRAW(
			work_ptr, width * 4,
			dst_ptr, width * 3,
			width, height);
		break;
	}
	case RAW_FRAME_UNCOMPRESSED_BGR:
	{	// LOGI("RAW_FRAME_UNCOMPRESSED_BGR");
		work.resize(width * height * 4);
		uint8_t *work_ptr = &work[0];
		// libyuvのRGB系はリトルエンディアンなのでABGRは実際のRGBAになる
		result = libyuv::I422ToABGR(
			src_y, src_w_y,
			src_u, src_w_u,
			src_v, src_w_v,
			work_ptr, width * 4,
			width, height
		);
		// LOGI("ARGBToRAW");
		//
		uint8_t *dst_ptr = &dst[0];
		// ABGRをARGBとして引き渡すので結果はBGRになる
		result = libyuv::ARGBToRAW(
			work_ptr, width * 4,
			dst_ptr, width * 3,
			width, height);
		break;
	}
	case RAW_FRAME_UNCOMPRESSED_XBGR: // OK
	{	// LOGI("I422ToRGBA");
		uint8_t *dst_ptr = &dst[0];
		// libyuvのRGB系はリトルエンディアンなのでRGBAは実際のABGRになる
		result = libyuv::I422ToRGBA(
			src_y, src_w_y,
			src_u, src_w_u,
			src_v, src_w_v,
			dst_ptr, width * 4,
			width, height
		);
		break;
	}
	case RAW_FRAME_UNCOMPRESSED_BGRX: // OK
	{	// LOGI("I422ToARGB");
		uint8_t *dst_ptr = &dst[0];
		// libyuvのRGB系はリトルエンディアンなのでARGBは実際のBGRAになる
		result = libyuv::I422ToARGB(
			src_y, src_w_y,
			src_u, src_w_u,
			src_v, src_w_v,
			dst_ptr, width * 4,
			width, height
		);
		break;
	}
	case RAW_FRAME_UNCOMPRESSED_XRGB: // OK
	{	// LOGI("I422ToBGRA");
		uint8_t *dst_ptr = &dst[0];
		// libyuvのRGB系はリトルエンディアンなのでBGRAは実際のARGBになる
		result = libyuv::I422ToBGRA(
			src_y, src_w_y,
			src_u, src_w_u,
			src_v, src_w_v,
			dst_ptr, width * 4,
			width, height
		);
		break;
	}
	case RAW_FRAME_UNCOMPRESSED_RGBX: // OK
	{	// LOGI("I422ToABGR");
		uint8_t *dst_ptr = &dst[0];
		// libyuvのRGB系はリトルエンディアンなのでABGRは実際のRGBAになる
		result = libyuv::I422ToABGR(
			src_y, src_w_y,
			src_u, src_w_u,
			src_v, src_w_v,
			dst_ptr, width * 4,
			width, height
		);
		break;
	}
	case RAW_FRAME_UNCOMPRESSED_444sp:
	case RAW_FRAME_UNCOMPRESSED_422sp:
	case RAW_FRAME_UNCOMPRESSED_440p:
	case RAW_FRAME_UNCOMPRESSED_440sp:
	case RAW_FRAME_UNCOMPRESSED_411p:
	case RAW_FRAME_UNCOMPRESSED_411sp:
	default:
		LOGW("unexpected dst frame type 0x%08x", dst.frame_type());
		break;
	}
	RETURN(result, int);
}

static int yuv444pxxx(
	const IVideoFrame &src, IVideoFrame &dst,
	std::vector<uint8_t> &work) {	// 変換用ワーク

	ENTER();

	const auto width = (int)src.width();
	const auto height = (int)src.height();
	const size_t src_sz_y = tjPlaneSizeYUV(0, width, 0, height, TJSAMP_444);
	const size_t src_sz_u = tjPlaneSizeYUV(1, width, 0, height, TJSAMP_444);
	const size_t src_sz_v = tjPlaneSizeYUV(2, width, 0, height, TJSAMP_444);
	const int src_w_y = tjPlaneWidth(0, width, TJSAMP_444);
	const int src_w_u = tjPlaneWidth(1, width, TJSAMP_444);
	const int src_w_v = tjPlaneWidth(2, width, TJSAMP_444);
	const uint8_t *src_y = src.frame();
	const uint8_t *src_u = src_y + src_sz_y;
	const uint8_t *src_v = src_u + src_sz_u;

	int result = USB_ERROR_NOT_SUPPORTED;
	switch (dst.frame_type()) {
	//case RAW_FRAME_UNCOMPRESSED:
	case RAW_FRAME_UNCOMPRESSED_YUYV:
	case RAW_FRAME_UNCOMPRESSED_UYVY:
		break;
	case RAW_FRAME_UNCOMPRESSED_GRAY8: // OK
	{
		uint8_t *dst_y = &dst[0];
		// yプレーンだけをコピーする
		libyuv::CopyPlane(
			src_y, src_w_y,
			dst_y, src_w_y,
			width, height);
		result = USB_SUCCESS;
		break;
	}
	//case RAW_FRAME_UNCOMPRESSED_BY8:
		break;
	case RAW_FRAME_UNCOMPRESSED_NV21: // OK
	{
		const size_t sz_y = tjPlaneSizeYUV(0, width, 0, height, TJSAMP_420);
		const int w_y = tjPlaneWidth(0, width, TJSAMP_420);
		const int w_vu = tjPlaneWidth(1, width, TJSAMP_420) * 2;
		uint8_t *dst_y = &dst[0];
		uint8_t *dst_vu = dst_y + sz_y;
		result = libyuv::I444ToNV21(
			src_y, src_w_y,
			src_u, src_w_u,
			src_v, src_w_v,
			dst_y, w_y,
			dst_vu, w_vu,
			width, height);
		break;
	}
	case RAW_FRAME_UNCOMPRESSED_NV12: // OK
	{
		const size_t sz_y = tjPlaneSizeYUV(0, width, 0, height, TJSAMP_420);
		const int w_y = tjPlaneWidth(0, width, TJSAMP_420);
		const int w_uv = tjPlaneWidth(1, width, TJSAMP_420) * 2;
		uint8_t *dst_y = &dst[0];
		uint8_t *dst_uv = dst_y + sz_y;
		result = libyuv::I444ToNV12(
			src_y, src_w_y,
			src_u, src_w_u,
			src_v, src_w_v,
			dst_y, w_y,
			dst_uv, w_uv,
			width, height);
		break;
	}
	case RAW_FRAME_UNCOMPRESSED_YV12: // OK
	{
		const size_t sz_y = tjPlaneSizeYUV(0, width, 0, height, TJSAMP_420);
		const size_t sz_u = tjPlaneSizeYUV(1, width, 0, height, TJSAMP_420);
		const int w_y = tjPlaneWidth(0, width, TJSAMP_420);
		const int w_u = tjPlaneWidth(1, width, TJSAMP_420);
		const int w_v = tjPlaneWidth(2, width, TJSAMP_420);
		uint8_t *dst_y = &dst[0];
		uint8_t *dst_u = dst_y + sz_y;	// uとvを逆にして呼び出す
		uint8_t *dst_v = dst_u + sz_u;
		result = libyuv::I444ToI420(
			src_y, src_w_y,
			src_u, src_w_u,
			src_v, src_w_v,
			dst_y, w_y,
			dst_v, w_v, // uとvを逆にして呼び出す
			dst_u, w_u,
			width, height);
		break;
	}
	case RAW_FRAME_UNCOMPRESSED_I420: // OK
	{
		const size_t sz_y = tjPlaneSizeYUV(0, width, 0, height, TJSAMP_420);
		const size_t sz_u = tjPlaneSizeYUV(1, width, 0, height, TJSAMP_420);
		const int w_y = tjPlaneWidth(0, width, TJSAMP_420);
		const int w_u = tjPlaneWidth(1, width, TJSAMP_420);
		const int w_v = tjPlaneWidth(2, width, TJSAMP_420);
		uint8_t *dst_y = &dst[0];
		uint8_t *dst_u = dst_y + sz_y;
		uint8_t *dst_v = dst_u + sz_u;
		result = libyuv::I444ToI420(
			src_y, src_w_y,
			src_u, src_w_u,
			src_v, src_w_v,
			dst_y, w_y,
			dst_u, w_u,
			dst_v, w_v,
			width, height);
		break;
	}
	case RAW_FRAME_UNCOMPRESSED_422p: // OK
	{
		const size_t sz_y = tjPlaneSizeYUV(0, width, 0, height, TJSAMP_422);
		const size_t sz_u = tjPlaneSizeYUV(1, width, 0, height, TJSAMP_422);
		const int w_y = tjPlaneWidth(0, width, TJSAMP_422);
		const int w_u = tjPlaneWidth(1, width, TJSAMP_422);
		const int w_v = tjPlaneWidth(2, width, TJSAMP_422);
		uint8_t *dst_y = &dst[0];
		uint8_t *dst_u = dst_y + sz_y;
		uint8_t *dst_v = dst_u + sz_u;

		// 多少uvの解像度が下がるのかもしれないけどARGBを経由するより速い
		const size_t sz_y0 = tjPlaneSizeYUV(0, width, 0, height, TJSAMP_420);
		const size_t sz_u0 = tjPlaneSizeYUV(1, width, 0, height, TJSAMP_420);
		const size_t sz_v0 = tjPlaneSizeYUV(2, width, 0, height, TJSAMP_420);
		const size_t sz0 = sz_y0 + sz_u0 + sz_v0;
		const int w_y0 = tjPlaneWidth(0, width, TJSAMP_420);
		const int w_u0 = tjPlaneWidth(1, width, TJSAMP_420);
		const int w_v0 = tjPlaneWidth(2, width, TJSAMP_420);
		work.resize(sz0);
		if (work.size() >= sz0) {
			uint8_t *work_y = &work[0];
			uint8_t *work_u = work_y + sz_y0;
			uint8_t *work_v = work_u + sz_u0;
			libyuv::I444ToI420(
				src_y, src_w_y,
				src_u, src_w_u,
				src_v, src_w_v,
				work_y, w_y0,
				work_u, w_u0,
				work_v, w_v0,
				width, height);
			result = libyuv::I420ToI422(
				work_y, w_y0,
				work_u, w_u0,
				work_v, w_v0,
				dst_y, w_y,
				dst_u, w_u,
				dst_v, w_v,
				width, height);
		} else {
			result = USB_ERROR_NO_MEM;
		}
		break;
	}
	case RAW_FRAME_UNCOMPRESSED:
	case RAW_FRAME_UNCOMPRESSED_YUV_ANY:
	case RAW_FRAME_UNCOMPRESSED_444p:
		src.copy_to(dst);
		break;
	case RAW_FRAME_UNCOMPRESSED_Y16:
	case RAW_FRAME_UNCOMPRESSED_RGBP:
	case RAW_FRAME_UNCOMPRESSED_M420:
	case RAW_FRAME_UNCOMPRESSED_YCbCr:
		break;
	case RAW_FRAME_UNCOMPRESSED_RGB565: // OK
	{	// I444からRGB565は直接変換する関数がないのでI444 → NV12 → RGB565とする
		const size_t sz0 = get_pixel_bytes(RAW_FRAME_UNCOMPRESSED_NV12).frame_bytes(width, height);
		work.resize(sz0);
		if (work.size() >= sz0) {
			const size_t sz_y = tjPlaneSizeYUV(0, width, 0, height, TJSAMP_420);
			const int w_y = tjPlaneWidth(0, width, TJSAMP_420);
			const int w_uv = tjPlaneWidth(1, width, TJSAMP_420) * 2;
			uint8_t *work_y = &work[0];
			uint8_t *work_uv = work_y + sz_y;
			result = libyuv::I444ToNV12(
				src_y, src_w_y,
				src_u, src_w_u,
				src_v, src_w_v,
				work_y, w_y,
				work_uv, w_uv,
				width, height);
			if (!result) {
				uint8_t *dst_ptr = &dst[0];
				result = libyuv::NV12ToRGB565(
					work_y, w_y,
					work_uv, w_uv,
					dst_ptr, width * 2,
					width, height);
			}
		}
		break;
	}
	case RAW_FRAME_UNCOMPRESSED_RGB:
	case RAW_FRAME_UNCOMPRESSED_BGR:
	case RAW_FRAME_UNCOMPRESSED_XRGB:
		break;
	case RAW_FRAME_UNCOMPRESSED_RGBX: // OK
	{	// LOGI("I444ToABGR");
		uint8_t *dst_ptr = &dst[0];
		result = libyuv::I444ToABGR(
			src_y, src_w_y,
			src_u, src_w_u,
			src_v, src_w_v,
			dst_ptr, width * 4,
			width, height);
		break;
	}
	case RAW_FRAME_UNCOMPRESSED_BGRX: // OK
	{	// LOGI("I444ToARGB");
		uint8_t *dst_ptr = &dst[0];
		result = libyuv::I444ToARGB(
			src_y, src_w_y,
			src_u, src_w_u,
			src_v, src_w_v,
			dst_ptr, width * 4,
			width, height);
		break;
	}
	case RAW_FRAME_UNCOMPRESSED_444sp:
	case RAW_FRAME_UNCOMPRESSED_422sp:
	case RAW_FRAME_UNCOMPRESSED_440p:
	case RAW_FRAME_UNCOMPRESSED_440sp:
	case RAW_FRAME_UNCOMPRESSED_411p:
	case RAW_FRAME_UNCOMPRESSED_411sp:
	default:
		LOGW("unexpected dst frame type 0x%08x", dst.frame_type());
		break;
	}
	RETURN(result, int);
}

/**
 * copy_toのヘルパー関数
 * rgbをdest.frame_typeに変換
 * 変換できなかればUSB_ERROR_NOT_SUPPORTEDを返す
 * @param src
 * @param dst
 * @return
 */
static int rgb2xxx(
	const IVideoFrame &src, IVideoFrame & dst,
	std::vector<uint8_t> &work) {	// 変換用ワーク

	ENTER();

	int result = USB_ERROR_NOT_SUPPORTED;

	switch (dst.frame_type()) {
	case RAW_FRAME_UNCOMPRESSED_RGB565:
		result = rgb2rgb565(src, dst);
		break;
	case RAW_FRAME_UNCOMPRESSED_RGBX:
		result = rgb2rgbx(src, dst);
		break;
	case RAW_FRAME_UNCOMPRESSED_XRGB:
	case RAW_FRAME_UNCOMPRESSED_YUYV:
	case RAW_FRAME_UNCOMPRESSED_UYVY:
	case RAW_FRAME_UNCOMPRESSED_GRAY8:
	case RAW_FRAME_UNCOMPRESSED_BY8:
	case RAW_FRAME_UNCOMPRESSED_NV21:
	case RAW_FRAME_UNCOMPRESSED_YV12:
	case RAW_FRAME_UNCOMPRESSED_I420:
	case RAW_FRAME_UNCOMPRESSED_Y16:
	case RAW_FRAME_UNCOMPRESSED_RGBP:
	case RAW_FRAME_UNCOMPRESSED_M420:
	case RAW_FRAME_UNCOMPRESSED_NV12:
	case RAW_FRAME_UNCOMPRESSED_YCbCr:
	case RAW_FRAME_UNCOMPRESSED_444sp:
	case RAW_FRAME_UNCOMPRESSED_422sp:
	case RAW_FRAME_UNCOMPRESSED_440p:
	case RAW_FRAME_UNCOMPRESSED_440sp:
	case RAW_FRAME_UNCOMPRESSED_411p:
	case RAW_FRAME_UNCOMPRESSED_411sp:
	default:
		LOGW("unexpected dst frame type 0x%08x", dst.frame_type());
		break;
	}

	RETURN(result, int);
}

/**
 * copy_toのヘルパー関数
 * mjpegをdest.frame_typeに変換
 * 変換できなかればUSB_ERROR_NOT_SUPPORTEDを返す
 * @param src
 * @param dst
 * @param jpegDecompressor
 * @param dct_mode
 * @param work
 * @return
 */
static int mjpeg2xxx(
	tjhandle &jpegDecompressor,
	const dct_mode_t &dct_mode,
	const IVideoFrame &src, IVideoFrame &dst,
	std::vector<uint8_t> &work1,	// 変換用ワーク
	std::vector<uint8_t> &work2) {	// 変換用ワーク

	ENTER();

	int result = USB_ERROR_NOT_SUPPORTED;

	switch (dst.frame_type()) {
	case RAW_FRAME_MJPEG:
		src.copy_to(dst);
		result = USB_SUCCESS;
		break;
//	case RAW_FRAME_UNCOMPRESSED:
	case RAW_FRAME_UNCOMPRESSED_YUYV:
	{
		result = mjpeg2yuyv_turbo(jpegDecompressor, src, dst, work1, dct_mode, false);
		break;
	}
	case RAW_FRAME_UNCOMPRESSED_UYVY:
	{
		result = mjpeg2yuyv_turbo(jpegDecompressor, src, dst, work1, dct_mode, true);
		break;
	}
	case RAW_FRAME_UNCOMPRESSED_RGB565:
		// これはlibjpeg-turboに対応する処理がないのでlibjpegとして呼び出す
		result = mjpeg2rgb(src, dst, JCS_RGB565, dct_mode);
		break;
	case RAW_FRAME_UNCOMPRESSED_RGB:
		result = mjpeg2rgb_turbo(jpegDecompressor, src, dst, TJPF_RGB, dct_mode);
		if (UNLIKELY(result)) {
			LOGD("フォールバック");
			result = mjpeg2rgb(src, dst, JCS_RGB, dct_mode);
		}
		break;
	case RAW_FRAME_UNCOMPRESSED_BGR:
		result = mjpeg2rgb_turbo(jpegDecompressor, src, dst, TJPF_BGR, dct_mode);
		if (UNLIKELY(result)) {
			LOGD("フォールバック");
			result = mjpeg2rgb(src, dst, JCS_EXT_BGR, dct_mode);
		}
		break;
	case RAW_FRAME_UNCOMPRESSED_RGBX:
		result = mjpeg2rgb_turbo(jpegDecompressor, src, dst, TJPF_RGBA, dct_mode);
		if (UNLIKELY(result)) {
			LOGD("フォールバック");
			result = mjpeg2rgb(src, dst, JCS_EXT_RGBA, dct_mode);
		}
		break;
	case RAW_FRAME_UNCOMPRESSED_XRGB:
		result = mjpeg2rgb_turbo(jpegDecompressor, src, dst, TJPF_ARGB, dct_mode);
		if (UNLIKELY(result)) {
			LOGD("フォールバック");
			result = mjpeg2rgb(src, dst, JCS_EXT_ARGB, dct_mode);
		}
		break;
	case RAW_FRAME_UNCOMPRESSED_BGRX:
		result = mjpeg2rgb_turbo(jpegDecompressor, src, dst, TJPF_BGRA, dct_mode);
		if (UNLIKELY(result)) {
			LOGD("フォールバック");
			result = mjpeg2rgb(src, dst, JCS_EXT_BGRA, dct_mode);
		}
		break;
	case RAW_FRAME_UNCOMPRESSED_XBGR:
		result = mjpeg2rgb_turbo(jpegDecompressor, src, dst, TJPF_ABGR, dct_mode);
		if (UNLIKELY(result)) {
			LOGD("フォールバック");
			result = mjpeg2rgb(src, dst, JCS_EXT_ABGR, dct_mode);
		}
		break;
	case RAW_FRAME_UNCOMPRESSED_GRAY8:
		result = mjpeg2rgb_turbo(jpegDecompressor, src, dst, TJPF_GRAY, dct_mode);
		if (UNLIKELY(result)) {
			LOGD("フォールバック");
//			result = mjpeg2rgb(src, dst, JCS_GRAYSCALE, dct_mode); // XXX これはなぜか1フレーム目にクラッシュするときがある
			result = mjpeg2YUVxxx_turbo(jpegDecompressor, src, dst, work1, work2, dct_mode);
		}
		break;
	case RAW_FRAME_UNCOMPRESSED_YCbCr:
		// これはlibjpeg-turboに対応する処理がないのでlibjpegとして呼び出す
		 result = mjpeg2rgb(src, dst, JCS_YCbCr, dct_mode);
		 break;
	//case RAW_FRAME_UNCOMPRESSED_BY8:
	//case RAW_FRAME_UNCOMPRESSED_Y16:
	//case RAW_FRAME_UNCOMPRESSED_RGBP:
	//	break;
	case RAW_FRAME_UNCOMPRESSED_NV21:	// YUV420sp(y->vu)
	case RAW_FRAME_UNCOMPRESSED_YV12:	// YUV420p(y->v->u)
	case RAW_FRAME_UNCOMPRESSED_I420:	// YUV420p(y->u->v)
	case RAW_FRAME_UNCOMPRESSED_M420:
	case RAW_FRAME_UNCOMPRESSED_NV12:	// YUV420sp(y->uv)
	case RAW_FRAME_UNCOMPRESSED_444p:
	case RAW_FRAME_UNCOMPRESSED_444sp:
	case RAW_FRAME_UNCOMPRESSED_422p:
	case RAW_FRAME_UNCOMPRESSED_422sp:
	case RAW_FRAME_UNCOMPRESSED_440p:
	case RAW_FRAME_UNCOMPRESSED_440sp:
	case RAW_FRAME_UNCOMPRESSED_411p:
	case RAW_FRAME_UNCOMPRESSED_411sp:
		result = mjpeg2YUVxxx_turbo(jpegDecompressor, src, dst, work1, work2, dct_mode);
		break;
	case RAW_FRAME_UNCOMPRESSED_YUV_ANY:
		result = mjpeg2YUVAnyPlanner_turbo(jpegDecompressor, src, dst, dct_mode);
		break;
	default:
		LOGW("Unsupported dst frame format,0x%08x", dst.frame_type());
		break;
	}

	RETURN(result, int);
}

//--------------------------------------------------------------------------------
static int mjpeg2YUVAnyPlanner_turbo(tjhandle &jpegDecompressor,
	const IVideoFrame &src, VideoImage_t &dst,
	const dct_mode_t &dct_mode) {

	ENTER();
	
	size_t jpeg_bytes;
	int jpeg_subsamp, jpeg_width, jpeg_height;
	int result = parse_mjpeg_header(jpegDecompressor, src, jpeg_bytes, jpeg_subsamp, jpeg_width, jpeg_height);
	if (UNLIKELY(result)) {
		LOGD("parse_mjpeg_header failed,err=%d", result);
		RETURN(result, int);
	}
	if (UNLIKELY((jpeg_width != src.width()) || (jpeg_height != src.height()))) {
		// 入力映像フレームに指定されている映像サイズとjpegのヘッダーから取得した映像サイズが異なる場合はエラー
		LOGD("unexpected image size:expected(%dx%d), actual(%dx%d)",
			src.width(), src.height(), jpeg_width, jpeg_height);
		RETURN(VIDEO_ERROR_FRAME, int);
	}
	// libjpegturbo側関数が本来はconst uint8_t *のところがuint8_t *を受け取るのでキャスト
	auto compressed = (uint8_t *)src.frame();
	uint8_t *dst_planes[3] {
		dst.ptr_y,
		dst.ptr_u,
		dst.ptr_v,
	};

	result = tjDecompressToYUVPlanes(jpegDecompressor, compressed,
		jpeg_bytes, dst_planes, jpeg_width, nullptr/*strides*/, jpeg_height,
			(dct_mode == DCT_MODE_ISLOW || dct_mode == DCT_MODE_FLOAT)
				? TJFLAG_ACCURATEDCT : TJFLAG_FASTDCT);
	if (UNLIKELY(result)) {
		// has error
		const int err = tjGetErrorCode(jpegDecompressor);
#if defined(__ANDROID__)
		LOGD("tjDecompressToYUVPlanes failed,result=%d,err=%d,%s",
			result,
			err, tjGetErrorStr2(jpegDecompressor));
#else
           LOGD("tjDecompressToYUVPlanes failed,result=%d", result);
#endif
		result = err == TJERR_WARNING ? USB_SUCCESS : USB_ERROR_OTHER;
	}
	RETURN(result, int);
}

/**
 * mjpegを展開して必要であれば指定されたフレームフォーマットになるように変換する
 * @param jpegDecompressor
 * @param src
 * @param dst
 * @param work
 * @param dct_mode
 * @return
 */
static int mjpeg2YUVxxx_turbo(
	tjhandle &jpegDecompressor,
	const IVideoFrame &src, VideoImage_t &dst,
	std::vector<uint8_t> &work1,
	std::vector<uint8_t> &work2,
	const dct_mode_t &dct_mode) {

	if (UNLIKELY(!jpegDecompressor)) {
		RETURN(USB_ERROR_INVALID_PARAM, int);
	}

	// (m)jpegのサイズやサブサンプリングを取得する
	size_t jpeg_bytes;
	int jpeg_subsamp, jpeg_width, jpeg_height;
	int result = parse_mjpeg_header(jpegDecompressor, src, jpeg_bytes, jpeg_subsamp, jpeg_width, jpeg_height);
	if (UNLIKELY(result)) {
		LOGD("parse_mjpeg_header failed,err=%d", result);
		RETURN(result, int);
	}
	if (UNLIKELY((jpeg_width != src.width()) || (jpeg_height != src.height()))) {
		// 入力映像フレームに指定されている映像サイズとjpegのヘッダーから取得した映像サイズが異なる場合はエラー
		LOGD("unexpected image size:expected(%dx%d), actual(%dx%d)",
			src.width(), src.height(), jpeg_width, jpeg_height);
		RETURN(VIDEO_ERROR_FRAME, int);
	}
	// サブサンプリングからraw_frame_tを取得
	const raw_frame_t jpeg_frame_type = tjsamp2raw_frame(jpeg_subsamp);
	if (jpeg_frame_type == RAW_FRAME_UNKNOWN) {
		// 未対応のjpegサブサンプリングの場合
		RETURN(VIDEO_ERROR_FRAME, int);
	} else if (jpeg_frame_type == dst.frame_type) {
		// mjpegを展開するだけでOKな場合
		RETURN(mjpeg2YUVAnyPlanner_turbo(jpegDecompressor, src, dst, dct_mode), int);
	}
	// mjepgを展開後フォーマットを変換する場合
	// libjpegturbo側関数が本来はconst uint8_t *のところがuint8_t *を受け取るのでキャスト
	auto compressed = (uint8_t *)src.frame();
#if 0
	LOGI("sz_yuv=(%lu,%lu,%lu)",
		tjPlaneSizeYUV(0, width, 0, height, jpegSubsamp),
		tjPlaneSizeYUV(1, width, 0, height, jpegSubsamp),
		tjPlaneSizeYUV(2, width, 0, height, jpegSubsamp));
#endif
	// ワーク用バッファへyuv-planerとしてデコードする
	size_t src_sz_y, src_sz_u, src_sz_v;
	uint8_t *dst_planes[3];
	result = prepare_mjpeg_plane(
		jpeg_bytes, jpeg_subsamp,
		jpeg_width, jpeg_height,
		work1, dst_planes,
		src_sz_y, src_sz_u, src_sz_v);
	if (LIKELY(!result)) {
		result = tjDecompressToYUVPlanes(jpegDecompressor, compressed,
			jpeg_bytes, dst_planes, jpeg_width, nullptr/*strides*/, jpeg_height,
				(dct_mode == DCT_MODE_ISLOW || dct_mode == DCT_MODE_FLOAT)
					? TJFLAG_ACCURATEDCT : TJFLAG_FASTDCT);
		if (result) {
			// failed
			const int err = tjGetErrorCode(jpegDecompressor);
#if defined(__ANDROID__)
			LOGD("tjDecompressToYUVPlanes failed,result=%d,err=%d,%s",
				result,
				err, tjGetErrorStr2(jpegDecompressor));
#else
			LOGD("tjDecompressToYUVPlanes failed,result=%d", result);
#endif
			result = err == TJERR_WARNING ? USB_SUCCESS : USB_ERROR_OTHER;
		}
	} else {
		LOGW("failed to prepare output planes,result=%d", result);
		result = VIDEO_ERROR_FRAME;
	}
	if (!result) {
		// ワークへのデコードと出力先のバッファサイズ調整が成功した時
		// 各プレーンの先頭ポインタとサイズを取得
		VideoImage_t src_image {
			.frame_type = jpeg_frame_type,
			.width = static_cast<uint32_t>(jpeg_width),
			.height = static_cast<uint32_t>(jpeg_height),
			.ptr_y = dst_planes[0],
			.ptr_u = dst_planes[1],
			.ptr_v = dst_planes[2],
			.stride_y = static_cast<uint32_t>(tjPlaneWidth(0, jpeg_width, jpeg_subsamp)),
			.stride_u = static_cast<uint32_t>(tjPlaneWidth(1, jpeg_width, jpeg_subsamp)),
			.stride_v = static_cast<uint32_t>(tjPlaneWidth(2, jpeg_width, jpeg_subsamp)),
			.pixel_stride_y = 1,
			.pixel_stride_u = 1,
			.pixel_stride_v = 1,
		};
		result = yuv_copy(src_image, dst, work2);
	};

	RETURN(result, int);
}

/**
 * copy_toのヘルパー関数
 * mjpegをdest.frame_typeに変換
 * 変換できなかればUSB_ERROR_NOT_SUPPORTEDを返す
 * @param jpegDecompressor
 * @param dct_mode
 * @param src
 * @param dst
 * @param work1
 * @param work2
 * @return
 */
static int mjpeg2xxx(
	tjhandle &jpegDecompressor,
	const dct_mode_t &dct_mode,
	const IVideoFrame &src, VideoImage_t &dst,
	std::vector<uint8_t> &work1,	// 変換用ワーク
	std::vector<uint8_t> &work2) {	// 変換用ワーク

	ENTER();
	int result = USB_ERROR_NOT_SUPPORTED;

	// FIXME 未実装
	switch (dst.frame_type) {
	//--------------------------------------------------------------------------------
	// インターリーブ形式
	case RAW_FRAME_UNCOMPRESSED_YUYV:	/** 0x010005, YUY2/YUYV/V422/YUV422, インターリーブ */
	{
		result = mjpeg2yuyv_turbo(jpegDecompressor, src, dst, work1, dct_mode, false);
		break;
	}
	case RAW_FRAME_UNCOMPRESSED_UYVY:	/** 0x020005, YUYVの順序違い、インターリーブ */
	{
		result = mjpeg2yuyv_turbo(jpegDecompressor, src, dst, work1, dct_mode, true);
		break;
	}
	case RAW_FRAME_UNCOMPRESSED_BY8:	/** 0x040005, 8ビットグレースケール, ベイヤー配列 */
	case RAW_FRAME_UNCOMPRESSED_Y16:	/** 0x080005, 16ビットグレースケール */
		break;
	case RAW_FRAME_UNCOMPRESSED_YCbCr:	/** 0x0c0005, Y->Cb->Cr, インターリーブ(色空間を別にすればY->U->Vと同じ並び) */
		// これはlibjpeg-turboに対応する処理がないのでlibjpegとして呼び出す
		 result = mjpeg2rgb(src, dst, JCS_YCbCr, dct_mode);
		break;
	case RAW_FRAME_UNCOMPRESSED_RGBP:	/** 0x090005, 16ビットインターリーブRGB(16ビット/5+6+5カラー), RGB565と並びが逆, RGB565 LE, BGR565 */
		break;
	case RAW_FRAME_UNCOMPRESSED_RGB565:	/** 0x0d0005, 8ビットインターリーブRGB(16ビット/5+6+5カラー) */
		// これはlibjpeg-turboに対応する処理がないのでlibjpegとして呼び出す
		result = mjpeg2rgb(src, dst, JCS_RGB565, dct_mode);
		break;
	case RAW_FRAME_UNCOMPRESSED_RGB:	/** 0x0e0005, 8ビットインターリーブRGB(24ビットカラー), RGB24 */
		result = mjpeg2rgb_turbo(jpegDecompressor, src, dst.ptr, TJPF_RGB, dct_mode);
		if (UNLIKELY(result)) {
			LOGD("フォールバック");
			result = mjpeg2rgb(src, dst, JCS_RGB, dct_mode);
		}
		break;
	case RAW_FRAME_UNCOMPRESSED_BGR:	/** 0x0f0005, 8ビットインターリーブBGR(24ビットカラー), BGR24 */
		result = mjpeg2rgb_turbo(jpegDecompressor, src, dst.ptr, TJPF_BGR, dct_mode);
		if (UNLIKELY(result)) {
			LOGD("フォールバック");
			result = mjpeg2rgb(src, dst, JCS_EXT_BGR, dct_mode);
		}
		break;
	case RAW_FRAME_UNCOMPRESSED_RGBX:	/** 0x100005, 8ビットインターリーブRGBX(32ビットカラー), RGBX32 */
		result = mjpeg2rgb_turbo(jpegDecompressor, src, dst.ptr, TJPF_RGBA, dct_mode);
		if (UNLIKELY(result)) {
			LOGD("フォールバック");
			result = mjpeg2rgb(src, dst, JCS_EXT_RGBA, dct_mode);
		}
		break;
	case RAW_FRAME_UNCOMPRESSED_XRGB:	/** 0x1a0005, 8ビットインターリーブXRGB(32ビットカラー), XRGB32 */
		result = mjpeg2rgb_turbo(jpegDecompressor, src, dst.ptr, TJPF_ARGB, dct_mode);
		if (UNLIKELY(result)) {
			LOGD("フォールバック");
			result = mjpeg2rgb(src, dst, JCS_EXT_ARGB, dct_mode);
		}
		break;
	case RAW_FRAME_UNCOMPRESSED_BGRX:	/** 0x1c0005, 8ビットインターリーブBGRX(32ビットカラー), BGRX32 */
		result = mjpeg2rgb_turbo(jpegDecompressor, src, dst.ptr, TJPF_BGRA, dct_mode);
		if (UNLIKELY(result)) {
			LOGD("フォールバック");
			result = mjpeg2rgb(src, dst, JCS_EXT_BGRA, dct_mode);
		}
		break;
	case RAW_FRAME_UNCOMPRESSED_XBGR:	/** 0x1b0005, 8ビットインターリーブXBGR(32ビットカラー), XBGR32 */
		result = mjpeg2rgb_turbo(jpegDecompressor, src, dst.ptr, TJPF_ABGR, dct_mode);
		if (UNLIKELY(result)) {
			LOGD("フォールバック");
			result = mjpeg2rgb(src, dst, JCS_EXT_ABGR, dct_mode);
		}
		break;
	//--------------------------------------------------------------------------------
	// YUVプラナー/セミプラナー系
	case RAW_FRAME_UNCOMPRESSED_GRAY8:	/** 0x030005, 8ビットグレースケール, Y800/Y8/I400 */
	case RAW_FRAME_UNCOMPRESSED_NV21:	/** 0x050005, YVU420 SemiPlanar(y->vu), YVU420sp */
	case RAW_FRAME_UNCOMPRESSED_NV12:	/** 0x0b0005, YUV420 SemiPlanar(y->uv) NV21とU/Vが逆 */
	case RAW_FRAME_UNCOMPRESSED_YV12:	/** 0x060005, YVU420 Planar(y->v->u), YVU420p, I420とU/Vが逆 */
	case RAW_FRAME_UNCOMPRESSED_I420:	/** 0x070005, YUV420 Planar(y->u->v), YUV420p, YV12とU/Vが逆 */
	case RAW_FRAME_UNCOMPRESSED_444p:	/** 0x110005, YUV444 Planar(y->u->v), YUV444p */
	case RAW_FRAME_UNCOMPRESSED_444sp:	/** 0x120005, YUV444 semi Planar(y->uv), NV24/YUV444sp */
	case RAW_FRAME_UNCOMPRESSED_422p:	/** 0x130005, YUV422 Planar(y->u->v), YUV422p */
	case RAW_FRAME_UNCOMPRESSED_422sp:	/** 0x140005, YUV422 SemiPlanar(y->uv), NV16/YUV422sp */
	{
		auto decoded = get_mjpeg_decode_type(jpegDecompressor, src);
		if (decoded == dst.frame_type) {
			// 展開するだけでOKの時
			result = mjpeg2YUVAnyPlanner_turbo(jpegDecompressor, src, dst, dct_mode);
		} else {
			// 展開してから変換しないといけないとき
			result = mjpeg2YUVxxx_turbo(jpegDecompressor, src, dst, work1, work2, dct_mode);
		}
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
	case RAW_FRAME_UNKNOWN:				/** 不明なフレームフォーマット */
	case RAW_FRAME_UNCOMPRESSED:		/** 0x000005 不明な非圧縮フレームフォーマット */
	case RAW_FRAME_STILL:				/** 0x000003 静止画 */
	case RAW_FRAME_UNCOMPRESSED_M420:	/** 0x0a0005, YUV420でY2の次に1/2UVが並ぶのを繰り返す, YYYY/YYYY/UVUV->YYYY/YYYY/UVUV->... @deprecated */
	case RAW_FRAME_UNCOMPRESSED_440p:	/** 0x150005, YUV440 Planar(y->u->v), YUV440p */
	case RAW_FRAME_UNCOMPRESSED_440sp:	/** 0x160005, YUV440 Planar(y->u->v), YUV440sp */
	case RAW_FRAME_UNCOMPRESSED_411p:	/** 0x170005, YUV411 Planar(y->u->v), YUV411p */
	case RAW_FRAME_UNCOMPRESSED_411sp:	/** 0x180005, YUV411 SemiPlanar(y->uv), YUV411sp */
	case RAW_FRAME_UNCOMPRESSED_YUV_ANY:
	default:
		break;
	}

	RETURN(result, int);
}

//================================================================================
//
//================================================================================
/**
 * コンストラクタ
 * @param dct_mode
 */
/*public*/
VideoConverter::VideoConverter(const dct_mode_t &dct_mode)
:	jpegDecompressor(nullptr),
	_dct_mode(dct_mode)
{
	ENTER();
	EXIT();
}

/**
 * コピーコンストラクタ
 * @param src
 */
/*public*/
VideoConverter::VideoConverter(const VideoConverter &src)
:	jpegDecompressor(nullptr),
	_dct_mode(src._dct_mode)
{
	ENTER();
	EXIT();
}

/**
 * デストラクタ
 */
/*public*/
VideoConverter::~VideoConverter() {
	ENTER();
	if (jpegDecompressor) {
		tjDestroy(jpegDecompressor);
		jpegDecompressor = nullptr;
	}
	EXIT();
}

/**
 * libjpeg-turboを(m)jpeg展開用に初期化する
 * 既に初期化済みの場合はなにもしない
 * @return
 */
/*private*/
int VideoConverter::init_jpeg_turbo() {
	ENTER();

	int result = USB_SUCCESS;
	if (UNLIKELY(!jpegDecompressor)) {
		jpegDecompressor = tjInitDecompress();
		LOGD("tjInitDecompress");
		if (!jpegDecompressor) {
			const char *err = tjGetErrorStr();
			if (err) {
				LOGE("failed to call tjInitDecompress:%s", err);
			} else {
				LOGE("failed to call tjInitDecompress:unknown err");
			}
			result = USB_ERROR_OTHER;
		}
	}

	RETURN(result, int);
}

/**
 * mjpegフレームデータをデコードした時のフレームタイプを取得
 * 指定したIVideoFrameがmjpegフレームでないときなどはRAW_FRAME_UNKNOWNを返す
 * @param src
 * @return
 */
/*public*/
raw_frame_t VideoConverter::get_mjpeg_decode_type(const IVideoFrame &src) {
	ENTER();

	raw_frame_t result = RAW_FRAME_UNKNOWN;
	if (src.frame_type() == RAW_FRAME_MJPEG) {
		int r = init_jpeg_turbo();
		if (LIKELY(!r && jpegDecompressor)) {
			result = core::get_mjpeg_decode_type(jpegDecompressor, src);
		} else {
			LOGW("Failed to init libjpeg-turbo,err=%d", r);
		}
	} else {
		LOGD("Not a mjpeg frame!");
	}

	RETURN(result, raw_frame_t);
}

/**
 * 映像データをコピー
 * 必要であれば映像フォーマットの変換を行う
 * @param src
 * @param dst
 * @param dst_type コピー先の映像フォーマット
 * @return
 */
/*public*/
int VideoConverter::copy_to(
	const IVideoFrame &src,
	IVideoFrame &dst,
	const raw_frame_t &dst_type) {

	ENTER();

//	LOGD("frame_type:src=0x%08x,dst=0x%08x", src.frame_type(), dst.frame_type());
	if (UNLIKELY(dst_type == RAW_FRAME_UNKNOWN)) {
		// コピー先のフレームタイプが不明な時は単純コピー(ディープコピー)
		src.copy_to(dst);
		RETURN(USB_ERROR_INVALID_PARAM, int);
	}

	if (src.frame_type() == dst_type) {
		// フレームタイプが同じなら単純コピー(ディープコピー)
		src.copy_to(dst);
		RETURN(USB_SUCCESS, int);
	}

	if (UNLIKELY(dst.resize(src, dst_type))) {
		// 出力先フォーマットに合わせてバッファサイズを調整
		return USB_ERROR_NO_MEM;
	}

	dst.clear();
	int result = USB_ERROR_NOT_SUPPORTED;
	switch (src.frame_type()) {
	case RAW_FRAME_UNCOMPRESSED_YUYV:
		result = yuyv2xxx(src, dst, _work1);
		break;
	case RAW_FRAME_UNCOMPRESSED_UYVY:
		result = uyvy2xxx(src, dst, _work1);
		break;
//	case RAW_FRAME_UNCOMPRESSED_GRAY8:
//	case RAW_FRAME_UNCOMPRESSED_BY8:
	case RAW_FRAME_UNCOMPRESSED_NV21:
		result = nv21xxx(src, dst, _work1);
		break;
	case RAW_FRAME_UNCOMPRESSED_YV12:
		result = yv12xxx(src, dst, _work1);
		break;
	case RAW_FRAME_UNCOMPRESSED_I420:
		result = i420xxx(src, dst, _work1);
		break;
//	case RAW_FRAME_UNCOMPRESSED_Y16:
//	case RAW_FRAME_UNCOMPRESSED_RGBP:
//	case RAW_FRAME_UNCOMPRESSED_RGB565:
//		break;
	case RAW_FRAME_UNCOMPRESSED_RGB:
		result = rgb2xxx(src, dst, _work1);
		break;
//	case RAW_FRAME_UNCOMPRESSED_BGR:
//	case RAW_FRAME_UNCOMPRESSED_RGBX:
//	case RAW_FRAME_UNCOMPRESSED_XRGB:
//		break;
	case RAW_FRAME_UNCOMPRESSED_M420:
		break;
	case RAW_FRAME_UNCOMPRESSED_NV12:
		result = nv12xxx(src, dst, _work1);
		break;
	case RAW_FRAME_UNCOMPRESSED_YCbCr:
		break;
	case RAW_FRAME_UNCOMPRESSED_444p:
		result = yuv444pxxx(src, dst, _work1);
		break;
	case RAW_FRAME_UNCOMPRESSED_444sp:
		break;
	case RAW_FRAME_UNCOMPRESSED_422p:
		result = yuv422pxxx(src, dst, _work1);
		break;
	case RAW_FRAME_UNCOMPRESSED_422sp:
	case RAW_FRAME_UNCOMPRESSED_440p:
	case RAW_FRAME_UNCOMPRESSED_440sp:
	case RAW_FRAME_UNCOMPRESSED_411p:
	case RAW_FRAME_UNCOMPRESSED_411sp:
		// FIXME 未実装
		break;
	case RAW_FRAME_MJPEG:
		result = init_jpeg_turbo();
		if (LIKELY(!result && jpegDecompressor)) {
			result = mjpeg2xxx(jpegDecompressor, _dct_mode, src, dst, _work1, _work2);
		}
		break;
	case RAW_FRAME_H264:
	case RAW_FRAME_FRAME_H264:
//	case RAW_FRAME_H264_SIMULCAST:
		LOGD("h264フレームが来た:dst=%d", dst.frame_type());
		// Java側へ一旦引き渡してRGBXに変換する?
		// ...非同期での変換になるので難しいので今は単純コピー(ディープコピー)
		src.copy_to(dst);
		RETURN(USB_SUCCESS, int);
//	case RAW_FRAME_FRAME_BASED:
//	case RAW_FRAME_MPEG2TS:
//	case RAW_FRAME_DV:
//		break;
	case RAW_FRAME_VP8:
	case RAW_FRAME_FRAME_VP8:
//	case RAW_FRAME_VP8_SIMULCAST:
		LOGD("vp8フレームが来た:dst=%d", dst.frame_type());
		// Java側へ一旦引き渡してRGBXに変換する?
		// ...非同期での変換になるので難しいので今は単純コピー(ディープコピー)
		src.copy_to(dst);
		RETURN(USB_SUCCESS, int);
	case RAW_FRAME_H265:				/** H.265単独フレーム */
		LOGD("h265フレームが来た:dst=%d", dst.frame_type());
		// Java側へ一旦引き渡してRGBXに変換する?
		// ...非同期での変換になるので難しいので今は単純コピー(ディープコピー)
		src.copy_to(dst);
		RETURN(USB_SUCCESS, int);
//	case RAW_FRAME_UNCOMPRESSED_YUV_ANY	// これは変換先専用なのでソース映像には来ないし来てはだめ
	default:
		break;
	}
	if (UNLIKELY(result)) {
		LOGD("unsupported frame format=%d,r=%d", src.frame_type(), result);
		LOGD("frame:%s", bin2hex(src.frame(), 32).c_str());
	}

	if (LIKELY(!result)) {
		// 変換成功したときはその他のアトリビュートもコピーする
		dst.set_attribute(src);
	}
	RETURN(result, int);
}

/**
 * 映像データをコピー
 * 必要であれば映像フォーマットの変換を行う
 * FIXME yuv系からJPEGへの圧縮など対応していない変換もあるので注意！(未対応時はUSB_ERROR_NOT_SUPPORTEDを返す)
 * @param src
 * @param dst
 * @param dst_type
 * @return
 */
int VideoConverter::copy_to(
	IVideoFrame &src,
	VideoImage_t &dst) {

	int result = USB_ERROR_NOT_SUPPORTED;

	if (dst.frame_type == RAW_FRAME_UNKNOWN) {
		RETURN(result, int);
	}

	if (src.frame_type() == dst.frame_type) {
		// フレームタイプが同じ時
		VideoImage_t image{};
		result = src.get_image(image);
		if (!result) {
			result = copy(image, dst, _work1);
#if defined(__ANDROID__)
			auto _src = dynamic_cast<HWBufferVideoFrame *>(&src);
			if (_src) {
				_src->unlock();
			}
#endif
		}
		RETURN(result, int);
	}

	if (src.frame_type() == RAW_FRAME_MJPEG) {
		// mjpegを展開するとき
		result = init_jpeg_turbo();
		if (LIKELY(!result && jpegDecompressor)) {
			result = mjpeg2xxx(jpegDecompressor, _dct_mode, src, dst, _work1, _work2);
		}
	}

	// その他は未実装

	RETURN(result, int);
}

#if 0
	switch (frame_type) {
	case RAW_FRAME_UNKNOWN:
	case RAW_FRAME_STILL:
	case RAW_FRAME_UNCOMPRESSED:
	case RAW_FRAME_UNCOMPRESSED_YUYV:
	case RAW_FRAME_UNCOMPRESSED_UYVY:
	case RAW_FRAME_UNCOMPRESSED_GRAY8:
	case RAW_FRAME_UNCOMPRESSED_BY8:
	case RAW_FRAME_UNCOMPRESSED_NV21:
	case RAW_FRAME_UNCOMPRESSED_YV12:
	case RAW_FRAME_UNCOMPRESSED_I420:
	case RAW_FRAME_UNCOMPRESSED_Y16:
	case RAW_FRAME_UNCOMPRESSED_RGBP:
	case RAW_FRAME_UNCOMPRESSED_M420:
	case RAW_FRAME_UNCOMPRESSED_NV12:
	case RAW_FRAME_UNCOMPRESSED_YCbCr:
	case RAW_FRAME_UNCOMPRESSED_RGB565:
	case RAW_FRAME_UNCOMPRESSED_RGB:
	case RAW_FRAME_UNCOMPRESSED_BGR:
	case RAW_FRAME_UNCOMPRESSED_RGBX:
	case RAW_FRAME_UNCOMPRESSED_XRGB:
	case RAW_FRAME_UNCOMPRESSED_444p:
	case RAW_FRAME_UNCOMPRESSED_444sp:
	case RAW_FRAME_UNCOMPRESSED_422p:
	case RAW_FRAME_UNCOMPRESSED_422sp:
	case RAW_FRAME_UNCOMPRESSED_440p:
	case RAW_FRAME_UNCOMPRESSED_440sp:
	case RAW_FRAME_UNCOMPRESSED_411p:
	case RAW_FRAME_UNCOMPRESSED_411sp:
	case RAW_FRAME_UNCOMPRESSED_YUV_ANY:
	//
	case RAW_FRAME_MJPEG:
	case RAW_FRAME_FRAME_BASED:
	case RAW_FRAME_MPEG2TS:
	case RAW_FRAME_DV:
	case RAW_FRAME_FRAME_H264:
	case RAW_FRAME_FRAME_VP8:
	case RAW_FRAME_H264:
	case RAW_FRAME_H264_SIMULCAST:
	case RAW_FRAME_VP8:
	case RAW_FRAME_VP8_SIMULCAST
	default:
		break;
	}
#endif

} // namespace serenegiant::usb
