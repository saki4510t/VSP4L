/*
 * aAndUsb
 * Copyright (c) 2014-2023 saki t_saki@serenegiant.com
 * Distributed under the terms of the GNU Lesser General Public License (LGPL v3.0) License.
 * License details are in the file license.txt, distributed as part of this software.
 */

#ifndef AANDUVC_H_
#define AANDUVC_H_

#include <memory>
#include <linux/usb/video.h>
// core
#include "core/video.h"
#include "core/video_frame_base.h"
#include "core/video_frame_queue.h"
#include "core/frame_queue.h"
// usb
#include "usb/aandusb.h"

namespace serenegiant::usb::uvc {

typedef core::BaseVideoFrame VideoFrame;
typedef core::BaseVideoFrameSp VideoFrameSp;
typedef core::VideoFrameQueue UVCFrameQueue;
typedef core::VideoFrameSpQueue UVCFrameSpQueue;
typedef std::shared_ptr<UVCFrameQueue> UVCFrameQueueSp;
typedef std::unique_ptr<UVCFrameQueue> UVCFrameQueueUp;
typedef std::shared_ptr<core::VideoFrameSpQueue> VideoFrameSpQueueSp;
typedef std::unique_ptr<core::VideoFrameSpQueue> VideoFrameSpQueueUp;

// ステータス転送を使う時1
#define USE_STATUS_TRANSFER 1

#if 0	// これはvideo.h内で定義済み
#define UVC_VS_UNDEFINED 0x00
#define UVC_VS_INPUT_HEADER 0x01
#define UVC_VS_OUTPUT_HEADER 0x02
#define UVC_VS_STILL_IMAGE_FRAME 0x03
#define UVC_VS_FORMAT_UNCOMPRESSED 0x04
#define UVC_VS_FRAME_UNCOMPRESSED 0x05
#define UVC_VS_FORMAT_MJPEG 0x06
#define UVC_VS_FRAME_MJPEG 0x07
//#define UVC_reserved 0x08
//#define UVC_reserved 0x09
#define UVC_VS_FORMAT_MPEG2TS 0x0a
//#define UVC_reserved 0x0b
#define UVC_VS_FORMAT_DV 0x0c
#define UVC_VS_COLORFORMAT 0x0d
//#define UVC_reserved 0x0e
//#define UVC_reserved 0x0f
#define UVC_VS_FORMAT_FRAME_BASED 0x10
#define UVC_VS_FRAME_FRAME_BASED 0x11
#define UVC_VS_FORMAT_STREAM_BASED 0x12

#endif // ここまではvideo.h内で定義済み

#define VC_ENCODING_UNIT 0x07

enum uvc_status_class {
	UVC_STATUS_CLASS_UNKNOWN = 0,
	UVC_STATUS_CLASS_CONTROL = 0x10,
	UVC_STATUS_CLASS_CONTROL_CAMERA = 0x11,
	UVC_STATUS_CLASS_CONTROL_PROCESSING = 0x12,

