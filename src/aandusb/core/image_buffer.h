/*
 * aAndUsb
 * Copyright (c) 2014-2023 saki t_saki@serenegiant.com
 * Distributed under the terms of the GNU Lesser General Public License (LGPL v3.0) License.
 * License details are in the file license.txt, distributed as part of this software.
 */

#ifndef AANDUSB_IMAGE_BUFFER_H
#define AANDUSB_IMAGE_BUFFER_H

#include <cstdio>
#include <vector>
#include <memory>

#if defined(__ANDROID__)
// common
#include "hardware_buffer_stub.h"
#endif

// core
#include "core/video_image.h"
#include "core/video_frame_base.h"

namespace serenegiant::core {

/**
 * 通常バッファー(vector<uint8_t>)とハードウエアバッファ両対応のイメージバッファークラス
 * YV12, I420, NV12, NV21に対応
 */
class ImageBuffer {
private:
	const raw_frame_t m_frame_type;
	const uint32_t m_width;
	const uint32_t m_height;
	const bool m_enable_hw_buffer;
	const PixelBytes pixel_bytes;
	bool locked;
#if defined(__ANDROID__)
	AHardwareBuffer *graphicBuffer;
#endif
	std::vector<uint8_t> buffer;

protected:
public:
	/**
	 * コンストラクタ
	 * @param frame_type
	 * @param width
	 * @param height
	 * @param enable_hw_buffer Android以外は無視される, デフォルトfalse
	 */
	ImageBuffer(
		const raw_frame_t &frame_type,
		const uint32_t &width, const uint32_t &height,
		const bool &enable_hw_buffer = false);
	/**
	 * デストラクタ
	 */
	virtual ~ImageBuffer();
	/**
	 * 保持している映像フレームタイプを取得
	 * @return
	 */
	inline raw_frame_t frame_type() const { return m_frame_type; }
	/**
	 * 映像幅を取得
	 * @return
	 */
	inline uint32_t width() const { return m_width; };
	/**
	 * 映像高さを取得
	 * @return
	 */
	inline uint32_t height() const { return m_height; };
	/**
	 * ハードウエアバッファが有効かどうかを取得
	 * @return
	 */
	inline bool enable_hw_buffer() const { return m_enable_hw_buffer; };
#if defined(__ANDROID__)
	/**
	 * ハードウエアバッファを使っているかどうかを取得
	 * @return
	 */
	inline bool is_hw_buffer() const { return graphicBuffer != nullptr; }
	/**
	 * 内部で保持しているAHardwareBufferポインターを取得する
	 * lock/unlockとの併用には注意が必要
	 * @return
	 */
	AHardwareBuffer *get_hw_buffer() { return graphicBuffer; };
#else
	inline bool is_hw_buffer() const { return false; }
#endif
	/**
	 * ロック中かどうかを取得
	 * @return
	 */
	inline bool is_locked() const { return locked; };
	/**
	 * ハードウエアバッファーの時はロックしてVideoImage_tへセットして返す
	 * 通常バッファーの場合はVideoImage_tへのセットして返すだけ
	 * @return
	 */
	VideoImage_t lock(const uint64_t &lock_usage = DEFAULT_LOCK_USAGE);
	/**
	 * ハードウエアバッファーのロック解除
	 * 通常バッファーの場合はフラグをロック中フラグをクリアするだけ
	 * @return
	 */
	int unlock();
};

typedef std::shared_ptr<ImageBuffer> ImageBufferSp;
typedef std::unique_ptr<ImageBuffer> ImageBufferUp;

}	// namespace serenegiant::usb

#endif //AANDUSB_IMAGE_BUFFER_H
