/*
 * aAndUsb
 * Copyright (c) 2014-2023 saki t_saki@serenegiant.com
 * Distributed under the terms of the GNU Lesser General Public License (LGPL v3.0) License.
 * License details are in the file license.txt, distributed as part of this software.
 */

#ifndef AANDUSB_DESCRIPTOR_DEFS_H
#define AANDUSB_DESCRIPTOR_DEFS_H

namespace serenegiant::usb {

// core
#define	KEY_INDEX					"index"
#define KEY_TYPE					"type"
#define KEY_SUBTYPE					"subType"
#define KEY_WIDTH					"width"
#define KEY_HEIGHT					"height"
#define	KEY_VALUE					"value"
#define KEY_DETAIL					"detail"
#define KEY_DEFAULT                 "default"

#define DESCRIPTION					"descriptor"
#define DESC_SUBTYPE				KEY_SUBTYPE
#define	DESC_VENDERID				"venderId"
#define	DESC_PRODUCTID				"productId"
#define	DESC_SERIALNUMBER			"serialNumber"
#define DESC_MANIFUCTURE			"manifuctureName"
#define DESC_PRODUCT				"productName"
#define DESC_UVC					"uvc"
#define DESC_VIDEO_CONTROL			"videoControl"
#define DESC_INTERFACES				"interfaces"
#define DESC_BOS					"bos"

#define INTERFACE_TYPE 				KEY_TYPE
#define INTERFACE_TYPE_VIDEOSTREAM	"videoStreaming"
#define INTERFACE_TYPE_AUDIOSTREAM	"audioStreaming"
#define INTERFACE_INDEX				KEY_INDEX
#define INTERFACE_ENDPOINT_ADDR		"endpointAddress"

// uvc
#define	FORMATS						"formats"
#define FORMAT_INDEX				KEY_INDEX
// #define FORMAT_NAME				"format"
#define FORMAT_DETAIL				KEY_DETAIL
#define FORMAT_BITS_PER_PIXEL		"bitsPerPixel"
#define FORMAT_GUID					"GUID"
#define FORMAT_DEFAULT_FRAME_INDEX	"defaultFrameIndex"
#define FORMAT_ASPECTRATIO_X		"aspectRatioX"
#define FORMAT_ASPECTRATIO_Y		"aspectRatioY"
#define FORMAT_INTERLACE_FLAGS		"interlaceFlags"
#define FORMAT_COPY_PROTECT			"copyProtect"
#define FORMAT_FRAMEDESCRIPTORS		"frameDescriptors"

#define STILL_FRAMES				"still_frames"

#define FRAME_INDEX					KEY_INDEX
#define FRAME_CAPABILITIES			"capabilities"
#define FRAME_SIZE                  "size"
#define FRAME_TYPE                  "frame_type"
#define FRAME_DEFAULT               KEY_DEFAULT
#define	FRAME_WIDTH					KEY_WIDTH
#define	FRAME_HEIGHT				KEY_HEIGHT
#define FRAME_BITRATE_MIN			"minBitRate"
#define FRAME_BITRATE_MAX			"maxBitRate"
#define FRAME_FRAMEBUFFERSIZE_MAX	"maxFrameBufferSize"
#define FRAME_INTERVAL_DEFAULT		"defaultFrameInterval"
#define FRAME_FPS_DEFAULT			"defaultFps"
#define FRAME_FRAME_RATE	       	"frameRate"
#define FRAME_INTERVALS			    "intervals"
#define FRAME_INTERVAL_INDEX		KEY_INDEX
#define FRAME_INTERVAL_VALUE		KEY_VALUE
#define FRAME_INTERVAL_FPS	   	    "fps"
#define FRAME_INTERVAL_MIN		    "minFrameInterval"
#define FRAME_INTERVAL_MAX		    "maxFrameInterval"
#define FRAME_INTERVAL_STEP	    	"frameIntervalStep"

} // namespace serenegiant::usb

#endif //AANDUSB_DESCRIPTOR_DEFS_H