	UVC_STATUS_CLASS_TRIGGER = 0x20,
};

enum uvc_status_attribute {
	UVC_STATUS_ATTRIBUTE_VALUE_CHANGE = 0x00,
	UVC_STATUS_ATTRIBUTE_INFO_CHANGE = 0x01,
	UVC_STATUS_ATTRIBUTE_FAILURE_CHANGE = 0x02,
	UVC_STATUS_ATTRIBUTE_UNKNOWN = 0xff
};

/*
// コントロールインターフェースの値取得・設定用の要求コード
typedef enum uvc_req_code {
//	REQ_UNDEFINED = 0x00,
	REQ_SET_CUR = UVC_SET_CUR,	// bmRequestType=0x21
//	REQ_SET_MIN = UVC_SET_MIN,	// ↑
//	REQ_SET_MAX = UVC_SET_MAX,	// ↑
//	REQ_SET_RES = UVC_SET_RES,	// ↑
//	REQ_SET_LEN = UVC_SET_LEN,	// ↑
//	REQ_SET_INFO = UVC_SET_INFO,// ↑
//	REQ_SET_DEF = UVC_SET_DEF,	// ↑
	REQ_GET_CUR = UVC_GET_CUR,	// bmRequestType=0xa1
	REQ_GET_MIN = UVC_GET_MIN,	// ↑
	REQ_GET_MAX = UVC_GET_MAX,	// ↑
	REQ_GET_RES = UVC_GET_RES,	// ↑
	REQ_GET_LEN = UVC_GET_LEN,	// ↑
	REQ_GET_INFO = UVC_GET_INFO,// ↑
	REQ_GET_DEF = UVC_GET_DEF	// ↑
} uvc_req_code_t; */

typedef enum device_power_mode {
	VC_VIDEO_POWER_MODE_FULL = 0x000b,
	VC_VIDEO_POWER_MODE_DEVICE_DEPENDENT = 0x001b,
} device_power_mode_t;

// Selector Unit Control Selectors (A.9.3)
typedef enum su_ctrl_selector {
	SU_CONTROL_UNDEFINED = 0100,
	SU_INPUT_SELECT_CONTROL = 0x01,
} su_ctrl_selector_t;

// Camera terminal control selector (A.9.4)
typedef enum ct_ctrl_selector {
	CT_CONTROL_UNDEFINED = UVC_CT_CONTROL_UNDEFINED, // 0x00,
	CT_SCANNING_MODE_CONTROL = UVC_CT_SCANNING_MODE_CONTROL, // = 0x01,
	CT_AE_MODE_CONTROL = UVC_CT_AE_MODE_CONTROL, // 0x02,
	CT_AE_PRIORITY_CONTROL = UVC_CT_AE_PRIORITY_CONTROL, // 0x03,
	CT_EXPOSURE_TIME_ABSOLUTE_CONTROL = UVC_CT_EXPOSURE_TIME_ABSOLUTE_CONTROL, // 0x04,
	CT_EXPOSURE_TIME_RELATIVE_CONTROL = UVC_CT_EXPOSURE_TIME_RELATIVE_CONTROL, // 0x05,
	CT_FOCUS_ABSOLUTE_CONTROL = UVC_CT_FOCUS_ABSOLUTE_CONTROL, // 0x06,
	CT_FOCUS_RELATIVE_CONTROL = UVC_CT_FOCUS_RELATIVE_CONTROL, // 0x07,
	CT_FOCUS_AUTO_CONTROL = UVC_CT_FOCUS_AUTO_CONTROL, // 0x08,
	CT_IRIS_ABSOLUTE_CONTROL = UVC_CT_IRIS_ABSOLUTE_CONTROL, // 0x09,
	CT_IRIS_RELATIVE_CONTROL = UVC_CT_IRIS_RELATIVE_CONTROL, // 0x0a,
	CT_ZOOM_ABSOLUTE_CONTROL = UVC_CT_ZOOM_ABSOLUTE_CONTROL, // 0x0b,
	CT_ZOOM_RELATIVE_CONTROL = UVC_CT_ZOOM_RELATIVE_CONTROL, // 0x0c,
	CT_PANTILT_ABSOLUTE_CONTROL = UVC_CT_PANTILT_ABSOLUTE_CONTROL, // 0x0d,
	CT_PANTILT_RELATIVE_CONTROL = UVC_CT_PANTILT_RELATIVE_CONTROL, // 0x0e,
	CT_ROLL_ABSOLUTE_CONTROL = UVC_CT_ROLL_ABSOLUTE_CONTROL, // 0x0f,
	CT_ROLL_RELATIVE_CONTROL = UVC_CT_ROLL_RELATIVE_CONTROL, // 0x10,
	CT_PRIVACY_CONTROL = UVC_CT_PRIVACY_CONTROL, // 0x11,
	CT_FOCUS_SIMPLE_CONTROL = /*UVC_CT_FOCUS_SIMPLE_CONTROL*/ 0x12,
	CT_DIGITAL_WINDOW_CONTROL = /*UVC_CT_DIGITAL_WINDOW_CONTROL*/ 0x13,
	CT_REGION_OF_INTEREST_CONTROL = /*UVC_CT_REGION_OF_INTEREST_CONTROL*/ 0x14,
} ct_ctrl_selector_t;

// Processing unit control selector (A.9.5)
typedef enum pu_ctrl_selector {
	PU_CONTROL_UNDEFINED = UVC_PU_CONTROL_UNDEFINED, // 0x00,
	PU_BACKLIGHT_COMPENSATION_CONTROL = UVC_PU_BACKLIGHT_COMPENSATION_CONTROL, // 0x01,
	PU_BRIGHTNESS_CONTROL = UVC_PU_BRIGHTNESS_CONTROL, // 0x02,
	PU_CONTRAST_CONTROL = UVC_PU_CONTRAST_CONTROL, // 0x03,
	PU_GAIN_CONTROL = UVC_PU_GAIN_CONTROL, // 0x04,
	PU_POWER_LINE_FREQUENCY_CONTROL = UVC_PU_POWER_LINE_FREQUENCY_CONTROL, // 0x05,
	PU_HUE_CONTROL = UVC_PU_HUE_CONTROL, // 0x06,
	PU_SATURATION_CONTROL = UVC_PU_SATURATION_CONTROL, // 0x07,
	PU_SHARPNESS_CONTROL = UVC_PU_SHARPNESS_CONTROL, // 0x08,
	PU_GAMMA_CONTROL = UVC_PU_GAMMA_CONTROL, // 0x09,
	PU_WHITE_BALANCE_TEMPERATURE_CONTROL = UVC_PU_WHITE_BALANCE_TEMPERATURE_CONTROL, // 0x0a,
	PU_WHITE_BALANCE_TEMPERATURE_AUTO_CONTROL = UVC_PU_WHITE_BALANCE_TEMPERATURE_AUTO_CONTROL, // 0x0b,
	PU_WHITE_BALANCE_COMPONENT_CONTROL = UVC_PU_WHITE_BALANCE_COMPONENT_CONTROL, // 0x0c,
	PU_WHITE_BALANCE_COMPONENT_AUTO_CONTROL = UVC_PU_WHITE_BALANCE_COMPONENT_AUTO_CONTROL, // 0x0d,
	PU_DIGITAL_MULTIPLIER_CONTROL = UVC_PU_DIGITAL_MULTIPLIER_CONTROL, // 0x0e,
	PU_DIGITAL_MULTIPLIER_LIMIT_CONTROL = UVC_PU_DIGITAL_MULTIPLIER_LIMIT_CONTROL, // 0x0f,
	PU_HUE_AUTO_CONTROL = UVC_PU_HUE_AUTO_CONTROL, // 0x10,
	PU_ANALOG_VIDEO_STANDARD_CONTROL = UVC_PU_ANALOG_VIDEO_STANDARD_CONTROL, // 0x11,
	PU_ANALOG_LOCK_STATUS_CONTROL = UVC_PU_ANALOG_LOCK_STATUS_CONTROL, // 0x12,
	PU_CONTRAST_AUTO_CONTROL = /*UVC_PU_CONTRAST_AUTO_CONTROL*/ 0x13,
} pu_ctrl_selector_t;

// Encoding Unit Control Selector (A.9.6)
typedef enum eu_ctrl_selector {
	EU_CONTROL_UNDEFINED = 0x00,
	EU_SELECT_LAYER_CONTROL = 0x01,
	EU_PROFILE_TOOLSET_CONTROL = 0x02,
	EU_VIDEO_RESOLUTION_CONTROL = 0x03,
	EU_MIN_FRAME_INTERVAL_CONTROL = 0x04,
	EU_SLICE_MODE_CONTROL = 0x05,
	EU_RATE_CONTROL_MODE_CONTROL = 0x06,
	EU_AVERAGE_BITRATE_CONTROL = 0x07,
	EU_CPB_SIZE_CONTROL = 0x08,
	EU_PEAK_BIT_RATE_CONTROL = 0x09,
	EU_QUANTIZATION_PARAMS_CONTROL = 0x0A,
	EU_SYNC_REF_FRAME_CONTROL = 0x0B,
	EU_LTR_BUFFER_CONTROL = 0x0C,
	EU_LTR_PICTURE_CONTROL = 0x0D,
	EU_LTR_VALIDATION_CONTROL = 0x0E,
	EU_LEVEL_IDC_LIMIT_CONTROL = 0x0F,
	EU_SEI_PAYLOADTYPE_CONTROL = 0x10,
	EU_QP_RANGE_CONTROL = 0x11,
	EU_PRIORITY_CONTROL = 0x12,
	EU_START_OR_STOP_LAYER_CONTROL = 0x13,
	EU_ERROR_RESILIENCY_CONTROL = 0x14,
} eu_ctrl_selector_t;

typedef struct uvc_encoding_unit_descriptor {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint8_t bDescriptorSubType;
	uint8_t bUnitID;
	uint8_t bSourceID;
	uint8_t iEncoding;
	uint8_t bControlSize;
	uint8_t bmControls[3];
	uint8_t bmControlsRuntime[3];
} __attribute__((__packed__)) encoding_unit_descriptor_t;

typedef struct uvc_descriptor_header uvc_descriptor_header_t;
typedef struct uvc_header_descriptor uvc_header_descriptor_t;
typedef struct uvc_input_terminal_descriptor input_terminal_descriptor_t;
typedef struct uvc_output_terminal_descriptor output_terminal_descriptor_t;
typedef struct uvc_camera_terminal_descriptor camera_terminal_descriptor_t;
typedef struct uvc_selector_unit_descriptor selector_unit_descriptor_t;
typedef struct uvc_processing_unit_descriptor processing_unit_descriptor_t;
typedef struct uvc_extension_unit_descriptor extension_unit_descriptor_t;
typedef struct uvc_control_endpoint_descriptor control_endpoint_descriptor_t;
typedef struct uvc_input_header_descriptor input_header_descriptor_t;
typedef struct uvc_output_header_descriptor output_header_descriptor_t;
typedef struct uvc_color_matching_descriptor color_matching_descriptor_t;
typedef struct uvc_format_uncompressed format_uncompressed_t;
typedef struct uvc_frame_uncompressed frame_uncompressed_t;
typedef struct uvc_format_mjpeg format_mjpeg_t;
typedef struct uvc_frame_mjpeg frame_mjpeg_t;

// h264が単独のインターフェースとして実装されている時のフォーマットディスクリプタ
typedef struct uvc_format_h264 {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint8_t bDescriptorSubType;		// VS_FORMAT_H264(0x13) or VS_FORMAT_H264_SIMULCAST(0x15)
	uint8_t bFormatIndex;
	uint8_t bNumFrameDescriptors;
	uint8_t bDefaultFrameIndex;
	uint8_t bMaxCodecConfigDelay;
	uint8_t bmSupportedSliceModes;
	uint8_t bmSupportedSyncFrameTypes;
	uint8_t bResolutionScaling;
	uint8_t Reserved1;
	uint8_t bmSupportedRateControlModes;
	uint16_t wMaxMBperSecOneResolutionNoScalability;
	uint16_t wMaxMBperSecTwoResolutionsNoScalability;
	uint16_t wMaxMBperSecThreeResolutionsNoScalability;
	uint16_t wMaxMBperSecFourResolutionsNoScalability;
	uint16_t wMaxMBperSecOneResolutionTemporalScalability;
	uint16_t wMaxMBperSecTwoResolutionsTemporalScalablility;
	uint16_t wMaxMBperSecThreeResolutionsTemporalScalability;
	uint16_t wMaxMBperSecFourResolutionsTemporalScalability;
	uint16_t wMaxMBperSecOneResolutionTemporalQualityScalability;
	uint16_t wMaxMBperSecTwoResolutionsTemporalQualityScalability;
	uint16_t wMaxMBperSecThreeResolutionsTemporalQualityScalablity;
	uint16_t wMaxMBperSecFourResolutionsTemporalQualityScalability;
	uint16_t wMaxMBperSecOneResolutionsTemporalSpatialScalability;
	uint16_t wMaxMBperSecTwoResolutionsTemporalSpatialScalability;
	uint16_t wMaxMBperSecThreeResolutionsTemporalSpatialScalability;
	uint16_t wMaxMBperSecFourResolutionsTemporalSpatialScalability;
	uint16_t wMaxMBperSecOneResolutionFullScalability;
	uint16_t wMaxMBperSecTwoResolutionsFullScalability;
	uint16_t wMaxMBperSecThreeResolutionsFullScalability;
	uint16_t wMaxMBperSecFourResolutionsFullScalability;
} __attribute__((__packed__)) format_h264_t;

typedef format_h264_t format_h264_simulcast_t;

// h264が単独のインターフェースとして実装されている時のフレームディスクリプタ
typedef struct uvc_frame_h264 {
	uint8_t bLength;			// 44 + (bNumFrameIntervals ✕ 4)バイト
	uint8_t bDescriptorType;	// CS_INTERFACE
	uint8_t bDescriptorSubType;	// VS_FRAME_H264(0x14)
	uint8_t bFrameIndex;
	uint16_t wWidth;
	uint16_t wHeight;
	uint16_t wSARwidth;
	uint16_t wSARheight;
	uint16_t wProfile;
	uint8_t bLevelIDC;
	uint16_t wConstrainedToolset;
	uint32_t bmSupportedUsages;
	uint16_t bmCapabilities;
	uint32_t bmSVCCapabilities;
	uint32_t bmMVCCapabilities;
	uint32_t dwMinBitRate;
	uint32_t dwMaxBitRate;
	uint32_t dwDefaultFrameInterval;
	uint8_t bNumFrameIntervals;
	uint32_t dwFrameInterval[];
} __attribute__((__packed__)) frame_h264_t;

// Table 3-3 Updates to the Profile Toolset Control for H.264 Payloads
typedef enum uvc_h264_profile_type {
	H264_PROFILE_TYPE_BASELINE_CONSTRAINED = 0x4240,
	H264_PROFILE_TYPE_BASELINE = 0x4200,
	H264_PROFILE_TYPE_MAIN = 0x4d00,
	H264_PROFILE_TYPE_HIGH_CONSTRAINED = 0x640c,
	H264_PROFILE_TYPE_HIGH = 0x6400,
	H264_PROFILE_TYPE_SCALABLE_BASELINE_CONSTRAINED = 0x5304,
	H264_PROFILE_TYPE_SCALABLE_BASELINE = 0x5300,
	H264_PROFILE_TYPE_SCALABLE_HIGH_CONSTRAINED = 0x5604,
	H264_PROFILE_TYPE_SCALABLE_HIGH = 0x5600,
	H264_PROFILE_TYPE_HIGH_MULTIVIEW = 0x7600,
	H264_PROFILE_TYPE_HIGH_STEREO = 0x8000,
} h264_profile_type_t;

typedef enum uvc_h264_profile_setting {
	UVC_H264_PROFILE_SETTING_CAVLC_ONLY = 0x01,						// D0: このD0とD1を同時にonしゃちゃだめ
	UVC_H264_PROFILE_SETTING_CABAC_ONLY = 0x02,						// D1:
	UVC_H264_PROFILE_SETTING_CONSTANT_FRAME_RATE = 0x04,			// D2:
	UVC_H264_PROFILE_SETTING_SEPARETE_QP_FRAME = 0x18,				// D3,D4: D4が1ならD3は必ず1
	UVC_H264_PROFILE_SETTING_SEPARETE_QP_FRAME_LUMA_CHROMA = 0x08,	// D3: Separate QP for luma/chroma
	UVC_H264_PROFILE_SETTING_SEPARETE_QP_FRAME_CBCR = 0x10,			// D4: Separate QP for Cb/Cr
	UVC_H264_PROFILE_SETTING_NO_PICTURE_RECORDING = 0x20, 			// D5:
	// D15-D8は常にゼロ
} uvc_h264_profile_setting_t;

typedef struct uvc_format_vp8 {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint8_t bDescriptorSubType;		// VS_FORMAT_VP8(0x14) or VS_FORMAT_VP8_SIMULCAST(0x16)
	uint8_t bFormatIndex;
	uint8_t bNumFrameDescriptors;
	uint8_t bDefaultFrameIndex;
	uint8_t bMaxCodecConfigDelay;
	uint8_t bSupportedPartitionCount;
	uint8_t bmSupportedSyncFrameTypes;
	uint8_t bResolutionScaling;
	uint8_t bmSupportedRateControlModes;
	uint16_t wMaxMBperSec;
} __attribute__((__packed__)) format_vp8_t;

typedef format_vp8_t format_vp8_simulcast_t;

typedef struct uvc_frame_vp8 {
	uint8_t bLength;			// 31 + (bNumFrameIntervals ✕ 4)バイト
	uint8_t bDescriptorType;	// CS_INTERFACE
	uint8_t bDescriptorSubType;	// VS_FRAME_VP8
	uint8_t bFrameIndex;
	uint16_t wWidth;
	uint16_t wHeight;
	uint32_t dwSupportedUsages;
	uint16_t bmCapabilities;
	uint32_t bmScalabilityCapabilities;
	uint32_t dwMinBitRate;
	uint32_t dwMaxBitRate;
	uint32_t dwDefaultFrameInterval;
	uint8_t bNumFrameIntervals;
	uint32_t dwFrameInterval[];
} __attribute__((__packed__)) frame_vp8_t;

typedef struct uvc_format_frame_based {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint8_t bDescriptorSubType;		// VS_FORMAT_FRAME_BASED(0x10)
	uint8_t bFormatIndex;
	uint8_t bNumFrameDescriptors;
	uint8_t guidFormat[16];
	uint8_t bBitsPerPixel;
	uint8_t bDefaultFrameIndex;
	uint8_t bAspectRatioX;
	uint8_t bAspectRatioY;
	uint8_t bmInterlaceFlags;
	uint8_t bCopyProtect;
	uint8_t bVariableSize;
} __attribute__((__packed__)) format_frame_based_t;

typedef struct uvc_frame_frame_based {
	uint8_t bLength;			// 31 + (bNumFrameIntervals ✕ 4)バイト
	uint8_t bDescriptorType;	// CS_INTERFACE
	uint8_t bDescriptorSubType;	// VS_FRAME_FRAME_BASED(0x11)
	uint8_t bFrameIndex;
	uint8_t bmCapabilities;
	uint16_t wWidth;
	uint16_t wHeight;
	uint32_t dwMinBitRate;
	uint32_t dwMaxBitRate;
	uint32_t dwDefaultFrameInterval;
	uint8_t bFrameIntervalType;	// 0:連続フレームインターバル, 1-255:個数
	uint32_t dwBytesPerLine;
	uint32_t dwFrameInterval[];	// bFrameIntervalType=0の時はmin,max,stepの3つ, bFrameIntervalType>1の時はbFrameIntervalType個
} __attribute__((__packed__)) frame_frame_based_t;

// MJPEGフレーム中に埋めこまれたペイロードのヘッダー定義
typedef struct uvc_payload_header_mux {
	uint16_t wVersion;
	uint16_t wLength;
	uint32_t dwStreamType;
	uint16_t wWidth;
	uint16_t wHeight;
	uint32_t dwFrameIntervals;
	uint16_t wDelay;
	uint32_t dwPresentationTime;	// 規格上はここまでがペイロードヘッダー
	uint32_t dwPayloadSize;			// こっちはペイロード自体に含まれる
} __attribute__((__packed__)) uvc_payload_header_mux_t;

typedef struct uvc_format_common {
	 uint8_t bLength;
	 uint8_t bDescriptorType;
	 uint8_t bDescriptorSubType;
	 uint8_t bFormatIndex;
	 uint8_t bNumFrameDescriptors;
	 uint8_t guidFormat[16];
	 uint8_t bBitsPerPixel;
	 uint8_t bDefaultFrameIndex;
	 uint8_t bAspectRatioX;
	 uint8_t bAspectRatioY;
	 uint8_t bmInterfaceFlags;
	 uint8_t bCopyProtect;
} __attribute__((__packed__)) format_common_t;

typedef struct uvc_frame_common {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint8_t bDescriptorSubType;
	uint8_t bFrameIndex;
//	uint8_t bmCapabilities;
//	uint16_t wWidth;
//	uint16_t wHeight;
//	uint32_t dwMinBitRate;
//	uint32_t dwMaxBitRate;
//	uint32_t dwMaxVideoFrameBufferSize;
//	uint32_t dwDefaultFrameInterval;
//	uint8_t bFrameIntervalType;
//	uint32_t dwFrameInterval[];
} __attribute__((__packed__)) frame_common_t;

typedef struct _format_descriptor {
	union {
		const format_uncompressed_t *uncompressed;
		const format_mjpeg_t *mjpeg;
		const format_h264_t *h264;
		const format_vp8_t *vp8;
		const format_frame_based_t *frame_based;
		const format_common_t *common;
	};
	uint8_t format_index;
	core::raw_frame_t frame_type;
	uint8_t defaultFrameIndex;
} format_descriptor_t;

typedef struct _frame_descriptor {
	union {
		const frame_uncompressed_t *uncompressed;
		const frame_mjpeg_t *mjpeg;
		const frame_h264_t *h264;
		const frame_vp8_t *vp8;
		const frame_frame_based_t *frame_based;
		const frame_common_t *common;
	};
	uint8_t frame_index;
	core::raw_frame_t frame_type;
	uint16_t width;
	uint16_t height;
	uint32_t minBitRate;
	uint32_t maxBitRate;
	uint32_t maxVideoFrameBufferSize;
	uint8_t frameIntervalType;
	const uint32_t *frameIntervals;
} frame_descriptor_t;

typedef enum _still_capture_method {
	/** アプリ側自体で映像ストリームから静止画を生成、静止画と映像ストリームの解像度は同じ */
	UVC_STILL_CAPTURE_METHOD0 = 0,
	/** ハードウエアトリガーによりMethod0を行う場合。 静止画と映像ストリームの解像度は同じ */
	UVC_STILL_CAPTURE_METHOD1 = 1,
	/** 映像ストリームを使って静止画を送ってくる。静止画の方が解像度が高いかもしれない。ハードウエアトリガーをサポート可
	  * 一旦映像ストリームを止めて静止画用にネゴシエーションして静止画を受信。その後映像ストリームを再開させる */
	UVC_STILL_CAPTURE_METHOD2 = 2,
	/** 映像ストリームとは別のパイプで静止画を送ってくる。静止画の方が解像度が高いかもしれない。ハードウエアトリガーをサポート可 */
	UVC_STILL_CAPTURE_METHOD3 = 3,
} still_capture_method_t;

//
typedef enum still_trigger_mode {
	// 通常状態に復帰(静止画の転送は無い状態)
	UVC_STILL_NORMAL = 0,
	// 静止画の転送要求
	UVC_STILL_TRANSMIT,
	// 予め指定したバルク転送エンドポイントを使って静止画の転送要求(メソッド3)の時のみ有効
	UVC_STILL_TRANSMIT_BULK,
	// 静止画の転送中止要求
	UVC_STILL_ABORT,
} still_trigger_mode_t;

typedef struct _frame_size_t {
	uint16_t wWidth;
	uint16_t wHeight;
} __attribute__((__packed__)) frame_size_t;

// 静止画フレームディスクリプタ
// bNumCompressionPtn以下はアクセス用のコードを作ること
typedef struct _still_image_frame_descriptor {
	uint8_t bLength;
	uint8_t bDescriptorType;		// 0x24 CS_INTERFACE
	uint8_t bDescriptorSubType;		// 0x03 VS_STILL_FRAME
	uint8_t bEndpointAddress;
	uint8_t bNumImageSizePatterns;	// イメージサイズの数n
	// イメージの幅と高さのペアがn個
	frame_size_t tFrameSize[];
//	uint8_t bNumCompressionPtn;		// 圧縮率の数m
//	uint8_t bCompression;			// m個
} __attribute__((__packed__)) still_image_frame_descriptor_t;

/**
 * プルーブ・コミットコントロールの引数用構造体
 * この中のフィールドにはwとかdwで始まっているのも含めてすべてホスト側のエンディアン
 */
typedef struct _uvc_streaming_control {
	uint16_t bmHint;
	uint8_t bFormatIndex;
	uint8_t bFrameIndex;
	uint32_t dwFrameInterval;
	uint16_t wKeyFrameRate;
	uint16_t wPFrameRate;
	uint16_t wCompQuality;
	uint16_t wCompWindowSize;
	uint16_t wDelay;
	uint32_t dwMaxVideoFrameSize;
	uint32_t dwMaxPayloadTransferSize;
// UVC 1.1 parameters
	uint32_t dwClockFrequency;
	uint8_t bmFramingInfo;
	uint8_t bPreferedVersion;
	uint8_t bMinVersion;
	uint8_t bMaxVersion;
// UVC 1.5 parameters
	uint8_t bUsage;
	uint8_t bBitDepthLuna;
	uint8_t bmSettings;
	uint8_t bMaxNumberOfRefFramesPlus1;
	uint16_t bmRateControlModes;
	uint64_t bmLayoutPerStream;
} streaming_control_t;

/**
 * プルーブ・コミットコントロールの引数用構造体
 * この中のフィールドにはwとかdwで始まっているのも含めてすべてホスト側のエンディアン
 */
typedef struct _uvc_video_still_control {
	uint8_t bFormatIndex;
	uint8_t bFrameIndex;
	uint8_t bCompressionIndex;
	uint32_t dwMaxVideoFrameSize;
	uint32_t dwMaxPayloadTransferSize;
} video_still_control_t;


typedef struct _uvc_still_image_trigger_control {
	uint8_t bTrigger;
} __attribute__((__packed__)) still_image_trigger_control_t;

typedef enum vc_error_code_control {
	VC_ERROR_CODECTRL_NO_ERROR = 0x00,
	VC_ERROR_CODECTRL_NOT_READY = 0x01,
	VC_ERROR_CODECTRL_WRONG_STATE = 0x02,
	VC_ERROR_CODECTRL_POWER = 0x03,
	VC_ERROR_CODECTRL_OUT_OF_RANGE = 0x04,
	VC_ERROR_CODECTRL_INVALID_UINT = 0x05,
	VC_ERROR_CODECTRL_INVALID_CONTROL = 0x06,
	VC_ERROR_CODECTRL_INVALID_REQUEST = 0x07,
	VC_ERROR_CODECTRL_INVALID_VALUE = 0x08,
	VC_ERROR_CODECTRL_UNKNOWN = 0xff,
	VC_ERROR_CODECTRL_ERROR = 0xff,
} vc_error_code_control_t;

// 規格書では0-8の数字だけど実際はビットフィールドなのかも
typedef enum vs_error_code_control {
	VS_ERROR_CODECTRL_NO_ERROR = 0,
	VS_ERROR_CODECTRL_PROTECTED = 1,
	VS_ERROR_CODECTRL_IN_BUFEER_UNDERRUN = 2,
	VS_ERROR_CODECTRL_DATA_DISCONTINUITY = 3,
	VS_ERROR_CODECTRL_OUT_BUFEER_UNDERRUN = 4,
	VS_ERROR_CODECTRL_OUT_BUFEER_OVERRUN = 5,
	VS_ERROR_CODECTRL_FORMAT_CHANGE = 6,
	VS_ERROR_CODECTRL_STILL_CAPTURE_ERROR = 7,
	VS_ERROR_CODECTRL_UNKNOWN = 8,
	VS_ERROR_CODECTRL_ERROR = 0xff,
} vs_error_code_control_t;

//********************************************************************************
//********************************************************************************
// Camera Terminal DescriptorのbmControlsフィールドのビットマスク
#define	CTRL_SCANNING		0x000001	// D0:  Scanning Mode
#define	CTRL_AE				0x000002	// D1:  Auto-Exposure Mode
#define	CTRL_AE_PRIORITY	0x000004	// D2:  Auto-Exposure Priority
#define	CTRL_AE_ABS			0x000008	// D3:  Exposure Time (Absolute)
#define	CTRL_AE_REL			0x000010	// D4:  Exposure Time (Relative)
#define CTRL_FOCUS_ABS    	0x000020	// D5:  Focus (Absolute)
#define CTRL_FOCUS_REL		0x000040	// D6:  Focus (Relative)
#define CTRL_IRIS_ABS		0x000080	// D7:  Iris (Absolute)
#define	CTRL_IRIS_REL		0x000100	// D8:  Iris (Relative)
#define	CTRL_ZOOM_ABS		0x000200	// D9:  Zoom (Absolute)
#define CTRL_ZOOM_REL		0x000400	// D10: Zoom (Relative)
#define	CTRL_PANTILT_ABS	0x000800	// D11: PanTilt (Absolute)
#define CTRL_PANTILT_REL	0x001000	// D12: PanTilt (Relative)
#define CTRL_ROLL_ABS		0x002000	// D13: Roll (Absolute)
#define CTRL_ROLL_REL		0x004000	// D14: Roll (Relative)
#define CTRL_D15			0x008000	// D15: Reserved
#define CTRL_D16			0x010000	// D16: Reserved
#define CTRL_FOCUS_AUTO		0x020000	// D17: Focus, Auto
#define CTRL_PRIVACY		0x040000	// D18: Privacy
#define CTRL_FOCUS_SIMPLE	0x080000	// D19: Focus, Simple
#define CTRL_WINDOW			0x100000	// D20: Window
#define CTRL_ROI			0x200000	// D21: ROI
#define CTRL_D22			0x400000	// D22: Reserved
#define CTRL_D23			0x800000	// D23: Reserved

// Processing Unit DescriptorのbmControlsフィールドのビットマスク
#define PU_BRIGHTNESS		0x000001	// D0: Brightness
#define PU_CONTRAST			0x000002	// D1: Contrast
#define PU_HUE				0x000004	// D2: Hue
#define	PU_SATURATION		0x000008	// D3: Saturation
#define PU_SHARPNESS		0x000010	// D4: Sharpness
#define PU_GAMMA			0x000020	// D5: Gamma
#define	PU_WB_TEMP			0x000040	// D6: White Balance Temperature
#define	PU_WB_COMPO			0x000080	// D7: White Balance Component
#define	PU_BACKLIGHT		0x000100	// D8: Backlight Compensation
#define PU_GAIN				0x000200	// D9: Gain
#define PU_POWER_LF			0x000400	// D10: Power Line Frequency
#define PU_HUE_AUTO			0x000800	// D11: Hue, Auto
#define PU_WB_TEMP_AUTO		0x001000	// D12: White Balance Temperature, Auto
#define PU_WB_COMPO_AUTO	0x002000	// D13: White Balance Component, Auto
#define PU_DIGITAL_MULT		0x004000	// D14: Digital Multiplier
#define PU_DIGITAL_LIMIT	0x008000	// D15: Digital Multiplier Limit
#define PU_AVIDEO_STD		0x010000	// D16: Analog Video Standard
#define PU_AVIDEO_LOCK		0x020000	// D17: Analog Video Lock Status
#define PU_CONTRAST_AUTO	0x040000	// D18: Contrast, Auto
#define PU_D19				0x080000	// D19: Reserved
#define PU_D20				0x100000	// D20: Reserved
#define PU_D21				0x200000	// D21: Reserved
#define PU_D22				0x400000	// D22: Reserved
#define PU_D23				0x800000	// D23: Reserved

// FIXME control_value_xxx_tですべてintで保持しているのをやめて対応する型に合わせたのに作り変える

/**
 * 32ビット整数で最小値/最大値/ステップ値/デフォルト値/現在値を保持する
 * FIXME 今はv4l2用だけどuvc用もコレに統一したい
 */
typedef struct control_value32 {
public:
	uint32_t id;
	uint32_t type;
	int32_t min;
	int32_t max;
	int32_t step;
	int32_t def;
	int32_t current;
} control_value32_t;

struct control_value_bool;
typedef struct control_value_bool {
public:
	bool initialized;
	bool has_min_max;
	int def;		// デフォルト値
	int current;	// 現在値

	control_value_bool() : control_value_bool(false) {}
	control_value_bool(bool _has_min_max) : initialized(false), has_min_max(_has_min_max) { def = current = 0; }
} control_value_bool_t;

typedef struct control_value : public control_value_bool_t {
public:
	int res;		// 分解能/設定可能値(ビット😷)
	int min;		// 最小値
	int max;		// 最大値

	control_value() : control_value(true) { }
	control_value(bool _has_min_max) : control_value_bool_t(_has_min_max) { res = min = max = 0; }
} control_value_t;

typedef struct control_value2 : public control_value_t {
public:
	int res2;
	int min2;
	int max2;
	int def2;
	int current2;

	control_value2() : control_value2(true) {}
	control_value2(bool _has_min_max) : control_value_t(_has_min_max) { res2 = min2 = max2 = current2 = 0; }
} control_value2_t;

typedef struct control_value3 : public control_value2_t {
public:
	int res3;
	int min3;
	int max3;
	int def3;
	int current3;

	control_value3() : control_value3(true) {}
	control_value3(bool _has_min_max) : control_value2_t(_has_min_max) { res3 = min3 = max3 = current3 = 0; }
} control_value3_t;

typedef struct control_value4 : public control_value3_t {
public:
	int res4;
	int min4;
	int max4;
	int def4;
	int current4;

	control_value4() : control_value4(true) {}
	control_value4(bool _has_min_max) : control_value3_t(_has_min_max) { res4 = min4 = max4 = current4 = 0; }
} control_value4_t;

// カメラコントロールの設定値取得関数ポインタ
class UVCControl;

// FIXME functionalを使って書き換える...簡単にはいかないのでペンディング
typedef int (UVCControl::*paramget_func_i8)(int8_t &value, const req_code_t &req_code);
typedef int (UVCControl::*paramget_func_i16)(int16_t &value, const req_code_t &req_code);
typedef int (UVCControl::*paramget_func_i32)(int32_t &value, const req_code_t &req_code);
typedef int (UVCControl::*paramget_func_u8)(uint8_t &value, const req_code_t &req_code);
typedef int (UVCControl::*paramget_func_u16)(uint16_t &value, const req_code_t &req_code);
typedef int (UVCControl::*paramget_func_u32)(uint32_t &value, const req_code_t &req_code);
typedef int (UVCControl::*paramget_func_u8u8)(uint8_t &value1, uint8_t &value2, const req_code_t &req_code);
typedef int (UVCControl::*paramget_func_i8u8)(int8_t &value1, uint8_t &value2, const req_code_t &req_code);
typedef int (UVCControl::*paramget_func_i8u8u8)(int8_t &value1, uint8_t &value2, uint8_t &value3, const req_code_t &req_code);
typedef int (UVCControl::*paramget_func_i8u8i8u8)(int8_t &value1, uint8_t &value2, int8_t &value3, uint8_t &value4, const req_code_t &req_code);
typedef int (UVCControl::*paramget_func_i32i32)(int32_t &value1, int32_t &value2, const req_code_t &req_code);

typedef int (UVCControl::*paramset_func_i8)(const int8_t &value);
typedef int (UVCControl::*paramset_func_i16)(const int16_t &value);
typedef int (UVCControl::*paramset_func_i32)(const int32_t &value);
typedef int (UVCControl::*paramset_func_u8)(const uint8_t &value);
typedef int (UVCControl::*paramset_func_u16)(const uint16_t &value);
typedef int (UVCControl::*paramset_func_u32)(const uint32_t &value);
typedef int (UVCControl::*paramset_func_u8u8)(const uint8_t &value1, const uint8_t &value2);
typedef int (UVCControl::*paramset_func_i8u8)(const int8_t &value1, const uint8_t &value2);
typedef int (UVCControl::*paramset_func_i8u8u8)(const int8_t &value1, const uint8_t &value2, const uint8_t &value3);
typedef int (UVCControl::*paramset_func_i8u8i8u8)(const int8_t &value1, const uint8_t &value2, const int8_t &value3, const uint8_t &value4);
typedef int (UVCControl::*paramset_func_i32i32)(const int32_t &value1, const int32_t &value2);

#define UVC_REQ_MASK_CTRL_SCANNING (SET_CUR | GET_CUR | GET_INFO)												//	D0:  Scanning Mode,
#define UVC_REQ_MASK_CTRL_AE_MODE (SET_CUR | GET_CUR | GET_RES | GET_INFO | GET_DEF)							//	D1:  Auto-Exposure Mode
#define UVC_REQ_MASK_CTRL_AE_PRIORITY (SET_CUR |  GET_CUR | GET_INFO)											//	D2:  Auto-Exposure Priority
#define UVC_REQ_MASK_CTRL_AE_ABS (SET_CUR | GET_CUR | GET_MIN | GET_MAX | GET_RES | GET_INFO | GET_DEF)			//	D3:  Exposure Time (Absolute)
#define UVC_REQ_MASK_CTRL_AE_REL (SET_CUR |  GET_CUR | GET_INFO)												//	D4:  Exposure Time (Relative)
#define UVC_REQ_MASK_CTRL_FOCUS_ABS (SET_CUR | GET_CUR | GET_MIN | GET_MAX | GET_RES | GET_INFO | GET_DEF)		//	D5:  Focus (Absolute)
#define UVC_REQ_MASK_CTRL_FOCUS_REL (SET_CUR |  GET_CUR | GET_MIN | GET_MAX | GET_RES | GET_INFO | GET_DEF)		//	D6:  Focus (Relative)
#define UVC_REQ_MASK_CTRL_IRIS_ABS (SET_CUR | GET_CUR | GET_MIN | GET_MAX | GET_RES | GET_INFO | GET_DEF)		//	D7:  Iris (Absolute)
#define UVC_REQ_MASK_CTRL_IRIS_REL (SET_CUR |  GET_CUR | GET_INFO)												//	D8:  Iris (Relative)
#define UVC_REQ_MASK_CTRL_ZOOM_ABS (SET_CUR | GET_CUR | GET_MIN | GET_MAX | GET_RES | GET_INFO | GET_DEF)		//	D9:  Zoom (Absolute)
#define UVC_REQ_MASK_CTRL_ZOOM_REL (SET_CUR |  GET_CUR | GET_MIN | GET_MAX | GET_RES | GET_INFO | GET_DEF)		//	D10: Zoom (Relative)
#define UVC_REQ_MASK_CTRL_PANTILT_ABS (SET_CUR | GET_CUR | GET_MIN | GET_MAX | GET_RES | GET_INFO | GET_DEF)	//	D11: PanTilt (Absolute)
#define UVC_REQ_MASK_CTRL_PAN_ABS (SET_CUR | GET_CUR | GET_MIN | GET_MAX | GET_RES | GET_INFO | GET_DEF)		//	D11: PanTilt (Absolute)
#define UVC_REQ_MASK_CTRL_TILT_ABS (SET_CUR | GET_CUR | GET_MIN | GET_MAX | GET_RES | GET_INFO | GET_DEF)		//	D11: PanTilt (Absolute)
#define UVC_REQ_MASK_CTRL_PANTILT_REL (SET_CUR |  GET_CUR | GET_MIN | GET_MAX | GET_RES | GET_INFO | GET_DEF)	//	D12: PanTilt (Relative)
#define UVC_REQ_MASK_CTRL_PAN_REL (SET_CUR |  GET_CUR | GET_MIN | GET_MAX | GET_RES | GET_INFO | GET_DEF)		//	D12: PanTilt (Relative)
#define UVC_REQ_MASK_CTRL_TILT_REL (SET_CUR |  GET_CUR | GET_MIN | GET_MAX | GET_RES | GET_INFO | GET_DEF)		//	D12: PanTilt (Relative)
#define UVC_REQ_MASK_CTRL_ROLL_ABS (SET_CUR | GET_CUR | GET_MIN | GET_MAX | GET_RES | GET_INFO | GET_DEF)		//	D13: Roll (Absolute)
#define UVC_REQ_MASK_CTRL_ROLL_REL (SET_CUR |  GET_CUR | GET_MIN | GET_MAX | GET_RES | GET_INFO | GET_DEF)		//	D14: Roll (Relative)
#define UVC_REQ_MASK_CTRL_FOCUS_AUTO (SET_CUR |  GET_CUR | GET_INFO | GET_DEF)									//	D17: Focus, Auto
#define UVC_REQ_MASK_CTRL_PRIVACY (SET_CUR | GET_CUR | GET_INFO)												//	D18: Privacy
#define UVC_REQ_MASK_CTRL_FOCUS_SIMPLE (SET_CUR |  GET_CUR | GET_INFO | GET_DEF)								//	D19: Focus, Simple
#define UVC_REQ_MASK_CTRL_WINDOW (SET_CUR |  GET_CUR | GET_MIN | GET_MAX | GET_DEF)								//	D20: Window

#define UVC_REQ_MASK_PU_BRIGHTNESS (SET_CUR |  GET_CUR | GET_MIN | GET_MAX | GET_RES | GET_INFO | GET_DEF)		//	D0: Brightness
#define UVC_REQ_MASK_PU_CONTRAST (SET_CUR |  GET_CUR | GET_MIN | GET_MAX | GET_RES | GET_INFO | GET_DEF)		//	D1: Contrast
#define UVC_REQ_MASK_PU_HUE (SET_CUR | GET_CUR | GET_MIN | GET_MAX | GET_RES | GET_INFO | GET_DEF)				//	D2: Hue
#define UVC_REQ_MASK_PU_SATURATION (SET_CUR |  GET_CUR | GET_MIN | GET_MAX | GET_RES | GET_INFO | GET_DEF)		//	D3: Saturation
#define UVC_REQ_MASK_PU_SHARPNESS (SET_CUR |  GET_CUR | GET_MIN | GET_MAX | GET_RES | GET_INFO | GET_DEF)		//	D4: Sharpness
#define UVC_REQ_MASK_PU_GAMMA (SET_CUR |  GET_CUR | GET_MIN | GET_MAX | GET_RES | GET_INFO | GET_DEF)			//	D5: Gamma
#define UVC_REQ_MASK_PU_WB_TEMP (SET_CUR | GET_CUR | GET_MIN | GET_MAX | GET_RES | GET_INFO | GET_DEF)			//	D6: White Balance Temperature
#define UVC_REQ_MASK_PU_WB_COMPO (SET_CUR | GET_CUR | GET_MIN | GET_MAX | GET_RES | GET_INFO | GET_DEF)			//	D7: White Balance Component
#define UVC_REQ_MASK_PU_BACKLIGHT (SET_CUR |  GET_CUR | GET_MIN | GET_MAX | GET_RES | GET_INFO | GET_DEF)		//	D8: Backlight Compensation
#define UVC_REQ_MASK_PU_GAIN (SET_CUR |  GET_CUR | GET_MIN | GET_MAX | GET_RES | GET_INFO | GET_DEF)			//	D9: Gain
#define UVC_REQ_MASK_PU_POWER_LF (SET_CUR |  GET_CUR | GET_INFO | GET_DEF)										//	D10: Power Line Frequency
#define UVC_REQ_MASK_PU_HUE_AUTO (SET_CUR |  GET_CUR | GET_INFO | GET_DEF)										//	D11: Hue, Auto
#define UVC_REQ_MASK_PU_WB_TEMP_AUTO (SET_CUR |  GET_CUR | GET_INFO | GET_DEF)									//	D12: White Balance Temperature | Auto
#define UVC_REQ_MASK_PU_WB_COMPO_AUTO (SET_CUR |  GET_CUR | GET_INFO | GET_DEF)									//	D13: White Balance Component | Auto
#define UVC_REQ_MASK_PU_DIGITAL_MULT (SET_CUR |  GET_CUR | GET_MIN | GET_MAX | GET_RES | GET_INFO | GET_DEF)	//	D14: Digital Multiplier
#define UVC_REQ_MASK_PU_DIGITAL_LIMIT (SET_CUR |  GET_CUR | GET_MIN | GET_MAX | GET_RES | GET_INFO | GET_DEF)	//	D15: Digital Multiplier Limit
#define UVC_REQ_MASK_PU_AVIDEO_STD (GET_CUR | GET_INFO)															//	D16: Analog Video Standard
#define UVC_REQ_MASK_PU_AVIDEO_LOCK (GET_CUR | GET_INFO)														//	D17: Analog Video Lock Status
#define UVC_REQ_MASK_PU_CONTRAST_AUTO (SET_CUR | GET_CUR | GET_INFO | GET_DEF)									//	D18: Contrast, Auto

}	// end of namespace serenegiant::usb::uvc

#endif /* AANDUVC_H_ */
