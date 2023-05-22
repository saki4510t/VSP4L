/*
 * aAndUsb
 * Copyright (c) 2014-2023 saki t_saki@serenegiant.com
 * Distributed under the terms of the GNU Lesser General Public License (LGPL v3.0) License.
 * License details are in the file license.txt, distributed as part of this software.
 */

#ifndef AANDUSB_H_
#define AANDUSB_H_

#include <list>
#include <vector>
#include <string>
#include <cstring>

#include <linux/usbdevice_fs.h>
#include <linux/usb/ch9.h>
// core
#include "core/core.h"

namespace serenegiant::usb {

#define MAX_USBFS_BUFFER_SIZE (16384)
#define MAX_DESCRIPTOR_LEN (4096)

#define USB_REQ_STANDARD_DEVICE_SET (USB_DIR_OUT | USB_TYPE_STANDARD | USB_RECIP_DEVICE)		// 0x10
#define USB_REQ_STANDARD_DEVICE_GET (USB_DIR_IN | USB_TYPE_STANDARD | USB_RECIP_DEVICE)			// 0x90
#define USB_REQ_STANDARD_INTERFACE_SET (USB_DIR_OUT | USB_TYPE_STANDARD | USB_RECIP_INTERFACE)	// 0x11
#define USB_REQ_STANDARD_INTERFACE_GET (USB_DIR_IN | USB_TYPE_STANDARD | USB_RECIP_INTERFACE)	// 0x91
#define USB_REQ_STANDARD_ENDPOINT_SET (USB_DIR_OUT | USB_TYPE_STANDARD | USB_RECIP_ENDPOINT)	// 0x12
#define USB_REQ_STANDARD_ENDPOINT_GET (USB_DIR_IN | USB_TYPE_STANDARD | USB_RECIP_ENDPOINT)		// 0x92

#define USB_REQ_CS_DEVICE_SET (USB_DIR_OUT | USB_TYPE_CLASS | USB_RECIP_DEVICE)					// 0x20
#define USB_REQ_CS_DEVICE_GET (USB_DIR_IN | USB_TYPE_CLASS | USB_RECIP_DEVICE)					// 0xa0
#define USB_REQ_CS_INTERFACE_SET (USB_DIR_OUT | USB_TYPE_CLASS | USB_RECIP_INTERFACE)			// 0x21
#define USB_REQ_CS_INTERFACE_GET (USB_DIR_IN | USB_TYPE_CLASS | USB_RECIP_INTERFACE)			// 0xa1
#define USB_REQ_CS_ENDPOINT_SET (USB_DIR_OUT | USB_TYPE_CLASS | USB_RECIP_ENDPOINT)				// 0x22
#define USB_REQ_CS_ENDPOINT_GET (USB_DIR_IN | USB_TYPE_CLASS | USB_RECIP_ENDPOINT)				// 0xa2

#define USB_REQ_VENDER_DEVICE_SET (USB_DIR_OUT | USB_TYPE_VENDOR | USB_RECIP_DEVICE)			// 0x40
#define USB_REQ_VENDER_DEVICE_GET (USB_DIR_IN | USB_TYPE_VENDOR | USB_RECIP_DEVICE)				// 0xc0
#define USB_REQ_VENDER_INTERFACE_SET (USB_DIR_OUT | USB_TYPE_VENDOR | USB_RECIP_INTERFACE)		// 0x41
#define USB_REQ_VENDER_INTERFACE_GET (USB_DIR_IN | USB_TYPE_VENDOR | USB_RECIP_INTERFACE)		// 0xc1
#define USB_REQ_VENDER_ENDPOINT_SET (USB_DIR_OUT | USB_TYPE_VENDOR | USB_RECIP_ENDPOINT)		// 0x42
#define USB_REQ_VENDER_ENDPOINT_GET (USB_DIR_IN | USB_TYPE_VENDOR | USB_RECIP_ENDPOINT)			// 0xc2

// 型名が長いので少し短く再定義
typedef struct usbdevfs_ctrltransfer ctrl_transfer_t;
typedef struct usbdevfs_bulktransfer bulk_transfer_t;

typedef struct usb_descriptor_header descriptor_header_t;
typedef struct usb_device_descriptor device_descriptor_t;
typedef struct usb_config_descriptor config_descriptor_t;
typedef struct usb_interface_assoc_descriptor interface_assoc_descriptor_t;
typedef struct usb_string_descriptor string_descriptor_t;
typedef struct usb_interface_descriptor interface_descriptor_t;
typedef struct usb_endpoint_descriptor endpoint_descriptor_t;
/** SuperSpeed Endpoint Companion Descriptor*/
typedef struct usb_ss_ep_comp_descriptor ss_ep_comp_descriptor_t;
/** SuperSpeedPlus Isochronous Endpoint Companion descriptor */
typedef struct usb_ssp_isoc_ep_comp_descriptor ssp_isoc_ep_comp_descriptor_t;
typedef struct usb_qualifier_descriptor qualifier_descriptor_t;
typedef struct usb_otg_descriptor otg_descriptor_t;
typedef struct usb_debug_descriptor debug_descriptor_t;
typedef struct usb_security_descriptor security_descriptor_t;
typedef struct usb_key_descriptor key_descriptor_t;
typedef struct usb_encryption_descriptor encryption_descriptor_t;
typedef struct usb_bos_descriptor bos_descriptor_t;
typedef struct usb_dev_cap_header dev_cap_header_t;
typedef struct usb_wireless_cap_descriptor wireless_cap_descriptor_t;
typedef struct usb_ext_cap_descriptor ext_cap_descriptor_t;
typedef struct usb_ss_cap_descriptor ss_cap_descriptor_t;
typedef struct usb_ss_container_id_descriptor ss_container_id_descriptor_t;
typedef struct usb_wireless_ep_comp_descriptor wireless_ep_comp_descriptor_t;

typedef enum device_capability {
	DEV_CAP_WIRELESS = 0x01,
	DEV_CAP_USB2_EXT = 0x02,
	DEV_CAP_SUPERSPEED = 0x03,
	DEV_CAP_CONTAINER_ID = 0x04,
	DEV_CAP_PLATFORM = 0x05,
	DEV_CAP_POWER_DELIVERY = 0x06,
	DEV_CAP_BATTERY_INFO = 0x07,
	DEV_CAP_PD_CONSUMER_PORT = 0x08,
	DEV_CAP_PD_PROVIDER_PORT = 0x09,
	DEV_CAP_SUPERSPEED_PLUS = 0x0a,
	DEV_CAP_PRECISION_TIME_MEAS = 0x0b,
	DEV_CAP_WIRELESS_EXT = 0x0c,
	// 0x00, 0x0d-0xffはreserved
} device_capability_t;

// コントロールインターフェースの値取得・設定用の要求コード
#define REQ_SET_ 0x00
#define REQ_GET_ 0x80
#define REQ__CUR 0x1
#define REQ__MIN 0x2
#define REQ__MAX 0x3
#define REQ__RES 0x4
#define REQ__MEM 0x5
#define REQ__LEN 0x5
#define REQ__INFO 0x6
#define REQ__DEF 0x7

typedef enum req_code {
	REQ_UNDEFINED = 0x00,
	REQ_SET_CUR = (REQ_SET_ | REQ__CUR),	// bmRequestType=0x21
	REQ_SET_MIN = (REQ_SET_ | REQ__MIN),
	REQ_SET_MAX = (REQ_SET_ | REQ__MAX),
	REQ_SET_RES = (REQ_SET_ | REQ__RES),
	REQ_SET_LEN = (REQ_SET_ | REQ__LEN),	REQ_SET_MEM = (REQ_SET_ | REQ__MEM),
	REQ_SET_INFO = (REQ_SET_ | REQ__INFO),
	REQ_SET_DEF = (REQ_SET_ | REQ__DEF),
//
	REQ_GET_CUR = (REQ_GET_ | REQ__CUR),	// bmRequestType=0xa1
	REQ_GET_MIN = (REQ_GET_ | REQ__MIN),
	REQ_GET_MAX = (REQ_GET_ | REQ__MAX),	// ↑
	REQ_GET_RES = (REQ_GET_ | REQ__RES),	// ↑
	REQ_GET_LEN = (REQ_GET_ | REQ__LEN),	REQ_GET_MEM = (REQ_GET_ | REQ__MEM),
	REQ_GET_INFO = (REQ_GET_ | REQ__INFO),// ↑
	REQ_GET_DEF = (REQ_GET_ | REQ__DEF),	// ↑
//
	REQ_GET_START = 0xff,
} req_code_t;

/**
 * ペイロードヘッダーのptsとstreaming_control_tのdwClockFrequencyから
 * presentationTimeUsを計算する。正常に計算できなかったときは0を返す
 * @param lock_frequency
 * @param pts
 * @param scr
 * @param sof
 * @return presentationTimeUs
 */
nsecs_t calc_presentation_time_us(
	const uint32_t &clock_frequency,
	const nsecs_t &pts,
	const uint32_t &stc = 0/*unused*/, const uint32_t &sof = 0/*unused*/);	// stcとsofをあわせてSCR

typedef enum usb_device_speed usb_speed_t;

#define ENDPOINT_TRANSFER_MASK USB_ENDPOINT_XFERTYPE_MASK
typedef enum endpoint_transfer_type {
	ENDPOINT_TRANSFER_TYPE_CONTROL = USB_ENDPOINT_XFER_CONTROL,		// 0x00
	ENDPOINT_TRANSFER_TYPE_ISOCHRONOUS = USB_ENDPOINT_XFER_ISOC,	// 0x01
	ENDPOINT_TRANSFER_TYPE_BULK = USB_ENDPOINT_XFER_BULK,			// 0x02
	ENDPOINT_TRANSFER_TYPE_INTERRUPT = USB_ENDPOINT_XFER_INT,		// 0x03

} endpopint_transfer_type_t;

#ifndef USBDEVFS_GET_SPEED
#define USBDEVFS_GET_SPEED _IO('U', 31)
#endif

}	// end of namespace serenegiant::usb

#endif /* AANDUSB_H_ */
