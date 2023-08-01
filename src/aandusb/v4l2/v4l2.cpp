/*
 * aAndUsb
 * Copyright (c) 2014-2023 saki t_saki@serenegiant.com
 * Distributed under the terms of the GNU Lesser General Public License (LGPL v3.0) License.
 * License details are in the file license.txt, distributed as part of this software.
 */

#define LOG_TAG "V4L2"

#define MEAS_TIME (0)				// 1フレーム当たりの描画時間を測定する時1

#if 1	// デバッグ情報を出さない時は1
	#ifndef LOG_NDEBUG
		#define	LOG_NDEBUG		// LOGV/LOGD/MARKを出力しない時
	#endif
	#undef USE_LOGALL			// 指定したLOGxだけを出力
#else
	// #define USE_LOGALL
	#define USE_LOGD
	#undef LOG_NDEBUG
	#undef NDEBUG
#endif

#include <sys/ioctl.h>

#include "utilbase.h"
#include "charutils.h"

// usb
#include "usb/descriptor_defs.h"
// v4l2
#include "v4l2/v4l2.h"

namespace serenegiant::v4l2 {

/**
 * V4L2_PIX_FMT_xxxはFourCCを32ビット符号無し整数にパックしているので
 * 逆変換して文字列に戻すためのヘルパー関数
 * @param v4l2_pix_fmt
 * @return
 */
std::string V4L2_PIX_FMT_to_string(const uint32_t &v4l2_pix_fmt) {
	return serenegiant::format("%c%c%c%c",
		v4l2_pix_fmt >> 0, v4l2_pix_fmt >> 8,
		v4l2_pix_fmt >> 16, v4l2_pix_fmt >> 24);
}

/**
 * V4L2_PIX_FMT_XXXをraw_frame_tへ変換
 * @param v4l2_pix_fmt
 * @return
 */
core::raw_frame_t V4L2_PIX_FMT_to_raw_frame(const uint32_t &v4l2_pix_fmt) {
	switch (v4l2_pix_fmt) {
	case V4L2_PIX_FMT_YUYV:			return core::RAW_FRAME_UNCOMPRESSED_YUYV;
	case V4L2_PIX_FMT_UYVY:			return core::RAW_FRAME_UNCOMPRESSED_UYVY;
	case V4L2_PIX_FMT_GREY:			return core::RAW_FRAME_UNCOMPRESSED_GRAY8;
//									return core::RAW_FRAME_UNCOMPRESSED_BY8;
	case V4L2_PIX_FMT_NV21:			return core::RAW_FRAME_UNCOMPRESSED_NV21;
	case V4L2_PIX_FMT_YVU420:		return core::RAW_FRAME_UNCOMPRESSED_YV12;
//									return core::RAW_FRAME_UNCOMPRESSED_I420;
	case V4L2_PIX_FMT_Y16:			return core::RAW_FRAME_UNCOMPRESSED_Y16;
//									return core::RAW_FRAME_UNCOMPRESSED_RGBP;
	case V4L2_PIX_FMT_M420:			return core::RAW_FRAME_UNCOMPRESSED_M420;
	case V4L2_PIX_FMT_NV12:			return core::RAW_FRAME_UNCOMPRESSED_NV12;
//									return core::RAW_FRAME_UNCOMPRESSED_YCbCr;
	case V4L2_PIX_FMT_RGB565:		return core::RAW_FRAME_UNCOMPRESSED_RGB565;
	case V4L2_PIX_FMT_RGB24:		return core::RAW_FRAME_UNCOMPRESSED_RGB;
	case V4L2_PIX_FMT_BGR24:		return core::RAW_FRAME_UNCOMPRESSED_BGR;
	case V4L2_PIX_FMT_RGBA32:
	case V4L2_PIX_FMT_RGBX32:		return core::RAW_FRAME_UNCOMPRESSED_RGBX;
	case V4L2_PIX_FMT_YUV444:		return core::RAW_FRAME_UNCOMPRESSED_444p;
//									return core::RAW_FRAME_UNCOMPRESSED_444sp;
	case V4L2_PIX_FMT_YUV422P:		return core::RAW_FRAME_UNCOMPRESSED_422p;
	case V4L2_PIX_FMT_NV16:			return core::RAW_FRAME_UNCOMPRESSED_422sp;
//									return core::RAW_FRAME_UNCOMPRESSED_440p;
//									return core::RAW_FRAME_UNCOMPRESSED_440sp;
//									return core::RAW_FRAME_UNCOMPRESSED_411p;
//									return core::RAW_FRAME_UNCOMPRESSED_411sp;
//									return core::RAW_FRAME_UNCOMPRESSED_YUV_ANY;
	case V4L2_PIX_FMT_ARGB32:
	case V4L2_PIX_FMT_XRGB32:		return core::RAW_FRAME_UNCOMPRESSED_XRGB;
	case V4L2_PIX_FMT_ABGR32:
	case V4L2_PIX_FMT_XBGR32:		return core::RAW_FRAME_UNCOMPRESSED_XBGR;
	case V4L2_PIX_FMT_BGRA32:
	case V4L2_PIX_FMT_BGRX32:		return core::RAW_FRAME_UNCOMPRESSED_BGRX;
	case V4L2_PIX_FMT_JPEG:
	case V4L2_PIX_FMT_MJPEG:		return core::RAW_FRAME_MJPEG;
//									return core::RAW_FRAME_FRAME_BASED;
	case V4L2_PIX_FMT_MPEG2:		return core::RAW_FRAME_MPEG2TS;
//									return core::RAW_FRAME_DV;
//									return core::RAW_FRAME_FRAME_H264;
//	case V4L2_PIX_FMT_VP8_FRAME:	return core::RAW_FRAME_FRAME_VP8;
	case V4L2_PIX_FMT_H264:
	case V4L2_PIX_FMT_H264_NO_SC:
	case V4L2_PIX_FMT_H264_MVC:		return core::RAW_FRAME_H264;
//	case V4L2_PIX_FMT_H264_SLICE:	return core::RAW_FRAME_H264;
//									return core::RAW_FRAME_H264_SIMULCAST;
	case V4L2_PIX_FMT_VP8:			return core::RAW_FRAME_VP8;
//									return core::RAW_FRAME_VP8_SIMULCAST;
	default:
		return core::RAW_FRAME_UNKNOWN;
	}
}

uint32_t raw_frame_to_V4L2_PIX_FMT(const core::raw_frame_t &frame_type) {
	switch (frame_type) {
	case core::RAW_FRAME_UNCOMPRESSED_YUYV:		return V4L2_PIX_FMT_YUYV;
	case core::RAW_FRAME_UNCOMPRESSED_UYVY:		return V4L2_PIX_FMT_UYVY;
	case core::RAW_FRAME_UNCOMPRESSED_GRAY8:	return V4L2_PIX_FMT_GREY;
//	case core::RAW_FRAME_UNCOMPRESSED_BY8:
	case core::RAW_FRAME_UNCOMPRESSED_NV21:		return V4L2_PIX_FMT_NV21;
	case core::RAW_FRAME_UNCOMPRESSED_YV12:		return V4L2_PIX_FMT_YVU420;
//	case core::RAW_FRAME_UNCOMPRESSED_I420:
	case core::RAW_FRAME_UNCOMPRESSED_Y16:		return V4L2_PIX_FMT_Y16;
//	case core::RAW_FRAME_UNCOMPRESSED_RGBP;
	case core::RAW_FRAME_UNCOMPRESSED_M420:		return V4L2_PIX_FMT_M420;
	case core::RAW_FRAME_UNCOMPRESSED_NV12:		return V4L2_PIX_FMT_NV12;
//	case core::RAW_FRAME_UNCOMPRESSED_YCbCr:
	case core::RAW_FRAME_UNCOMPRESSED_RGB565:	return V4L2_PIX_FMT_RGB565;
	case core::RAW_FRAME_UNCOMPRESSED_RGB:		return V4L2_PIX_FMT_RGB24;
	case core::RAW_FRAME_UNCOMPRESSED_BGR:		return V4L2_PIX_FMT_BGR24;
	case core::RAW_FRAME_UNCOMPRESSED_RGBX:		return V4L2_PIX_FMT_RGBX32;	// V4L2_PIX_FMT_RGBA32
	case core::RAW_FRAME_UNCOMPRESSED_444p:		return V4L2_PIX_FMT_YUV444;
//	case core::RAW_FRAME_UNCOMPRESSED_444sp:
	case core::RAW_FRAME_UNCOMPRESSED_422p:		return V4L2_PIX_FMT_YUV422P;
	case core::RAW_FRAME_UNCOMPRESSED_422sp:	return V4L2_PIX_FMT_NV16;
//	case core::RAW_FRAME_UNCOMPRESSED_440p:
//	case core::RAW_FRAME_UNCOMPRESSED_440sp:
//	case core::RAW_FRAME_UNCOMPRESSED_411p:
//	case core::RAW_FRAME_UNCOMPRESSED_411sp:
//	case core::RAW_FRAME_UNCOMPRESSED_YUV_ANY:
	case core::RAW_FRAME_UNCOMPRESSED_XRGB:		return V4L2_PIX_FMT_XRGB32;	// V4L2_PIX_FMT_ARGB32
	case core::RAW_FRAME_UNCOMPRESSED_XBGR:		return V4L2_PIX_FMT_XBGR32;	// V4L2_PIX_FMT_ABGR32
	case core::RAW_FRAME_UNCOMPRESSED_BGRX:		return V4L2_PIX_FMT_BGRX32;	// V4L2_PIX_FMT_BGRA32
	case core::RAW_FRAME_MJPEG:					return V4L2_PIX_FMT_MJPEG;	// V4L2_PIX_FMT_JPEG
//	case core::RAW_FRAME_FRAME_BASED:
	case core::RAW_FRAME_MPEG2TS:				return V4L2_PIX_FMT_MPEG2;
//	case core::RAW_FRAME_DV;
	case core::RAW_FRAME_FRAME_H264:			return V4L2_PIX_FMT_H264;
	case core::RAW_FRAME_H264:					return V4L2_PIX_FMT_H264;	// V4L2_PIX_FMT_H264_NO_SC, V4L2_PIX_FMT_H264_MVC, V4L2_PIX_FMT_H264_SLICE
//	case core::RAW_FRAME_H264_SIMULCAST:
	case core::RAW_FRAME_VP8:					return V4L2_PIX_FMT_VP8;
//	case core::RAW_FRAME_VP8_SIMULCAST:
	default:
		return 0;
	}
}

/**
 * ioctl呼び出し中にシグナルを受け取るとエラー終了してしまうので
 * シグナル受信時(errno==EINTR)ならリトライするためのヘルパー関数
 * @param fd
 * @param request
 * @param arg
 * @return
 */
int xioctl(int fd, int request, void *arg) {
	ENTER();

	int r;

	do {
		r = ::ioctl(fd, request, arg);
	} while ((r == -1) && (errno == EINTR));

	RETURN(r, int);
}

/**
 * コントロール機能をIDを文字列に変換
 * @param id
 * @param const char *
*/
const char *ctrl_id_string(const uint32_t &id) {
	switch (id) {
//	case V4L2_CID_BASE: 						return "V4L2_CID_BASE";
// 	case V4L2_CID_USER_BASE:					return "V4L2_CID_USER_BASE";
	case V4L2_CID_USER_CLASS:					return "V4L2_CID_USER_CLASS";
	case V4L2_CID_BRIGHTNESS:					return "V4L2_CID_BRIGHTNESS";
	case V4L2_CID_CONTRAST:						return "V4L2_CID_CONTRAST";
	case V4L2_CID_SATURATION:					return "V4L2_CID_SATURATION";
	case V4L2_CID_HUE:							return "V4L2_CID_HUE";
	case V4L2_CID_BLACK_LEVEL:					return "V4L2_CID_BLACK_LEVEL";
	case V4L2_CID_AUTO_WHITE_BALANCE:			return "V4L2_CID_AUTO_WHITE_BALANCE";
	case V4L2_CID_DO_WHITE_BALANCE:				return "V4L2_CID_DO_WHITE_BALANCE";
	case V4L2_CID_RED_BALANCE:					return "V4L2_CID_RED_BALANCE";
	case V4L2_CID_BLUE_BALANCE:					return "V4L2_CID_BLUE_BALANCE";
	case V4L2_CID_GAMMA:						return "V4L2_CID_GAMMA";
// 	case V4L2_CID_WHITENESS:					return "V4L2_CID_WHITENESS";
	case V4L2_CID_EXPOSURE:						return "V4L2_CID_EXPOSURE";
	case V4L2_CID_AUTOGAIN:						return "V4L2_CID_AUTOGAIN";
	case V4L2_CID_GAIN:							return "V4L2_CID_GAIN";
	case V4L2_CID_HFLIP:						return "V4L2_CID_HFLIP";
	case V4L2_CID_VFLIP:						return "V4L2_CID_VFLIP";
	case V4L2_CID_POWER_LINE_FREQUENCY:			return "V4L2_CID_POWER_LINE_FREQUENCY";
	case V4L2_CID_HUE_AUTO:						return "V4L2_CID_HUE_AUTO";
	case V4L2_CID_WHITE_BALANCE_TEMPERATURE:	return "V4L2_CID_WHITE_BALANCE_TEMPERATURE";
	case V4L2_CID_SHARPNESS:					return "V4L2_CID_SHARPNESS";
	case V4L2_CID_BACKLIGHT_COMPENSATION:		return "V4L2_CID_BACKLIGHT_COMPENSATION";
	case V4L2_CID_CHROMA_AGC:					return "V4L2_CID_CHROMA_AGC";
	case V4L2_CID_COLOR_KILLER:					return "V4L2_CID_COLOR_KILLER";
	case V4L2_CID_COLORFX:						return "V4L2_CID_COLORFX";
	case V4L2_CID_AUTOBRIGHTNESS:				return "V4L2_CID_AUTOBRIGHTNESS";
	case V4L2_CID_BAND_STOP_FILTER:				return "V4L2_CID_BAND_STOP_FILTER";
	case V4L2_CID_ROTATE:						return "V4L2_CID_ROTATE";
	case V4L2_CID_BG_COLOR:						return "V4L2_CID_BG_COLOR";
	case V4L2_CID_CHROMA_GAIN:					return "V4L2_CID_CHROMA_GAIN";
	case V4L2_CID_ILLUMINATORS_1:				return "V4L2_CID_ILLUMINATORS_1";
	case V4L2_CID_ILLUMINATORS_2:				return "V4L2_CID_ILLUMINATORS_2";
	case V4L2_CID_MIN_BUFFERS_FOR_CAPTURE:		return "V4L2_CID_MIN_BUFFERS_FOR_CAPTURE";
	case V4L2_CID_MIN_BUFFERS_FOR_OUTPUT:		return "V4L2_CID_MIN_BUFFERS_FOR_OUTPUT";
	case V4L2_CID_ALPHA_COMPONENT:				return "V4L2_CID_ALPHA_COMPONENT";
	case V4L2_CID_COLORFX_CBCR:					return "V4L2_CID_COLORFX_CBCR";
	case V4L2_CID_LASTP1:						return "V4L2_CID_LASTP1";
	case V4L2_CID_USER_MEYE_BASE:				return "V4L2_CID_USER_MEYE_BASE";
	case V4L2_CID_USER_BTTV_BASE:				return "V4L2_CID_USER_BTTV_BASE";
	case V4L2_CID_USER_S2255_BASE:				return "V4L2_CID_USER_S2255_BASE";
	case V4L2_CID_USER_SI476X_BASE:				return "V4L2_CID_USER_SI476X_BASE";
	case V4L2_CID_USER_TI_VPE_BASE:				return "V4L2_CID_USER_TI_VPE_BASE";
	case V4L2_CID_USER_SAA7134_BASE:			return "V4L2_CID_USER_SAA7134_BASE";
	case V4L2_CID_USER_ADV7180_BASE:			return "V4L2_CID_USER_ADV7180_BASE";
	case V4L2_CID_USER_TC358743_BASE:			return "V4L2_CID_USER_TC358743_BASE";
	case V4L2_CID_USER_MAX217X_BASE:			return "V4L2_CID_USER_MAX217X_BASE";
	case V4L2_CID_USER_IMX_BASE:				return "V4L2_CID_USER_IMX_BASE";
	case V4L2_CID_USER_ATMEL_ISC_BASE:			return "V4L2_CID_USER_ATMEL_ISC_BASE";
	case V4L2_CID_USER_CODA_BASE:				return "V4L2_CID_USER_CODA_BASE";
	case V4L2_CID_USER_CCS_BASE:				return "V4L2_CID_USER_CCS_BASE";
	case V4L2_CID_CODEC_BASE:					return "V4L2_CID_CODEC_BASE";
	case V4L2_CID_CODEC_CLASS:					return "V4L2_CID_CODEC_CLASS";
	case V4L2_CID_CAMERA_CLASS_BASE:			return "V4L2_CID_CAMERA_CLASS_BASE";
	case V4L2_CID_CAMERA_CLASS:					return "V4L2_CID_CAMERA_CLASS";
	case V4L2_CID_EXPOSURE_AUTO:				return "V4L2_CID_EXPOSURE_AUTO";
	case V4L2_CID_EXPOSURE_ABSOLUTE:			return "V4L2_CID_EXPOSURE_ABSOLUTE";
	case V4L2_CID_EXPOSURE_AUTO_PRIORITY:		return "V4L2_CID_EXPOSURE_AUTO_PRIORITY";
	case V4L2_CID_PAN_RELATIVE:					return "V4L2_CID_PAN_RELATIVE";
	case V4L2_CID_TILT_RELATIVE:				return "V4L2_CID_TILT_RELATIVE";
	case V4L2_CID_PAN_RESET:					return "V4L2_CID_PAN_RESET";
	case V4L2_CID_TILT_RESET:					return "V4L2_CID_TILT_RESET";
	case V4L2_CID_PAN_ABSOLUTE:					return "V4L2_CID_PAN_ABSOLUTE";
	case V4L2_CID_TILT_ABSOLUTE:				return "V4L2_CID_TILT_ABSOLUTE";
	case V4L2_CID_FOCUS_ABSOLUTE:				return "V4L2_CID_FOCUS_ABSOLUTE";
	case V4L2_CID_FOCUS_RELATIVE:				return "V4L2_CID_FOCUS_RELATIVE";
	case V4L2_CID_FOCUS_AUTO:					return "V4L2_CID_FOCUS_AUTO";
	case V4L2_CID_ZOOM_ABSOLUTE:				return "V4L2_CID_ZOOM_ABSOLUTE";
	case V4L2_CID_ZOOM_RELATIVE:				return "V4L2_CID_ZOOM_RELATIVE";
	case V4L2_CID_ZOOM_CONTINUOUS:				return "V4L2_CID_ZOOM_CONTINUOUS";
	case V4L2_CID_PRIVACY:						return "V4L2_CID_PRIVACY";
	case V4L2_CID_IRIS_ABSOLUTE:				return "V4L2_CID_IRIS_ABSOLUTE";
	case V4L2_CID_IRIS_RELATIVE:				return "V4L2_CID_IRIS_RELATIVE";
	case V4L2_CID_AUTO_EXPOSURE_BIAS:			return "V4L2_CID_AUTO_EXPOSURE_BIAS";
	case V4L2_CID_AUTO_N_PRESET_WHITE_BALANCE:		return "V4L2_CID_AUTO_N_PRESET_WHITE_BALANCE";
	case V4L2_CID_WIDE_DYNAMIC_RANGE:			return "V4L2_CID_WIDE_DYNAMIC_RANGE";
	case V4L2_CID_IMAGE_STABILIZATION:			return "V4L2_CID_IMAGE_STABILIZATION";
	case V4L2_CID_ISO_SENSITIVITY:				return "V4L2_CID_ISO_SENSITIVITY";
	case V4L2_CID_ISO_SENSITIVITY_AUTO:			return "V4L2_CID_ISO_SENSITIVITY_AUTO";
	case V4L2_CID_EXPOSURE_METERING:			return "V4L2_CID_EXPOSURE_METERING";
	case V4L2_CID_SCENE_MODE:					return "V4L2_CID_SCENE_MODE";
	case V4L2_CID_3A_LOCK:						return "V4L2_CID_3A_LOCK";
	case V4L2_CID_AUTO_FOCUS_START:				return "V4L2_CID_AUTO_FOCUS_START";
	case V4L2_CID_AUTO_FOCUS_STOP:				return "V4L2_CID_AUTO_FOCUS_STOP";
	case V4L2_CID_AUTO_FOCUS_STATUS:			return "V4L2_CID_AUTO_FOCUS_STATUS";
	case V4L2_CID_AUTO_FOCUS_RANGE:				return "V4L2_CID_AUTO_FOCUS_RANGE";
	case V4L2_CID_PAN_SPEED:					return "V4L2_CID_PAN_SPEED";
	case V4L2_CID_TILT_SPEED:					return "V4L2_CID_TILT_SPEED";
	case V4L2_CID_CAMERA_ORIENTATION:			return "V4L2_CID_CAMERA_ORIENTATION";
	case V4L2_CID_CAMERA_SENSOR_ROTATION:		return "V4L2_CID_CAMERA_SENSOR_ROTATION";
	case V4L2_CID_PILOT_TONE_ENABLED:			return "V4L2_CID_PILOT_TONE_ENABLED";
	case V4L2_CID_PILOT_TONE_DEVIATION:			return "V4L2_CID_PILOT_TONE_DEVIATION";
	case V4L2_CID_PILOT_TONE_FREQUENCY:			return "V4L2_CID_PILOT_TONE_FREQUENCY";
	case V4L2_CID_FLASH_CLASS_BASE:				return "V4L2_CID_FLASH_CLASS_BASE";
	case V4L2_CID_FLASH_CLASS:					return "V4L2_CID_FLASH_CLASS";
	case V4L2_CID_FLASH_LED_MODE:				return "V4L2_CID_FLASH_LED_MODE";
	case V4L2_CID_FLASH_STROBE_SOURCE:			return "V4L2_CID_FLASH_STROBE_SOURCE";
	case V4L2_CID_FLASH_STROBE:					return "V4L2_CID_FLASH_STROBE";
	case V4L2_CID_FLASH_STROBE_STOP:			return "V4L2_CID_FLASH_STROBE_STOP";
	case V4L2_CID_FLASH_STROBE_STATUS:			return "V4L2_CID_FLASH_STROBE_STATUS";
	case V4L2_CID_FLASH_TIMEOUT:				return "V4L2_CID_FLASH_TIMEOUT";
	case V4L2_CID_FLASH_INTENSITY:				return "V4L2_CID_FLASH_INTENSITY";
	case V4L2_CID_FLASH_TORCH_INTENSITY:		return "V4L2_CID_FLASH_TORCH_INTENSITY";
	case V4L2_CID_FLASH_INDICATOR_INTENSITY:	return "V4L2_CID_FLASH_INDICATOR_INTENSITY";
	case V4L2_CID_FLASH_FAULT:					return "V4L2_CID_FLASH_FAULT";
	case V4L2_CID_FLASH_CHARGE:					return "V4L2_CID_FLASH_CHARGE";
	case V4L2_CID_FLASH_READY:					return "V4L2_CID_FLASH_READY";
	case V4L2_CID_JPEG_CLASS_BASE:				return "V4L2_CID_JPEG_CLASS_BASE";
	case V4L2_CID_JPEG_CLASS:					return "V4L2_CID_JPEG_CLASS";
	case V4L2_CID_JPEG_CHROMA_SUBSAMPLING:		return "V4L2_CID_JPEG_CHROMA_SUBSAMPLING";
	case V4L2_CID_JPEG_RESTART_INTERVAL:		return "V4L2_CID_JPEG_RESTART_INTERVAL";
	case V4L2_CID_JPEG_COMPRESSION_QUALITY:		return "V4L2_CID_JPEG_COMPRESSION_QUALITY";
	case V4L2_CID_JPEG_ACTIVE_MARKER:			return "V4L2_CID_JPEG_ACTIVE_MARKER";
	case V4L2_CID_IMAGE_SOURCE_CLASS_BASE:		return "V4L2_CID_IMAGE_SOURCE_CLASS_BASE";
	case V4L2_CID_IMAGE_SOURCE_CLASS:			return "V4L2_CID_IMAGE_SOURCE_CLASS";
	case V4L2_CID_VBLANK:						return "V4L2_CID_VBLANK";
	case V4L2_CID_HBLANK:						return "V4L2_CID_HBLANK";
	case V4L2_CID_ANALOGUE_GAIN:				return "V4L2_CID_ANALOGUE_GAIN";
	case V4L2_CID_TEST_PATTERN_RED:				return "V4L2_CID_TEST_PATTERN_RED";
	case V4L2_CID_TEST_PATTERN_GREENR:			return "V4L2_CID_TEST_PATTERN_GREENR";
	case V4L2_CID_TEST_PATTERN_BLUE:			return "V4L2_CID_TEST_PATTERN_BLUE";
	case V4L2_CID_TEST_PATTERN_GREENB:			return "V4L2_CID_TEST_PATTERN_GREENB";
	case V4L2_CID_UNIT_CELL_SIZE:				return "V4L2_CID_UNIT_CELL_SIZE";
	case V4L2_CID_IMAGE_PROC_CLASS_BASE:		return "V4L2_CID_IMAGE_PROC_CLASS_BASE";
	case V4L2_CID_IMAGE_PROC_CLASS:				return "V4L2_CID_IMAGE_PROC_CLASS";
	case V4L2_CID_LINK_FREQ:					return "V4L2_CID_LINK_FREQ";
	case V4L2_CID_PIXEL_RATE:					return "V4L2_CID_PIXEL_RATE";
	case V4L2_CID_TEST_PATTERN:					return "V4L2_CID_TEST_PATTERN";
	case V4L2_CID_DEINTERLACING_MODE:			return "V4L2_CID_DEINTERLACING_MODE";
	case V4L2_CID_DIGITAL_GAIN:					return "V4L2_CID_DIGITAL_GAIN";
// 	case V4L2_CID_COLORIMETRY_CLASS_BASE:		return "V4L2_CID_COLORIMETRY_CLASS_BASE";
	case V4L2_CID_COLORIMETRY_CLASS:			return "V4L2_CID_COLORIMETRY_CLASS";
// 	case V4L2_CID_COLORIMETRY_HDR10_CLL_INFO:	return "V4L2_CID_COLORIMETRY_HDR10_CLL_INFO";
	case V4L2_CID_COLORIMETRY_HDR10_MASTERING_DISPLAY:		return "V4L2_CID_COLORIMETRY_HDR10_MASTERING_DISPLAY";
	default:
		return "UNKNOWN V4L2_CID";
	}
}

/**
 * コントロール機能の種類を文字列に変換
 * @param type
 * @param const char *
*/
const char *ctrl_type_string(const uint32_t &type) {
	switch (type) {
	case V4L2_CTRL_TYPE_INTEGER:		// 1,
		return "V4L2_CTRL_TYPE_INTEGER";
	case V4L2_CTRL_TYPE_BOOLEAN:		// 2,
		return "V4L2_CTRL_TYPE_BOOLEAN";
	case V4L2_CTRL_TYPE_MENU:			// 3,
		return "V4L2_CTRL_TYPE_MENU";
	case V4L2_CTRL_TYPE_BUTTON:			// 4,
		return "V4L2_CTRL_TYPE_BUTTON";
	case V4L2_CTRL_TYPE_INTEGER64:		// 5,
		return "V4L2_CTRL_TYPE_INTEGER64";
	case V4L2_CTRL_TYPE_CTRL_CLASS:		// 6,
		return "V4L2_CTRL_TYPE_CTRL_CLASS";
	case V4L2_CTRL_TYPE_STRING:			// 7,
		return "V4L2_CTRL_TYPE_STRING";
	case V4L2_CTRL_TYPE_BITMASK:		// 8,
		return "V4L2_CTRL_TYPE_BITMASK";
	case V4L2_CTRL_TYPE_INTEGER_MENU:	// 9,
		return "V4L2_CTRL_TYPE_INTEGER_MENU";

	/* Compound types are >= 0x0100 */
	// case V4L2_CTRL_COMPOUND_TYPES:		// 0x0100,
	// 	return "V4L2_CTRL_COMPOUND_TYPES";
	case V4L2_CTRL_TYPE_U8:				// 0x0100,
		return "V4L2_CTRL_TYPE_U8";
	case V4L2_CTRL_TYPE_U16:			// 0x0101,
		return "V4L2_CTRL_TYPE_U16";
	case V4L2_CTRL_TYPE_U32:			// 0x0102,
		return "V4L2_CTRL_TYPE_U32";
	// case V4L2_CTRL_TYPE_AREA:			// 0x0106,
	// 	return "V4L2_CTRL_TYPE_AREA";

	// case V4L2_CTRL_TYPE_HDR10_CLL_INFO:	// 0x0110,
	// 	return "V4L2_CTRL_TYPE_HDR10_CLL_INFO";
	// case V4L2_CTRL_TYPE_HDR10_MASTERING_DISPLAY:	// 0x0111,
	// 	return "V4L2_CTRL_TYPE_HDR10_MASTERING_DISPLAY";

	// case V4L2_CTRL_TYPE_H264_SPS:		// 0x0200,
	// 	return "V4L2_CTRL_TYPE_H264_SPS";
	// case V4L2_CTRL_TYPE_H264_PPS:		// 0x0201,
	// 	return "V4L2_CTRL_TYPE_H264_PPS";
	// case V4L2_CTRL_TYPE_H264_SCALING_MATRIX:	// 0x0202,
	// 	return "V4L2_CTRL_TYPE_H264_SCALING_MATRIX";
	// case V4L2_CTRL_TYPE_H264_SLICE_PARAMS:		// 0x0203,
	// 	return "V4L2_CTRL_TYPE_H264_SLICE_PARAMS";
	// case V4L2_CTRL_TYPE_H264_DECODE_PARAMS:		// 0x0204,
	// 	return "V4L2_CTRL_TYPE_H264_DECODE_PARAMS";
	// case V4L2_CTRL_TYPE_H264_PRED_WEIGHTS:		// 0x0205,
	// 	return "V4L2_CTRL_TYPE_H264_PRED_WEIGHTS";

	// case V4L2_CTRL_TYPE_FWHT_PARAMS:	// 0x0220,
	// 	return "V4L2_CTRL_TYPE_FWHT_PARAMS";

	// case V4L2_CTRL_TYPE_VP8_FRAME:		// 0x0240,
	// 	return "V4L2_CTRL_TYPE_VP8_FRAME";

	// case V4L2_CTRL_TYPE_MPEG2_QUANTISATION:	// 0x0250,
	// 	return "V4L2_CTRL_TYPE_MPEG2_QUANTISATION";
	// case V4L2_CTRL_TYPE_MPEG2_SEQUENCE:		// 0x0251,
	// 	return "V4L2_CTRL_TYPE_MPEG2_SEQUENCE";
	// case V4L2_CTRL_TYPE_MPEG2_PICTURE:		// 0x0252,
	// 	return "V4L2_CTRL_TYPE_MPEG2_PICTURE";
	default:
		return "UNKNOWN V4L2_CTRL_TYPE";
	}
}

/**
 * コントロール機能がメニュータイプの場合の設定項目値をログ出力する
 * @param fd V4L2機器のファイルディスクリプタ
 * @param query
 */
void list_menu_ctrl(int fd, const struct v4l2_queryctrl &query) {
	LOGI("  type=V4L2_CTRL_TYPE_MENU(0x0003):");
	for (uint32_t i = query.minimum; i <= query.maximum; i++) {
		struct v4l2_querymenu menu {
			.id = query.id,
			.index = i,
		};
		if (ioctl(fd, VIDIOC_QUERYMENU, &menu) == 0) {
			LOGI("    %s", menu.name);
		} else {
			break;
		}
	}
}

/**
 * v4l2_queryctrlをログへ出力
 * @param fd V4L2機器のファイルディスクリプタ
 * @param query
 */
void dump_ctrl(int fd, const struct v4l2_queryctrl &query) {
	LOGI("supported:%s/%s(0x%08x)", query.name, ctrl_id_string(query.id), query.id);
	if (query.type == V4L2_CTRL_TYPE_MENU) {
		list_menu_ctrl(fd, query);
	} else {
		LOGI("  type=%s(0x%04x):",
			ctrl_type_string(query.type), query.type);
		LOGI("  min=%d,max=%d,step=%d,default=%d,flags=0x%08x",
			query.minimum, query.maximum,
			query.step, query.default_value, query.flags);
	}
}

/**
 * 対応するコントロール機能一覧をマップに登録する
 * @param fd V4L2機器のファイルディスクリプタ
 * @param supported
 * @param dump 見つかったコントロール機能をログへ出力するかどうか, デフォルト=false(ログへ出力しない)
 */
void update_ctrl_all_locked(int fd, std::unordered_map<uint32_t, QueryCtrlSp> &supported, const bool &dump) {
	ENTER();

	supported.clear();
	// v4l2標準コントロール一覧
	for (uint32_t i = V4L2_CID_BASE; i <  V4L2_CID_LASTP1; i++) {
		struct v4l2_queryctrl query {
			.id = i,
		};
		int r = xioctl(fd, VIDIOC_QUERYCTRL, &query);
		if ((r != -1) && !(query.flags & V4L2_CTRL_FLAG_DISABLED)) {
			supported[i] = std::make_shared<struct v4l2_queryctrl>(query);
			if (dump) {
				dump_ctrl(fd, query);
			}
		}
	}
	// v4l2標準のカメラコントロール一覧
	for (uint32_t i = V4L2_CID_EXPOSURE_AUTO + 1; i < V4L2_CID_LASTP1; i++) {
		struct v4l2_queryctrl query {
			.id = i,
		};
		int r = xioctl(fd, VIDIOC_QUERYCTRL, &query);
		if ((r != -1) && !(query.flags & V4L2_CTRL_FLAG_DISABLED)) {
			supported[i] = std::make_shared<struct v4l2_queryctrl>(query);
			if (dump) {
				dump_ctrl(fd, query);
			}
		}
	}
	// ドライバー固有コントロール一覧
	for (uint32_t i = V4L2_CID_PRIVATE_BASE; ; i++) {
		struct v4l2_queryctrl query {
			.id = i,
		};
		int r = xioctl(fd, VIDIOC_QUERYCTRL, &query);
		if (r == 0) {
			if (!(query.flags & V4L2_CTRL_FLAG_DISABLED)) {
				supported[i] = std::make_shared<struct v4l2_queryctrl>(query);
			if (dump) {
				dump_ctrl(fd, query);
			}
			}
		} else {
			break;
		}
	}

	EXIT();
}

/**
 * jsonへフレームレート設定を出力するためのヘルパー関数
 * @param writer
 * @param frames
 * @param max_width
 * @param max_height
 */
void write_fps(Writer<StringBuffer> &writer,
	const std::vector<FrameInfoSp> &frames,
	const int &max_width, const int &max_height) {

	char buf[1024];

	// frameRateキーにFPS配列を各解像度毎の配列として追加
	writer.String(FRAME_FRAME_RATE);
	writer.StartArray();
	for (const auto& frame: frames) {
		if ( ((max_width > 0) && (frame->width > max_width))
			|| ((max_height > 0) && (frame->height > max_height)) ) continue;

		if (!frame->frame_rates.empty()) {
			writer.StartArray();
			for (const auto &fps: frame->frame_rates) {
				switch (fps->type) {
				case V4L2_FRMIVAL_TYPE_DISCRETE:
				{
					snprintf(buf, sizeof(buf), "%4.1f", (float)fps->discrete.denominator / (float)fps->discrete.numerator);
					buf[sizeof(buf) - 1] = '\0';
					writer.String(buf);
					break;
				}
				case V4L2_FRMIVAL_TYPE_CONTINUOUS:
				{	// XXX 階層が1つ深いかもしれない
					writer.StartObject();
					// 最小fps
					write(writer, FRAME_INTERVAL_MIN, (float)fps->stepwise.min.denominator / (float)fps->stepwise.min.numerator);
					// 最大fps
					write(writer, FRAME_INTERVAL_MAX, (float)fps->stepwise.max.denominator / (float)fps->stepwise.max.numerator);
					writer.EndObject();
					break;
				}
				case V4L2_FRMIVAL_TYPE_STEPWISE:
				{	// XXX 階層が1つ深いかもしれない
					writer.StartObject();
					// 最小fps
					write(writer, FRAME_INTERVAL_MIN, (float)fps->stepwise.min.denominator / (float)fps->stepwise.min.numerator);
					// 最大fps
					write(writer, FRAME_INTERVAL_MAX, (float)fps->stepwise.max.denominator / (float)fps->stepwise.max.numerator);
					// fpsステップ
					write(writer, FRAME_INTERVAL_STEP, (float)fps->stepwise.step.denominator / (float)fps->stepwise.step.numerator);
					writer.EndObject();
					break;
				}
				default:
					LOGW("unknown type, %d", fps->type);
					break;
				}
			}
			writer.EndArray();
		}
	}
	writer.EndArray();	// FPS

	// intervalsキーにinterval配列を各解像度毎の配列として追加
	writer.String(FRAME_INTERVALS);
	writer.StartArray();
	for (const auto& frame: frames) {
		if ( ((max_width > 0) && (frame->width > max_width))
			|| ((max_height > 0) && (frame->height > max_height)) ) continue;

		if (!frame->frame_rates.empty()) {
			writer.StartArray();
			for (const auto &fps: frame->frame_rates) {
				switch (fps->type) {
				case V4L2_FRMIVAL_TYPE_DISCRETE:
				{
					snprintf(buf, sizeof(buf), "%d", (uint32_t)((10000000 * fps->discrete.numerator) / fps->discrete.denominator));
					buf[sizeof(buf) - 1] = '\0';
					writer.String(buf);
					break;
				}
				case V4L2_FRMIVAL_TYPE_CONTINUOUS:
				{	// XXX 階層が1つ深いかもしれない
					writer.StartObject();
					// 最小fps
					write(writer, FRAME_INTERVAL_MIN, (uint32_t)((10000000 * fps->stepwise.min.numerator) / fps->stepwise.min.denominator));
					// 最大fps
					write(writer, FRAME_INTERVAL_MAX, (uint32_t)((10000000 * fps->stepwise.max.numerator) / fps->stepwise.max.denominator));
					writer.EndObject();
					break;
				}
				case V4L2_FRMIVAL_TYPE_STEPWISE:
				{	// XXX 階層が1つ深いかもしれない
					writer.StartObject();
					// 最小fps
					write(writer, FRAME_INTERVAL_MIN, (uint32_t)((10000000 * fps->stepwise.min.numerator) / fps->stepwise.min.denominator));
					// 最大fps
					write(writer, FRAME_INTERVAL_MAX, (uint32_t)((10000000 * fps->stepwise.max.numerator) / fps->stepwise.max.denominator));
					// fpsステップ
					write(writer, FRAME_INTERVAL_MAX, (uint32_t)((10000000 * fps->stepwise.step.numerator) / fps->stepwise.step.denominator));
					writer.EndObject();
					break;
				}
				default:
					LOGW("unknown type, %d", fps->type);
					break;
				}
			}
			writer.EndArray();
		}
	}
	writer.EndArray();	// FPS
}

/**
 * 対応するピクセルフォーマット・解像度・フレームレートをjson文字列として取得する
 * ::get_supported_size, ::get_supported_frameの下請け
 * @param fd
 * @param frame
 */
void get_supported_fps(int fd, const FrameInfoSp &frame) {

	ENTER();

	LOGD("fmt=%s,sz(%dx%d)",
		V4L2_PIX_FMT_to_string(frame->pixel_format).c_str(),
		frame->width, frame->height);

	int r = 0;
	for (int i = 0; r != -1; i++) {
		struct v4l2_frmivalenum fmt {
			.pixel_format = frame->pixel_format,
			.width = frame->width,
			.height = frame->height,
		};
		fmt.index = i;
		r = xioctl(fd, VIDIOC_ENUM_FRAMEINTERVALS, &fmt);
		if (r != -1) {
			switch (fmt.type) {
			case V4L2_FRMIVAL_TYPE_DISCRETE:
			{
				LOGD("DISCRETE%i:fmt=%s,sz(%dx%d),fps=%d/%d",
					fmt.index,
				  	V4L2_PIX_FMT_to_string(fmt.pixel_format).c_str(),
				  	fmt.width, fmt.height,
					fmt.discrete.numerator, fmt.discrete.denominator);
				frame->frame_rates.push_back(std::make_shared<struct v4l2_frmivalenum>(fmt));
				break;
			}
			case V4L2_FRMIVAL_TYPE_CONTINUOUS:
			{
				LOGD("STEPWISE%i:fmt=%s,sz(%dx%d),fps=%d/%d-%d/%d",
					fmt.index,
					V4L2_PIX_FMT_to_string(fmt.pixel_format).c_str(),
					fmt.width, fmt.height,
					fmt.stepwise.min.numerator, fmt.stepwise.min.denominator,
					fmt.stepwise.max.numerator, fmt.stepwise.max.denominator);
				frame->frame_rates.push_back(std::make_shared<struct v4l2_frmivalenum>(fmt));
				break;
			}
			case V4L2_FRMIVAL_TYPE_STEPWISE:
			{
				LOGD("STEPWISE%i:fmt=%s,sz(%dx%d),fps=%d/%d(%d)-%d/%d'%d)", fmt.index,
					V4L2_PIX_FMT_to_string(fmt.pixel_format).c_str(),
					fmt.width, fmt.height,
					fmt.stepwise.min.numerator, fmt.stepwise.min.denominator, fmt.stepwise.step.numerator,
					fmt.stepwise.max.numerator, fmt.stepwise.max.denominator, fmt.stepwise.step.denominator);
				frame->frame_rates.push_back(std::make_shared<struct v4l2_frmivalenum>(fmt));
				break;
			}
			default:
				LOGW("unknown type, %d", fmt.type);
				break;
			}
		} else if (i == 0) {
			LOGD("not found, r=%d,errno=%d", r, errno);
		}
	}

	EXIT();
}

/**
 * 対応するピクセルフォーマット・解像度・フレームレートをjson文字列として取得する
 * ::get_supported_sizeの下請け
 * @param fd
 * @param format
 */
void get_supported_frame_size(int fd, FormatInfoSp &format) {

	ENTER();

	const std::string fourcc = V4L2_PIX_FMT_to_string(format->pixel_format);
	LOGD("fourcc=%s", fourcc.c_str());
	int r = 0;
	for (int i = 0; r != -1; i++) {
		struct v4l2_frmsizeenum fmt {
			.pixel_format = format->pixel_format,
		};
		fmt.index = i;
		r = xioctl(fd, VIDIOC_ENUM_FRAMESIZES, &fmt);
		if (r != -1) {
			switch (fmt.type) {
			case V4L2_FRMSIZE_TYPE_DISCRETE:
			{
				LOGV("%s:DISCRETE%i)(%dx%d)", fourcc.c_str(), fmt.index,
					fmt.discrete.width, fmt.discrete.height);
				auto frame = std::make_shared<frame_info_t>(i, format->pixel_format, fmt.type);
				frame->width = fmt.discrete.width;
				frame->height = fmt.discrete.height;
				get_supported_fps(fd, frame);
				format->frames.push_back(frame);
				break;
			}
			case V4L2_FRMSIZE_TYPE_CONTINUOUS:
			{
				LOGD("%s:CONTINUOUS%i)(%dx%d)-(%dx%d)", fourcc.c_str(), fmt.index,
					fmt.stepwise.min_width, fmt.stepwise.max_width,
					fmt.stepwise.min_height, fmt.stepwise.max_height);
				// FIXME 未実装
				break;
			}
			case V4L2_FRMSIZE_TYPE_STEPWISE:
			{
				LOGD("%s:STEPWISE%i)(%dx%d)%d-(%dx%d)%d", fourcc.c_str(), fmt.index,
					fmt.stepwise.min_width, fmt.stepwise.max_width, fmt.stepwise.step_width,
					fmt.stepwise.min_height, fmt.stepwise.max_height, fmt.stepwise.step_height);
				// FIXME 未実装
				break;
			}
			default:
				LOGW("%i:%s unsupported type %d", fmt.index, fourcc.c_str(), fmt.type);
				break;
			}
		} else if (i == 0) {
			LOGD("not found, errno=%d", errno);
		}
	}

	EXIT();
}

/**
 * 指定したピクセルフォーマット・解像度・フレームレートに対応しているかどうかを確認
 * ::find_stream, ::find_frameの下請け
 * @param fd
 * @param pixel_format
 * @param width
 * @param height
 * @param min_fps
 * @param max_fps
 * @return
 */
int find_fps(int fd,
	const uint32_t &pixel_format,
	const uint32_t &width, const uint32_t &height,
	const float &min_fps, const float &max_fps) {

	ENTER();

	int result = core::USB_ERROR_NOT_SUPPORTED;

	int r = 0;
	for (int i = 0; result && (r != -1); i++) {
		struct v4l2_frmivalenum fmt {
			.pixel_format = pixel_format,
			.width = width,
			.height = height,
		};
		fmt.index = i;
		r = xioctl(fd, VIDIOC_ENUM_FRAMEINTERVALS, &fmt);
		if (r != -1) {
			switch (fmt.type) {
			case V4L2_FRMIVAL_TYPE_DISCRETE:
				{
					const float fps = (float)fmt.discrete.numerator / (float)fmt.discrete.denominator;
					result = (fps >= min_fps) && (fps < max_fps);
#if defined(USE_LOGD)
					if (!result) {
						LOGD("found DISCRETE%i:fmt=%s,sz(%dx%d),fps=%d/%d",
							fmt.index,
							V4L2_PIX_FMT_to_string(fmt.pixel_format).c_str(),
							fmt.width, fmt.height,
							fmt.discrete.numerator, fmt.discrete.denominator);
					}
#endif
				}
				break;
			case V4L2_FRMIVAL_TYPE_CONTINUOUS:
				{
					const float min = (float)fmt.stepwise.min.numerator / (float)fmt.stepwise.min.denominator;
					const float max = (float)fmt.stepwise.max.numerator / (float)fmt.stepwise.max.denominator;
					result = ((min >= min_fps) && (min < max_fps))
						|| ((max >= min_fps) && (max < max_fps));
#if defined(USE_LOGD)
					if (!result) {
						LOGD("found STEPWISE%i:fmt=%s,sz(%dx%d),fps=%d/%d-%d/%d",
							fmt.index,
							V4L2_PIX_FMT_to_string(fmt.pixel_format).c_str(),
							fmt.width, fmt.height,
							fmt.stepwise.min.numerator, fmt.stepwise.min.denominator,
							fmt.stepwise.max.numerator, fmt.stepwise.max.denominator);
					}
#endif
				}
				break;
			case V4L2_FRMIVAL_TYPE_STEPWISE:
				{
					const float min = (float)fmt.stepwise.min.numerator / (float)fmt.stepwise.min.denominator;
					const float max = (float)fmt.stepwise.max.numerator / (float)fmt.stepwise.max.denominator;
					result = ((min >= min_fps) && (min < max_fps))
						|| ((max >= min_fps) && (max < max_fps));
#if defined(USE_LOGD)
					if (!result) {
						LOGD("found STEPWISE%i:fmt=%s,sz(%dx%d),fps=%d/%d-%d/%d((%d/%d)", fmt.index,
							V4L2_PIX_FMT_to_string(fmt.pixel_format).c_str(),
							fmt.width, fmt.height,
							fmt.stepwise.min.numerator, fmt.stepwise.min.denominator,
							fmt.stepwise.max.numerator, fmt.stepwise.max.denominator,
							fmt.stepwise.step.numerator, fmt.stepwise.step.denominator);
					}
#endif
				}
				break;
			}
		} else if (i == 0) {
			// なぜかフレームレートを取得できないときがあるのでその場合は見つかったことにする
			LOGD("not found, r=%d,errno=%d", r, errno);
			result = core::USB_SUCCESS;
		}
	}

	RETURN(result, int);
}

/**
 * 指定したピクセルフォーマット・解像度・フレームレートに対応しているかどうかを確認
 * ::find_streamの下請け
 * @param fd
 * @param pixel_format
 * @param width
 * @param height
 * @param min_fps
 * @param max_fps
 * @return 0: 対応している, 0以外: 対応していない
 */
int find_frame_size(int fd,
	const uint32_t &pixel_format,
	const uint32_t &width, const uint32_t &height,
	const float &min_fps, const float &max_fps) {

	ENTER();

	int result = core::USB_ERROR_NOT_SUPPORTED;
	int r = 0;
	LOGD("pixel format=0x%08x=%s",
		pixel_format, V4L2_PIX_FMT_to_string(pixel_format).c_str());
	for (int i = 0; result && (r != -1); i++) {
		struct v4l2_frmsizeenum fmt {
			.pixel_format = pixel_format,
		};
		fmt.index = i;
		r = xioctl(fd, VIDIOC_ENUM_FRAMESIZES, &fmt);
		if (r != -1) {
			switch (fmt.type) {
			case V4L2_FRMSIZE_TYPE_DISCRETE:
				LOGV("DISCRETE%i:(%dx%d)", fmt.index,
					fmt.discrete.width, fmt.discrete.height);
				if ((width == fmt.discrete.width) && (height == fmt.discrete.height)) {
					result = find_fps(fd, pixel_format, width, height, min_fps, max_fps);
				}
				break;
			case V4L2_FRMSIZE_TYPE_CONTINUOUS:
				LOGV("CONTINUOUS%i:(%dx%d)-(%dx%d)", fmt.index,
					fmt.stepwise.min_width, fmt.stepwise.max_width,
					fmt.stepwise.min_height, fmt.stepwise.max_height);
				if ((width >= fmt.stepwise.min_width) && (width < fmt.stepwise.max_width)
					&& (height >= fmt.stepwise.min_height) && (height < fmt.stepwise.max_height)) {

					result = find_fps(fd, pixel_format, width, height, min_fps, max_fps);
				}
				break;
			case V4L2_FRMSIZE_TYPE_STEPWISE:
				LOGV("STEPWISE%i:(%dx%d)%d-(%dx%d)%d", fmt.index,
					fmt.stepwise.min_width, fmt.stepwise.max_width, fmt.stepwise.step_width,
					fmt.stepwise.min_height, fmt.stepwise.max_height, fmt.stepwise.step_height);
				if ((width >= fmt.stepwise.min_width) && (width < fmt.stepwise.max_width)
					&& (height >= fmt.stepwise.min_height) && (height < fmt.stepwise.max_height)) {

					result = find_fps(fd, pixel_format, width, height, min_fps, max_fps);
				}
				break;
			default:
				LOGW("%i:Unsupported type %d", fmt.index, fmt.type);
				break;
			}
		} else {
			LOGD("VIDIOC_ENUM_FRAMESIZES failed,ix=%d),err=%d", i, r);
		}
	}

	RETURN(result, int);
}

/**
 * 指定したピクセルフォーマットのフレームサイズ設定の数を取得する
 * @param fd
 * @param pixel_format
 * @return VIDIOC_ENUM_FRAMESIZES呼び出しが成功した回数
*/
int get_frame_size_nums(int fd, const uint32_t &pixel_format) {
	ENTER();

	int result = 0;
	int r = 0;

	for (int i = 0; result && (r != -1); i++) {
		struct v4l2_frmsizeenum fmt {
			.pixel_format = pixel_format,
		};
		fmt.index = i;
		r = xioctl(fd, VIDIOC_ENUM_FRAMESIZES, &fmt);
		if (r != -1) {
			result++;
		}
	}

	RETURN(result, int);
}

}   // namespace serenegiant::v4l2
