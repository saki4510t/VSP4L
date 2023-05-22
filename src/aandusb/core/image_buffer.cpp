/*
 * aAndUsb
 * Copyright (c) 2014-2023 saki t_saki@serenegiant.com
 * Distributed under the terms of the GNU Lesser General Public License (LGPL v3.0) License.
 * License details are in the file license.txt, distributed as part of this software.
 */

#define LOG_TAG "ImageBuffer"

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
#endif

#include <algorithm>

#include "utilbase.h"

// core
#include "core/image_buffer.h"
#if defined(__ANDROID__)
#include "core/hardware_buffer_utils.h"
#endif
// usb
#include "usb/aandusb.h"

namespace serenegiant::core {

/**
 * コンストラクタ
 * @param frame_type
 * @param width
 * @param height
 * @param enable_hw_buffer Android以外は無視される, デフォルトfalse
 */
ImageBuffer::ImageBuffer(
	const raw_frame_t &frame_type,
	const uint32_t &width, const uint32_t &height,
	const bool &enable_hw_buffer)
:	m_frame_type(frame_type),
	m_width(width), m_height(height),
	m_enable_hw_buffer(enable_hw_buffer),
	pixel_bytes(get_pixel_bytes(frame_type)),
	locked(false)
#if defined(__ANDROID__)
	, graphicBuffer(nullptr)
#endif
{
	ENTER();

	int result = USB_ERROR_NOT_SUPPORTED;
#if defined(__ANDROID__)
	if (enable_hw_buffer) {
		result = allocate_hardware_buffer(
			frame_type, width, height, graphicBuffer);
	}
#endif
	if (result || !is_hw_buffer()) {
		LOGD("%s", enable_hw_buffer ? "通常バッファへフォールバック" : "通常バッファを生成");
		buffer.resize(pixel_bytes.frame_bytes(width, height));
	}

	EXIT();
}

/**
 * デストラクタ
 */
ImageBuffer::~ImageBuffer() {
	ENTER();

#if defined(__ANDROID__)
	if (graphicBuffer) {
		AAHardwareBuffer_release(graphicBuffer);
		graphicBuffer = nullptr;
	}
#endif

	EXIT();
}

/**
 * ハードウエアバッファーの時はロックしてVideoImage_tへセットして返す
 * 通常バッファーの場合はVideoImage_tへのセットして返すだけ
 * @return
 */
VideoImage_t ImageBuffer::lock(const uint64_t &lock_usage) {
	ENTER();

	VideoImage_t image{};

	if (!locked) {
#if defined(__ANDROID__)
		if (is_hw_buffer()) {
			// ハードウエアバッファーの時
			int result = lock_and_assign(image,
				frame_type(), width(), height(),
				graphicBuffer, lock_usage);
			if (!result) {
				locked = true;
			} else {
				// だめならハードウエアバッファは使えない
				unlock();
				image.frame_type = RAW_FRAME_UNKNOWN;
				AAHardwareBuffer_release(graphicBuffer);
				graphicBuffer = nullptr;
			}
		} else {
			// 通常のバッファーの時
			int result = assign(image,
				frame_type(), width(), height(),
				&buffer[0], buffer.capacity());
			if (!result) {
				locked = true;
			} else {
				image.frame_type = RAW_FRAME_UNKNOWN;
			}
		}
#else
		// 通常のバッファーの時
		int result = assign(image,
			frame_type(), width(), height(),
			&buffer[0], buffer.capacity());
		if (!result) {
			locked = true;
		} else {
			image.frame_type = RAW_FRAME_UNKNOWN;
		}
#endif
	} else {
		image.frame_type = RAW_FRAME_UNKNOWN;
	}

	RET(image);
}

/**
 * ハードウエアバッファーのロック解除
 * 通常バッファーの場合はフラグをロック中フラグをクリアするだけ
 * @return
 */
int ImageBuffer::unlock() {
	ENTER();

	if (locked) {
		locked = false;
#if defined(__ANDROID__)
		if (is_hw_buffer()) {
			LOGV("AAHardwareBuffer_unlock");
			AAHardwareBuffer_unlock(graphicBuffer, nullptr);
			AAHardwareBuffer_release(graphicBuffer);
		}
#endif
	}

	RETURN(USB_SUCCESS, int);
}

}	// namespace serenegiant::usb
